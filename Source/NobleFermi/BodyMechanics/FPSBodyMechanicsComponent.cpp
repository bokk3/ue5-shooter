#include "BodyMechanics/FPSBodyMechanicsComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"

UFPSBodyMechanicsComponent::UFPSBodyMechanicsComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UFPSBodyMechanicsComponent::BeginPlay()
{
	Super::BeginPlay();

	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (OwnerCharacter)
	{
		OwnerSkeletalMesh = OwnerCharacter->GetMesh();
		OwnerMovementComponent = OwnerCharacter->GetCharacterMovement();
	}

	// Initialize all damage zones
	ZoneTraumaMap.Add(EFPSBodyDamageZone::Head, 0.0f);
	ZoneTraumaMap.Add(EFPSBodyDamageZone::Neck, 0.0f);
	ZoneTraumaMap.Add(EFPSBodyDamageZone::UpperTorso, 0.0f);
	ZoneTraumaMap.Add(EFPSBodyDamageZone::LowerTorso, 0.0f);
	ZoneTraumaMap.Add(EFPSBodyDamageZone::Pelvis, 0.0f);
	ZoneTraumaMap.Add(EFPSBodyDamageZone::UpperArms, 0.0f);
	ZoneTraumaMap.Add(EFPSBodyDamageZone::LowerArms, 0.0f);
	ZoneTraumaMap.Add(EFPSBodyDamageZone::Hands, 0.0f);
	ZoneTraumaMap.Add(EFPSBodyDamageZone::Thighs, 0.0f);
	ZoneTraumaMap.Add(EFPSBodyDamageZone::Calves, 0.0f);
	ZoneTraumaMap.Add(EFPSBodyDamageZone::Feet, 0.0f);
}

void UFPSBodyMechanicsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateSpineLeaning(DeltaTime);
	UpdateFlinchPhysics(DeltaTime);

	if (bEnableFootIK)
	{
		UpdateProceduralFootIK(DeltaTime);
	}
}

void UFPSBodyMechanicsComponent::UpdateSpineLeaning(float DeltaTime)
{
	AActor* Owner = GetOwner();
	if (!Owner || !OwnerMovementComponent)
	{
		return;
	}

	const FVector Velocity = OwnerMovementComponent->Velocity;
	const float Speed = Velocity.Size2D();

	if (Speed > 10.0f)
	{
		// Transform world velocity into local actor space
		const FVector LocalVel = Owner->GetActorTransform().InverseTransformVector(Velocity);
		const float NormalizedLateral = FMath::Clamp(LocalVel.Y / 600.0f, -1.0f, 1.0f);
		const float NormalizedForward = FMath::Clamp(LocalVel.X / 700.0f, -1.0f, 1.0f);

		// Roll: Lean into turn / lateral strafe
		const float TargetRoll = NormalizedLateral * MaxSpineLeanDegrees;

		// Pitch: Forward lean during sprint, backward lean during braking
		const float TargetPitch = -NormalizedForward * (MaxSpineLeanDegrees * 0.5f);

		TargetSpineLeanRot = FRotator(TargetPitch, 0.0f, TargetRoll);
	}
	else
	{
		TargetSpineLeanRot = FRotator::ZeroRotator;
	}

	// Smoothly interpolate current spine rotation
	CurrentSpineLeanRot = FMath::RInterpTo(CurrentSpineLeanRot, TargetSpineLeanRot, DeltaTime, SpineLeanInterpSpeed);
}

void UFPSBodyMechanicsComponent::UpdateFlinchPhysics(float DeltaTime)
{
	// Second-order spring-mass-damper physics simulation for aim punch & flinch
	// Spring constant K, Damping constant C
	const float SpringK = 180.0f;
	const float DampingC = 22.0f;

	// Calculate restoring spring torque: -K * X
	const FRotator SpringTorque = CurrentAimPunchRot * -SpringK;

	// Calculate velocity damping: -C * V
	const FRotator DampingTorque = AimPunchVelocity * -DampingC;

	// Total angular acceleration
	const FRotator AngularAcc = SpringTorque + DampingTorque;

	// Numerical Euler integration
	AimPunchVelocity += AngularAcc * DeltaTime;
	CurrentAimPunchRot += AimPunchVelocity * DeltaTime;

	// Zero out micro-oscillations near rest to avoid floating point jitter
	if (CurrentAimPunchRot.IsNearlyZero(0.01f) && AimPunchVelocity.IsNearlyZero(0.02f))
	{
		CurrentAimPunchRot = FRotator::ZeroRotator;
		AimPunchVelocity = FRotator::ZeroRotator;
	}
}

void UFPSBodyMechanicsComponent::UpdateProceduralFootIK(float DeltaTime)
{
	if (!OwnerMovementComponent || !OwnerMovementComponent->IsMovingOnGround())
	{
		CurrentLeftFootIKOffset = FMath::VInterpTo(CurrentLeftFootIKOffset, FVector::ZeroVector, DeltaTime, FootIKInterpSpeed);
		CurrentRightFootIKOffset = FMath::VInterpTo(CurrentRightFootIKOffset, FVector::ZeroVector, DeltaTime, FootIKInterpSpeed);
		CurrentPelvisIKOffset = FMath::FInterpTo(CurrentPelvisIKOffset, 0.0f, DeltaTime, FootIKInterpSpeed);
		return;
	}

	FVector LeftHitNormal = FVector::UpVector;
	FVector RightHitNormal = FVector::UpVector;

	const float LeftOffsetZ = PerformFootTrace(TEXT("foot_l"), LeftHitNormal);
	const float RightOffsetZ = PerformFootTrace(TEXT("foot_r"), RightHitNormal);

	// Pelvis drops to lowest foot to prevent leg stretching beyond physical limit
	const float TargetPelvisOffset = FMath::Min(0.0f, FMath::Min(LeftOffsetZ, RightOffsetZ));

	const FVector TargetLeftIK(0.0f, 0.0f, LeftOffsetZ - TargetPelvisOffset);
	const FVector TargetRightIK(0.0f, 0.0f, RightOffsetZ - TargetPelvisOffset);

	CurrentLeftFootIKOffset = FMath::VInterpTo(CurrentLeftFootIKOffset, TargetLeftIK, DeltaTime, FootIKInterpSpeed);
	CurrentRightFootIKOffset = FMath::VInterpTo(CurrentRightFootIKOffset, TargetRightIK, DeltaTime, FootIKInterpSpeed);
	CurrentPelvisIKOffset = FMath::FInterpTo(CurrentPelvisIKOffset, TargetPelvisOffset, DeltaTime, FootIKInterpSpeed);
}

float UFPSBodyMechanicsComponent::PerformFootTrace(FName FootBoneName, FVector& OutHitNormal)
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return 0.0f;
	}

	FVector FootLocation = Owner->GetActorLocation();
	if (OwnerSkeletalMesh && OwnerSkeletalMesh->DoesSocketExist(FootBoneName))
	{
		FootLocation = OwnerSkeletalMesh->GetSocketLocation(FootBoneName);
	}
	else
	{
		// Fallback estimation for human male standing width (left -15cm, right +15cm)
		const float SideOffset = FootBoneName.ToString().Contains(TEXT("l")) ? -15.0f : 15.0f;
		FootLocation += Owner->GetActorRightVector() * SideOffset;
	}

	const FVector TraceStart = FVector(FootLocation.X, FootLocation.Y, Owner->GetActorLocation().Z + 15.0f);
	const FVector TraceEnd = FVector(FootLocation.X, FootLocation.Y, Owner->GetActorLocation().Z - FootTraceDistanceCm);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Owner);
	QueryParams.bTraceComplex = false;

	const bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams);
	if (bHit)
	{
		OutHitNormal = HitResult.ImpactNormal;
		// Distance difference from standard floor plane
		const float CapsuleBaseZ = Owner->GetActorLocation().Z - (OwnerMovementComponent ? OwnerMovementComponent->GetCharacterOwner()->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 88.0f);
		return HitResult.ImpactPoint.Z - CapsuleBaseZ;
	}

	OutHitNormal = FVector::UpVector;
	return 0.0f;
}

void UFPSBodyMechanicsComponent::ApplyBoneHitImpulse(FName HitBoneName, const FVector& ShotDirection, float DamageAmount)
{
	const EFPSBodyDamageZone Zone = FFPSHumanSkeleton::GetDamageZoneForBone(HitBoneName);
	const FFPSHumanBoneData* BoneData = FFPSHumanSkeleton::FindBoneDataByName(HitBoneName);

	// Accumulate limb trauma
	if (float* FoundTrauma = ZoneTraumaMap.Find(Zone))
	{
		*FoundTrauma = FMath::Clamp(*FoundTrauma + (DamageAmount / 100.0f), 0.0f, 1.0f);
		OnLimbTraumaChanged.Broadcast(Zone, *FoundTrauma);
	}

	// Calculate realistic directional flinch impulse & aim punch
	float FlinchMagnitude = DamageAmount * 0.15f * AimPunchIntensity;

	switch (Zone)
	{
	case EFPSBodyDamageZone::Head:
		// Severe vertical kick + random lateral disorientation
		AimPunchVelocity.Pitch += FMath::RandRange(25.0f, 40.0f) * AimPunchIntensity;
		AimPunchVelocity.Yaw += FMath::RandRange(-18.0f, 18.0f) * AimPunchIntensity;
		AimPunchVelocity.Roll += FMath::RandRange(-12.0f, 12.0f) * AimPunchIntensity;
		break;

	case EFPSBodyDamageZone::Neck:
	case EFPSBodyDamageZone::UpperTorso:
		// Chest impact: pushes torso backward and snaps pitch up
		AimPunchVelocity.Pitch += FMath::RandRange(12.0f, 20.0f) * AimPunchIntensity;
		AimPunchVelocity.Yaw += FMath::RandRange(-8.0f, 8.0f) * AimPunchIntensity;
		break;

	case EFPSBodyDamageZone::UpperArms:
	case EFPSBodyDamageZone::LowerArms:
	case EFPSBodyDamageZone::Hands:
		// Arm impact: causes horizontal weapon jerk
		{
			const bool bHitLeft = HitBoneName.ToString().Contains(TEXT("_l"));
			AimPunchVelocity.Yaw += (bHitLeft ? 15.0f : -15.0f) * AimPunchIntensity;
			AimPunchVelocity.Pitch += FMath::RandRange(-5.0f, 8.0f) * AimPunchIntensity;
		}
		break;

	case EFPSBodyDamageZone::Thighs:
	case EFPSBodyDamageZone::Calves:
	case EFPSBodyDamageZone::Feet:
		// Leg impact: downward stumble flinch
		AimPunchVelocity.Pitch -= FMath::RandRange(6.0f, 12.0f) * AimPunchIntensity;
		break;

	default:
		AimPunchVelocity.Pitch += FMath::RandRange(8.0f, 14.0f) * AimPunchIntensity;
		break;
	}

	OnBoneHitReaction.Broadcast(HitBoneName, ShotDirection * DamageAmount, FlinchMagnitude);
}

float UFPSBodyMechanicsComponent::GetZoneTraumaPercent(EFPSBodyDamageZone Zone) const
{
	const float* Found = ZoneTraumaMap.Find(Zone);
	return Found ? *Found : 0.0f;
}

bool UFPSBodyMechanicsComponent::IsLegInjured() const
{
	return GetZoneTraumaPercent(EFPSBodyDamageZone::Thighs) > 0.45f ||
		   GetZoneTraumaPercent(EFPSBodyDamageZone::Calves) > 0.45f;
}

bool UFPSBodyMechanicsComponent::IsArmInjured() const
{
	return GetZoneTraumaPercent(EFPSBodyDamageZone::UpperArms) > 0.45f ||
		   GetZoneTraumaPercent(EFPSBodyDamageZone::LowerArms) > 0.45f ||
		   GetZoneTraumaPercent(EFPSBodyDamageZone::Hands) > 0.45f;
}

void UFPSBodyMechanicsComponent::TriggerDeathRagdoll(const FVector& ImpactImpulse, const FVector& HitLocation, FName HitBoneName)
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character)
	{
		return;
	}

	// Disable movement and character capsule collision
	if (Character->GetCapsuleComponent())
	{
		Character->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Character->GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);
	}

	if (OwnerMovementComponent)
	{
		OwnerMovementComponent->DisableMovement();
		OwnerMovementComponent->StopMovementImmediately();
	}

	// Enable physical ragdoll simulation on skeletal mesh
	if (OwnerSkeletalMesh)
	{
		OwnerSkeletalMesh->SetCollisionProfileName(TEXT("Ragdoll"));
		OwnerSkeletalMesh->SetAllBodiesSimulatePhysics(true);
		OwnerSkeletalMesh->SetSimulatePhysics(true);
		OwnerSkeletalMesh->WakeAllRigidBodies();
		OwnerSkeletalMesh->bBlendPhysics = true;

		// Apply final kinetic bullet / explosive impulse at the exact impact bone
		const FName ValidBone = HitBoneName.IsNone() ? FFPSHumanSkeleton::GetPelvisBoneName() : HitBoneName;
		OwnerSkeletalMesh->AddImpulseAtLocation(ImpactImpulse * 15.0f, HitLocation, ValidBone);
	}
}

void UFPSBodyMechanicsComponent::DrawDebugSkeletalMechanics(float Duration)
{
	UWorld* World = GetWorld();
	AActor* Owner = GetOwner();
	if (!World || !Owner)
	{
		return;
	}

	const FVector ActorPos = Owner->GetActorLocation();
	const FRotator ActorRot = Owner->GetActorRotation();

	const TArray<FFPSHumanBoneData>& Bones = FFPSHumanSkeleton::GetMaleBoneRegistry();

	TMap<FName, FVector> ComputedWorldBonePositions;

	// Compute world positions for the general human male hierarchy
	for (const FFPSHumanBoneData& Bone : Bones)
	{
		FVector BoneWorldPos = ActorPos;

		if (Bone.ParentBoneName.IsNone() || Bone.ParentBoneName == TEXT("root"))
		{
			BoneWorldPos = ActorPos + ActorRot.RotateVector(Bone.LocalOffsetCm);
		}
		else if (const FVector* ParentPos = ComputedWorldBonePositions.Find(Bone.ParentBoneName))
		{
			BoneWorldPos = *ParentPos + ActorRot.RotateVector(Bone.LocalOffsetCm);
		}

		ComputedWorldBonePositions.Add(Bone.BoneName, BoneWorldPos);

		// Determine visual color by damage zone
		FColor BoneColor = FColor::White;
		switch (Bone.DamageZone)
		{
		case EFPSBodyDamageZone::Head:
		case EFPSBodyDamageZone::Neck:
			BoneColor = FColor::Red;
			break;
		case EFPSBodyDamageZone::UpperTorso:
		case EFPSBodyDamageZone::LowerTorso:
		case EFPSBodyDamageZone::Pelvis:
			BoneColor = Bone.bIsPlateCarrierProtected ? FColor::Yellow : FColor::Orange;
			break;
		case EFPSBodyDamageZone::UpperArms:
		case EFPSBodyDamageZone::LowerArms:
		case EFPSBodyDamageZone::Hands:
			BoneColor = FColor::Cyan;
			break;
		case EFPSBodyDamageZone::Thighs:
		case EFPSBodyDamageZone::Calves:
		case EFPSBodyDamageZone::Feet:
			BoneColor = FColor::Green;
			break;
		}

		// Draw joint sphere
		DrawDebugSphere(World, BoneWorldPos, 3.5f, 8, BoneColor, false, Duration, 0, 1.2f);

		// Draw anatomical collision capsule
		DrawDebugCapsule(World, BoneWorldPos, Bone.CollisionHalfHeightCm, Bone.CollisionRadiusCm, ActorRot.Quaternion(), BoneColor, false, Duration, 0, 0.8f);

		// Connect to parent bone with a skeletal line
		if (!Bone.ParentBoneName.IsNone() && Bone.ParentBoneName != TEXT("root"))
		{
			if (const FVector* ParentPos = ComputedWorldBonePositions.Find(Bone.ParentBoneName))
			{
				DrawDebugLine(World, *ParentPos, BoneWorldPos, FColor::White, false, Duration, 0, 2.0f);
			}
		}
	}
}
