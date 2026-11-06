// FPSAnimInstance.cpp
// Implementation of native C++ AnimInstance and AnimInstanceProxy for AFPSCharacter & AFPSCharacterPreviewActor.

#include "FPSAnimInstance.h"
#include "FPSCharacter.h"
#include "../Weapons/FPSWeaponBase.h"
#include "../Combat/FPSEquipmentComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "BonePose.h"
#include "TwoBoneIK.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/AnimationPoseData.h"

namespace
{
	FCompactPoseBoneIndex FindBoneCompactIndex(
		const FBoneContainer& BoneContainer,
		const FReferenceSkeleton& RefSkel,
		std::initializer_list<const TCHAR*> CandidateNames)
	{
		for (const TCHAR* Name : CandidateNames)
		{
			const int32 SkelIdx = RefSkel.FindBoneIndex(FName(Name));
			if (SkelIdx != INDEX_NONE)
			{
				const FCompactPoseBoneIndex CompactIdx = BoneContainer.GetCompactPoseIndexFromSkeletonIndex(SkelIdx);
				if (CompactIdx != INDEX_NONE)
				{
					return CompactIdx;
				}
			}
		}
		return FCompactPoseBoneIndex(INDEX_NONE);
	}

	/** Propagates a single child bone from its parent's current CS transform using rest-pose local offset. */
	void PropagateSingleChildBoneCS(
		FComponentSpacePoseContext& CSPose,
		const TArray<FTransform>& RestCompBases,
		FCompactPoseBoneIndex ParentIdx,
		FCompactPoseBoneIndex ChildIdx,
		const FQuat& AdditionalChildCompRot = FQuat::Identity)
	{
		if (ParentIdx == INDEX_NONE || ChildIdx == INDEX_NONE)
		{
			return;
		}
		const int32 PInt = ParentIdx.GetInt();
		const int32 CInt = ChildIdx.GetInt();
		if (!RestCompBases.IsValidIndex(PInt) || !RestCompBases.IsValidIndex(CInt))
		{
			return;
		}

		const FTransform& CurParentCS = CSPose.Pose.GetComponentSpaceTransform(ParentIdx);
		const FTransform& RestParentCS = RestCompBases[PInt];
		const FTransform& RestChildCS = RestCompBases[CInt];

		const FTransform LocalRestChild = RestChildCS.GetRelativeTransform(RestParentCS);
		FTransform NewChildCS = LocalRestChild * CurParentCS;
		if (!AdditionalChildCompRot.Equals(FQuat::Identity))
		{
			NewChildCS.SetRotation((AdditionalChildCompRot * NewChildCS.GetRotation()).GetNormalized());
		}
		CSPose.Pose.SetComponentSpaceTransform(ChildIdx, NewChildCS);
	}

	/** Axis-agnostic, crash-proof quaternion alignment between two vectors. Guards against zero length, NaNs, and antiparallel singularity. */
	FQuat SafeFindBetweenNormals(const FVector& V1, const FVector& V2, const FVector& FallbackAxis = FVector::UpVector)
	{
		if (V1.ContainsNaN() || V2.ContainsNaN() || V1.IsNearlyZero(1e-4f) || V2.IsNearlyZero(1e-4f))
		{
			return FQuat::Identity;
		}
		const FVector N1 = V1.GetSafeNormal();
		const FVector N2 = V2.GetSafeNormal();
		const float Dot = FMath::Clamp(FVector::DotProduct(N1, N2), -1.0f, 1.0f);
		if (Dot > 0.9999f)
		{
			return FQuat::Identity;
		}
		if (Dot < -0.9999f)
		{
			FVector Axis = FVector::CrossProduct(N1, FallbackAxis).GetSafeNormal();
			if (Axis.IsNearlyZero(1e-4f))
			{
				Axis = FVector::CrossProduct(N1, FVector::RightVector).GetSafeNormal();
			}
			if (Axis.IsNearlyZero(1e-4f))
			{
				Axis = FVector::CrossProduct(N1, FVector::ForwardVector).GetSafeNormal();
			}
			return FQuat(Axis, PI);
		}
		const FVector Cross = FVector::CrossProduct(N1, N2);
		const float W = 1.0f + Dot;
		const FQuat Result(Cross.X, Cross.Y, Cross.Z, W);
		const FQuat NormalizedResult = Result.GetNormalized();
		return NormalizedResult.ContainsNaN() ? FQuat::Identity : NormalizedResult;
	}
}

UFPSAnimInstance::UFPSAnimInstance()
{
}

void UFPSAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	OwningFPSCharacter = Cast<AFPSCharacter>(TryGetPawnOwner());
	if (AActor* OwningActor = GetOwningActor())
	{
		OwningBeltGear = OwningActor->FindComponentByClass<UFPSBeltGearComponent>();
	}
}

void UFPSAnimInstance::NotifyWeaponFired(float Strength)
{
	RecoilImpulseAlpha = FMath::Clamp(RecoilImpulseAlpha + Strength, 0.0f, 1.5f);
	TriggerAlertHighReady(3.5f);
}

void UFPSAnimInstance::TriggerAlertHighReady(float HoldDurationSeconds)
{
	AlertHighReadyTimer = FMath::Max(AlertHighReadyTimer, HoldDurationSeconds);
}

void UFPSAnimInstance::NotifyGrenadeThrowStarted(EFPSBeltItemType ThrowItemType, float DurationSeconds)
{
	bIsThrowingGrenade = true;
	ActiveThrowItemType = ThrowItemType;
	ThrowTotalDuration = FMath::Max(DurationSeconds, 0.25f);
	ThrowElapsedSeconds = 0.0f;
	ThrowProgressAlpha = 0.0f;
	TriggerAlertHighReady(3.5f);
}

void UFPSAnimInstance::SetActiveMocapSequence(UAnimSequenceBase* InSequence, float InPlayRate)
{
	if (ActiveMocapSequence != InSequence)
	{
		ActiveMocapSequence = InSequence;
		MocapAccumulatedTime = 0.0f;
	}
	MocapPlayRate = InPlayRate;
}

void UFPSAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!OwningFPSCharacter)
	{
		OwningFPSCharacter = Cast<AFPSCharacter>(TryGetPawnOwner());
	}
	if (!OwningBeltGear)
	{
		if (AActor* OwningActor = GetOwningActor())
		{
			OwningBeltGear = OwningActor->FindComponentByClass<UFPSBeltGearComponent>();
		}
	}

	// Continuous biological & posture oscillators (0.32 Hz breathing, 0.18 Hz weight transfer)
	IdleBreathPhase = FMath::Fmod(IdleBreathPhase + DeltaSeconds * (0.32f * 2.0f * PI), 2.0f * PI);
	WeightTransferPhase = FMath::Fmod(WeightTransferPhase + DeltaSeconds * (0.18f * 2.0f * PI), 2.0f * PI);
	HeadLookTime += DeltaSeconds;

	// Query component-space locations of belt gear sockets if present
	if (OwningBeltGear)
	{
		BeltMag1TargetCS = OwningBeltGear->GetBeltItemComponentSpaceLocation(EFPSBeltItemType::Mag1);
		BeltFragTargetCS = OwningBeltGear->GetBeltItemComponentSpaceLocation(EFPSBeltItemType::FragGrenade);
		BeltFlashTargetCS = OwningBeltGear->GetBeltItemComponentSpaceLocation(EFPSBeltItemType::Flashbang);
	}

	// Update grenade throw timer if active
	if (bIsThrowingGrenade)
	{
		ThrowElapsedSeconds += DeltaSeconds;
		ThrowProgressAlpha = FMath::Clamp(ThrowElapsedSeconds / ThrowTotalDuration, 0.0f, 1.0f);
		if (ThrowElapsedSeconds >= ThrowTotalDuration)
		{
			bIsThrowingGrenade = false;
			ThrowProgressAlpha = 0.0f;
		}
	}

	if (!OwningFPSCharacter)
	{
		// Hangar preview or generic actor mode
		bIsMenuPreview = true;
		bUnarmedRelaxedIdle = (MenuPoseIndex != 1);
		bForceHighReady = (MenuPoseIndex == 1);
		bIsADS = (MenuPoseIndex == 1);
		Speed = (MenuPoseIndex == 2) ? 215.0f : 0.0f;

		const float TargetReady = bForceHighReady ? 1.0f : 0.0f;
		HighReadyAlpha = FMath::FInterpTo(HighReadyAlpha, TargetReady, DeltaSeconds, 5.0f);
		SmoothADSAlpha = FMath::FInterpTo(SmoothADSAlpha, bIsADS ? 1.0f : 0.0f, DeltaSeconds, 8.0f);

		if (Speed > 5.0f)
		{
			const float StrideFreq = FMath::GetMappedRangeValueClamped(FVector2D(0.0f, 650.0f), FVector2D(1.2f, 3.4f), Speed);
			LocomotionCyclePhase = FMath::Fmod(LocomotionCyclePhase + DeltaSeconds * StrideFreq * 2.0f * PI, 2.0f * PI);
		}
		else
		{
			LocomotionCyclePhase = FMath::FInterpTo(LocomotionCyclePhase, 0.0f, DeltaSeconds, 6.0f);
		}

		if (OwningBeltGear)
		{
			OwningBeltGear->UpdateReloadBeltGrabState(false, 0.0f, true);
			OwningBeltGear->UpdateThrowBeltGrabState(false, 0.0f, EFPSBeltItemType::FragGrenade, true, true);
		}
		return;
	}

	// 1. Velocity and Horizontal Ground Speed
	const FVector Velocity = OwningFPSCharacter->GetVelocity();
	const FVector HorizontalVelocity(Velocity.X, Velocity.Y, 0.0f);
	Speed = HorizontalVelocity.Size();

	// 2. Signed Forward / Right Speeds & Movement Direction [-180, 180]
	const FRotator ActorRotation = OwningFPSCharacter->GetActorRotation();
	const FVector ForwardVec = FRotationMatrix(ActorRotation).GetUnitAxis(EAxis::X);
	const FVector RightVec = FRotationMatrix(ActorRotation).GetUnitAxis(EAxis::Y);

	ForwardSpeed = FVector::DotProduct(HorizontalVelocity, ForwardVec);
	RightSpeed = FVector::DotProduct(HorizontalVelocity, RightVec);

	if (Speed > 3.0f)
	{
		Direction = FMath::RadiansToDegrees(FMath::Atan2(RightSpeed, ForwardSpeed));
	}
	else
	{
		Direction = 0.0f;
	}

	// 3. Movement State Flags
	if (const UCharacterMovementComponent* MoveComp = OwningFPSCharacter->GetCharacterMovement())
	{
		bIsInAir = MoveComp->IsFalling();
	}
	else
	{
		bIsInAir = false;
	}

	bIsCrouching = OwningFPSCharacter->bIsCrouched;
	bIsSprinting = OwningFPSCharacter->IsSprinting();
	bIsADS = OwningFPSCharacter->IsADS();
	bIsSliding = OwningFPSCharacter->IsSliding();

	// 4. Aim Offset Pitch & Yaw
	AimPitch = FMath::Clamp(FRotator::NormalizeAxis(OwningFPSCharacter->GetReplicatedAimPitch()), -89.0f, 89.0f);

	const FRotator ControlRot = OwningFPSCharacter->GetControlRotation();
	const FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(ControlRot, ActorRotation);
	AimYaw = FMath::Clamp(DeltaRot.Yaw, -90.0f, 90.0f);

	// 5. Weapon Firing, Reload & Belt-Grab State
	bool bHasReserveMags = true;
	if (const AFPSWeaponBase* Weapon = OwningFPSCharacter->GetCurrentWeapon())
	{
		bIsReloading = Weapon->IsReloading();
		ReloadProgressAlpha = Weapon->GetReloadProgress();
		bHasReserveMags = (Weapon->GetReserveAmmo() > 0);
	}
	else
	{
		bIsReloading = false;
		ReloadProgressAlpha = 0.0f;
	}

	// Update belt gear visibility & hand-held proxy visibility on Game Thread
	if (OwningBeltGear)
	{
		OwningBeltGear->UpdateReloadBeltGrabState(bIsReloading, ReloadProgressAlpha, bHasReserveMags);

		bool bHasFragsLeft = true;
		bool bHasFlashesLeft = true;
		if (const UFPSEquipmentComponent* EquipComp = OwningFPSCharacter->GetEquipmentComponent())
		{
			bHasFragsLeft = (EquipComp->GetLethalCount() > 0);
			bHasFlashesLeft = (EquipComp->GetTacticalCount() > 0);
		}
		OwningBeltGear->UpdateThrowBeltGrabState(bIsThrowingGrenade, ThrowProgressAlpha, ActiveThrowItemType, bHasFragsLeft, bHasFlashesLeft);
	}

	// 6. Low-Ready Patrol Cradle vs High-Ready / ADS Blend
	if (bIsADS || bIsFiring || bIsReloading || bIsThrowingGrenade || Speed > 15.0f || FMath::Abs(AimPitch) > 12.0f)
	{
		AlertHighReadyTimer = FMath::Max(AlertHighReadyTimer, 2.5f);
	}
	if (AlertHighReadyTimer > 0.0f)
	{
		AlertHighReadyTimer = FMath::Max(0.0f, AlertHighReadyTimer - DeltaSeconds);
	}

	const float TargetHighReady = (AlertHighReadyTimer > 0.0f || bForceHighReady || bIsADS) ? 1.0f : 0.0f;
	HighReadyAlpha = FMath::FInterpTo(HighReadyAlpha, TargetHighReady, DeltaSeconds, (TargetHighReady > HighReadyAlpha) ? 9.5f : 2.8f);

	// 7. Smooth State Alphas & Phase Accumulators
	SmoothCrouchAlpha = FMath::FInterpTo(SmoothCrouchAlpha, bIsCrouching ? 1.0f : 0.0f, DeltaSeconds, 10.0f);
	SmoothSprintAlpha = FMath::FInterpTo(SmoothSprintAlpha, (bIsSprinting && Speed > 200.0f) ? 1.0f : 0.0f, DeltaSeconds, 8.0f);
	SmoothADSAlpha = FMath::FInterpTo(SmoothADSAlpha, bIsADS ? 1.0f : 0.0f, DeltaSeconds, 14.0f);
	SmoothSlideAlpha = FMath::FInterpTo(SmoothSlideAlpha, bIsSliding ? 1.0f : 0.0f, DeltaSeconds, 12.0f);
	RecoilImpulseAlpha = FMath::FInterpTo(RecoilImpulseAlpha, 0.0f, DeltaSeconds, 18.0f);

	if (Speed > 5.0f && !bIsInAir && !bIsSliding)
	{
		const float StrideFreq = FMath::GetMappedRangeValueClamped(FVector2D(0.0f, 650.0f), FVector2D(1.2f, 3.4f), Speed);
		LocomotionCyclePhase = FMath::Fmod(LocomotionCyclePhase + DeltaSeconds * StrideFreq * 2.0f * PI, 2.0f * PI);
	}
	else
	{
		LocomotionCyclePhase = FMath::FInterpTo(LocomotionCyclePhase, 0.0f, DeltaSeconds, 6.0f);
	}

	// Advance optional mocap sequence playhead
	if (ActiveMocapSequence)
	{
		const float SeqLength = FMath::Max(ActiveMocapSequence->GetPlayLength(), 0.05f);
		MocapAccumulatedTime = FMath::Fmod(MocapAccumulatedTime + DeltaSeconds * MocapPlayRate, SeqLength);
		if (MocapAccumulatedTime < 0.0f)
		{
			MocapAccumulatedTime += SeqLength;
		}
	}
}

// ============================================================================
// FFPSAnimInstanceProxy Implementation
// ============================================================================

void FFPSAnimInstanceProxy::Initialize(UAnimInstance* InAnimInstance)
{
	FAnimInstanceProxy::Initialize(InAnimInstance);
	bBonesResolved = false;
}

void FFPSAnimInstanceProxy::CacheBones()
{
	FAnimInstanceProxy::CacheBones();
	bBonesResolved = false;
}

void FFPSAnimInstanceProxy::PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds)
{
	FAnimInstanceProxy::PreUpdate(InAnimInstance, DeltaSeconds);

	if (const UFPSAnimInstance* FPSAnim = Cast<UFPSAnimInstance>(InAnimInstance))
	{
		Speed = FPSAnim->Speed;
		Direction = FPSAnim->Direction;
		ForwardSpeed = FPSAnim->ForwardSpeed;
		RightSpeed = FPSAnim->RightSpeed;
		AimPitch = FPSAnim->AimPitch;
		AimYaw = FPSAnim->AimYaw;
		bIsInAir = FPSAnim->bIsInAir;
		bIsCrouching = FPSAnim->bIsCrouching;
		bIsSprinting = FPSAnim->bIsSprinting;
		bIsADS = FPSAnim->bIsADS;
		bIsSliding = FPSAnim->bIsSliding;
		bIsFiring = FPSAnim->bIsFiring;
		RecoilImpulseAlpha = FPSAnim->RecoilImpulseAlpha;
		LocomotionCyclePhase = FPSAnim->LocomotionCyclePhase;
		IdleBreathPhase = FPSAnim->IdleBreathPhase;
		WeightTransferPhase = FPSAnim->WeightTransferPhase;
		HeadLookTime = FPSAnim->HeadLookTime;
		SmoothCrouchAlpha = FPSAnim->SmoothCrouchAlpha;
		SmoothSprintAlpha = FPSAnim->SmoothSprintAlpha;
		SmoothADSAlpha = FPSAnim->SmoothADSAlpha;
		SmoothSlideAlpha = FPSAnim->SmoothSlideAlpha;
		HighReadyAlpha = FPSAnim->HighReadyAlpha;
		bIsMenuPreview = FPSAnim->bIsMenuPreview;
		bUnarmedRelaxedIdle = FPSAnim->bUnarmedRelaxedIdle;
		bIsReloading = FPSAnim->bIsReloading;
		ReloadProgressAlpha = FPSAnim->ReloadProgressAlpha;
		BeltMag1TargetCS = FPSAnim->BeltMag1TargetCS;
		BeltFragTargetCS = FPSAnim->BeltFragTargetCS;
		BeltFlashTargetCS = FPSAnim->BeltFlashTargetCS;
		bIsThrowingGrenade = FPSAnim->bIsThrowingGrenade;
		ThrowProgressAlpha = FPSAnim->ThrowProgressAlpha;
		ActiveThrowItemType = FPSAnim->ActiveThrowItemType;
		ActiveSequence = FPSAnim->ActiveMocapSequence;
		SequencePlayTime = FPSAnim->MocapAccumulatedTime;
	}
}

void FFPSAnimInstanceProxy::ResolveSkeletonBones(const FBoneContainer& BoneContainer)
{
	const FReferenceSkeleton& RefSkel = BoneContainer.GetReferenceSkeleton();

	PelvisIdx    = FindBoneCompactIndex(BoneContainer, RefSkel, { TEXT("pelvis"), TEXT("Hips"), TEXT("mixamorig:Hips") });
	Spine01Idx   = FindBoneCompactIndex(BoneContainer, RefSkel, { TEXT("spine_01"), TEXT("Spine"), TEXT("mixamorig:Spine") });
	Spine02Idx   = FindBoneCompactIndex(BoneContainer, RefSkel, { TEXT("spine_02"), TEXT("Spine1"), TEXT("mixamorig:Spine1") });
	Spine03Idx   = FindBoneCompactIndex(BoneContainer, RefSkel, { TEXT("spine_03"), TEXT("Spine2"), TEXT("mixamorig:Spine2") });
	Neck01Idx    = FindBoneCompactIndex(BoneContainer, RefSkel, { TEXT("neck_01"), TEXT("Neck"), TEXT("mixamorig:Neck") });
	HeadIdx      = FindBoneCompactIndex(BoneContainer, RefSkel, { TEXT("head"), TEXT("Head"), TEXT("mixamorig:Head") });

	ClavicleLIdx = FindBoneCompactIndex(BoneContainer, RefSkel, { TEXT("clavicle_l"), TEXT("LeftShoulder"), TEXT("mixamorig:LeftShoulder") });
	UpperArmLIdx = FindBoneCompactIndex(BoneContainer, RefSkel, { TEXT("upperarm_l"), TEXT("LeftArm"), TEXT("mixamorig:LeftArm") });
	LowerArmLIdx = FindBoneCompactIndex(BoneContainer, RefSkel, { TEXT("lowerarm_l"), TEXT("LeftForeArm"), TEXT("mixamorig:LeftForeArm") });
	HandLIdx     = FindBoneCompactIndex(BoneContainer, RefSkel, { TEXT("hand_l"), TEXT("LeftHand"), TEXT("mixamorig:LeftHand") });

	ClavicleRIdx = FindBoneCompactIndex(BoneContainer, RefSkel, { TEXT("clavicle_r"), TEXT("RightShoulder"), TEXT("mixamorig:RightShoulder") });
	UpperArmRIdx = FindBoneCompactIndex(BoneContainer, RefSkel, { TEXT("upperarm_r"), TEXT("RightArm"), TEXT("mixamorig:RightArm") });
	LowerArmRIdx = FindBoneCompactIndex(BoneContainer, RefSkel, { TEXT("lowerarm_r"), TEXT("RightForeArm"), TEXT("mixamorig:RightForeArm") });
	HandRIdx     = FindBoneCompactIndex(BoneContainer, RefSkel, { TEXT("hand_r"), TEXT("RightHand"), TEXT("mixamorig:RightHand") });
	WeaponRIdx   = FindBoneCompactIndex(BoneContainer, RefSkel, { TEXT("weapon_r"), TEXT("ik_hand_gun") });

	ThighLIdx    = FindBoneCompactIndex(BoneContainer, RefSkel, { TEXT("thigh_l"), TEXT("LeftUpLeg"), TEXT("mixamorig:LeftUpLeg") });
	CalfLIdx     = FindBoneCompactIndex(BoneContainer, RefSkel, { TEXT("calf_l"), TEXT("LeftLeg"), TEXT("mixamorig:LeftLeg") });
	FootLIdx     = FindBoneCompactIndex(BoneContainer, RefSkel, { TEXT("foot_l"), TEXT("LeftFoot"), TEXT("mixamorig:LeftFoot") });
	BallLIdx     = FindBoneCompactIndex(BoneContainer, RefSkel, { TEXT("ball_l"), TEXT("toes_l"), TEXT("LeftToeBase"), TEXT("mixamorig:LeftToeBase") });

	ThighRIdx    = FindBoneCompactIndex(BoneContainer, RefSkel, { TEXT("thigh_r"), TEXT("RightUpLeg"), TEXT("mixamorig:RightUpLeg") });
	CalfRIdx     = FindBoneCompactIndex(BoneContainer, RefSkel, { TEXT("calf_r"), TEXT("RightLeg"), TEXT("mixamorig:RightLeg") });
	FootRIdx     = FindBoneCompactIndex(BoneContainer, RefSkel, { TEXT("foot_r"), TEXT("RightFoot"), TEXT("mixamorig:RightFoot") });
	BallRIdx     = FindBoneCompactIndex(BoneContainer, RefSkel, { TEXT("ball_r"), TEXT("toes_r"), TEXT("RightToeBase"), TEXT("mixamorig:RightToeBase") });

	bBonesResolved = true;
}

bool FFPSAnimInstanceProxy::Evaluate(FPoseContext& Output)
{
	const FBoneContainer& BoneContainer = Output.Pose.GetBoneContainer();
	if (!BoneContainer.IsValid())
	{
		Output.ResetToRefPose();
		return true;
	}

	if (!bBonesResolved)
	{
		ResolveSkeletonBones(BoneContainer);
	}

	// Build component-space rest transforms for anatomical reference & axis-agnostic math
	const FReferenceSkeleton& RefSkel = BoneContainer.GetReferenceSkeleton();
	const TArray<FTransform>& RefBonePose = RefSkel.GetRefBonePose();
	TArray<FTransform> RestCompBases;
	RestCompBases.SetNumUninitialized(RefBonePose.Num());
	for (int32 i = 0; i < RefBonePose.Num(); ++i)
	{
		const int32 ParentIdx = RefSkel.GetParentIndex(i);
		if (ParentIdx != INDEX_NONE)
		{
			RestCompBases[i] = RefBonePose[i] * RestCompBases[ParentIdx];
		}
		else
		{
			RestCompBases[i] = RefBonePose[i];
		}
	}

	// Derive anatomical basis vectors in Component Space from rest pose
	if (PelvisIdx != INDEX_NONE && HeadIdx != INDEX_NONE && UpperArmLIdx != INDEX_NONE && UpperArmRIdx != INDEX_NONE)
	{
		const FVector PelvisPos = RestCompBases[PelvisIdx.GetInt()].GetLocation();
		const FVector HeadPos = RestCompBases[HeadIdx.GetInt()].GetLocation();
		const FVector ShLPos = RestCompBases[UpperArmLIdx.GetInt()].GetLocation();
		const FVector ShRPos = RestCompBases[UpperArmRIdx.GetInt()].GetLocation();

		CompUpDir = (HeadPos - PelvisPos).GetSafeNormal();
		if (CompUpDir.IsNearlyZero()) CompUpDir = FVector::UpVector;

		CompRightDir = (ShRPos - ShLPos).GetSafeNormal();
		CompRightDir = (CompRightDir - (CompRightDir | CompUpDir) * CompUpDir).GetSafeNormal();
		if (CompRightDir.IsNearlyZero()) CompRightDir = FVector(1.0f, 0.0f, 0.0f);

		CompForwardDir = FVector::CrossProduct(CompRightDir, CompUpDir).GetSafeNormal();
		if (CompForwardDir.IsNearlyZero()) CompForwardDir = FVector(0.0f, -1.0f, 0.0f);

		// Guarantee CompForwardDir points out of the chest (towards negative Y in SK_BaseMale space)
		if (CompForwardDir.Y > 0.0f)
		{
			CompForwardDir = -CompForwardDir;
		}
	}

	// Evaluate procedural resting idle & locomotion directly on the Vanguard skeleton so Mixamo/Vanguard bone rolls stay 100% intact!
	const bool bUseMocapBase = false;
	Output.ResetToRefPose();

	FComponentSpacePoseContext CSPose(this);
	CSPose.Pose.InitPose(Output.Pose);

	// 1. Lower Body: Grounded 180cm Resting Idle (with 0.18 Hz weight-transfer & Two-Bone Leg IK) or Locomotion
	if (!bUseMocapBase)
	{
		ApplyRestingIdleAndLocomotion(CSPose, RestCompBases);
	}

	// 2. Spine & Head: 0.32 Hz Thoracic Breathing + Aim Pitch/Yaw + Situational Micro-Look
	ApplySpineBreathingAndHeadLook(CSPose, RestCompBases);

	// 3. Upper Body Arms & Belt-Grab IK:
	//    - If throwing a grenade: Right hand reaches to BeltFragComp / BeltFlashComp (0-25%), pulls pin (25-45%), hurls (45-60%), returns (60-100%)
	//    - Else if reloading: Right hand holds rifle steady in canted cradle while Left hand reaches to BeltMagComp_1 (20-40%) and seats mag (40-75%)
	//    - Else: Both hands hold the rifle in either Low-Ready Patrol Cradle (HighReadyAlpha=0) or High-Ready / ADS (HighReadyAlpha=1)
	if (bIsThrowingGrenade)
	{
		SolveGrenadeThrowBeltIK(CSPose, RestCompBases);
	}
	else
	{
		SolveRightArmRifleIK(CSPose, RestCompBases);
		if (bIsReloading)
		{
			SolveLeftHandReloadIK(CSPose, RestCompBases);
		}
		else
		{
			SolveLeftHandTwoBoneIK(CSPose, RestCompBases);
		}
	}

	// Convert modified Component-Space pose back into Local CompactPose
	FCSPose<FCompactPose>::ConvertComponentPosesToLocalPoses(MoveTemp(CSPose.Pose), Output.Pose);
	return true;
}

void FFPSAnimInstanceProxy::ApplyRestingIdleAndLocomotion(
	FComponentSpacePoseContext& CSPose,
	const TArray<FTransform>& RestCompBases)
{
	if (PelvisIdx == INDEX_NONE || ThighLIdx == INDEX_NONE || CalfLIdx == INDEX_NONE || FootLIdx == INDEX_NONE ||
		ThighRIdx == INDEX_NONE || CalfRIdx == INDEX_NONE || FootRIdx == INDEX_NONE)
	{
		return;
	}

	const float MoveAlpha = FMath::Clamp(Speed / 220.0f, 0.0f, 1.0f);

	// 1. Pelvis Transform: Tall, grounded 180cm athletic posture + 0.18 Hz weight transfer
	FTransform PelvisCS = CSPose.Pose.GetComponentSpaceTransform(PelvisIdx);
	FVector PelvisPos = PelvisCS.GetLocation();
	FQuat PelvisRot = PelvisCS.GetRotation();

	// Slow 0.18 Hz weight-transfer oscillation between left and right leg when stationary
	const float WeightSine = FMath::Sin(WeightTransferPhase);
	const float IdleWeightAlpha = (1.0f - MoveAlpha) * (1.0f - SmoothCrouchAlpha);

	// Lifelike weight-transfer (±1.65 cm lateral sway) with grounding settle
	const float BreathSinVal = FMath::Sin(IdleBreathPhase);
	PelvisPos += CompRightDir * (WeightSine * 1.65f * IdleWeightAlpha);
	PelvisPos += CompUpDir * (-0.35f + FMath::Cos(WeightTransferPhase * 2.0f) * 0.35f + BreathSinVal * 0.30f) * IdleWeightAlpha;

	// Natural contrapposto hip roll (±1.8 deg) and grounded tactical stance yaw
	const FQuat HipRollQ(CompForwardDir, FMath::DegreesToRadians(WeightSine * 1.8f * IdleWeightAlpha));
	const FQuat StanceYawQ(CompUpDir, FMath::DegreesToRadians(5.5f * (1.0f - SmoothADSAlpha * 0.5f) * IdleWeightAlpha));
	PelvisRot = (StanceYawQ * HipRollQ * PelvisRot).GetNormalized();

	// Walk cycle bounce if previewing walk cycle in Hangar (`Speed > 5.0f`)
	if (MoveAlpha > 0.01f)
	{
		const float WalkBob = FMath::Sin(LocomotionCyclePhase * 2.0f) * 2.2f * MoveAlpha;
		const float WalkSway = FMath::Cos(LocomotionCyclePhase) * 1.6f * MoveAlpha;
		PelvisPos += CompUpDir * WalkBob + CompRightDir * WalkSway;
	}

	PelvisCS.SetLocation(PelvisPos);
	PelvisCS.SetRotation(PelvisRot);
	CSPose.Pose.SetComponentSpaceTransform(PelvisIdx, PelvisCS);

	// 2. Grounded Lower-Body Weight-Transfer & Walk Stride (preserves upright 180cm Mixamo leg bone rolls!)
	const float WeightRollRad = FMath::DegreesToRadians(WeightSine * 1.15f * IdleWeightAlpha);
	const FQuat ThighWeightDelta(CompForwardDir, -WeightRollRad);

	const float SinL = FMath::Sin(LocomotionCyclePhase);
	const float SinR = FMath::Sin(LocomotionCyclePhase + PI);
	const float WalkHipPitchL  = FMath::DegreesToRadians(SinL * 22.0f * MoveAlpha);
	const float WalkKneeFlexL  = FMath::DegreesToRadians(-FMath::Max(0.0f, -SinL) * 34.0f * MoveAlpha);
	const float WalkHipPitchR  = FMath::DegreesToRadians(SinR * 22.0f * MoveAlpha);
	const float WalkKneeFlexR  = FMath::DegreesToRadians(-FMath::Max(0.0f, -SinR) * 34.0f * MoveAlpha);

	const FQuat ThighLDelta = FQuat(CompRightDir, WalkHipPitchL) * ThighWeightDelta;
	const FQuat CalfLDelta  = FQuat(CompRightDir, WalkKneeFlexL);
	const FQuat FootLDelta  = FQuat(CompRightDir, -(WalkHipPitchL + WalkKneeFlexL) * 0.65f);

	PropagateSingleChildBoneCS(CSPose, RestCompBases, PelvisIdx, ThighLIdx, ThighLDelta);
	PropagateSingleChildBoneCS(CSPose, RestCompBases, ThighLIdx, CalfLIdx, CalfLDelta);
	PropagateSingleChildBoneCS(CSPose, RestCompBases, CalfLIdx, FootLIdx, FootLDelta);
	PropagateSingleChildBoneCS(CSPose, RestCompBases, FootLIdx, BallLIdx);

	// Right leg: natural relaxed resting flare (+7.5 deg yaw, -4.0 deg knee ease) when stationary
	const FQuat RightStanceFlare = FQuat(CompUpDir, FMath::DegreesToRadians(7.5f * IdleWeightAlpha));
	const FQuat RightKneeEase    = FQuat(CompRightDir, FMath::DegreesToRadians(-4.0f * IdleWeightAlpha));

	const FQuat ThighRDelta = FQuat(CompRightDir, WalkHipPitchR) * ThighWeightDelta * RightStanceFlare;
	const FQuat CalfRDelta  = FQuat(CompRightDir, WalkKneeFlexR) * RightKneeEase;
	const FQuat FootRDelta  = FQuat(CompRightDir, -(WalkHipPitchR + WalkKneeFlexR) * 0.65f) * FQuat(CompUpDir, FMath::DegreesToRadians(-3.5f * IdleWeightAlpha));

	PropagateSingleChildBoneCS(CSPose, RestCompBases, PelvisIdx, ThighRIdx, ThighRDelta);
	PropagateSingleChildBoneCS(CSPose, RestCompBases, ThighRIdx, CalfRIdx, CalfRDelta);
	PropagateSingleChildBoneCS(CSPose, RestCompBases, CalfRIdx, FootRIdx, FootRDelta);
	PropagateSingleChildBoneCS(CSPose, RestCompBases, FootRIdx, BallRIdx);
}

void FFPSAnimInstanceProxy::ApplySpineBreathingAndHeadLook(
	FComponentSpacePoseContext& CSPose,
	const TArray<FTransform>& RestCompBases)
{
	if (PelvisIdx == INDEX_NONE || Spine01Idx == INDEX_NONE || Spine02Idx == INDEX_NONE || Spine03Idx == INDEX_NONE)
	{
		return;
	}

	// 1. Thoracic Breathing: 0.32 Hz deep tactical breathing with noticeable chest rise/fall
	const float BreathSin = FMath::Sin(IdleBreathPhase);
	const float BreathPitchDeg = BreathSin * 3.4f; // Lifelike chest lift & expansion on inhale

	// 2. Aim Pitch & Yaw + Counter-rotation for Pelvis Stance Yaw
	const float EffectivePitch = FMath::Clamp(AimPitch, -75.0f, 75.0f);
	const float EffectiveYaw = FMath::Clamp(AimYaw, -75.0f, 75.0f);
	const float RecoilKickDeg = -RecoilImpulseAlpha * 4.5f;

	// Counter the +5 deg pelvis yaw across the spine so the chest faces toward the target
	const float SpineCounterYawDeg = -1.6f * (1.0f - SmoothADSAlpha * 0.5f);

	const FQuat Spine01Delta =
		FQuat(CompUpDir, FMath::DegreesToRadians(EffectiveYaw * 0.22f + SpineCounterYawDeg)) *
		FQuat(CompRightDir, FMath::DegreesToRadians(EffectivePitch * 0.18f + BreathPitchDeg * 0.25f));

	const FQuat Spine02Delta =
		FQuat(CompUpDir, FMath::DegreesToRadians(EffectiveYaw * 0.28f + SpineCounterYawDeg)) *
		FQuat(CompRightDir, FMath::DegreesToRadians(EffectivePitch * 0.26f + BreathPitchDeg * 0.50f + RecoilKickDeg * 0.4f));

	const FQuat Spine03Delta =
		FQuat(CompUpDir, FMath::DegreesToRadians(EffectiveYaw * 0.30f + SpineCounterYawDeg)) *
		FQuat(CompRightDir, FMath::DegreesToRadians(EffectivePitch * 0.32f + BreathPitchDeg * 0.75f + RecoilKickDeg * 0.6f));

	// Propagate Spine01 -> Spine02 -> Spine03 with accurate child position updates
	PropagateSingleChildBoneCS(CSPose, RestCompBases, PelvisIdx, Spine01Idx, Spine01Delta);
	PropagateSingleChildBoneCS(CSPose, RestCompBases, Spine01Idx, Spine02Idx, Spine02Delta);
	PropagateSingleChildBoneCS(CSPose, RestCompBases, Spine02Idx, Spine03Idx, Spine03Delta);

	// Anatomical chest expansion translation: ribcage expands up (+0.70cm) and forward (+0.55cm) on deep inhale
	FTransform ExpandedSpine03CS = CSPose.Pose.GetComponentSpaceTransform(Spine03Idx);
	ExpandedSpine03CS.AddToTranslation(CompUpDir * (BreathSin * 0.70f) + CompForwardDir * (BreathSin * 0.55f));
	CSPose.Pose.SetComponentSpaceTransform(Spine03Idx, ExpandedSpine03CS);

	// 3. Clavicle Breathing Lift: shoulders rise +2.6 deg on inhale, drop on exhale
	const float ClavicleLiftRad = FMath::DegreesToRadians(BreathSin * 2.6f);
	const FQuat ClavLDelta(CompForwardDir, -ClavicleLiftRad);
	const FQuat ClavRDelta(CompForwardDir, ClavicleLiftRad);

	PropagateSingleChildBoneCS(CSPose, RestCompBases, Spine03Idx, ClavicleLIdx, ClavLDelta);
	PropagateSingleChildBoneCS(CSPose, RestCompBases, ClavicleLIdx, UpperArmLIdx);
	PropagateSingleChildBoneCS(CSPose, RestCompBases, UpperArmLIdx, LowerArmLIdx);
	PropagateSingleChildBoneCS(CSPose, RestCompBases, LowerArmLIdx, HandLIdx);

	PropagateSingleChildBoneCS(CSPose, RestCompBases, Spine03Idx, ClavicleRIdx, ClavRDelta);
	PropagateSingleChildBoneCS(CSPose, RestCompBases, ClavicleRIdx, UpperArmRIdx);
	PropagateSingleChildBoneCS(CSPose, RestCompBases, UpperArmRIdx, LowerArmRIdx);
	PropagateSingleChildBoneCS(CSPose, RestCompBases, LowerArmRIdx, HandRIdx);

	// 4. Situational Awareness Head/Neck Micro-Look (±6 deg yaw, ±2 deg pitch when at rest)
	const float IdleLookScale = 1.0f - HighReadyAlpha * 0.75f;
	const float LookYawDeg = (FMath::Sin(HeadLookTime * 0.65f) * 4.2f + FMath::Sin(HeadLookTime * 1.45f) * 1.8f) * IdleLookScale;
	const float LookPitchDeg = (FMath::Cos(HeadLookTime * 0.85f) * 1.6f + BreathSin * 1.2f) * IdleLookScale;

	const FQuat NeckDelta =
		FQuat(CompUpDir, FMath::DegreesToRadians(EffectiveYaw * 0.10f + LookYawDeg * 0.45f)) *
		FQuat(CompRightDir, FMath::DegreesToRadians(EffectivePitch * 0.12f + LookPitchDeg * 0.45f));

	const FQuat HeadDelta =
		FQuat(CompUpDir, FMath::DegreesToRadians(EffectiveYaw * 0.10f + LookYawDeg * 0.55f)) *
		FQuat(CompRightDir, FMath::DegreesToRadians(EffectivePitch * 0.12f + LookPitchDeg * 0.55f));

	PropagateSingleChildBoneCS(CSPose, RestCompBases, Spine03Idx, Neck01Idx, NeckDelta);
	PropagateSingleChildBoneCS(CSPose, RestCompBases, Neck01Idx, HeadIdx, HeadDelta);
}

void FFPSAnimInstanceProxy::PropagateHandChildBones(
	FComponentSpacePoseContext& CSPose,
	const TArray<FTransform>& RestCompBases,
	FCompactPoseBoneIndex HandBoneIdx,
	const FTransform& NewHandCS)
{
	if (HandBoneIdx == INDEX_NONE)
	{
		return;
	}
	const int32 HandInt = HandBoneIdx.GetInt();
	if (!RestCompBases.IsValidIndex(HandInt))
	{
		return;
	}

	const FTransform& RestHandCS = RestCompBases[HandInt];
	const FBoneContainer& BoneContainer = CSPose.Pose.GetPose().GetBoneContainer();
	const FReferenceSkeleton& RefSkel = BoneContainer.GetReferenceSkeleton();
	const int32 NumBones = RefSkel.GetNum();

	// Propagate all descendant bones of HandBoneIdx (e.g. thumb/index/middle/ring/pinky fingers and weapon_r)
	// When unarmed at rest, curl fingers naturally into the palm instead of stiff open T-pose!
	const int32 HandSkelIdx = BoneContainer.GetSkeletonIndex(HandBoneIdx);
	const bool bIsLeft = (HandBoneIdx == HandLIdx);
	const float CurlSign = bIsLeft ? -1.0f : 1.0f;
	const float RelaxedCurlDeg = (bUnarmedRelaxedIdle && HighReadyAlpha < 0.05f) ? (22.0f * CurlSign) : 0.0f;
	const FQuat FingerCurlRot = FQuat(FVector::UpVector, FMath::DegreesToRadians(RelaxedCurlDeg));

	for (int32 SkelIdx = HandSkelIdx + 1; SkelIdx < NumBones; ++SkelIdx)
	{
		if (RefSkel.BoneIsChildOf(SkelIdx, HandSkelIdx))
		{
			const FCompactPoseBoneIndex ChildCompactIdx = BoneContainer.GetCompactPoseIndexFromSkeletonIndex(SkelIdx);
			if (ChildCompactIdx != INDEX_NONE && RestCompBases.IsValidIndex(ChildCompactIdx.GetInt()))
			{
				FTransform LocalToHand = RestCompBases[ChildCompactIdx.GetInt()].GetRelativeTransform(RestHandCS);
				if (FMath::Abs(RelaxedCurlDeg) > 0.01f)
				{
					LocalToHand.SetRotation((LocalToHand.GetRotation() * FingerCurlRot).GetNormalized());
				}
				CSPose.Pose.SetComponentSpaceTransform(ChildCompactIdx, LocalToHand * NewHandCS);
			}
		}
	}
}

void FFPSAnimInstanceProxy::SolveRightArmRifleIK(
	FComponentSpacePoseContext& CSPose,
	const TArray<FTransform>& RestCompBases)
{
	if (UpperArmRIdx == INDEX_NONE || LowerArmRIdx == INDEX_NONE || HandRIdx == INDEX_NONE || Spine03Idx == INDEX_NONE)
	{
		return;
	}

	const float WalkMoveAlpha = FMath::Clamp(Speed / 220.0f, 0.0f, 1.0f);

	const FTransform Spine03CS = CSPose.Pose.GetComponentSpaceTransform(Spine03Idx);
	const FVector ChestPos = Spine03CS.GetLocation();
	const float BreathOffset = FMath::Sin(IdleBreathPhase) * 0.45f;
	const float RecoilBack = RecoilImpulseAlpha * 2.8f;

	// Authentic Tier-1 Plate Carrier / Molle Rest ("Thumbs in Vest" / Tactical Rest):
	// Hands rest naturally high on front Molle straps / upper chest rig near clavicle,
	// elbows relaxed downward and slightly flared to clear side armor / pouches.
	const FVector RelaxedHandR =
		ChestPos
		+ CompRightDir * 15.5f
		+ CompForwardDir * 16.0f
		- CompUpDir * 9.5f
		+ CompUpDir * (BreathOffset * 0.95f)
		+ CompForwardDir * (BreathOffset * 0.45f);

	const FVector RelaxedJointR =
		ChestPos
		+ CompRightDir * 25.5f
		- CompForwardDir * 2.0f
		- CompUpDir * 20.0f;

	// Low-Ready Patrol Cradle: pistol grip held comfortably near right ribcage/pecs
	const FVector LowReadyHandR =
		ChestPos
		+ CompRightDir * 13.5f
		+ CompForwardDir * 21.0f
		+ CompUpDir * (-7.5f + BreathOffset);

	// High-Ready / Combat Ready: pistol grip raised toward shoulder pocket
	const FVector HighReadyHandR =
		ChestPos
		+ CompRightDir * (11.0f - SmoothADSAlpha * 4.5f)
		+ CompForwardDir * (24.0f - SmoothADSAlpha * 3.0f - RecoilBack)
		+ CompUpDir * (4.5f + SmoothADSAlpha * 3.5f + BreathOffset * 0.4f);

	const FVector ArmedJointR =
		ChestPos
		+ CompRightDir * 30.0f
		- CompForwardDir * 12.0f
		- CompUpDir * 20.0f;

	FVector TargetHandR = bUnarmedRelaxedIdle
		? FMath::Lerp(RelaxedHandR, HighReadyHandR, HighReadyAlpha)
		: FMath::Lerp(LowReadyHandR, HighReadyHandR, HighReadyAlpha);

	FVector JointTargetR = bUnarmedRelaxedIdle
		? FMath::Lerp(RelaxedJointR, ArmedJointR, HighReadyAlpha)
		: ArmedJointR;

	// During reload, tilt the rifle slightly inward so the magwell faces the left hand
	if (bIsReloading)
	{
		TargetHandR = ChestPos + CompRightDir * 11.5f + CompForwardDir * 22.0f + CompUpDir * (-2.0f + BreathOffset * 0.5f);
	}

	FTransform UpperArmRCS = CSPose.Pose.GetComponentSpaceTransform(UpperArmRIdx);
	FTransform LowerArmRCS = CSPose.Pose.GetComponentSpaceTransform(LowerArmRIdx);
	FTransform HandRCS     = CSPose.Pose.GetComponentSpaceTransform(HandRIdx);

	AnimationCore::SolveTwoBoneIK(UpperArmRCS, LowerArmRCS, HandRCS, JointTargetR, TargetHandR, false, 1.0f, 1.05f);

	const int32 UIdx = UpperArmRIdx.GetInt();
	const int32 LIdx = LowerArmRIdx.GetInt();
	const int32 HIdx = HandRIdx.GetInt();

	// Guard against degenerate Two-Bone IK transforms propagating NaNs to the GPU
	if (UpperArmRCS.ContainsNaN() && RestCompBases.IsValidIndex(UIdx)) UpperArmRCS = RestCompBases[UIdx];
	if (LowerArmRCS.ContainsNaN() && RestCompBases.IsValidIndex(LIdx)) LowerArmRCS = RestCompBases[LIdx];
	if (HandRCS.ContainsNaN() && RestCompBases.IsValidIndex(HIdx))     HandRCS = RestCompBases[HIdx];

	// Align right forearm & hand rotation cleanly along the forearm direction using SafeFindBetweenNormals
	if (RestCompBases.IsValidIndex(UIdx) && RestCompBases.IsValidIndex(LIdx) && RestCompBases.IsValidIndex(HIdx))
	{
		const FVector RefUpperDir = (RestCompBases[LIdx].GetLocation() - RestCompBases[UIdx].GetLocation()).GetSafeNormal();
		const FVector NewUpperDir = (LowerArmRCS.GetLocation() - UpperArmRCS.GetLocation()).GetSafeNormal();
		if (!RefUpperDir.IsNearlyZero() && !NewUpperDir.IsNearlyZero())
		{
			UpperArmRCS.SetRotation((SafeFindBetweenNormals(RefUpperDir, NewUpperDir) * RestCompBases[UIdx].GetRotation()).GetNormalized());
		}

		const FVector RefLowerDir = (RestCompBases[HIdx].GetLocation() - RestCompBases[LIdx].GetLocation()).GetSafeNormal();
		const FVector NewLowerDir = (HandRCS.GetLocation() - LowerArmRCS.GetLocation()).GetSafeNormal();
		if (!RefLowerDir.IsNearlyZero() && !NewLowerDir.IsNearlyZero())
		{
			const FQuat AlignLower = SafeFindBetweenNormals(RefLowerDir, NewLowerDir);
			LowerArmRCS.SetRotation((AlignLower * RestCompBases[LIdx].GetRotation()).GetNormalized());
			HandRCS.SetRotation((AlignLower * RestCompBases[HIdx].GetRotation()).GetNormalized());
		}
	}

	CSPose.Pose.SetComponentSpaceTransform(UpperArmRIdx, UpperArmRCS);
	CSPose.Pose.SetComponentSpaceTransform(LowerArmRIdx, LowerArmRCS);
	CSPose.Pose.SetComponentSpaceTransform(HandRIdx, HandRCS);
	PropagateHandChildBones(CSPose, RestCompBases, HandRIdx, HandRCS);
}

void FFPSAnimInstanceProxy::SolveLeftHandTwoBoneIK(
	FComponentSpacePoseContext& CSPose,
	const TArray<FTransform>& RestCompBases)
{
	if (UpperArmLIdx == INDEX_NONE || LowerArmLIdx == INDEX_NONE || HandLIdx == INDEX_NONE || HandRIdx == INDEX_NONE || Spine03Idx == INDEX_NONE)
	{
		return;
	}

	const float WalkMoveAlpha = FMath::Clamp(Speed / 220.0f, 0.0f, 1.0f);

	const FTransform HandRCS = CSPose.Pose.GetComponentSpaceTransform(HandRIdx);
	const FTransform Spine03CS = CSPose.Pose.GetComponentSpaceTransform(Spine03Idx);
	const FVector HandRPos = HandRCS.GetLocation();
	const FVector ChestPos = Spine03CS.GetLocation();
	const float BreathOffsetL = FMath::Sin(IdleBreathPhase) * 0.45f;

	// Authentic Tier-1 Plate Carrier / Molle Rest ("Thumbs in Vest" / Tactical Rest):
	const FVector RelaxedHandL =
		ChestPos
		- CompRightDir * 15.5f
		+ CompForwardDir * 16.0f
		- CompUpDir * 9.5f
		+ CompUpDir * (BreathOffsetL * 0.95f)
		+ CompForwardDir * (BreathOffsetL * 0.45f);

	const FVector RelaxedJointL =
		ChestPos
		- CompRightDir * 25.5f
		- CompForwardDir * 2.0f
		- CompUpDir * 20.0f;

	// Low-Ready Patrol Cradle: Left hand supports the handguard across the chest,
	// angling the barrel ~18 degrees downward and across the body toward the left hip/front.
	const FVector LowReadyBarrelDir = (
		-CompRightDir * 0.62f
		+ CompForwardDir * 0.72f
		- CompUpDir * 0.31f // ~18 deg downward pitch
	).GetSafeNormal();

	const float PitchRad = FMath::DegreesToRadians(FMath::Clamp(AimPitch, -65.0f, 65.0f));
	const FVector HighReadyBarrelDir = (
		-CompRightDir * (0.18f * (1.0f - SmoothADSAlpha))
		+ CompForwardDir * FMath::Cos(PitchRad)
		+ CompUpDir * FMath::Sin(PitchRad)
	).GetSafeNormal();

	const FVector BarrelDir = FMath::Lerp(LowReadyBarrelDir, HighReadyBarrelDir, HighReadyAlpha).GetSafeNormal();
	const FVector ArmedHandL = HandRPos + BarrelDir * 31.0f - CompUpDir * 1.5f;

	const FVector ArmedJointL =
		ChestPos
		- CompRightDir * 28.0f
		+ CompForwardDir * 4.0f
		- CompUpDir * 24.0f;

	const FVector EffectorTargetCS = bUnarmedRelaxedIdle
		? FMath::Lerp(RelaxedHandL, ArmedHandL, HighReadyAlpha)
		: ArmedHandL;

	const FVector JointTargetCS = bUnarmedRelaxedIdle
		? FMath::Lerp(RelaxedJointL, ArmedJointL, HighReadyAlpha)
		: ArmedJointL;

	FTransform UpperArmLCS = CSPose.Pose.GetComponentSpaceTransform(UpperArmLIdx);
	FTransform LowerArmLCS = CSPose.Pose.GetComponentSpaceTransform(LowerArmLIdx);
	FTransform HandLCS     = CSPose.Pose.GetComponentSpaceTransform(HandLIdx);

	AnimationCore::SolveTwoBoneIK(UpperArmLCS, LowerArmLCS, HandLCS, JointTargetCS, EffectorTargetCS, false, 1.0f, 1.05f);

	const int32 UIdx = UpperArmLIdx.GetInt();
	const int32 LIdx = LowerArmLIdx.GetInt();
	const int32 HIdx = HandLIdx.GetInt();

	// Guard against degenerate Two-Bone IK transforms propagating NaNs to the GPU
	if (UpperArmLCS.ContainsNaN() && RestCompBases.IsValidIndex(UIdx)) UpperArmLCS = RestCompBases[UIdx];
	if (LowerArmLCS.ContainsNaN() && RestCompBases.IsValidIndex(LIdx)) LowerArmLCS = RestCompBases[LIdx];
	if (HandLCS.ContainsNaN() && RestCompBases.IsValidIndex(HIdx))     HandLCS = RestCompBases[HIdx];
	if (RestCompBases.IsValidIndex(UIdx) && RestCompBases.IsValidIndex(LIdx) && RestCompBases.IsValidIndex(HIdx))
	{
		const FVector RefUpperDir = (RestCompBases[LIdx].GetLocation() - RestCompBases[UIdx].GetLocation()).GetSafeNormal();
		const FVector NewUpperDir = (LowerArmLCS.GetLocation() - UpperArmLCS.GetLocation()).GetSafeNormal();
		if (!RefUpperDir.IsNearlyZero() && !NewUpperDir.IsNearlyZero())
		{
			UpperArmLCS.SetRotation((SafeFindBetweenNormals(RefUpperDir, NewUpperDir) * RestCompBases[UIdx].GetRotation()).GetNormalized());
		}

		const FVector RefLowerDir = (RestCompBases[HIdx].GetLocation() - RestCompBases[LIdx].GetLocation()).GetSafeNormal();
		const FVector NewLowerDir = (HandLCS.GetLocation() - LowerArmLCS.GetLocation()).GetSafeNormal();
		if (!RefLowerDir.IsNearlyZero() && !NewLowerDir.IsNearlyZero())
		{
			const FQuat AlignLower = SafeFindBetweenNormals(RefLowerDir, NewLowerDir);
			LowerArmLCS.SetRotation((AlignLower * RestCompBases[LIdx].GetRotation()).GetNormalized());
			HandLCS.SetRotation((AlignLower * RestCompBases[HIdx].GetRotation()).GetNormalized());
		}
	}

	CSPose.Pose.SetComponentSpaceTransform(UpperArmLIdx, UpperArmLCS);
	CSPose.Pose.SetComponentSpaceTransform(LowerArmLIdx, LowerArmLCS);
	CSPose.Pose.SetComponentSpaceTransform(HandLIdx, HandLCS);
	PropagateHandChildBones(CSPose, RestCompBases, HandLIdx, HandLCS);
}

void FFPSAnimInstanceProxy::SolveLeftHandReloadIK(
	FComponentSpacePoseContext& CSPose,
	const TArray<FTransform>& RestCompBases)
{
	if (UpperArmLIdx == INDEX_NONE || LowerArmLIdx == INDEX_NONE || HandLIdx == INDEX_NONE || HandRIdx == INDEX_NONE || Spine03Idx == INDEX_NONE)
	{
		return;
	}

	const FTransform HandRCS = CSPose.Pose.GetComponentSpaceTransform(HandRIdx);
	const FTransform Spine03CS = CSPose.Pose.GetComponentSpaceTransform(Spine03Idx);
	const FVector HandRPos = HandRCS.GetLocation();
	const FVector ChestPos = Spine03CS.GetLocation();

	// Canted rifle barrel vector during reload
	const FVector CantedBarrelDir = (-CompRightDir * 0.42f + CompForwardDir * 0.86f + CompUpDir * 0.12f).GetSafeNormal();

	// Key spatial anchors in Component Space:
	const FVector ForegripPos = HandRPos + CantedBarrelDir * 30.0f;
	const FVector MagwellPos  = HandRPos + CantedBarrelDir * 14.5f - CompUpDir * 6.5f;
	const FVector MagPullPos  = MagwellPos - CompUpDir * 13.0f - CompRightDir * 3.0f;
	const FVector BeltPouchPos = BeltMag1TargetCS + CompUpDir * 4.0f + CompForwardDir * 2.0f; // Top grip of BeltMagComp_1
	const FVector MidArcPos   = FMath::Lerp(BeltPouchPos, MagPullPos, 0.5f) + CompForwardDir * 10.0f - CompRightDir * 4.0f;
	const FVector BoltSlapPos = MagwellPos + CompUpDir * 8.5f - CompRightDir * 4.0f;

	const float A = FMath::Clamp(ReloadProgressAlpha, 0.0f, 1.0f);
	FVector TargetHandL = ForegripPos;

	if (A < 0.20f)
	{
		// Stage 1 (0% - 20%): Move from foregrip to magwell & strip spent magazine downward
		const float SubT = FMath::Clamp(A / 0.20f, 0.0f, 1.0f);
		if (SubT < 0.5f)
		{
			TargetHandL = FMath::Lerp(ForegripPos, MagwellPos, FMath::SmoothStep(0.0f, 1.0f, SubT * 2.0f));
		}
		else
		{
			TargetHandL = FMath::Lerp(MagwellPos, MagPullPos, FMath::SmoothStep(0.0f, 1.0f, (SubT - 0.5f) * 2.0f));
		}
	}
	else if (A < 0.40f)
	{
		// Stage 2 (20% - 40%): Reach down to BeltMagComp_1 on front-left belt and grip fresh STANAG mag
		const float SubT = FMath::Clamp((A - 0.20f) / 0.20f, 0.0f, 1.0f);
		TargetHandL = FMath::Lerp(MagPullPos, BeltPouchPos, FMath::SmoothStep(0.0f, 1.0f, SubT));
	}
	else if (A < 0.75f)
	{
		// Stage 3 (40% - 75%): Extract fresh STANAG mag from belt pouch and guide it along a smooth arc into the magwell
		const float SubT = FMath::Clamp((A - 0.40f) / 0.35f, 0.0f, 1.0f);
		if (SubT < 0.45f)
		{
			const float T1 = FMath::SmoothStep(0.0f, 1.0f, SubT / 0.45f);
			TargetHandL = FMath::Lerp(BeltPouchPos, MidArcPos, T1);
		}
		else
		{
			const float T2 = FMath::SmoothStep(0.0f, 1.0f, (SubT - 0.45f) / 0.55f);
			TargetHandL = FMath::Lerp(MidArcPos, MagwellPos, T2);
		}
	}
	else if (A < 0.82f)
	{
		// Stage 4 (75% - 82%): Tap baseplate / bolt-catch slap
		const float SubT = FMath::Clamp((A - 0.75f) / 0.07f, 0.0f, 1.0f);
		TargetHandL = FMath::Lerp(MagwellPos, BoltSlapPos, FMath::Sin(SubT * PI));
	}
	else
	{
		// Stage 5 (82% - 100%): Return smoothly from magwell/bolt-catch to the rifle foregrip
		const float SubT = FMath::Clamp((A - 0.82f) / 0.18f, 0.0f, 1.0f);
		TargetHandL = FMath::Lerp(MagwellPos, ForegripPos, FMath::SmoothStep(0.0f, 1.0f, SubT));
	}

	const FVector JointTargetCS =
		ChestPos
		- CompRightDir * 30.0f
		- CompForwardDir * 6.0f
		- CompUpDir * 24.0f;

	FTransform UpperArmLCS = CSPose.Pose.GetComponentSpaceTransform(UpperArmLIdx);
	FTransform LowerArmLCS = CSPose.Pose.GetComponentSpaceTransform(LowerArmLIdx);
	FTransform HandLCS     = CSPose.Pose.GetComponentSpaceTransform(HandLIdx);

	AnimationCore::SolveTwoBoneIK(UpperArmLCS, LowerArmLCS, HandLCS, JointTargetCS, TargetHandL, false, 1.0f, 1.05f);

	const int32 UIdx = UpperArmLIdx.GetInt();
	const int32 LIdx = LowerArmLIdx.GetInt();
	const int32 HIdx = HandLIdx.GetInt();

	// Guard against degenerate Two-Bone IK transforms propagating NaNs to the GPU
	if (UpperArmLCS.ContainsNaN() && RestCompBases.IsValidIndex(UIdx)) UpperArmLCS = RestCompBases[UIdx];
	if (LowerArmLCS.ContainsNaN() && RestCompBases.IsValidIndex(LIdx)) LowerArmLCS = RestCompBases[LIdx];
	if (HandLCS.ContainsNaN() && RestCompBases.IsValidIndex(HIdx))     HandLCS = RestCompBases[HIdx];
	if (RestCompBases.IsValidIndex(UIdx) && RestCompBases.IsValidIndex(LIdx) && RestCompBases.IsValidIndex(HIdx))
	{
		const FVector RefUpperDir = (RestCompBases[LIdx].GetLocation() - RestCompBases[UIdx].GetLocation()).GetSafeNormal();
		const FVector NewUpperDir = (LowerArmLCS.GetLocation() - UpperArmLCS.GetLocation()).GetSafeNormal();
		if (!RefUpperDir.IsNearlyZero() && !NewUpperDir.IsNearlyZero())
		{
			UpperArmLCS.SetRotation((SafeFindBetweenNormals(RefUpperDir, NewUpperDir) * RestCompBases[UIdx].GetRotation()).GetNormalized());
		}

		const FVector RefLowerDir = (RestCompBases[HIdx].GetLocation() - RestCompBases[LIdx].GetLocation()).GetSafeNormal();
		const FVector NewLowerDir = (HandLCS.GetLocation() - LowerArmLCS.GetLocation()).GetSafeNormal();
		if (!RefLowerDir.IsNearlyZero() && !NewLowerDir.IsNearlyZero())
		{
			const FQuat AlignLower = SafeFindBetweenNormals(RefLowerDir, NewLowerDir);
			LowerArmLCS.SetRotation((AlignLower * RestCompBases[LIdx].GetRotation()).GetNormalized());
			HandLCS.SetRotation((AlignLower * RestCompBases[HIdx].GetRotation()).GetNormalized());
		}
	}

	CSPose.Pose.SetComponentSpaceTransform(UpperArmLIdx, UpperArmLCS);
	CSPose.Pose.SetComponentSpaceTransform(LowerArmLIdx, LowerArmLCS);
	CSPose.Pose.SetComponentSpaceTransform(HandLIdx, HandLCS);
	PropagateHandChildBones(CSPose, RestCompBases, HandLIdx, HandLCS);
}

void FFPSAnimInstanceProxy::SolveGrenadeThrowBeltIK(
	FComponentSpacePoseContext& CSPose,
	const TArray<FTransform>& RestCompBases)
{
	if (UpperArmRIdx == INDEX_NONE || LowerArmRIdx == INDEX_NONE || HandRIdx == INDEX_NONE ||
		UpperArmLIdx == INDEX_NONE || LowerArmLIdx == INDEX_NONE || HandLIdx == INDEX_NONE || Spine03Idx == INDEX_NONE)
	{
		return;
	}

	const FTransform Spine03CS = CSPose.Pose.GetComponentSpaceTransform(Spine03Idx);
	const FVector ChestPos = Spine03CS.GetLocation();

	const FVector BeltItemPos = (ActiveThrowItemType == EFPSBeltItemType::FragGrenade)
		? BeltFragTargetCS
		: BeltFlashTargetCS;

	const FVector RifleGripPos = ChestPos + CompRightDir * 11.0f + CompForwardDir * 22.0f + CompUpDir * 2.0f;
	const FVector PinPullPosR  = ChestPos + CompRightDir * 8.0f  + CompForwardDir * 24.0f + CompUpDir * 12.0f;
	const FVector CockBackPosR = ChestPos + CompRightDir * 22.0f - CompForwardDir * 8.0f  + CompUpDir * 24.0f;
	const FVector ReleasePosR  = ChestPos + CompRightDir * 10.0f + CompForwardDir * 44.0f + CompUpDir * 18.0f;

	const FVector CradleHandL  = ChestPos - CompRightDir * 10.0f + CompForwardDir * 22.0f - CompUpDir * 4.0f;
	const FVector PinPullPosL  = PinPullPosR - CompRightDir * 5.0f + CompForwardDir * 2.0f + CompUpDir * 2.0f;

	const float T = FMath::Clamp(ThrowProgressAlpha, 0.0f, 1.0f);
	FVector TargetHandR = RifleGripPos;
	FVector TargetHandL = CradleHandL;

	if (T < 0.25f)
	{
		// Stage 1 (0% - 25%): Right hand reaches down to BeltFragComp or BeltFlashComp on belt
		const float SubT = FMath::SmoothStep(0.0f, 1.0f, T / 0.25f);
		TargetHandR = FMath::Lerp(RifleGripPos, BeltItemPos, SubT);
		TargetHandL = CradleHandL;
	}
	else if (T < 0.45f)
	{
		// Stage 2 (25% - 45%): Lift unclipped grenade to chest/ear while left hand reaches up to pull pin
		const float SubT = FMath::SmoothStep(0.0f, 1.0f, (T - 0.25f) / 0.20f);
		TargetHandR = FMath::Lerp(BeltItemPos, PinPullPosR, SubT);
		TargetHandL = FMath::Lerp(CradleHandL, PinPullPosL, FMath::Sin(SubT * PI));
	}
	else if (T < 0.60f)
	{
		// Stage 3 (45% - 60%): Cock back & hurl grenade forward (projectile spawns at 55%)
		const float SubT = (T - 0.45f) / 0.15f;
		if (SubT < 0.4f)
		{
			TargetHandR = FMath::Lerp(PinPullPosR, CockBackPosR, FMath::SmoothStep(0.0f, 1.0f, SubT / 0.4f));
		}
		else
		{
			TargetHandR = FMath::Lerp(CockBackPosR, ReleasePosR, FMath::SmoothStep(0.0f, 1.0f, (SubT - 0.4f) / 0.6f));
		}
		TargetHandL = CradleHandL;
	}
	else
	{
		// Stage 4 (60% - 100%): Return right hand smoothly to the rifle pistol grip
		const float SubT = FMath::SmoothStep(0.0f, 1.0f, (T - 0.60f) / 0.40f);
		TargetHandR = FMath::Lerp(ReleasePosR, RifleGripPos, SubT);
		TargetHandL = CradleHandL;
	}

	// Solve Right Arm IK
	const FVector PoleR = ChestPos + CompRightDir * 30.0f - CompForwardDir * 14.0f - CompUpDir * 14.0f;
	FTransform UpperArmRCS = CSPose.Pose.GetComponentSpaceTransform(UpperArmRIdx);
	FTransform LowerArmRCS = CSPose.Pose.GetComponentSpaceTransform(LowerArmRIdx);
	FTransform HandRCS     = CSPose.Pose.GetComponentSpaceTransform(HandRIdx);
	AnimationCore::SolveTwoBoneIK(UpperArmRCS, LowerArmRCS, HandRCS, PoleR, TargetHandR, false, 1.0f, 1.05f);

	const int32 UR = UpperArmRIdx.GetInt();
	const int32 LR = LowerArmRIdx.GetInt();
	const int32 HR = HandRIdx.GetInt();

	if (UpperArmRCS.ContainsNaN() && RestCompBases.IsValidIndex(UR)) UpperArmRCS = RestCompBases[UR];
	if (LowerArmRCS.ContainsNaN() && RestCompBases.IsValidIndex(LR)) LowerArmRCS = RestCompBases[LR];
	if (HandRCS.ContainsNaN() && RestCompBases.IsValidIndex(HR))     HandRCS = RestCompBases[HR];
	if (RestCompBases.IsValidIndex(UR) && RestCompBases.IsValidIndex(LR) && RestCompBases.IsValidIndex(HR))
	{
		const FVector RefUDir = (RestCompBases[LR].GetLocation() - RestCompBases[UR].GetLocation()).GetSafeNormal();
		const FVector NewUDir = (LowerArmRCS.GetLocation() - UpperArmRCS.GetLocation()).GetSafeNormal();
		if (!RefUDir.IsNearlyZero() && !NewUDir.IsNearlyZero())
		{
			UpperArmRCS.SetRotation((SafeFindBetweenNormals(RefUDir, NewUDir) * RestCompBases[UR].GetRotation()).GetNormalized());
		}
		const FVector RefLDir = (RestCompBases[HR].GetLocation() - RestCompBases[LR].GetLocation()).GetSafeNormal();
		const FVector NewLDir = (HandRCS.GetLocation() - LowerArmRCS.GetLocation()).GetSafeNormal();
		if (!RefLDir.IsNearlyZero() && !NewLDir.IsNearlyZero())
		{
			const FQuat AlignL = SafeFindBetweenNormals(RefLDir, NewLDir);
			LowerArmRCS.SetRotation((AlignL * RestCompBases[LR].GetRotation()).GetNormalized());
			HandRCS.SetRotation((AlignL * RestCompBases[HR].GetRotation()).GetNormalized());
		}
	}
	CSPose.Pose.SetComponentSpaceTransform(UpperArmRIdx, UpperArmRCS);
	CSPose.Pose.SetComponentSpaceTransform(LowerArmRIdx, LowerArmRCS);
	CSPose.Pose.SetComponentSpaceTransform(HandRIdx, HandRCS);
	PropagateHandChildBones(CSPose, RestCompBases, HandRIdx, HandRCS);

	// Solve Left Arm IK
	const FVector PoleL = ChestPos - CompRightDir * 28.0f - CompForwardDir * 10.0f - CompUpDir * 22.0f;
	FTransform UpperArmLCS = CSPose.Pose.GetComponentSpaceTransform(UpperArmLIdx);
	FTransform LowerArmLCS = CSPose.Pose.GetComponentSpaceTransform(LowerArmLIdx);
	FTransform HandLCS     = CSPose.Pose.GetComponentSpaceTransform(HandLIdx);
	AnimationCore::SolveTwoBoneIK(UpperArmLCS, LowerArmLCS, HandLCS, PoleL, TargetHandL, false, 1.0f, 1.05f);

	const int32 UL = UpperArmLIdx.GetInt();
	const int32 LL = LowerArmLIdx.GetInt();
	const int32 HL = HandLIdx.GetInt();

	if (UpperArmLCS.ContainsNaN() && RestCompBases.IsValidIndex(UL)) UpperArmLCS = RestCompBases[UL];
	if (LowerArmLCS.ContainsNaN() && RestCompBases.IsValidIndex(LL)) LowerArmLCS = RestCompBases[LL];
	if (HandLCS.ContainsNaN() && RestCompBases.IsValidIndex(HL))     HandLCS = RestCompBases[HL];
	if (RestCompBases.IsValidIndex(UL) && RestCompBases.IsValidIndex(LL) && RestCompBases.IsValidIndex(HL))
	{
		const FVector RefUDir = (RestCompBases[LL].GetLocation() - RestCompBases[UL].GetLocation()).GetSafeNormal();
		const FVector NewUDir = (LowerArmLCS.GetLocation() - UpperArmLCS.GetLocation()).GetSafeNormal();
		if (!RefUDir.IsNearlyZero() && !NewUDir.IsNearlyZero())
		{
			UpperArmLCS.SetRotation((SafeFindBetweenNormals(RefUDir, NewUDir) * RestCompBases[UL].GetRotation()).GetNormalized());
		}
		const FVector RefLDir = (RestCompBases[HL].GetLocation() - RestCompBases[LL].GetLocation()).GetSafeNormal();
		const FVector NewLDir = (HandLCS.GetLocation() - LowerArmLCS.GetLocation()).GetSafeNormal();
		if (!RefLDir.IsNearlyZero() && !NewLDir.IsNearlyZero())
		{
			const FQuat AlignL = SafeFindBetweenNormals(RefLDir, NewLDir);
			LowerArmLCS.SetRotation((AlignL * RestCompBases[LL].GetRotation()).GetNormalized());
			HandLCS.SetRotation((AlignL * RestCompBases[HL].GetRotation()).GetNormalized());
		}
	}
	CSPose.Pose.SetComponentSpaceTransform(UpperArmLIdx, UpperArmLCS);
	CSPose.Pose.SetComponentSpaceTransform(LowerArmLIdx, LowerArmLCS);
	CSPose.Pose.SetComponentSpaceTransform(HandLIdx, HandLCS);
	PropagateHandChildBones(CSPose, RestCompBases, HandLIdx, HandLCS);
}
