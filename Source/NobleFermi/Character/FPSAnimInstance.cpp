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
#include "Misc/CommandLine.h"

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

	static void CalculateUnifiedRiflePose(
		const FVector& ChestPos,
		const FVector& CompUpDir,
		const FVector& CompRightDir,
		const FVector& CompForwardDir,
		float HighReadyAlpha,
		float SmoothADSAlpha,
		float AimPitch,
		bool bIsInspectingWeapon,
		float WeaponInspectionAlpha,
		bool bUnarmedRelaxedIdle,
		float IdleBreathPhase,
		float RecoilImpulseAlpha,
		FVector& OutReceiverOriginCS,
		FQuat& OutRifleQuatCS)
	{
		const FVector LowReadyCradleDir = (
			-CompRightDir * 0.58f
			+ CompForwardDir * 0.76f
			- CompUpDir * 0.28f
		).GetSafeNormal();

		const FVector SteadyInspectDir = (
			-CompRightDir * 0.58f
			+ CompForwardDir * 0.76f
			- CompUpDir * 0.20f
		).GetSafeNormal();

		const float PitchRad = FMath::DegreesToRadians(FMath::Clamp(AimPitch, -65.0f, 65.0f));
		const FVector HighReadyBarrelDir = (
			-CompRightDir * (0.18f * (1.0f - SmoothADSAlpha))
			+ CompForwardDir * FMath::Cos(PitchRad)
			+ CompUpDir * FMath::Sin(PitchRad)
		).GetSafeNormal();

		const bool bInspecting = (bIsInspectingWeapon && HighReadyAlpha < 0.25f && !bUnarmedRelaxedIdle);
		const float W = bInspecting ? FMath::Clamp(WeaponInspectionAlpha, 0.0f, 1.0f) : 0.0f;

		float SteadyInspectBlend = 0.0f;
		float CantRollBlend = 0.0f;
		if (bInspecting)
		{
			if (W < 0.08f)
			{
				SteadyInspectBlend = FMath::SmoothStep(0.0f, 1.0f, W / 0.08f);
			}
			else if (W < 0.82f)
			{
				SteadyInspectBlend = 1.0f;
			}
			else if (W < 0.90f)
			{
				SteadyInspectBlend = 1.0f - FMath::SmoothStep(0.0f, 1.0f, (W - 0.82f) / 0.08f);
			}

			if (W < 0.10f)
			{
				CantRollBlend = FMath::SmoothStep(0.0f, 1.0f, W / 0.10f);
			}
			else if (W < 0.46f)
			{
				CantRollBlend = 1.0f;
			}
			else if (W < 0.56f)
			{
				CantRollBlend = 1.0f - FMath::SmoothStep(0.0f, 1.0f, (W - 0.46f) / 0.10f);
			}
		}

		const FVector BaseLowDir = FMath::Lerp(LowReadyCradleDir, SteadyInspectDir, SteadyInspectBlend).GetSafeNormal();
		FVector BarrelDirCS = FMath::Lerp(BaseLowDir, HighReadyBarrelDir, HighReadyAlpha).GetSafeNormal();
		if (BarrelDirCS.ContainsNaN() || BarrelDirCS.IsNearlyZero(1e-3f))
		{
			BarrelDirCS = SteadyInspectDir;
		}

		FVector RightVecCS = FVector::CrossProduct(CompUpDir, BarrelDirCS).GetSafeNormal();
		if (RightVecCS.ContainsNaN() || RightVecCS.IsNearlyZero(1e-3f))
		{
			RightVecCS = CompRightDir;
		}
		const FVector OrthoUpCS = FVector::CrossProduct(BarrelDirCS, RightVecCS).GetSafeNormal();
		OutRifleQuatCS = FRotationMatrix::MakeFromXZ(BarrelDirCS, OrthoUpCS).ToQuat();
		if (CantRollBlend > 0.01f)
		{
			const FQuat CantRollQuat(BarrelDirCS, FMath::DegreesToRadians(-22.0f * CantRollBlend));
			OutRifleQuatCS = (CantRollQuat * OutRifleQuatCS).GetNormalized();
		}

		const float BreathOffset = FMath::Sin(IdleBreathPhase) * 0.45f;
		const float RecoilBack = RecoilImpulseAlpha * 2.8f;

		// Exact right hand grip position across stances
		const FVector LowReadyHandR =
			ChestPos
			+ CompRightDir * 12.0f
			+ CompForwardDir * 16.5f
			+ CompUpDir * (-5.5f + BreathOffset);

		const FVector HighReadyHandR =
			ChestPos
			+ CompRightDir * (11.0f - SmoothADSAlpha * 4.5f)
			+ CompForwardDir * (24.0f - SmoothADSAlpha * 3.0f - RecoilBack)
			+ CompUpDir * (4.5f + SmoothADSAlpha * 3.5f + BreathOffset * 0.4f);

		const FVector ArmedHandR = FMath::Lerp(LowReadyHandR, HighReadyHandR, HighReadyAlpha);

		// Pistol grip right palm web anchor is local (-10.56f, 2.2f, -7.04f) in receiver space.
		// Therefore: ReceiverOrigin = HandR + RifleQuat * (10.56f, -2.2f, 7.04f) ensures exact grip-to-hand alignment.
		OutReceiverOriginCS = ArmedHandR + OutRifleQuatCS.RotateVector(FVector(10.56f, -2.2f, 7.04f));

		if (bInspecting)
		{
			float TapJoltZ = 0.0f;
			if (W >= 0.41f && W <= 0.47f)
			{
				TapJoltZ = FMath::Sin(((W - 0.41f) / 0.06f) * PI) * 1.5f;
			}
			OutReceiverOriginCS += CompUpDir * TapJoltZ;
		}
	}

	static FVector PushOutOfSolidWeapon(
		const FVector& InPt,
		float Radius,
		bool bIsLeftHand,
		int32 LeftHandMode,
		const FString& BoneName)
	{
		FVector P = InPt;

		// 1. Handguard: X in [13.5 .. 33.0], Y in [-3.05 .. 3.05], Z in [-5.5 .. 7.5]
		// Underside surface is Z = -5.5 cm. Top rail is Z = +7.5 cm.
		if (P.X >= 13.5f - Radius && P.X <= 33.0f + Radius &&
			P.Y >= -3.05f - Radius && P.Y <= 3.05f + Radius &&
			P.Z >= -5.5f - Radius && P.Z <= 7.5f + Radius)
		{
			if (bIsLeftHand && LeftHandMode == 0)
			{
				// Foregrip cradle underside: palm and fingers cup beneath the underside
				if (P.Z < 0.0f)
				{
					P.Z = FMath::Min(P.Z, -5.5f - Radius);
				}
				else
				{
					if (P.Y >= 0.0f) P.Y = FMath::Max(P.Y, 3.05f + Radius);
					else P.Y = FMath::Min(P.Y, -3.05f - Radius);
				}
			}
			else if (bIsLeftHand && LeftHandMode == 2)
			{
				// Barrel shroud wipe: palm rests over top rail
				P.Z = FMath::Max(P.Z, 7.5f + Radius);
			}
			else
			{
				const float dMinZ = P.Z - (-5.5f - Radius);
				const float dMaxZ = (7.5f + Radius) - P.Z;
				const float dMinY = P.Y - (-3.05f - Radius);
				const float dMaxY = (3.05f + Radius) - P.Y;
				const float MinD = FMath::Min(FMath::Min(dMinZ, dMaxZ), FMath::Min(dMinY, dMaxY));
				if (MinD == dMinZ) P.Z = -5.5f - Radius;
				else if (MinD == dMaxZ) P.Z = 7.5f + Radius;
				else if (MinD == dMinY) P.Y = -3.05f - Radius;
				else P.Y = 3.05f + Radius;
			}
		}

		// 2. Receiver Main Body: X in [-16.0 .. 14.0], Y in [-2.7 .. 2.7], Z in [-4.5 .. 4.0]
		if (P.X >= -16.0f - Radius && P.X <= 14.0f + Radius &&
			P.Y >= -2.7f - Radius && P.Y <= 2.7f + Radius &&
			P.Z >= -4.5f - Radius && P.Z <= 4.0f + Radius)
		{
			if (!bIsLeftHand)
			{
				if (BoneName.Contains(TEXT("index")))
				{
					// Extended trigger finger resting along right receiver body in trigger discipline
					P.Y = FMath::Max(P.Y, 2.7f + Radius);
					P.Z = FMath::Clamp(P.Z, -4.0f, -2.5f);
				}
				else if (BoneName.Contains(TEXT("thumb")))
				{
					// Thumb on left flank
					P.Y = FMath::Min(P.Y, -2.7f - Radius);
				}
				else
				{
					P.Y = FMath::Max(P.Y, 2.7f + Radius);
				}
			}
			else
			{
				const float dMinY = P.Y - (-2.7f - Radius);
				const float dMaxY = (2.7f + Radius) - P.Y;
				if (dMinY < dMaxY) P.Y = -2.7f - Radius;
				else P.Y = 2.7f + Radius;
			}
		}

		// 3. Pistol Grip: Z in [-12.5 .. -4.5], centered at X = -10.56 cm
		if (P.Z >= -12.5f - Radius && P.Z <= -4.5f + Radius)
		{
			// Grip center slopes slightly along backstrap
			const float CenterX = -10.56f + 0.20f * (P.Z + 8.5f);
			const float HalfThickX = 2.0f + Radius;
			const float HalfWidthY = 1.6f + Radius;

			if (P.X >= CenterX - HalfThickX && P.X <= CenterX + HalfThickX &&
				P.Y >= -HalfWidthY && P.Y <= HalfWidthY)
			{
				if (!bIsLeftHand)
				{
					if (BoneName.Contains(TEXT("thumb")))
					{
						P.Y = FMath::Min(P.Y, -1.6f - Radius);
					}
					else if (BoneName.Contains(TEXT("index")))
					{
						// Index finger should NOT be down on the grip; push above grip into receiver flank
						P.Z = FMath::Max(P.Z, -4.5f + Radius);
						P.Y = FMath::Max(P.Y, 1.6f + Radius);
					}
					else if (BoneName.Contains(TEXT("hand_r")))
					{
						// Palm sits against right flank
						P.Y = FMath::Max(P.Y, 1.6f + Radius);
					}
					else
					{
						// Middle, Ring, Pinky wrap around front strap (+X)
						P.X = FMath::Max(P.X, CenterX + HalfThickX);
					}
				}
			}
		}

		// 4. Trigger Guard Barrier: X in [-8.5 .. 0.5], Y in [-0.8 .. 0.8], Z in [-6.5 .. -4.0]
		if (P.X >= -8.5f - Radius && P.X <= 0.5f + Radius &&
			P.Y >= -0.8f - Radius && P.Y <= 0.8f + Radius &&
			P.Z >= -6.5f - Radius && P.Z <= -4.0f + Radius)
		{
			if (!bIsLeftHand)
			{
				if (BoneName.Contains(TEXT("index")))
				{
					P.Y = FMath::Max(P.Y, 1.0f + Radius);
					P.Z = FMath::Max(P.Z, -4.0f + Radius);
				}
				else
				{
					P.Z = FMath::Min(P.Z, -6.5f - Radius);
				}
			}
		}

		// 5. Seated Magazine: X in [0.0 .. 7.5], Y in [-1.2 .. 1.2], Z in [-21.5 .. -4.0]
		if (P.X >= 0.0f - Radius && P.X <= 7.5f + Radius &&
			P.Y >= -1.2f - Radius && P.Y <= 1.2f + Radius &&
			P.Z >= -21.5f - Radius && P.Z <= -4.0f + Radius)
		{
			if (bIsLeftHand && LeftHandMode == 1)
			{
				if (BoneName.Contains(TEXT("thumb")))
				{
					P.Y = FMath::Min(P.Y, -1.2f - Radius);
				}
				else
				{
					if (P.X > 5.5f) P.X = FMath::Max(P.X, 7.5f + Radius);
					else P.Y = FMath::Max(P.Y, 1.2f + Radius);
				}
			}
			else
			{
				// Right hand bones must NEVER penetrate the magazine
				if (!bIsLeftHand)
				{
					P.X = FMath::Min(P.X, 0.0f - Radius);
				}
				else
				{
					const float dMinY = P.Y - (-1.2f - Radius);
					const float dMaxY = (1.2f + Radius) - P.Y;
					if (dMinY < dMaxY) P.Y = -1.2f - Radius;
					else P.Y = 1.2f + Radius;
				}
			}
		}

		return P;
	}
}

void UFPSAnimInstance::ComputeUnifiedRiflePoseCS(
	const FVector& ChestPos,
	FVector& OutReceiverOriginCS,
	FQuat& OutRifleQuatCS) const
{
	CalculateUnifiedRiflePose(
		ChestPos,
		Proxy.CompUpDir,
		Proxy.CompRightDir,
		Proxy.CompForwardDir,
		HighReadyAlpha,
		SmoothADSAlpha,
		AimPitch,
		bIsInspectingWeapon,
		WeaponInspectionAlpha,
		bUnarmedRelaxedIdle,
		IdleBreathPhase,
		RecoilImpulseAlpha,
		OutReceiverOriginCS,
		OutRifleQuatCS);
}

void FFPSAnimInstanceProxy::ComputeUnifiedRiflePoseCS(
	const FVector& ChestPos,
	FVector& OutReceiverOriginCS,
	FQuat& OutRifleQuatCS) const
{
	CalculateUnifiedRiflePose(
		ChestPos,
		CompUpDir,
		CompRightDir,
		CompForwardDir,
		HighReadyAlpha,
		SmoothADSAlpha,
		AimPitch,
		bIsInspectingWeapon,
		WeaponInspectionAlpha,
		bUnarmedRelaxedIdle,
		IdleBreathPhase,
		RecoilImpulseAlpha,
		OutReceiverOriginCS,
		OutRifleQuatCS);
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
		// Hangar preview mode: grounded battle-ready operator holding primary rifle
		bIsMenuPreview = true;
		bUnarmedRelaxedIdle = false;
		bForceHighReady = (MenuPoseIndex == 1);
		bIsADS = (MenuPoseIndex == 1);
		Speed = (MenuPoseIndex == 2) ? 215.0f : 0.0f;

		InspectionCycleTimer += DeltaSeconds;
		float ForcedInspectTime = -1.0f;
		if (FParse::Value(FCommandLine::Get(), TEXT("InspectTime="), ForcedInspectTime) && ForcedInspectTime >= 0.0f)
		{
			InspectionCycleTimer = ForcedInspectTime;
		}
		if (MenuPoseIndex == 0)
		{
			// 12.0s cycle: [0.15s .. 5.55s] (5.4s) Mag-Check & Barrel-Wipe inspection -> [5.55s .. 12.0s] Grounded Battle-Ready Low-Ready
			const float CycleT = FMath::Fmod(InspectionCycleTimer, 12.0f);
			if (CycleT >= 0.15f && CycleT <= 5.55f)
			{
				bIsInspectingWeapon = true;
				WeaponInspectionAlpha = FMath::Clamp((CycleT - 0.15f) / 5.40f, 0.0f, 1.0f);
			}
			else
			{
				bIsInspectingWeapon = false;
				WeaponInspectionAlpha = 0.0f;
			}
		}
		else
		{
			bIsInspectingWeapon = false;
			WeaponInspectionAlpha = 0.0f;
		}

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
			// Show the 3D STANAG magazine with brass 5.56mm rounds in the left palm during magazine inspection while keeping belt pouches seated (-1.0f sentinel)
			const bool bShowInspectMag = bIsInspectingWeapon && (WeaponInspectionAlpha >= 0.11f && WeaponInspectionAlpha <= 0.41f);
			OwningBeltGear->UpdateReloadBeltGrabState(bShowInspectMag, bShowInspectMag ? -1.0f : 0.0f, true);
			OwningBeltGear->UpdateThrowBeltGrabState(false, 0.0f, EFPSBeltItemType::FragGrenade, true, true);
		}

		const float SeatedAlpha = FMath::Clamp(1.0f - FMath::Max(HighReadyAlpha, Speed / 220.0f * 4.0f), 0.0f, 1.0f);
		const FVector RefChestPos = Proxy.CompUpDir * 142.0f
			- Proxy.CompForwardDir * (14.5f * SeatedAlpha)
			- Proxy.CompUpDir * (12.0f * SeatedAlpha);
		ComputeUnifiedRiflePoseCS(RefChestPos, CurrentReceiverOriginCS, CurrentRifleQuatCS);

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

	const FVector RefChestPos = Proxy.CompUpDir * 142.0f - Proxy.CompUpDir * (20.0f * SmoothCrouchAlpha);
	ComputeUnifiedRiflePoseCS(RefChestPos, CurrentReceiverOriginCS, CurrentRifleQuatCS);
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
		bIsInspectingWeapon = FPSAnim->bIsInspectingWeapon;
		WeaponInspectionAlpha = FPSAnim->WeaponInspectionAlpha;
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
		CurrentReceiverOriginCS = FPSAnim->CurrentReceiverOriginCS;
		CurrentRifleQuatCS = FPSAnim->CurrentRifleQuatCS;
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
	Index01LIdx  = FindBoneCompactIndex(BoneContainer, RefSkel, { TEXT("index_01_l"), TEXT("LeftHandIndex1"), TEXT("mixamorig:LeftHandIndex1") });
	Middle01LIdx = FindBoneCompactIndex(BoneContainer, RefSkel, { TEXT("middle_01_l"), TEXT("LeftHandMiddle1"), TEXT("mixamorig:LeftHandMiddle1") });
	Pinky01LIdx  = FindBoneCompactIndex(BoneContainer, RefSkel, { TEXT("pinky_01_l"), TEXT("LeftHandPinky1"), TEXT("mixamorig:LeftHandPinky1") });

	ClavicleRIdx = FindBoneCompactIndex(BoneContainer, RefSkel, { TEXT("clavicle_r"), TEXT("RightShoulder"), TEXT("mixamorig:RightShoulder") });
	UpperArmRIdx = FindBoneCompactIndex(BoneContainer, RefSkel, { TEXT("upperarm_r"), TEXT("RightArm"), TEXT("mixamorig:RightArm") });
	LowerArmRIdx = FindBoneCompactIndex(BoneContainer, RefSkel, { TEXT("lowerarm_r"), TEXT("RightForeArm"), TEXT("mixamorig:RightForeArm") });
	HandRIdx     = FindBoneCompactIndex(BoneContainer, RefSkel, { TEXT("hand_r"), TEXT("RightHand"), TEXT("mixamorig:RightHand") });
	Index01RIdx  = FindBoneCompactIndex(BoneContainer, RefSkel, { TEXT("index_01_r"), TEXT("RightHandIndex1"), TEXT("mixamorig:RightHandIndex1") });
	Middle01RIdx = FindBoneCompactIndex(BoneContainer, RefSkel, { TEXT("middle_01_r"), TEXT("RightHandMiddle1"), TEXT("mixamorig:RightHandMiddle1") });
	Pinky01RIdx  = FindBoneCompactIndex(BoneContainer, RefSkel, { TEXT("pinky_01_r"), TEXT("RightHandPinky1"), TEXT("mixamorig:RightHandPinky1") });
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

		CompForwardDir = FVector::CrossProduct(CompUpDir, CompRightDir).GetSafeNormal();
		if (CompForwardDir.IsNearlyZero()) CompForwardDir = FVector(0.0f, -1.0f, 0.0f);

		// Guarantee CompForwardDir points out of the chest (towards negative Y in SK_BaseMale component space)
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

	// Update dynamic ReceiverOriginCS and RifleQuatCS from evaluated thoracic chest position
	const FVector EvaluatedChestPos = (Spine03Idx != INDEX_NONE)
		? CSPose.Pose.GetComponentSpaceTransform(Spine03Idx).GetLocation()
		: (CompUpDir * 142.0f);
	ComputeUnifiedRiflePoseCS(EvaluatedChestPos, CurrentReceiverOriginCS, CurrentRifleQuatCS);

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
		if (HandRIdx != INDEX_NONE && CurrentRifleQuatCS.IsNormalized())
		{
			const FVector EvaluatedHandRPos = CSPose.Pose.GetComponentSpaceTransform(HandRIdx).GetLocation();
			CurrentReceiverOriginCS = EvaluatedHandRPos + CurrentRifleQuatCS.RotateVector(FVector(10.56f, -2.2f, 7.04f));
		}
		if (UFPSAnimInstance* AnimInst = Cast<UFPSAnimInstance>(GetAnimInstanceObject()))
		{
			AnimInst->CurrentReceiverOriginCS = CurrentReceiverOriginCS;
			AnimInst->CurrentRifleQuatCS = CurrentRifleQuatCS;
		}
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
	const float IdleWeightAlpha = (1.0f - MoveAlpha) * (1.0f - SmoothCrouchAlpha);

	// Hangar Stance 0: Operator sits comfortably on the stacked Belgian Duvel beer crates with 100% planted zero-slip boots!
	const float SeatedAlpha = bIsMenuPreview
		? FMath::Clamp(1.0f - FMath::Max(HighReadyAlpha, MoveAlpha * 4.0f), 0.0f, 1.0f)
		: 0.0f;

	// 1. Pelvis Transform: 100% Zero-Slip Stationary Base (zero Pelvis sway/bob when MoveAlpha == 0 so boots NEVER slide!)
	FTransform PelvisCS = CSPose.Pose.GetComponentSpaceTransform(PelvisIdx);
	FVector PelvisPos = PelvisCS.GetLocation();
	FQuat PelvisRot = PelvisCS.GetRotation();

	// Shift pelvis back over the stacked Duvel crates when seated, or lower for crouch
	PelvisPos -= CompForwardDir * (14.5f * SeatedAlpha);
	PelvisPos += CompUpDir * (-20.0f * SmoothCrouchAlpha);

	const float StandingYawDeg = 7.5f * (1.0f - SmoothADSAlpha * 0.4f) * (1.0f - SeatedAlpha) * IdleWeightAlpha;
	const FQuat StanceYawQ(CompUpDir, FMath::DegreesToRadians(StandingYawDeg));
	PelvisRot = (StanceYawQ * PelvisRot).GetNormalized();

	if (MoveAlpha > 0.01f)
	{
		const float WalkBob = FMath::Sin(LocomotionCyclePhase * 2.0f) * 2.2f * MoveAlpha;
		const float WalkSway = FMath::Cos(LocomotionCyclePhase) * 1.6f * MoveAlpha;
		PelvisPos += CompUpDir * WalkBob + CompRightDir * WalkSway;
	}

	PelvisCS.SetLocation(PelvisPos);
	PelvisCS.SetRotation(PelvisRot);
	CSPose.Pose.SetComponentSpaceTransform(PelvisIdx, PelvisCS);

	// 2. Axis-Agnostic Seated Duvel Crate Posture, Standing Combat Stance & Walk Stride
	//    HipFlexAxis = Cross(-CompUpDir, CompForwardDir): positive angle ALWAYS swings thigh forward (+CompForwardDir).
	//    KneeAbductAxis = Cross(CompForwardDir, CompRightDir): positive angle ALWAYS swings knee toward +CompRightDir.
	const FVector HipFlexAxis = FVector::CrossProduct(-CompUpDir, CompForwardDir).GetSafeNormal();
	const FVector KneeAbductAxis = FVector::CrossProduct(CompForwardDir, CompRightDir).GetSafeNormal();

	const float SinL = FMath::Sin(LocomotionCyclePhase);
	const float SinR = FMath::Sin(LocomotionCyclePhase + PI);

	const float WalkHipPitchL = FMath::DegreesToRadians(SinL * 22.0f * MoveAlpha);
	const float WalkKneeFlexL = FMath::DegreesToRadians(-FMath::Max(0.0f, -SinL) * 34.0f * MoveAlpha);
	const float WalkHipPitchR = FMath::DegreesToRadians(SinR * 22.0f * MoveAlpha);
	const float WalkKneeFlexR = FMath::DegreesToRadians(-FMath::Max(0.0f, -SinR) * 34.0f * MoveAlpha);

	// Blend between Standing Combat Stance (+9 deg thigh / -15.5 deg calf) and Seated Duvel Crate Stance (+68 deg thigh / -64 deg calf)
	const float TargetThighFlexDegL = FMath::Lerp(9.0f, 68.0f, SeatedAlpha) * IdleWeightAlpha;
	const float TargetKneeFlexDegL  = FMath::Lerp(-15.5f, -64.0f, SeatedAlpha) * IdleWeightAlpha;
	const float TargetThighFlexDegR = FMath::Lerp(7.5f, 65.0f, SeatedAlpha) * IdleWeightAlpha;
	const float TargetKneeFlexDegR  = FMath::Lerp(-15.0f, -61.0f, SeatedAlpha) * IdleWeightAlpha;

	const float IdleThighPitchL = FMath::DegreesToRadians(TargetThighFlexDegL);
	const float IdleKneeFlexL   = FMath::DegreesToRadians(TargetKneeFlexDegL);
	const float IdleThighPitchR = FMath::DegreesToRadians(TargetThighFlexDegR);
	const float IdleKneeFlexR   = FMath::DegreesToRadians(TargetKneeFlexDegR);

	// Outward knee spread so knees sit naturally apart in a relaxed tactical posture on the Duvel crate
	const float AbductDegL = FMath::Lerp(-5.0f, -11.0f, SeatedAlpha) * IdleWeightAlpha;
	const float AbductDegR = FMath::Lerp(10.0f, 13.0f, SeatedAlpha) * IdleWeightAlpha;
	const FQuat LeftYawQ(KneeAbductAxis, FMath::DegreesToRadians(AbductDegL));
	const FQuat RightYawQ(KneeAbductAxis, FMath::DegreesToRadians(AbductDegR));

	// Hinge both hip and knee in the outward-angled vertical plane of each leg so shins hang straight down with zero sideways tilt
	const FVector HipFlexAxisL = LeftYawQ.RotateVector(HipFlexAxis).GetSafeNormal();
	const FVector HipFlexAxisR = RightYawQ.RotateVector(HipFlexAxis).GetSafeNormal();

	const FQuat ThighLDelta = FQuat(HipFlexAxisL, WalkHipPitchL + IdleThighPitchL) * LeftYawQ;
	const FQuat CalfLDelta  = FQuat(HipFlexAxisL, WalkKneeFlexL + IdleKneeFlexL);

	const FQuat ThighRDelta = FQuat(HipFlexAxisR, WalkHipPitchR + IdleThighPitchR) * RightYawQ;
	const FQuat CalfRDelta  = FQuat(HipFlexAxisR, WalkKneeFlexR + IdleKneeFlexR);

	PropagateSingleChildBoneCS(CSPose, RestCompBases, PelvisIdx, ThighLIdx, ThighLDelta);
	PropagateSingleChildBoneCS(CSPose, RestCompBases, ThighLIdx, CalfLIdx, CalfLDelta);
	PropagateSingleChildBoneCS(CSPose, RestCompBases, CalfLIdx, FootLIdx);

	PropagateSingleChildBoneCS(CSPose, RestCompBases, PelvisIdx, ThighRIdx, ThighRDelta);
	PropagateSingleChildBoneCS(CSPose, RestCompBases, ThighRIdx, CalfRIdx, CalfRDelta);
	PropagateSingleChildBoneCS(CSPose, RestCompBases, CalfRIdx, FootRIdx);

	// 3. Exact Ground-Plane Lock & Infinite-Friction Boot Sole Pinning:
	//    Measure the vertical ankle lift from knee/hip flexion and drop the pelvis & leg chains so both boot soles
	//    sit 100% flat at ground level (Z = 0) with 0.000 mm horizontal or vertical drift!
	const int32 FLIdx = FootLIdx.GetInt();
	const int32 FRIdx = FootRIdx.GetInt();
	if (RestCompBases.IsValidIndex(FLIdx) && RestCompBases.IsValidIndex(FRIdx))
	{
		FTransform FootLCS = CSPose.Pose.GetComponentSpaceTransform(FootLIdx);
		FTransform FootRCS = CSPose.Pose.GetComponentSpaceTransform(FootRIdx);

		if (IdleWeightAlpha > 0.01f)
		{
			const float LiftL = ((FootLCS.GetLocation() - RestCompBases[FLIdx].GetLocation()) | CompUpDir) * IdleWeightAlpha;
			const float LiftR = ((FootRCS.GetLocation() - RestCompBases[FRIdx].GetLocation()) | CompUpDir) * IdleWeightAlpha;
			const float AvgLift = 0.5f * (LiftL + LiftR);

			// Drop Pelvis onto the Duvel crate top surface (Z = 48 cm) and lock both ankles to the turntable floor
			PelvisCS.AddToTranslation(-CompUpDir * AvgLift);
			CSPose.Pose.SetComponentSpaceTransform(PelvisIdx, PelvisCS);

			FTransform ThighLCS = CSPose.Pose.GetComponentSpaceTransform(ThighLIdx);
			FTransform CalfLCS  = CSPose.Pose.GetComponentSpaceTransform(CalfLIdx);
			ThighLCS.AddToTranslation(-CompUpDir * LiftL);
			CalfLCS.AddToTranslation(-CompUpDir * LiftL);
			FootLCS.AddToTranslation(-CompUpDir * LiftL);
			CSPose.Pose.SetComponentSpaceTransform(ThighLIdx, ThighLCS);
			CSPose.Pose.SetComponentSpaceTransform(CalfLIdx, CalfLCS);

			FTransform ThighRCS = CSPose.Pose.GetComponentSpaceTransform(ThighRIdx);
			FTransform CalfRCS  = CSPose.Pose.GetComponentSpaceTransform(CalfRIdx);
			ThighRCS.AddToTranslation(-CompUpDir * LiftR);
			CalfRCS.AddToTranslation(-CompUpDir * LiftR);
			FootRCS.AddToTranslation(-CompUpDir * LiftR);
			CSPose.Pose.SetComponentSpaceTransform(ThighRIdx, ThighRCS);
			CSPose.Pose.SetComponentSpaceTransform(CalfRIdx, CalfRCS);
		}

		// Lock both boot soles 100% parallel to the horizontal deck
		const FQuat FootYawL(KneeAbductAxis, FMath::DegreesToRadians(AbductDegL));
		const FQuat FootYawR(KneeAbductAxis, FMath::DegreesToRadians(AbductDegR));
		const FQuat FootWalkPitchL(HipFlexAxisL, -(WalkHipPitchL + WalkKneeFlexL) * 0.65f * MoveAlpha);
		const FQuat FootWalkPitchR(HipFlexAxisR, -(WalkHipPitchR + WalkKneeFlexR) * 0.65f * MoveAlpha);

		FootLCS.SetRotation((FootWalkPitchL * FootYawL * RestCompBases[FLIdx].GetRotation()).GetNormalized());
		FootRCS.SetRotation((FootWalkPitchR * FootYawR * RestCompBases[FRIdx].GetRotation()).GetNormalized());

		CSPose.Pose.SetComponentSpaceTransform(FootLIdx, FootLCS);
		CSPose.Pose.SetComponentSpaceTransform(FootRIdx, FootRCS);

		PropagateHandChildBones(CSPose, RestCompBases, FootLIdx, FootLCS);
		PropagateHandChildBones(CSPose, RestCompBases, FootRIdx, FootRCS);
	}
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
	const float BreathPitchDeg = BreathSin * 3.0f;

	// 2. Aim Pitch & Yaw + Counter-rotation for Bladed Combat Pelvis Yaw (+7.5 deg)
	const float MoveAlpha = FMath::Clamp(Speed / 220.0f, 0.0f, 1.0f);
	const float SeatedAlpha = bIsMenuPreview
		? FMath::Clamp(1.0f - FMath::Max(HighReadyAlpha, MoveAlpha * 4.0f), 0.0f, 1.0f)
		: 0.0f;
	const FVector SpineForwardLeanAxis = FVector::CrossProduct(CompUpDir, CompForwardDir).GetSafeNormal();
	const float SeatedForwardLeanDeg = 4.5f * SeatedAlpha;

	const float EffectivePitch = FMath::Clamp(AimPitch, -75.0f, 75.0f);
	const float EffectiveYaw = FMath::Clamp(AimYaw, -75.0f, 75.0f);
	const float RecoilKickDeg = -RecoilImpulseAlpha * 4.5f;

	const float SpineCounterYawDeg = -2.4f * (1.0f - SmoothADSAlpha * 0.4f) * (1.0f - SeatedAlpha);

	const FQuat Spine01Delta =
		FQuat(SpineForwardLeanAxis, FMath::DegreesToRadians(SeatedForwardLeanDeg * 0.40f)) *
		FQuat(CompUpDir, FMath::DegreesToRadians(EffectiveYaw * 0.22f + SpineCounterYawDeg)) *
		FQuat(CompRightDir, FMath::DegreesToRadians(EffectivePitch * 0.18f + BreathPitchDeg * 0.25f));

	const FQuat Spine02Delta =
		FQuat(SpineForwardLeanAxis, FMath::DegreesToRadians(SeatedForwardLeanDeg * 0.35f)) *
		FQuat(CompUpDir, FMath::DegreesToRadians(EffectiveYaw * 0.28f + SpineCounterYawDeg)) *
		FQuat(CompRightDir, FMath::DegreesToRadians(EffectivePitch * 0.26f + BreathPitchDeg * 0.50f + RecoilKickDeg * 0.4f));

	const FQuat Spine03Delta =
		FQuat(SpineForwardLeanAxis, FMath::DegreesToRadians(SeatedForwardLeanDeg * 0.25f)) *
		FQuat(CompUpDir, FMath::DegreesToRadians(EffectiveYaw * 0.30f + SpineCounterYawDeg)) *
		FQuat(CompRightDir, FMath::DegreesToRadians(EffectivePitch * 0.32f + BreathPitchDeg * 0.75f + RecoilKickDeg * 0.6f));

	PropagateSingleChildBoneCS(CSPose, RestCompBases, PelvisIdx, Spine01Idx, Spine01Delta);
	PropagateSingleChildBoneCS(CSPose, RestCompBases, Spine01Idx, Spine02Idx, Spine02Delta);
	PropagateSingleChildBoneCS(CSPose, RestCompBases, Spine02Idx, Spine03Idx, Spine03Delta);

	FTransform ExpandedSpine03CS = CSPose.Pose.GetComponentSpaceTransform(Spine03Idx);
	ExpandedSpine03CS.AddToTranslation(CompUpDir * (BreathSin * 0.65f) + CompForwardDir * (BreathSin * 0.50f));
	CSPose.Pose.SetComponentSpaceTransform(Spine03Idx, ExpandedSpine03CS);

	// 3. Clavicle Breathing Lift & Tactical Weapon Protraction
	const float ClavicleLiftRad = FMath::DegreesToRadians(BreathSin * 2.2f);
	const FQuat ClavLDelta(CompForwardDir, -ClavicleLiftRad);
	const FQuat ClavRDelta(CompForwardDir, ClavicleLiftRad);

	// Tactical clavicle protraction: left shoulder swings forward into the weapon cradle
	const float ArmClavAlpha = (1.0f - HighReadyAlpha * 0.4f);
	const FQuat ClavProtractionL =
		FQuat(CompUpDir, FMath::DegreesToRadians(9.0f * ArmClavAlpha)) *
		FQuat(CompRightDir, FMath::DegreesToRadians(5.0f * ArmClavAlpha));
	const FQuat CombinedClavL = ClavLDelta * ClavProtractionL;

	PropagateSingleChildBoneCS(CSPose, RestCompBases, Spine03Idx, ClavicleLIdx, CombinedClavL);
	PropagateSingleChildBoneCS(CSPose, RestCompBases, ClavicleLIdx, UpperArmLIdx);
	PropagateSingleChildBoneCS(CSPose, RestCompBases, UpperArmLIdx, LowerArmLIdx);
	PropagateSingleChildBoneCS(CSPose, RestCompBases, LowerArmLIdx, HandLIdx);

	PropagateSingleChildBoneCS(CSPose, RestCompBases, Spine03Idx, ClavicleRIdx, ClavRDelta);
	PropagateSingleChildBoneCS(CSPose, RestCompBases, ClavicleRIdx, UpperArmRIdx);
	PropagateSingleChildBoneCS(CSPose, RestCompBases, UpperArmRIdx, LowerArmRIdx);
	PropagateSingleChildBoneCS(CSPose, RestCompBases, LowerArmRIdx, HandRIdx);

	// 4. Head/Neck Gaze: Tracks Magazine & Barrel during Pre-Deployment Inspection, then gazes out at camera
	const float IdleLookScale = 1.0f - HighReadyAlpha * 0.75f;
	float LookYawDeg = (FMath::Sin(HeadLookTime * 0.65f) * 3.8f + FMath::Sin(HeadLookTime * 1.45f) * 1.5f) * IdleLookScale;
	float LookPitchDeg = (FMath::Cos(HeadLookTime * 0.85f) * 1.4f + BreathSin * 1.1f) * IdleLookScale;

	if (bIsInspectingWeapon && HighReadyAlpha < 0.25f)
	{
		const float W = FMath::Clamp(WeaponInspectionAlpha, 0.0f, 1.0f);
		float InspectPitch = 0.0f;
		float InspectYaw = 0.0f;

		if (W < 0.48f)
		{
			// Looking down at magazine extraction, brass round check, and reseating tap
			const float Env = FMath::Sin(FMath::Clamp(W / 0.48f, 0.0f, 1.0f) * PI);
			InspectPitch = -14.0f * Env;
			InspectYaw = 5.0f * Env;
		}
		else if (W < 0.88f)
		{
			// Following left hand wiping down the barrel from receiver toward muzzle and back
			const float SubW = (W - 0.48f) / 0.40f;
			const float Env = FMath::Sin(SubW * PI);
			InspectPitch = -10.0f * Env;
			InspectYaw = FMath::Lerp(4.0f, -12.0f, FMath::Sin(SubW * PI));
		}

		LookPitchDeg = FMath::Lerp(LookPitchDeg, InspectPitch, 0.85f);
		LookYawDeg = FMath::Lerp(LookYawDeg, InspectYaw, 0.85f);
	}

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

	const FBoneContainer& BoneContainer = CSPose.Pose.GetPose().GetBoneContainer();
	const FReferenceSkeleton& RefSkel = BoneContainer.GetReferenceSkeleton();
	const int32 NumBones = RefSkel.GetNum();
	const int32 HandSkelIdx = BoneContainer.GetSkeletonIndex(HandBoneIdx);
	const bool bIsRightHand = (HandBoneIdx == HandRIdx);
	const bool bIsLeftHand  = (HandBoneIdx == HandLIdx);

	// Left-Hand inspection phase:
	// 0 = Foregrip cradle
	// 1 = Magazine body grip (during mag inspection)
	// 2 = Barrel shroud wipe (during overhand wipe pass)
	int32 LeftHandActionMode = 0;
	if (bIsInspectingWeapon && !bUnarmedRelaxedIdle && HighReadyAlpha < 0.25f)
	{
		const float W = FMath::Clamp(WeaponInspectionAlpha, 0.0f, 1.0f);
		if (W >= 0.11f && W <= 0.44f)
		{
			LeftHandActionMode = 1; // Grasping 30-round magazine body
		}
		else if (W >= 0.54f && W <= 0.86f)
		{
			LeftHandActionMode = 2; // Overhand palm cupping barrel shroud
		}
	}

	for (int32 SkelIdx = HandSkelIdx + 1; SkelIdx < NumBones; ++SkelIdx)
	{
		if (RefSkel.BoneIsChildOf(SkelIdx, HandSkelIdx))
		{
			const FCompactPoseBoneIndex ChildCompactIdx = BoneContainer.GetCompactPoseIndexFromSkeletonIndex(SkelIdx);
			const int32 ParentSkelIdx = RefSkel.GetParentIndex(SkelIdx);
			const FCompactPoseBoneIndex ParentCompactIdx = BoneContainer.GetCompactPoseIndexFromSkeletonIndex(ParentSkelIdx);

			if (ChildCompactIdx != INDEX_NONE && ParentCompactIdx != INDEX_NONE &&
				RestCompBases.IsValidIndex(ChildCompactIdx.GetInt()) && RestCompBases.IsValidIndex(ParentCompactIdx.GetInt()))
			{
				const FTransform CurParentCS = CSPose.Pose.GetComponentSpaceTransform(ParentCompactIdx);
				FTransform LocalToParent = RestCompBases[ChildCompactIdx.GetInt()].GetRelativeTransform(RestCompBases[ParentCompactIdx.GetInt()]);

				if (bIsRightHand)
				{
					const FName BoneName = RefSkel.GetBoneName(SkelIdx);
					const FString BoneNameStr = BoneName.ToString();

					if (BoneNameStr.Contains(TEXT("thumb_01")))
					{
						// Oppose thumb and curl inward around left side of pistol grip
						LocalToParent.SetRotation((LocalToParent.GetRotation()
							* FQuat(FVector(0.0f, 1.0f, 0.0f), FMath::DegreesToRadians(25.0f))
							* FQuat(FVector(0.0f, 0.0f, 1.0f), FMath::DegreesToRadians(20.0f))).GetNormalized());
					}
					else if (BoneNameStr.Contains(TEXT("thumb_02")) || BoneNameStr.Contains(TEXT("thumb_03")))
					{
						LocalToParent.SetRotation((LocalToParent.GetRotation()
							* FQuat(FVector(0.0f, 1.0f, 0.0f), FMath::DegreesToRadians(20.0f))).GetNormalized());
					}
					else if (BoneNameStr.Contains(TEXT("index_01")))
					{
						// Trigger discipline: index finger extended along receiver flank above trigger guard
						LocalToParent.SetRotation((LocalToParent.GetRotation()
							* FQuat(FVector(0.0f, 1.0f, 0.0f), FMath::DegreesToRadians(8.0f))).GetNormalized());
					}
					else if (BoneNameStr.Contains(TEXT("index_02")) || BoneNameStr.Contains(TEXT("index_03")))
					{
						LocalToParent.SetRotation((LocalToParent.GetRotation()
							* FQuat(FVector(0.0f, 1.0f, 0.0f), FMath::DegreesToRadians(5.0f))).GetNormalized());
					}
					else if (BoneNameStr.Contains(TEXT("middle_01")))
					{
						// Firm knuckle wrap around front strap of pistol grip
						LocalToParent.SetRotation((LocalToParent.GetRotation()
							* FQuat(FVector(0.0f, 1.0f, 0.0f), FMath::DegreesToRadians(45.0f))).GetNormalized());
					}
					else if (BoneNameStr.Contains(TEXT("middle_02")))
					{
						LocalToParent.SetRotation((LocalToParent.GetRotation()
							* FQuat(FVector(0.0f, 1.0f, 0.0f), FMath::DegreesToRadians(50.0f))).GetNormalized());
					}
					else if (BoneNameStr.Contains(TEXT("middle_03")))
					{
						LocalToParent.SetRotation((LocalToParent.GetRotation()
							* FQuat(FVector(0.0f, 1.0f, 0.0f), FMath::DegreesToRadians(38.0f))).GetNormalized());
					}
					else if (BoneNameStr.Contains(TEXT("ring_01")))
					{
						LocalToParent.SetRotation((LocalToParent.GetRotation()
							* FQuat(FVector(0.0f, 1.0f, 0.0f), FMath::DegreesToRadians(48.0f))).GetNormalized());
					}
					else if (BoneNameStr.Contains(TEXT("ring_02")))
					{
						LocalToParent.SetRotation((LocalToParent.GetRotation()
							* FQuat(FVector(0.0f, 1.0f, 0.0f), FMath::DegreesToRadians(52.0f))).GetNormalized());
					}
					else if (BoneNameStr.Contains(TEXT("ring_03")))
					{
						LocalToParent.SetRotation((LocalToParent.GetRotation()
							* FQuat(FVector(0.0f, 1.0f, 0.0f), FMath::DegreesToRadians(40.0f))).GetNormalized());
					}
					else if (BoneNameStr.Contains(TEXT("pinky_01")))
					{
						LocalToParent.SetRotation((LocalToParent.GetRotation()
							* FQuat(FVector(0.0f, 1.0f, 0.0f), FMath::DegreesToRadians(50.0f))).GetNormalized());
					}
					else if (BoneNameStr.Contains(TEXT("pinky_02")))
					{
						LocalToParent.SetRotation((LocalToParent.GetRotation()
							* FQuat(FVector(0.0f, 1.0f, 0.0f), FMath::DegreesToRadians(55.0f))).GetNormalized());
					}
					else if (BoneNameStr.Contains(TEXT("pinky_03")))
					{
						LocalToParent.SetRotation((LocalToParent.GetRotation()
							* FQuat(FVector(0.0f, 1.0f, 0.0f), FMath::DegreesToRadians(42.0f))).GetNormalized());
					}
				}
				else if (bIsLeftHand)
				{
					const FName BoneName = RefSkel.GetBoneName(SkelIdx);
					const FString BoneNameStr = BoneName.ToString();

					if (LeftHandActionMode == 2)
					{
						// Barrel Shroud Wipe: palm rests over top ridge, fingers cupped gently over sides
						if (BoneNameStr.Contains(TEXT("thumb")))
						{
							LocalToParent.SetRotation((LocalToParent.GetRotation()
								* FQuat(FVector(0.0f, 1.0f, 0.0f), FMath::DegreesToRadians(-15.0f))).GetNormalized());
						}
						else
						{
							LocalToParent.SetRotation((LocalToParent.GetRotation()
								* FQuat(FVector(0.0f, 1.0f, 0.0f), FMath::DegreesToRadians(-20.0f))).GetNormalized());
						}
					}
					else if (LeftHandActionMode == 1)
					{
						// Magazine Extraction & Round Check: fingers wrapped firmly around 30-round magazine
						if (BoneNameStr.Contains(TEXT("thumb_01")))
						{
							LocalToParent.SetRotation((LocalToParent.GetRotation()
								* FQuat(FVector(0.0f, 1.0f, 0.0f), FMath::DegreesToRadians(30.0f))
								* FQuat(FVector(0.0f, 0.0f, 1.0f), FMath::DegreesToRadians(15.0f))).GetNormalized());
						}
						else if (BoneNameStr.Contains(TEXT("thumb")))
						{
							LocalToParent.SetRotation((LocalToParent.GetRotation()
								* FQuat(FVector(0.0f, 1.0f, 0.0f), FMath::DegreesToRadians(25.0f))).GetNormalized());
						}
						else if (BoneNameStr.Contains(TEXT("_01")))
						{
							LocalToParent.SetRotation((LocalToParent.GetRotation()
								* FQuat(FVector(0.0f, 1.0f, 0.0f), FMath::DegreesToRadians(38.0f))).GetNormalized());
						}
						else if (BoneNameStr.Contains(TEXT("_02")))
						{
							LocalToParent.SetRotation((LocalToParent.GetRotation()
								* FQuat(FVector(0.0f, 1.0f, 0.0f), FMath::DegreesToRadians(42.0f))).GetNormalized());
						}
						else
						{
							LocalToParent.SetRotation((LocalToParent.GetRotation()
								* FQuat(FVector(0.0f, 1.0f, 0.0f), FMath::DegreesToRadians(32.0f))).GetNormalized());
						}
					}
					else
					{
						// Foregrip Cradle (Low-Ready / High-Ready): fingers cup underneath handguard
						if (BoneNameStr.Contains(TEXT("thumb_01")))
						{
							LocalToParent.SetRotation((LocalToParent.GetRotation()
								* FQuat(FVector(0.0f, 1.0f, 0.0f), FMath::DegreesToRadians(18.0f))).GetNormalized());
						}
						else if (BoneNameStr.Contains(TEXT("thumb")))
						{
							LocalToParent.SetRotation((LocalToParent.GetRotation()
								* FQuat(FVector(0.0f, 1.0f, 0.0f), FMath::DegreesToRadians(15.0f))).GetNormalized());
						}
						else if (BoneNameStr.Contains(TEXT("_01")))
						{
							LocalToParent.SetRotation((LocalToParent.GetRotation()
								* FQuat(FVector(0.0f, 1.0f, 0.0f), FMath::DegreesToRadians(28.0f))).GetNormalized());
						}
						else if (BoneNameStr.Contains(TEXT("_02")))
						{
							LocalToParent.SetRotation((LocalToParent.GetRotation()
								* FQuat(FVector(0.0f, 1.0f, 0.0f), FMath::DegreesToRadians(35.0f))).GetNormalized());
						}
						else
						{
							LocalToParent.SetRotation((LocalToParent.GetRotation()
								* FQuat(FVector(0.0f, 1.0f, 0.0f), FMath::DegreesToRadians(25.0f))).GetNormalized());
						}
					}
				}

				FTransform ChildCS = LocalToParent * CurParentCS;

				if (CurrentRifleQuatCS.IsNormalized())
				{
					const FName BoneName = RefSkel.GetBoneName(SkelIdx);
					const FString BoneNameStr = BoneName.ToString();

					float BoneRadius = 1.15f; // fingers
					if (BoneNameStr.Contains(TEXT("thumb_01")) || BoneNameStr.Contains(TEXT("index_01")) ||
						BoneNameStr.Contains(TEXT("middle_01")) || BoneNameStr.Contains(TEXT("ring_01")) || BoneNameStr.Contains(TEXT("pinky_01")))
					{
						BoneRadius = 1.45f; // knuckles
					}

					FVector BonePosCS = ChildCS.GetLocation();
					FVector BonePosLocal = CurrentRifleQuatCS.UnrotateVector(BonePosCS - CurrentReceiverOriginCS);
					BonePosLocal = PushOutOfSolidWeapon(BonePosLocal, BoneRadius, bIsLeftHand, LeftHandActionMode, BoneNameStr);
					ChildCS.SetLocation(CurrentReceiverOriginCS + CurrentRifleQuatCS.RotateVector(BonePosLocal));
				}

				CSPose.Pose.SetComponentSpaceTransform(ChildCompactIdx, ChildCS);
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

	const FTransform Spine03CS = CSPose.Pose.GetComponentSpaceTransform(Spine03Idx);
	const FVector ChestPos = Spine03CS.GetLocation();
	const float BreathOffset = FMath::Sin(IdleBreathPhase) * 0.45f;

	const FVector RelaxedHandR =
		ChestPos
		+ CompRightDir * 24.5f
		+ CompForwardDir * 4.5f
		- CompUpDir * 46.0f
		+ CompUpDir * (BreathOffset * 0.65f)
		+ CompForwardDir * (BreathOffset * 0.35f);

	const FVector RelaxedJointR =
		ChestPos
		+ CompRightDir * 31.5f
		- CompForwardDir * 6.5f
		- CompUpDir * 22.0f;

	const FVector ArmedJointR =
		ChestPos
		+ CompRightDir * 25.0f
		- CompForwardDir * 6.0f
		- CompUpDir * 22.0f;

	const FVector JointTargetR = bUnarmedRelaxedIdle
		? FMath::Lerp(RelaxedJointR, ArmedJointR, HighReadyAlpha)
		: ArmedJointR;

	// Low-Ready Patrol Cradle: pistol grip held comfortably near right ribcage/pecs
	const FVector LowReadyHandR =
		ChestPos
		+ CompRightDir * 12.0f
		+ CompForwardDir * 16.5f
		+ CompUpDir * (-5.5f + BreathOffset);

	const float RecoilBack = RecoilImpulseAlpha * 2.8f;

	// High-Ready / Combat Ready: pistol grip raised toward shoulder pocket
	const FVector HighReadyHandR =
		ChestPos
		+ CompRightDir * (11.0f - SmoothADSAlpha * 4.5f)
		+ CompForwardDir * (24.0f - SmoothADSAlpha * 3.0f - RecoilBack)
		+ CompUpDir * (4.5f + SmoothADSAlpha * 3.5f + BreathOffset * 0.4f);

	const FVector ArmedHandR = FMath::Lerp(LowReadyHandR, HighReadyHandR, HighReadyAlpha);

	const FVector TargetHandR = bUnarmedRelaxedIdle
		? FMath::Lerp(RelaxedHandR, HighReadyHandR, HighReadyAlpha)
		: ArmedHandR;

	const int32 UIdx = UpperArmRIdx.GetInt();
	const int32 LIdx = LowerArmRIdx.GetInt();
	const int32 HIdx = HandRIdx.GetInt();
	const int32 MIdx = (Middle01RIdx != INDEX_NONE) ? Middle01RIdx.GetInt() : INDEX_NONE;
	const int32 IIdx = (Index01RIdx != INDEX_NONE) ? Index01RIdx.GetInt() : INDEX_NONE;
	const int32 PIdx = (Pinky01RIdx != INDEX_NONE) ? Pinky01RIdx.GetInt() : INDEX_NONE;

	// Target orientation for the right hand gripping the angled pistol grip:
	// - Primary axis (wrist -> fingers) points down along the grip:
	const FVector GripFingerDir = CurrentRifleQuatCS.RotateVector(FVector(-0.522f, 0.0f, -0.853f));
	// - Palm normal points into the right flank of the grip (-Y in rifle local space):
	const FVector GripPalmNormal = CurrentRifleQuatCS.RotateVector(FVector(0.0f, -1.0f, 0.0f));
	const FQuat TargetHandRot = FRotationMatrix::MakeFromXZ(GripFingerDir, GripPalmNormal).ToQuat();

	// Rest pose reference hand orientation
	FQuat RestHandRot = FQuat::Identity;
	FVector RestWristToPalmR = FVector::ZeroVector;
	if (RestCompBases.IsValidIndex(HIdx) && RestCompBases.IsValidIndex(MIdx))
	{
		RestWristToPalmR = (RestCompBases[MIdx].GetLocation() - RestCompBases[HIdx].GetLocation()) * 0.75f;
		const FVector RestFingerDir = (RestCompBases[MIdx].GetLocation() - RestCompBases[HIdx].GetLocation()).GetSafeNormal();
		FVector RestPalmNormal = -CompUpDir;
		if (RestCompBases.IsValidIndex(IIdx) && RestCompBases.IsValidIndex(PIdx))
		{
			const FVector RestPinkyDir = (RestCompBases[PIdx].GetLocation() - RestCompBases[IIdx].GetLocation()).GetSafeNormal();
			RestPalmNormal = FVector::CrossProduct(RestFingerDir, RestPinkyDir).GetSafeNormal();
		}
		RestHandRot = FRotationMatrix::MakeFromXZ(RestFingerDir, RestPalmNormal).ToQuat();
	}

	// Solve Two-Bone IK
	FTransform UpperArmRCS = CSPose.Pose.GetComponentSpaceTransform(UpperArmRIdx);
	FTransform LowerArmRCS = CSPose.Pose.GetComponentSpaceTransform(LowerArmRIdx);
	FTransform HandRCS     = CSPose.Pose.GetComponentSpaceTransform(HandRIdx);

	AnimationCore::SolveTwoBoneIK(UpperArmRCS, LowerArmRCS, HandRCS, JointTargetR, TargetHandR, true, 0.95f, 1.15f);

	if (UpperArmRCS.ContainsNaN() && RestCompBases.IsValidIndex(UIdx)) UpperArmRCS = RestCompBases[UIdx];
	if (LowerArmRCS.ContainsNaN() && RestCompBases.IsValidIndex(LIdx)) LowerArmRCS = RestCompBases[LIdx];
	if (HandRCS.ContainsNaN() && RestCompBases.IsValidIndex(HIdx))     HandRCS = RestCompBases[HIdx];

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

			if (RestHandRot.IsNormalized())
			{
				const FQuat DeltaHandR = (TargetHandRot * RestHandRot.Inverse()).GetNormalized();
				HandRCS.SetRotation((DeltaHandR * RestCompBases[HIdx].GetRotation()).GetNormalized());
			}
			else
			{
				HandRCS.SetRotation((AlignLower * RestCompBases[HIdx].GetRotation()).GetNormalized());
			}
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
	if (UpperArmLIdx == INDEX_NONE || LowerArmLIdx == INDEX_NONE || HandLIdx == INDEX_NONE || Spine03Idx == INDEX_NONE)
	{
		return;
	}

	const FTransform Spine03CS = CSPose.Pose.GetComponentSpaceTransform(Spine03Idx);
	const FVector ChestPos = Spine03CS.GetLocation();
	const float BreathOffsetL = FMath::Sin(IdleBreathPhase) * 0.45f;

	const FVector RelaxedHandL =
		ChestPos
		- CompRightDir * 24.5f
		+ CompForwardDir * 4.5f
		- CompUpDir * 46.0f
		+ CompUpDir * (BreathOffsetL * 0.65f)
		+ CompForwardDir * (BreathOffsetL * 0.35f);

	const FVector RelaxedJointL =
		ChestPos
		- CompRightDir * 31.5f
		- CompForwardDir * 6.5f
		- CompUpDir * 22.0f;

	const FVector ArmedJointL =
		ChestPos
		- CompRightDir * 28.0f
		+ CompForwardDir * 4.0f
		- CompUpDir * 24.0f;

	const FVector WipeJointL =
		ChestPos
		- CompRightDir * 24.0f
		+ CompForwardDir * 6.0f
		- CompUpDir * 12.0f;

	const FTransform HandRCS = (HandRIdx != INDEX_NONE) ? CSPose.Pose.GetComponentSpaceTransform(HandRIdx) : FTransform::Identity;
	const FVector HandRPos = HandRCS.GetLocation();

	const FVector LowReadyBarrelDir = (
		-CompRightDir * 0.62f
		+ CompForwardDir * 0.72f
		- CompUpDir * 0.31f
	).GetSafeNormal();

	const float PitchRad = FMath::DegreesToRadians(FMath::Clamp(AimPitch, -65.0f, 65.0f));
	const FVector HighReadyBarrelDir = (
		-CompRightDir * (0.18f * (1.0f - SmoothADSAlpha))
		+ CompForwardDir * FMath::Cos(PitchRad)
		+ CompUpDir * FMath::Sin(PitchRad)
	).GetSafeNormal();

	const FVector BarrelDir = FMath::Lerp(LowReadyBarrelDir, HighReadyBarrelDir, HighReadyAlpha).GetSafeNormal();

	// Foregrip underside target in component space: anchored directly to the solid handguard underside
	const FVector ForegripHandL = CurrentReceiverOriginCS + CurrentRifleQuatCS.RotateVector(FVector(16.5f, 0.0f, -4.8f));

	const bool bInspecting = (bIsInspectingWeapon && HighReadyAlpha < 0.25f && !bUnarmedRelaxedIdle);
	const float W = bInspecting ? FMath::Clamp(WeaponInspectionAlpha, 0.0f, 1.0f) : 0.0f;

	int32 LeftHandActionMode = 0;
	FVector DesiredPalmTargetCS = ForegripHandL;
	float WipeElbowBlend = 0.0f;

	if (bInspecting)
	{
		const FVector MagwellGripL     = CurrentReceiverOriginCS + CurrentRifleQuatCS.RotateVector(FVector(3.5f, 0.0f, -10.0f));
		const FVector MagExtractL      = CurrentReceiverOriginCS + CurrentRifleQuatCS.RotateVector(FVector(3.5f, 0.0f, -14.5f));
		const FVector MagBaseTapL      = CurrentReceiverOriginCS + CurrentRifleQuatCS.RotateVector(FVector(3.5f, 0.0f, -15.0f));
		const FVector BarrelWipeStartL = CurrentReceiverOriginCS + CurrentRifleQuatCS.RotateVector(FVector(13.5f, 0.0f, 6.8f));
		const FVector BarrelWipeEndL   = CurrentReceiverOriginCS + CurrentRifleQuatCS.RotateVector(FVector(23.5f, 0.0f, 6.8f));

		if (W < 0.12f)
		{
			const float SubT = FMath::SmoothStep(0.0f, 1.0f, W / 0.12f);
			DesiredPalmTargetCS = FMath::Lerp(ForegripHandL, MagwellGripL, SubT);
			LeftHandActionMode = 1;
		}
		else if (W < 0.22f)
		{
			const float SubT = FMath::SmoothStep(0.0f, 1.0f, (W - 0.12f) / 0.10f);
			DesiredPalmTargetCS = FMath::Lerp(MagwellGripL, MagExtractL, SubT);
			LeftHandActionMode = 1;
		}
		else if (W < 0.35f)
		{
			const float SubT = (W - 0.22f) / 0.13f;
			DesiredPalmTargetCS = MagExtractL + CurrentRifleQuatCS.RotateVector(FVector(0.0f, 0.0f, 0.8f)) * FMath::Sin(SubT * PI);
			LeftHandActionMode = 1;
		}
		else if (W < 0.41f)
		{
			const float SubT = FMath::SmoothStep(0.0f, 1.0f, (W - 0.35f) / 0.06f);
			DesiredPalmTargetCS = FMath::Lerp(MagExtractL, MagwellGripL, SubT);
			LeftHandActionMode = 1;
		}
		else if (W < 0.48f)
		{
			const float SubT = (W - 0.41f) / 0.07f;
			DesiredPalmTargetCS = FMath::Lerp(MagBaseTapL, MagwellGripL, FMath::Sin(SubT * PI));
			LeftHandActionMode = 1;
		}
		else if (W < 0.56f)
		{
			const float SubT = FMath::SmoothStep(0.0f, 1.0f, (W - 0.48f) / 0.08f);
			DesiredPalmTargetCS = FMath::Lerp(MagwellGripL, BarrelWipeStartL, SubT);
			LeftHandActionMode = 2;
			WipeElbowBlend = SubT;
		}
		else if (W < 0.74f)
		{
			const float WipeA = FMath::SmoothStep(0.0f, 1.0f, FMath::Clamp((W - 0.56f) / 0.12f, 0.0f, 1.0f));
			DesiredPalmTargetCS = FMath::Lerp(BarrelWipeStartL, BarrelWipeEndL, WipeA);
			LeftHandActionMode = 2;
			WipeElbowBlend = 1.0f;
		}
		else if (W < 0.88f)
		{
			const float SubT = FMath::SmoothStep(0.0f, 1.0f, (W - 0.74f) / 0.14f);
			DesiredPalmTargetCS = FMath::Lerp(BarrelWipeEndL, ForegripHandL, SubT);
			LeftHandActionMode = 2;
			WipeElbowBlend = 1.0f - SubT;
		}
		else
		{
			DesiredPalmTargetCS = ForegripHandL;
			LeftHandActionMode = 0;
		}
	}

	const FVector ActiveArmedJointL = FMath::Lerp(ArmedJointL, WipeJointL, WipeElbowBlend);
	const FVector JointTargetCS = bUnarmedRelaxedIdle
		? FMath::Lerp(RelaxedJointL, ActiveArmedJointL, HighReadyAlpha)
		: ActiveArmedJointL;

	const int32 UIdx = UpperArmLIdx.GetInt();
	const int32 LIdx = LowerArmLIdx.GetInt();
	const int32 HIdx = HandLIdx.GetInt();
	const int32 MLIdx = (Middle01LIdx != INDEX_NONE) ? Middle01LIdx.GetInt() : INDEX_NONE;
	const int32 ILIdx = (Index01LIdx != INDEX_NONE) ? Index01LIdx.GetInt() : INDEX_NONE;
	const int32 PLIdx = (Pinky01LIdx != INDEX_NONE) ? Pinky01LIdx.GetInt() : INDEX_NONE;

	// Target orientation for the left hand across action modes:
	FVector TargetFingerDirL;
	FVector TargetPalmNormalL;
	if (LeftHandActionMode == 2)
	{
		// Barrel Shroud Wipe: palm rests over top rail, fingers point along barrel
		TargetFingerDirL = CurrentRifleQuatCS.RotateVector(FVector(1.0f, 0.0f, 0.0f));
		TargetPalmNormalL = CurrentRifleQuatCS.RotateVector(FVector(0.0f, 0.0f, -1.0f));
	}
	else if (LeftHandActionMode == 1)
	{
		// Magazine Check: fingers point up along magazine body, palm rests against magazine back/spine
		TargetFingerDirL = CurrentRifleQuatCS.RotateVector(FVector(0.0f, 0.0f, 1.0f));
		TargetPalmNormalL = CurrentRifleQuatCS.RotateVector(FVector(-1.0f, 0.0f, 0.0f));
	}
	else
	{
		// Foregrip Cradle: fingers point along handguard, palm cups underside
		TargetFingerDirL = CurrentRifleQuatCS.RotateVector(FVector(1.0f, 0.0f, 0.0f));
		TargetPalmNormalL = CurrentRifleQuatCS.RotateVector(FVector(0.0f, 0.0f, 1.0f));
	}
	const FQuat TargetHandRotL = FRotationMatrix::MakeFromXZ(TargetFingerDirL, TargetPalmNormalL).ToQuat();

	// Rest pose reference hand orientation
	FQuat RestHandRotL = FQuat::Identity;
	if (RestCompBases.IsValidIndex(HIdx) && RestCompBases.IsValidIndex(MLIdx))
	{
		const FVector RestFingerDirL = (RestCompBases[MLIdx].GetLocation() - RestCompBases[HIdx].GetLocation()).GetSafeNormal();
		FVector RestPalmNormalL = -CompUpDir;
		if (RestCompBases.IsValidIndex(ILIdx) && RestCompBases.IsValidIndex(PLIdx))
		{
			const FVector RestPinkyDirL = (RestCompBases[PLIdx].GetLocation() - RestCompBases[ILIdx].GetLocation()).GetSafeNormal();
			RestPalmNormalL = FVector::CrossProduct(RestFingerDirL, RestPinkyDirL).GetSafeNormal();
		}
		RestHandRotL = FRotationMatrix::MakeFromXZ(RestFingerDirL, RestPalmNormalL).ToQuat();
	}

	// Solve Two-Bone IK directly to the solid weapon contact point with natural stretch support
	FTransform UpperArmLCS = CSPose.Pose.GetComponentSpaceTransform(UpperArmLIdx);
	FTransform LowerArmLCS = CSPose.Pose.GetComponentSpaceTransform(LowerArmLIdx);
	FTransform HandLCS     = CSPose.Pose.GetComponentSpaceTransform(HandLIdx);

	AnimationCore::SolveTwoBoneIK(UpperArmLCS, LowerArmLCS, HandLCS, JointTargetCS, DesiredPalmTargetCS, true, 0.95f, 1.15f);

	const bool bUpperNaN = UpperArmLCS.ContainsNaN();
	const bool bLowerNaN = LowerArmLCS.ContainsNaN();
	const bool bHandNaN  = HandLCS.ContainsNaN();

	if (bUpperNaN && RestCompBases.IsValidIndex(UIdx)) UpperArmLCS = RestCompBases[UIdx];
	if (bLowerNaN && RestCompBases.IsValidIndex(LIdx)) LowerArmLCS = RestCompBases[LIdx];
	if (bHandNaN  && RestCompBases.IsValidIndex(HIdx)) HandLCS     = RestCompBases[HIdx];

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

			if (RestHandRotL.IsNormalized())
			{
				const FQuat DeltaHandL = (TargetHandRotL * RestHandRotL.Inverse()).GetNormalized();
				HandLCS.SetRotation((DeltaHandL * RestCompBases[HIdx].GetRotation()).GetNormalized());
			}
			else
			{
				HandLCS.SetRotation((AlignLower * RestCompBases[HIdx].GetRotation()).GetNormalized());
			}
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
	const bool bUpperNaN = UpperArmLCS.ContainsNaN();
	const bool bLowerNaN = LowerArmLCS.ContainsNaN();
	const bool bHandNaN  = HandLCS.ContainsNaN();

	if (bUpperNaN && RestCompBases.IsValidIndex(UIdx)) UpperArmLCS = RestCompBases[UIdx];
	if (bLowerNaN && RestCompBases.IsValidIndex(LIdx)) LowerArmLCS = RestCompBases[LIdx];
	if (bHandNaN  && RestCompBases.IsValidIndex(HIdx)) HandLCS     = RestCompBases[HIdx];

	static int32 DiagCounterL = 0;
	if (++DiagCounterL % 60 == 1)
	{
		const double ULen = RestCompBases.IsValidIndex(UIdx) && RestCompBases.IsValidIndex(LIdx) ? (RestCompBases[LIdx].GetLocation() - RestCompBases[UIdx].GetLocation()).Size() : 0.0;
		const double LLen = RestCompBases.IsValidIndex(LIdx) && RestCompBases.IsValidIndex(HIdx) ? (RestCompBases[HIdx].GetLocation() - RestCompBases[LIdx].GetLocation()).Size() : 0.0;
		UE_LOG(LogTemp, Warning, TEXT("[DIAG_LH_RELOAD_IK] NaN=(%d,%d,%d) ULen=%.1f LLen=%.1f MaxReach=%.1f Dist=%.1f Target=%s SolvedHand=%s RecCS=%s HighReadyA=%.2f W=%.2f bInspect=%d"),
			bUpperNaN, bLowerNaN, bHandNaN,
			ULen, LLen, ULen + LLen,
			FVector::Dist(UpperArmLCS.GetLocation(), TargetHandL),
			*TargetHandL.ToString(),
			*HandLCS.GetLocation().ToString(),
			*CurrentReceiverOriginCS.ToString(),
			HighReadyAlpha, WeaponInspectionAlpha, bIsInspectingWeapon);
	}
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
