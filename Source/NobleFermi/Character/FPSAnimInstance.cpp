#include "Character/FPSAnimInstance.h"
#include "Character/FPSCharacter.h"
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

	CacheBoneIndices();
}

void UFPSAnimInstance::CacheBoneIndices()
{
	USkeletalMeshComponent* SkelMesh = GetOwningComponent();
	if (!SkelMesh || !SkelMesh->GetSkeletalMeshAsset())
	{
		return;
	}

	UpperArmL_Index = SkelMesh->GetBoneIndex(FName(TEXT("upperarm_l")));
	LowerArmL_Index = SkelMesh->GetBoneIndex(FName(TEXT("lowerarm_l")));
	HandL_Index = SkelMesh->GetBoneIndex(FName(TEXT("hand_l")));
	Spine01_Index = SkelMesh->GetBoneIndex(FName(TEXT("spine_01")));
	Spine02_Index = SkelMesh->GetBoneIndex(FName(TEXT("spine_02")));
	Spine03_Index = SkelMesh->GetBoneIndex(FName(TEXT("spine_03")));
	Neck01_Index = SkelMesh->GetBoneIndex(FName(TEXT("neck_01")));
	Head_Index = SkelMesh->GetBoneIndex(FName(TEXT("head")));

	LeftFingerBoneIndices.Reset();
	const FReferenceSkeleton& RefSkeleton = SkelMesh->GetSkeletalMeshAsset()->GetRefSkeleton();
	const int32 NumBones = RefSkeleton.GetNum();
	for (int32 i = 0; i < NumBones; ++i)
	{
		const int32 ParentIdx = RefSkeleton.GetParentIndex(i);
		if (ParentIdx == HandL_Index || (ParentIdx != INDEX_NONE && LeftFingerBoneIndices.Contains(ParentIdx)))
		{
			LeftFingerBoneIndices.Add(i);
		}
	}

	bBoneIndicesCached = true;
}

void UFPSAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

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

	if (!OwningCharacter)
	{
		return;
	}

	if (!bBoneIndicesCached)
	{
		CacheBoneIndices();
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

	// 3. Aiming & View Rotation
	AFPSWeaponBase* ActiveWeapon = nullptr;
	if (OwningCharacter->GetInventoryComponent())
	{
		ActiveWeapon = OwningCharacter->GetInventoryComponent()->GetActiveWeapon();
		bIsAiming = ActiveWeapon && ActiveWeapon->IsAimingDownSights();
	}
	else
	{
		bIsAiming = false;
	}

	// Aim pitch normalized between -89 and +89 degrees
	const FRotator ControlRot = OwningCharacter->GetControlRotation();
	AimPitch = FRotator::NormalizeAxis(ControlRot.Pitch);
	AimYaw = FRotator::NormalizeAxis(ControlRot.Yaw - OwningCharacter->GetActorRotation().Yaw);
	ShoulderSideMultiplier = OwningCharacter->ShoulderSideMultiplier;

	// In ADS, blend weight is 1.0 (tight upper-body lock)
	UpperBodyBlendWeight = bIsAiming ? 1.0f : 0.85f;

	// 4. Procedural Spine Lean & Aim Punch Flinch
	if (OwningBodyMechanics)
	{
		ProceduralSpineLean = OwningBodyMechanics->GetProceduralSpineRotation();
		AimPunchRotation = OwningBodyMechanics->GetCurrentAimPunch();
		LeftFootIKOffset = OwningBodyMechanics->GetLeftFootIKOffset();
		RightFootIKOffset = OwningBodyMechanics->GetRightFootIKOffset();
		PelvisIKOffset = OwningBodyMechanics->GetPelvisIKOffset();
	}

	// 5. Update Left-Hand IK Target from active weapon
	UpdateIKTargetFromWeapon(ActiveWeapon);
}

void UFPSAnimInstance::UpdateIKTargetFromWeapon(AFPSWeaponBase* Weapon)
{
	if (!Weapon)
	{
		bLeftHandIKValid = false;
		return;
	}

	USkeletalMeshComponent* SkelMesh = GetOwningComponent();
	if (!SkelMesh)
	{
		bLeftHandIKValid = false;
		return;
	}

	FTransform GripWorldTransform = FTransform::Identity;
	bool bFoundTarget = false;

	// 1. Check if the weapon's AK47 static mesh has the dedicated socket
	if (Weapon->AK47StaticMesh)
	{
		if (Weapon->AK47StaticMesh->DoesSocketExist(FName(TEXT("weapon_l_ik"))))
		{
			GripWorldTransform = Weapon->AK47StaticMesh->GetSocketTransform(FName(TEXT("weapon_l_ik")), RTS_World);
			bFoundTarget = true;
		}
		else
		{
			// Transform handguard grip offset from AK47 local space to world space:
			// (24cm forward along barrel, -2cm left towards receiver, +4cm up onto handguard)
			const FTransform RelGrip(FRotator(10.0f, 0.0f, -15.0f), FVector(24.0f, -2.0f, 4.0f));
			GripWorldTransform = RelGrip * Weapon->AK47StaticMesh->GetComponentTransform();
			bFoundTarget = true;
		}
	}
	else if (Weapon->WeaponMesh)
	{
		if (Weapon->WeaponMesh->DoesSocketExist(FName(TEXT("weapon_l_ik"))))
		{
			GripWorldTransform = Weapon->WeaponMesh->GetSocketTransform(FName(TEXT("weapon_l_ik")), RTS_World);
			bFoundTarget = true;
		}
		else
		{
			const FTransform RelGrip(FRotator(10.0f, 0.0f, -15.0f), FVector(24.0f, -2.0f, 4.0f));
			GripWorldTransform = RelGrip * Weapon->WeaponMesh->GetComponentTransform();
			bFoundTarget = true;
		}
	}

	if (bFoundTarget)
	{
		// Convert world transform to character component space
		const FTransform MeshCompToWorld = SkelMesh->GetComponentTransform();
		LeftHandIKLocation = MeshCompToWorld.InverseTransformPosition(GripWorldTransform.GetLocation());
		LeftHandIKRotation = (MeshCompToWorld.InverseTransformRotation(GripWorldTransform.GetRotation())).Rotator();

		// Elbow pole vector hint (below and left of left shoulder in component space)
		if (UpperArmL_Index != INDEX_NONE && SkelMesh->GetComponentSpaceTransforms().IsValidIndex(UpperArmL_Index))
		{
			const FVector ShoulderLoc = SkelMesh->GetComponentSpaceTransforms()[UpperArmL_Index].GetLocation();
			LeftElbowPoleVector = ShoulderLoc + FVector(-25.0f, -25.0f, -25.0f);
		}
		else
		{
			LeftElbowPoleVector = LeftHandIKLocation + FVector(-20.0f, -20.0f, -15.0f);
		}

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
	if (SpaceBases.Num() == 0)
	{
		return;
	}

	// 1. Apply Thoracic Aim Pitch (+-89 deg distributed across spine_01..03, neck, head)
	ApplySpineAimPitch(SpaceBases);

	// 2. Apply Closed-Form Two-Bone IK locking hand_l to AK-47 forend
	SolveLeftHandTwoBoneIK(SpaceBases);
}

void UFPSAnimInstance::ApplySpineAimPitch(TArray<FTransform>& SpaceBases)
{
	if (FMath::IsNearlyZero(AimPitch, 0.05f))
	{
		return;
	}

	// Fractional distribution: spine_01 (15%), spine_02 (30%), spine_03 (35%), neck_01 (10%), head (10%)
	auto ApplyPitchToBone = [&](int32 BoneIndex, float Fraction)
	{
		if (BoneIndex != INDEX_NONE && SpaceBases.IsValidIndex(BoneIndex))
		{
			const float SubPitch = AimPitch * Fraction;
			const FQuat PitchDelta(FVector::RightVector, FMath::DegreesToRadians(-SubPitch));
			FTransform& BoneTrans = SpaceBases[BoneIndex];
			BoneTrans.SetRotation(PitchDelta * BoneTrans.GetRotation());
		}
	};

	ApplyPitchToBone(Spine01_Index, 0.15f);
	ApplyPitchToBone(Spine02_Index, 0.30f);
	ApplyPitchToBone(Spine03_Index, 0.35f);
	ApplyPitchToBone(Neck01_Index, 0.10f);
	ApplyPitchToBone(Head_Index, 0.10f);
}

void UFPSAnimInstance::SolveLeftHandTwoBoneIK(TArray<FTransform>& SpaceBases)
{
	if (!bLeftHandIKValid || UpperArmL_Index == INDEX_NONE || LowerArmL_Index == INDEX_NONE || HandL_Index == INDEX_NONE)
	{
		return;
	}

	if (!SpaceBases.IsValidIndex(UpperArmL_Index) || !SpaceBases.IsValidIndex(LowerArmL_Index) || !SpaceBases.IsValidIndex(HandL_Index))
	{
		return;
	}

	FTransform& RootTransform = SpaceBases[UpperArmL_Index];
	FTransform& JointTransform = SpaceBases[LowerArmL_Index];
	FTransform& EndTransform = SpaceBases[HandL_Index];

	const FTransform OldHandTransform = EndTransform;

	// Calculate elbow hint / pole vector in component space
	const FVector UpperArmLoc = RootTransform.GetLocation();
	const FVector JointTarget = LeftElbowPoleVector.IsNearlyZero()
		? (UpperArmLoc + FVector(-25.0f, -25.0f, -25.0f))
		: LeftElbowPoleVector;

	// Closed-form analytical Two-Bone IK solve from AnimationCore
	AnimationCore::SolveTwoBoneIK(
		RootTransform,
		JointTransform,
		EndTransform,
		JointTarget,
		LeftHandIKLocation,
		true,
		1.0,
		1.25
	);

	// Lock hand rotation to the weapon handguard orientation
	EndTransform.SetRotation(LeftHandIKRotation.Quaternion());

	// Propagate delta transform to any finger child bones of hand_l
	for (int32 FingerIndex : LeftFingerBoneIndices)
	{
		if (SpaceBases.IsValidIndex(FingerIndex))
		{
			const FTransform RelToHand = SpaceBases[FingerIndex].GetRelativeTransform(OldHandTransform);
			SpaceBases[FingerIndex] = RelToHand * EndTransform;
		}
	}
}
