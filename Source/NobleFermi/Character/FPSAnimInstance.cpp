#include "Character/FPSAnimInstance.h"
#include "Character/FPSCharacter.h"
#include "Character/FPSCharacterPreviewActor.h"
#include "Character/FPSCharacterMovementComponent.h"
#include "BodyMechanics/FPSBodyMechanicsComponent.h"
#include "Inventory/FPSInventoryComponent.h"
#include "Weapons/FPSWeaponBase.h"
#include "TwoBoneIK.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/KismetMathLibrary.h"

UFPSAnimInstance::UFPSAnimInstance()
{
}

void UFPSAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	APawn* PawnOwner = TryGetPawnOwner();
	if (PawnOwner)
	{
		OwningCharacter = Cast<AFPSCharacter>(PawnOwner);
		if (OwningCharacter)
		{
			OwningMovement = OwningCharacter->GetFPSMovementComponent();
			OwningBodyMechanics = OwningCharacter->GetBodyMechanicsComponent();
		}
	}
	else if (AActor* ActorOwner = GetOwningActor())
	{
		if (Cast<AFPSCharacterPreviewActor>(ActorOwner))
		{
			bIsMenuPreview = true;
		}
	}

	CacheBoneIndices();
}

void UFPSAnimInstance::CacheBoneIndices()
{
	USkeletalMeshComponent* SkelMesh = GetOwningComponent();
	if (!SkelMesh || !SkelMesh->GetSkeletalMeshAsset())
	{
		return;
	}

	Pelvis_Index = SkelMesh->GetBoneIndex(FName(TEXT("pelvis")));
	Spine01_Index = SkelMesh->GetBoneIndex(FName(TEXT("spine_01")));
	Spine02_Index = SkelMesh->GetBoneIndex(FName(TEXT("spine_02")));
	Spine03_Index = SkelMesh->GetBoneIndex(FName(TEXT("spine_03")));
	Neck01_Index = SkelMesh->GetBoneIndex(FName(TEXT("neck_01")));
	Head_Index = SkelMesh->GetBoneIndex(FName(TEXT("head")));

	ClavicleL_Index = SkelMesh->GetBoneIndex(FName(TEXT("clavicle_l")));
	ClavicleR_Index = SkelMesh->GetBoneIndex(FName(TEXT("clavicle_r")));
	UpperArmL_Index = SkelMesh->GetBoneIndex(FName(TEXT("upperarm_l")));
	UpperArmR_Index = SkelMesh->GetBoneIndex(FName(TEXT("upperarm_r")));
	LowerArmL_Index = SkelMesh->GetBoneIndex(FName(TEXT("lowerarm_l")));
	LowerArmR_Index = SkelMesh->GetBoneIndex(FName(TEXT("lowerarm_r")));
	HandL_Index = SkelMesh->GetBoneIndex(FName(TEXT("hand_l")));
	HandR_Index = SkelMesh->GetBoneIndex(FName(TEXT("hand_r")));

	ThighL_Index = SkelMesh->GetBoneIndex(FName(TEXT("thigh_l")));
	ThighR_Index = SkelMesh->GetBoneIndex(FName(TEXT("thigh_r")));
	CalfL_Index = SkelMesh->GetBoneIndex(FName(TEXT("calf_l")));
	CalfR_Index = SkelMesh->GetBoneIndex(FName(TEXT("calf_r")));
	FootL_Index = SkelMesh->GetBoneIndex(FName(TEXT("foot_l")));
	FootR_Index = SkelMesh->GetBoneIndex(FName(TEXT("foot_r")));

	LeftFingerBoneIndices.Reset();
	RightFingerBoneIndices.Reset();
	const FReferenceSkeleton& RefSkeleton = SkelMesh->GetSkeletalMeshAsset()->GetRefSkeleton();
	const int32 NumBones = RefSkeleton.GetNum();
	for (int32 i = 0; i < NumBones; ++i)
	{
		const int32 ParentIdx = RefSkeleton.GetParentIndex(i);
		if (ParentIdx == HandL_Index || (ParentIdx != INDEX_NONE && LeftFingerBoneIndices.Contains(ParentIdx)))
		{
			LeftFingerBoneIndices.Add(i);
		}
		if (ParentIdx == HandR_Index || (ParentIdx != INDEX_NONE && RightFingerBoneIndices.Contains(ParentIdx)))
		{
			RightFingerBoneIndices.Add(i);
		}
	}

	RestCompBases.SetNum(NumBones);
	for (int32 i = 0; i < NumBones; ++i)
	{
		const int32 ParentIdx = RefSkeleton.GetParentIndex(i);
		if (ParentIdx == INDEX_NONE)
		{
			RestCompBases[i] = RefSkeleton.GetRefBonePose()[i];
		}
		else
		{
			RestCompBases[i] = RefSkeleton.GetRefBonePose()[i] * RestCompBases[ParentIdx];
		}
	}

	bBoneIndicesCached = true;
}

void UFPSAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!bBoneIndicesCached)
	{
		CacheBoneIndices();
	}

	if (!OwningCharacter)
	{
		APawn* PawnOwner = TryGetPawnOwner();
		if (PawnOwner)
		{
			OwningCharacter = Cast<AFPSCharacter>(PawnOwner);
			if (OwningCharacter)
			{
				OwningMovement = OwningCharacter->GetFPSMovementComponent();
				OwningBodyMechanics = OwningCharacter->GetBodyMechanicsComponent();
			}
		}
	}

	// Menu Preview Actor Path (3D Hangar Diorama Turntable Operator)
	if (!OwningCharacter)
	{
		if (AFPSCharacterPreviewActor* PreviewOwner = Cast<AFPSCharacterPreviewActor>(GetOwningActor()))
		{
			bIsMenuPreview = true;
			MenuPoseIndex = PreviewOwner->GetPreviewStance(); // 0 = Low-Ready Carbine, 1 = High-Ready ADS, 2 = Patrol Walk

			// 0.25 Hz slow, authoritative chest respiration
			BreathingPhase += DeltaSeconds * (0.25f * 2.0f * PI);
			if (BreathingPhase > 2.0f * PI)
			{
				BreathingPhase -= 2.0f * PI;
			}

			if (MenuPoseIndex == 1) // High-Ready ADS
			{
				bIsAiming = true;
				CurrentADSAlpha = FMath::FInterpTo(CurrentADSAlpha, 1.0f, DeltaSeconds, 8.0f);
				bIsMoving = false;
				GroundSpeed = 0.0f;
				AimPitch = 0.0f;
				AimYaw = 0.0f;
				LocomotionPhase = FMath::FInterpTo(LocomotionPhase, 0.0f, DeltaSeconds, 6.0f);
			}
			else if (MenuPoseIndex == 2) // Combat Patrol Walk
			{
				bIsAiming = false;
				CurrentADSAlpha = FMath::FInterpTo(CurrentADSAlpha, 0.0f, DeltaSeconds, 8.0f);
				bIsMoving = true;
				GroundSpeed = 240.0f;
				AimPitch = -2.0f;
				AimYaw = 0.0f;
				LocomotionPhase += DeltaSeconds * 1.4f * 2.0f * PI;
				if (LocomotionPhase > 2.0f * PI)
				{
					LocomotionPhase -= 2.0f * PI;
				}
			}
			else // Default (0): Authoritative Tactical Low-Ready Carbine Stance (matching menu-prop.jpeg)
			{
				bIsAiming = false;
				CurrentADSAlpha = FMath::FInterpTo(CurrentADSAlpha, 0.0f, DeltaSeconds, 8.0f);
				bIsMoving = false;
				GroundSpeed = 0.0f;
				AimPitch = -3.0f; // Subtle -3 deg authoritative chin tuck
				AimYaw = 0.0f;
				LocomotionPhase = FMath::FInterpTo(LocomotionPhase, 0.0f, DeltaSeconds, 6.0f);
			}
		}
		return;
	}

	// 1. Ground Locomotion Telemetry Aligned with UFPSCharacterMovementComponent
	const FVector Velocity = OwningCharacter->GetVelocity();
	GroundSpeed = Velocity.Size2D();
	bIsMoving = GroundSpeed > 15.0f;

	if (bIsMoving)
	{
		const FRotator ActorRot = OwningCharacter->GetActorRotation();
		Direction = UKismetMathLibrary::NormalizedDeltaRotator(Velocity.Rotation(), ActorRot).Yaw;
	}
	else
	{
		Direction = 0.0f;
	}

	// 2. Authoritative Movement States
	if (OwningMovement)
	{
		bIsSprinting = OwningMovement->IsTacticalSprinting();
		bIsCrouching = OwningMovement->IsCrouching();
		bIsFalling = OwningMovement->IsFalling();
		bIsSliding = OwningMovement->IsSliding();
		bIsBallisticDiving = OwningMovement->IsBallisticDiving();
	}

	// 3. Gait & Breathing Phase Accumulation
	float StrideFreq = 0.0f;
	if (bIsSprinting)
	{
		StrideFreq = 3.6f;
	}
	else if (bIsSliding)
	{
		StrideFreq = 1.0f;
	}
	else if (bIsMoving)
	{
		StrideFreq = 2.4f * FMath::Clamp(GroundSpeed / 450.0f, 0.2f, 1.2f);
	}

	if (StrideFreq > 0.0f)
	{
		LocomotionPhase += DeltaSeconds * StrideFreq * 2.0f * PI;
		if (LocomotionPhase > 2.0f * PI)
		{
			LocomotionPhase -= 2.0f * PI;
		}
	}
	else
	{
		LocomotionPhase = FMath::FInterpTo(LocomotionPhase, 0.0f, DeltaSeconds, 6.0f);
	}

	BreathingPhase += DeltaSeconds * 1.5f;
	if (BreathingPhase > 2.0f * PI)
	{
		BreathingPhase -= 2.0f * PI;
	}

	// 4. Aiming & View Rotation
	AFPSWeaponBase* ActiveWeapon = nullptr;
	if (OwningCharacter->GetInventoryComponent())
	{
		ActiveWeapon = OwningCharacter->GetInventoryComponent()->GetActiveWeapon();
		bIsAiming = ActiveWeapon && ActiveWeapon->IsAimingDownSights();
		CurrentADSAlpha = ActiveWeapon ? ActiveWeapon->GetADSAlpha() : 0.0f;
	}
	else
	{
		bIsAiming = false;
		CurrentADSAlpha = 0.0f;
	}

	// Aim pitch normalized between -89 and +89 degrees
	const FRotator ControlRot = OwningCharacter->GetControlRotation();
	AimPitch = FRotator::NormalizeAxis(ControlRot.Pitch);
	AimYaw = FRotator::NormalizeAxis(ControlRot.Yaw - OwningCharacter->GetActorRotation().Yaw);
	ShoulderSideMultiplier = OwningCharacter->ShoulderSideMultiplier;

	// In ADS, blend weight is 1.0 (tight upper-body lock)
	UpperBodyBlendWeight = bIsAiming ? 1.0f : 0.85f;

	// 5. Procedural Spine Lean & Aim Punch Flinch
	if (OwningBodyMechanics)
	{
		ProceduralSpineLean = OwningBodyMechanics->GetProceduralSpineRotation();
		AimPunchRotation = OwningBodyMechanics->GetCurrentAimPunch();
		LeftFootIKOffset = OwningBodyMechanics->GetLeftFootIKOffset();
		RightFootIKOffset = OwningBodyMechanics->GetRightFootIKOffset();
		PelvisIKOffset = OwningBodyMechanics->GetPelvisIKOffset();
	}

	// 6. Update Left-Hand IK Target from active weapon
	UpdateIKTargetFromWeapon(ActiveWeapon);
}

void UFPSAnimInstance::UpdateIKTargetFromWeapon(AFPSWeaponBase* Weapon)
{
	USkeletalMeshComponent* SkelMesh = GetOwningComponent();
	if (!SkelMesh)
	{
		bLeftHandIKValid = false;
		return;
	}

	if (Weapon)
	{
		const FVector ForegripWorld = Weapon->GetForegripWorldLocation();
		const FTransform MeshCompToWorld = SkelMesh->GetComponentTransform();
		LeftHandIKLocation = MeshCompToWorld.InverseTransformPosition(ForegripWorld);
		bLeftHandIKValid = true;
	}
	else
	{
		bLeftHandIKValid = false;
	}
}

void UFPSAnimInstance::NativePostEvaluateAnimation()
{
	Super::NativePostEvaluateAnimation();

	USkeletalMeshComponent* SkelMesh = GetOwningComponent();
	if (!SkelMesh)
	{
		return;
	}

	TArray<FTransform>& SpaceBases = SkelMesh->GetEditableComponentSpaceTransforms();
	if (SpaceBases.Num() == 0 || RestCompBases.Num() == 0)
	{
		return;
	}

	// In-game character uses authoritative mocap animation sequences on SingleNode mode.
	// Only apply procedural posture IK when in menu diorama preview.
	if (bIsMenuPreview)
	{
		ApplyLowerBodyLocomotion(SpaceBases);
		ApplySpineAimPitchAndTwist(SpaceBases);
		SolveRightArmRifleIK(SpaceBases);
		SolveLeftHandTwoBoneIK(SpaceBases);
	}
}

void UFPSAnimInstance::ApplyLowerBodyLocomotion(TArray<FTransform>& SpaceBases)
{
	if (!SpaceBases.IsValidIndex(Pelvis_Index) || !SpaceBases.IsValidIndex(ThighL_Index) ||
		!SpaceBases.IsValidIndex(ThighR_Index) || !SpaceBases.IsValidIndex(CalfL_Index) ||
		!SpaceBases.IsValidIndex(CalfR_Index) || !SpaceBases.IsValidIndex(FootL_Index) ||
		!SpaceBases.IsValidIndex(FootR_Index))
	{
		return;
	}

	const float sL = FMath::Sin(LocomotionPhase);
	const float cL = FMath::Cos(LocomotionPhase);
	const float sR = -sL;
	const float cR = -cL;

	const float MoveAlpha = FMath::Clamp(GroundSpeed / 450.0f, 0.0f, 1.0f);
	const float SprintAlpha = bIsSprinting ? 1.0f : 0.0f;

	// Pelvis Displacement and Angles
	FVector PelvisDeltaLoc = FVector::ZeroVector;
	FRotator PelvisDeltaRot = FRotator::ZeroRotator;

	float ThighPitchL = 0.0f;
	float ThighPitchR = 0.0f;
	float KneeBendL = 0.0f;
	float KneeBendR = 0.0f;
	float FootPitchL = 0.0f;
	float FootPitchR = 0.0f;

	if (bIsBallisticDiving)
	{
		// Ballistic Dive: Pelvis pitch forward ~70 deg, drop down
		PelvisDeltaLoc = FVector(15.0f, 0.0f, -55.0f);
		PelvisDeltaRot = FRotator(-70.0f, 0.0f, 0.0f);
		ThighPitchL = 25.0f;
		ThighPitchR = 25.0f;
		KneeBendL = 10.0f;
		KneeBendR = 10.0f;
	}
	else if (bIsSliding)
	{
		// Tactical Knee Slide: Reclined torso, lead leg forward, trail leg folded
		PelvisDeltaLoc = FVector(-10.0f, 0.0f, -48.0f);
		PelvisDeltaRot = FRotator(18.0f, -12.0f, 0.0f);
		ThighPitchL = -75.0f;
		KneeBendL = 16.0f;
		FootPitchL = 14.0f;
		ThighPitchR = 20.0f;
		KneeBendR = 85.0f;
		FootPitchR = -25.0f;
	}
	else if (bIsCrouching)
	{
		// Low Tactical Crouch Stance: Pelvis drop -38cm, legs folded in support
		PelvisDeltaLoc = FVector(0.0f, 0.0f, -38.0f);
		PelvisDeltaRot = FRotator(-8.0f, -10.0f, 0.0f);
		if (bIsMoving)
		{
			PelvisDeltaLoc.Z += -2.0f * FMath::Cos(2.0f * LocomotionPhase);
			ThighPitchL = -50.0f - 14.0f * cL;
			ThighPitchR = -50.0f - 14.0f * cR;
			KneeBendL = 60.0f + ((sL > 0.0f) ? (22.0f * sL) : (8.0f * -sL));
			KneeBendR = 60.0f + ((sR > 0.0f) ? (22.0f * sR) : (8.0f * -sR));
			FootPitchL = -6.0f + 8.0f * cL;
			FootPitchR = -6.0f + 8.0f * cR;
		}
		else
		{
			ThighPitchL = -68.0f;
			KneeBendL = 78.0f;
			FootPitchL = -10.0f;
			ThighPitchR = 14.0f;
			KneeBendR = 88.0f;
			FootPitchR = -28.0f;
		}
	}
	else if (bIsMoving)
	{
		// Walk / Athletic Run / Tactical Sprint
		const float BobZ = -2.5f * MoveAlpha * (1.0f + 0.8f * SprintAlpha) * FMath::Cos(2.0f * LocomotionPhase);
		const float SwayY = 1.8f * MoveAlpha * sL;
		PelvisDeltaLoc = FVector(0.0f, SwayY, BobZ);
		PelvisDeltaRot = FRotator(3.0f * MoveAlpha, -5.0f * MoveAlpha * cL, 2.0f * MoveAlpha * sL);

		const float ThighAmp = FMath::Lerp(22.0f, 48.0f, SprintAlpha) * MoveAlpha;
		const float KneeAmp = FMath::Lerp(35.0f, 65.0f, SprintAlpha) * MoveAlpha;

		ThighPitchL = -ThighAmp * cL;
		ThighPitchR = -ThighAmp * cR;
		KneeBendL = (sL > 0.0f) ? (KneeAmp * sL) : (8.0f * MoveAlpha);
		KneeBendR = (sR > 0.0f) ? (KneeAmp * sR) : (8.0f * MoveAlpha);
		FootPitchL = 12.0f * cL * MoveAlpha;
		FootPitchR = 12.0f * cR * MoveAlpha;
	}
	else if (bIsMenuPreview && MenuPoseIndex == 0)
	{
		// Grounded Authoritative Shoulder-Width Low-Ready Menu Stance (matching menu-prop.jpeg)
		PelvisDeltaRot = FRotator(-0.8f, -4.0f, 0.0f);
		ThighPitchL = -5.0f;
		KneeBendL = 9.0f;
		FootPitchL = -4.0f;
		ThighPitchR = 4.0f;
		KneeBendR = 8.0f;
		FootPitchR = -8.0f;
	}
	else
	{
		// Idle Bladed Tactical Stance
		PelvisDeltaRot = FRotator(-1.5f, -8.0f, 0.0f);
		ThighPitchL = -10.0f;
		KneeBendL = 15.0f;
		FootPitchL = -5.0f;
		ThighPitchR = 8.0f;
		KneeBendR = 14.0f;
		FootPitchR = -14.0f;
	}

	// Apply to Pelvis
	FTransform& PelvisTrans = SpaceBases[Pelvis_Index];
	const FQuat PelvisDeltaQ = PelvisDeltaRot.Quaternion();
	PelvisTrans.SetLocation(RestCompBases[Pelvis_Index].GetLocation() + PelvisDeltaLoc);
	PelvisTrans.SetRotation(PelvisDeltaQ * RestCompBases[Pelvis_Index].GetRotation());

	// Forward Kinematics for Left Leg: Thigh -> Calf -> Foot
	{
		const FVector RestThighRelPelvis = RestCompBases[ThighL_Index].GetLocation() - RestCompBases[Pelvis_Index].GetLocation();
		const FVector ThighPivot = PelvisTrans.GetLocation() + PelvisDeltaQ.RotateVector(RestThighRelPelvis);
		const FQuat Q_Thigh = FQuat(FVector::RightVector, FMath::DegreesToRadians(ThighPitchL));
		const FQuat ThighWorldRot = Q_Thigh * PelvisDeltaQ * RestCompBases[ThighL_Index].GetRotation();
		SpaceBases[ThighL_Index].SetLocation(ThighPivot);
		SpaceBases[ThighL_Index].SetRotation(ThighWorldRot);

		const FVector RestCalfRelThigh = RestCompBases[CalfL_Index].GetLocation() - RestCompBases[ThighL_Index].GetLocation();
		const FVector CalfPivot = ThighPivot + (Q_Thigh * PelvisDeltaQ).RotateVector(RestCalfRelThigh);
		const FQuat Q_Knee = FQuat(FVector::RightVector, FMath::DegreesToRadians(KneeBendL));
		const FQuat CalfWorldRot = Q_Knee * ThighWorldRot;
		SpaceBases[CalfL_Index].SetLocation(CalfPivot);
		SpaceBases[CalfL_Index].SetRotation(CalfWorldRot);

		const FVector RestFootRelCalf = RestCompBases[FootL_Index].GetLocation() - RestCompBases[CalfL_Index].GetLocation();
		const FVector FootPivot = CalfPivot + (Q_Knee * Q_Thigh * PelvisDeltaQ).RotateVector(RestFootRelCalf);
		const FQuat Q_Foot = FQuat(FVector::RightVector, FMath::DegreesToRadians(FootPitchL));
		SpaceBases[FootL_Index].SetLocation(FootPivot);
		SpaceBases[FootL_Index].SetRotation(Q_Foot * CalfWorldRot);
	}

	// Forward Kinematics for Right Leg: Thigh -> Calf -> Foot
	{
		const FVector RestThighRelPelvis = RestCompBases[ThighR_Index].GetLocation() - RestCompBases[Pelvis_Index].GetLocation();
		const FVector ThighPivot = PelvisTrans.GetLocation() + PelvisDeltaQ.RotateVector(RestThighRelPelvis);
		const FQuat Q_Thigh = FQuat(FVector::RightVector, FMath::DegreesToRadians(ThighPitchR));
		const FQuat ThighWorldRot = Q_Thigh * PelvisDeltaQ * RestCompBases[ThighR_Index].GetRotation();
		SpaceBases[ThighR_Index].SetLocation(ThighPivot);
		SpaceBases[ThighR_Index].SetRotation(ThighWorldRot);

		const FVector RestCalfRelThigh = RestCompBases[CalfR_Index].GetLocation() - RestCompBases[ThighR_Index].GetLocation();
		const FVector CalfPivot = ThighPivot + (Q_Thigh * PelvisDeltaQ).RotateVector(RestCalfRelThigh);
		const FQuat Q_Knee = FQuat(FVector::RightVector, FMath::DegreesToRadians(KneeBendR));
		const FQuat CalfWorldRot = Q_Knee * ThighWorldRot;
		SpaceBases[CalfR_Index].SetLocation(CalfPivot);
		SpaceBases[CalfR_Index].SetRotation(CalfWorldRot);

		const FVector RestFootRelCalf = RestCompBases[FootR_Index].GetLocation() - RestCompBases[CalfR_Index].GetLocation();
		const FVector FootPivot = CalfPivot + (Q_Knee * Q_Thigh * PelvisDeltaQ).RotateVector(RestFootRelCalf);
		const FQuat Q_Foot = FQuat(FVector::RightVector, FMath::DegreesToRadians(FootPitchR));
		SpaceBases[FootR_Index].SetLocation(FootPivot);
		SpaceBases[FootR_Index].SetRotation(Q_Foot * CalfWorldRot);
	}
}

void UFPSAnimInstance::ApplySpineAimPitchAndTwist(TArray<FTransform>& SpaceBases)
{
	if (!SpaceBases.IsValidIndex(Spine01_Index) || !SpaceBases.IsValidIndex(Spine02_Index) ||
		!SpaceBases.IsValidIndex(Spine03_Index) || !SpaceBases.IsValidIndex(Neck01_Index) ||
		!SpaceBases.IsValidIndex(Head_Index))
	{
		return;
	}

	const float Breath = FMath::Sin(BreathingPhase) * (bIsMenuPreview ? 0.65f : 0.5f);
	const float EffectiveAimPitch = AimPitch + Breath;
	const float BaseTorsoYaw = (bIsMenuPreview && MenuPoseIndex == 0) ? 4.0f : (bIsAiming ? 8.0f : 14.0f);

	auto ApplyBoneDelta = [&](int32 BoneIdx, float PitchFraction, float YawFraction)
	{
		if (BoneIdx != INDEX_NONE && SpaceBases.IsValidIndex(BoneIdx))
		{
			const float SubPitch = EffectiveAimPitch * PitchFraction;
			const float SubYaw = BaseTorsoYaw * YawFraction;
			const FQuat DeltaQ = FQuat(FVector::UpVector, FMath::DegreesToRadians(SubYaw)) *
			                     FQuat(FVector::RightVector, FMath::DegreesToRadians(-SubPitch));
			FTransform& BoneTrans = SpaceBases[BoneIdx];
			BoneTrans.SetRotation(DeltaQ * BoneTrans.GetRotation());
		}
	};

	ApplyBoneDelta(Spine01_Index, 0.15f, 0.30f);
	ApplyBoneDelta(Spine02_Index, 0.30f, 0.40f);
	ApplyBoneDelta(Spine03_Index, 0.35f, 0.30f);
	ApplyBoneDelta(Neck01_Index, 0.10f, 0.0f);
	ApplyBoneDelta(Head_Index, 0.10f, 0.0f);

	// Propagate Spine03 transform to Clavicles so shoulders follow thoracic pitch, breathing, and twist
	if (SpaceBases.IsValidIndex(ClavicleR_Index) && SpaceBases.IsValidIndex(Spine03_Index))
	{
		const FVector Spine03Loc = SpaceBases[Spine03_Index].GetLocation();
		const FVector RestClavRelSpine = RestCompBases[ClavicleR_Index].GetLocation() - RestCompBases[Spine03_Index].GetLocation();
		const FQuat Spine03Rot = SpaceBases[Spine03_Index].GetRotation() * RestCompBases[Spine03_Index].GetRotation().Inverse();
		SpaceBases[ClavicleR_Index].SetLocation(Spine03Loc + Spine03Rot.RotateVector(RestClavRelSpine));
		SpaceBases[ClavicleR_Index].SetRotation(Spine03Rot * RestCompBases[ClavicleR_Index].GetRotation());
	}
	if (SpaceBases.IsValidIndex(ClavicleL_Index) && SpaceBases.IsValidIndex(Spine03_Index))
	{
		const FVector Spine03Loc = SpaceBases[Spine03_Index].GetLocation();
		const FVector RestClavRelSpine = RestCompBases[ClavicleL_Index].GetLocation() - RestCompBases[Spine03_Index].GetLocation();
		const FQuat Spine03Rot = SpaceBases[Spine03_Index].GetRotation() * RestCompBases[Spine03_Index].GetRotation().Inverse();
		SpaceBases[ClavicleL_Index].SetLocation(Spine03Loc + Spine03Rot.RotateVector(RestClavRelSpine));
		SpaceBases[ClavicleL_Index].SetRotation(Spine03Rot * RestCompBases[ClavicleL_Index].GetRotation());
	}
}

void UFPSAnimInstance::SolveRightArmRifleIK(TArray<FTransform>& SpaceBases)
{
	USkeletalMeshComponent* SkelMesh = GetOwningComponent();
	if (!SkelMesh || RestCompBases.Num() == 0 ||
		!SpaceBases.IsValidIndex(UpperArmR_Index) || !SpaceBases.IsValidIndex(LowerArmR_Index) ||
		!SpaceBases.IsValidIndex(HandR_Index) || !SpaceBases.IsValidIndex(Head_Index) ||
		!SpaceBases.IsValidIndex(Pelvis_Index) || !SpaceBases.IsValidIndex(UpperArmL_Index))
	{
		return;
	}

	// 1. Authoritative Anatomical Coordinate Frame for SK_BaseMale (Component Space)
	// In SK_BaseMale rest skeleton: UpDir = +Z, RightDir = +X, ForwardDir = -Y (Facing out of chest)
	const FVector HeadRefCS = RestCompBases[Head_Index].GetLocation();
	const FVector PelvisRefCS = RestCompBases[Pelvis_Index].GetLocation();
	const FVector ShoulderR_RefCS = RestCompBases[UpperArmR_Index].GetLocation();
	const FVector ShoulderL_RefCS = RestCompBases[UpperArmL_Index].GetLocation();

	const FVector UpDir = (HeadRefCS - PelvisRefCS).GetSafeNormal();
	FVector RightDir = (ShoulderR_RefCS - ShoulderL_RefCS).GetSafeNormal();
	FVector ForwardDir = FVector::CrossProduct(RightDir, UpDir).GetSafeNormal();
	RightDir = FVector::CrossProduct(UpDir, ForwardDir).GetSafeNormal();
	ForwardDir = FVector::CrossProduct(RightDir, UpDir).GetSafeNormal();

	const FVector ShoulderR_Pos = SpaceBases[UpperArmR_Index].GetLocation();
	const float BreathOffsetZ = FMath::Sin(BreathingPhase) * 0.45f;

	FVector HandR_Target;
	const bool bUseLowReadyPose = (bIsMenuPreview && MenuPoseIndex != 1) || (!bIsMenuPreview && !bIsAiming && CurrentADSAlpha < 0.05f);

	if (bUseLowReadyPose)
	{
		// Authoritative Tactical Low-Ready Stance:
		// Right hand gripping rifle pistol grip in front of right pectoral / lower chest
		HandR_Target = ShoulderR_Pos + ForwardDir * 24.0f - RightDir * 5.0f - UpDir * (15.0f - BreathOffsetZ);
	}
	else
	{
		// High-Ready / Aimed ADS Stance
		HandR_Target = ShoulderR_Pos + ForwardDir * 32.0f - RightDir * 6.0f - UpDir * 10.0f;
		if (bIsAiming || CurrentADSAlpha > 0.01f)
		{
			const FVector HandR_ADS = ShoulderR_Pos + ForwardDir * 30.0f - RightDir * 14.0f + UpDir * 3.5f;
			HandR_Target = FMath::Lerp(HandR_Target, HandR_ADS, CurrentADSAlpha);
		}
		HandR_Target += UpDir * (AimPitch * 0.25f);
	}

	// 2. Anatomical Right Elbow Pole Vector (downward, backward, slightly outward)
	const FVector ElbowPoleR = ShoulderR_Pos - UpDir * 30.0f - ForwardDir * 12.0f + RightDir * 14.0f;

	// 3. Solve Two-Bone IK using AnimationCore::SolveTwoBoneIK
	FTransform UpperArmR_Trans = SpaceBases[UpperArmR_Index];
	FTransform LowerArmR_Trans = SpaceBases[LowerArmR_Index];
	FTransform HandR_Trans = SpaceBases[HandR_Index];

	AnimationCore::SolveTwoBoneIK(UpperArmR_Trans, LowerArmR_Trans, HandR_Trans, ElbowPoleR, HandR_Target, false, 1.0, 1.2);

	SpaceBases[UpperArmR_Index] = UpperArmR_Trans;
	SpaceBases[LowerArmR_Index] = LowerArmR_Trans;
	SpaceBases[HandR_Index] = HandR_Trans;

	// 4. Orient Hand along Rifle Grip Axis
	FVector HandAimDir;
	if (bUseLowReadyPose)
	{
		// Diagonal Low-Ready barrel axis (~36 deg downward pitch toward left hip/thigh)
		HandAimDir = (ForwardDir * 0.65f - RightDir * 0.45f - UpDir * 0.60f).GetSafeNormal();
	}
	else
	{
		const FQuat AimDeltaQ = FQuat(UpDir, FMath::DegreesToRadians(AimYaw)) * FQuat(RightDir, FMath::DegreesToRadians(-AimPitch));
		HandAimDir = AimDeltaQ.RotateVector(ForwardDir);
	}

	const FVector RefLowerDirR = (RestCompBases[HandR_Index].GetLocation() - RestCompBases[LowerArmR_Index].GetLocation()).GetSafeNormal();
	const FQuat DeltaHandQ_R = FQuat::FindBetweenNormals(RefLowerDirR, HandAimDir);
	SpaceBases[HandR_Index].SetRotation((DeltaHandQ_R * RestCompBases[HandR_Index].GetRotation()).GetNormalized());

	RightHandIKLocation = SpaceBases[HandR_Index].GetLocation();
	RightHandIKRotation = HandAimDir.Rotation();

	// 5. Propagate Hand Transform to Finger Bones
	for (int32 FingerIndex : RightFingerBoneIndices)
	{
		if (SpaceBases.IsValidIndex(FingerIndex))
		{
			const FTransform RelToHand = RestCompBases[FingerIndex].GetRelativeTransform(RestCompBases[HandR_Index]);
			SpaceBases[FingerIndex] = RelToHand * SpaceBases[HandR_Index];
		}
	}
}

void UFPSAnimInstance::SolveLeftHandTwoBoneIK(TArray<FTransform>& SpaceBases)
{
	USkeletalMeshComponent* SkelMesh = GetOwningComponent();
	if (!SkelMesh || RestCompBases.Num() == 0 ||
		!SpaceBases.IsValidIndex(UpperArmL_Index) || !SpaceBases.IsValidIndex(LowerArmL_Index) ||
		!SpaceBases.IsValidIndex(HandL_Index) || !SpaceBases.IsValidIndex(Head_Index) ||
		!SpaceBases.IsValidIndex(Pelvis_Index) || !SpaceBases.IsValidIndex(UpperArmR_Index))
	{
		return;
	}

	// 1. Authoritative Anatomical Coordinate Frame for SK_BaseMale (Component Space)
	const FVector HeadRefCS = RestCompBases[Head_Index].GetLocation();
	const FVector PelvisRefCS = RestCompBases[Pelvis_Index].GetLocation();
	const FVector ShoulderR_RefCS = RestCompBases[UpperArmR_Index].GetLocation();
	const FVector ShoulderL_RefCS = RestCompBases[UpperArmL_Index].GetLocation();

	const FVector UpDir = (HeadRefCS - PelvisRefCS).GetSafeNormal();
	FVector RightDir = (ShoulderR_RefCS - ShoulderL_RefCS).GetSafeNormal();
	FVector ForwardDir = FVector::CrossProduct(RightDir, UpDir).GetSafeNormal();
	RightDir = FVector::CrossProduct(UpDir, ForwardDir).GetSafeNormal();
	ForwardDir = FVector::CrossProduct(RightDir, UpDir).GetSafeNormal();

	const FVector ShoulderL_Pos = SpaceBases[UpperArmL_Index].GetLocation();
	const bool bUseLowReadyPose = (bIsMenuPreview && MenuPoseIndex != 1) || (!bIsMenuPreview && !bIsAiming && CurrentADSAlpha < 0.05f);

	FVector HandL_Target;
	if (bUseLowReadyPose)
	{
		// Left hand wraps around the Carbine's M-LOK handguard 27 cm down-barrel along the diagonal Low-Ready axis
		const FVector DiagonalBarrelDir = (ForwardDir * 0.65f - RightDir * 0.45f - UpDir * 0.60f).GetSafeNormal();
		HandL_Target = RightHandIKLocation + DiagonalBarrelDir * 27.0f;
	}
	else
	{
		// High-Ready / Aimed ADS Stance
		const FQuat AimDeltaQ = FQuat(UpDir, FMath::DegreesToRadians(AimYaw)) * FQuat(RightDir, FMath::DegreesToRadians(-AimPitch));
		const FVector AimBarrelDir = AimDeltaQ.RotateVector(ForwardDir);
		HandL_Target = RightHandIKLocation + AimBarrelDir * 27.0f - RightDir * 1.5f + UpDir * 2.0f;
	}

	// 2. Anatomical Left Elbow Pole Vector (downward, forward, slightly outward)
	const FVector ElbowPoleL = ShoulderL_Pos - UpDir * 30.0f + ForwardDir * 12.0f - RightDir * 14.0f;

	// 3. Solve Two-Bone IK using AnimationCore::SolveTwoBoneIK
	FTransform UpperArmL_Trans = SpaceBases[UpperArmL_Index];
	FTransform LowerArmL_Trans = SpaceBases[LowerArmL_Index];
	FTransform HandL_Trans = SpaceBases[HandL_Index];

	AnimationCore::SolveTwoBoneIK(UpperArmL_Trans, LowerArmL_Trans, HandL_Trans, ElbowPoleL, HandL_Target, false, 1.0, 1.2);

	SpaceBases[UpperArmL_Index] = UpperArmL_Trans;
	SpaceBases[LowerArmL_Index] = LowerArmL_Trans;
	SpaceBases[HandL_Index] = HandL_Trans;

	// 4. Orient Left Hand grasping handguard
	const FVector RefLowerDirL = (RestCompBases[HandL_Index].GetLocation() - RestCompBases[LowerArmL_Index].GetLocation()).GetSafeNormal();
	const FVector HandAimDirL = (HandL_Target - RightHandIKLocation).GetSafeNormal();
	const FQuat DeltaHandQ_L = FQuat::FindBetweenNormals(RefLowerDirL, HandAimDirL);
	SpaceBases[HandL_Index].SetRotation((DeltaHandQ_L * RestCompBases[HandL_Index].GetRotation()).GetNormalized());

	LeftHandIKLocation = SpaceBases[HandL_Index].GetLocation();
	LeftHandIKRotation = HandAimDirL.Rotation();

	// 5. Propagate Hand Transform to Finger Bones
	for (int32 FingerIndex : LeftFingerBoneIndices)
	{
		if (SpaceBases.IsValidIndex(FingerIndex))
		{
			const FTransform RelToHand = RestCompBases[FingerIndex].GetRelativeTransform(RestCompBases[HandL_Index]);
			SpaceBases[FingerIndex] = RelToHand * SpaceBases[HandL_Index];
		}
	}
}
