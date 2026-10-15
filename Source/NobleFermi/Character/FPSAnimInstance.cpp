#include "Character/FPSAnimInstance.h"
#include "Character/FPSCharacter.h"
#include "Character/FPSCharacterMovementComponent.h"
#include "BodyMechanics/FPSBodyMechanicsComponent.h"
#include "Inventory/FPSInventoryComponent.h"
#include "Weapons/FPSWeaponBase.h"
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

	// 1. Velocity and Direction
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

	// 2. Movement States
	if (OwningMovement)
	{
		bIsSprinting = OwningMovement->IsTacticalSprinting();
		bIsCrouching = OwningMovement->IsCrouching();
		bIsFalling = OwningMovement->IsFalling();
	}

	// 3. Aiming State
	if (OwningCharacter->GetInventoryComponent() && OwningCharacter->GetInventoryComponent()->GetActiveWeapon())
	{
		bIsAiming = OwningCharacter->GetInventoryComponent()->GetActiveWeapon()->IsAimingDownSights();
	}
	else
	{
		bIsAiming = false;
	}

	// 4. Procedural Spine Lean & Aim Punch Flinch
	if (OwningBodyMechanics)
	{
		ProceduralSpineLean = OwningBodyMechanics->GetProceduralSpineRotation();
		AimPunchRotation = OwningBodyMechanics->GetCurrentAimPunch();
		LeftFootIKOffset = OwningBodyMechanics->GetLeftFootIKOffset();
		RightFootIKOffset = OwningBodyMechanics->GetRightFootIKOffset();
		PelvisIKOffset = OwningBodyMechanics->GetPelvisIKOffset();
	}
}
