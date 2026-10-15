#include "Character/FPSCharacterMovementComponent.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"

UFPSCharacterMovementComponent::UFPSCharacterMovementComponent()
{
	MaxWalkSpeed = 450.0f;
	MaxWalkSpeedCrouched = 260.0f;
	BrakingDecelerationWalking = 1800.0f;
	GroundFriction = 7.0f;
	AirControl = 0.35f;
	JumpZVelocity = 520.0f;
	bCanWalkOffLedgesWhenCrouching = true;
	NavAgentProps.bCanCrouch = true;
}

void UFPSCharacterMovementComponent::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
	Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);

	// Stamina drain and regeneration
	if (CurrentFPSMode == EFPSMovementMode::Sprinting && Velocity.Size2D() > WalkSpeed)
	{
		CurrentStamina = FMath::Max(0.0f, CurrentStamina - StaminaSprintDrainRate * DeltaSeconds);
		TimeSinceLastSprint = 0.0f;

		if (CurrentStamina <= 0.0f)
		{
			bIsStaminaExhausted = true;
			SetWantsToSprint(false);
		}
	}
	else
	{
		TimeSinceLastSprint += DeltaSeconds;
		if (TimeSinceLastSprint >= StaminaRegenDelay)
		{
			CurrentStamina = FMath::Min(MaxStamina, CurrentStamina + StaminaRegenRate * DeltaSeconds);
			if (CurrentStamina >= 25.0f)
			{
				bIsStaminaExhausted = false;
			}
		}
	}

	if (bIsSliding)
	{
		// Check if sliding velocity dropped below threshold
		float HorizontalSpeed = Velocity.Size2D();
		if (HorizontalSpeed < MinSlideSpeed || !IsMovingOnGround())
		{
			StopSlide();
		}
		else
		{
			// Apply sliding friction and directional momentum
			Velocity += SlideDirection * (BrakingDecelerationWalking * 0.2f * DeltaSeconds);
		}
	}
}

float UFPSCharacterMovementComponent::GetMaxSpeed() const
{
	switch (CurrentFPSMode)
	{
	case EFPSMovementMode::Sliding:
		return SlideImpulse;
	case EFPSMovementMode::Sprinting:
		return SprintSpeed;
	case EFPSMovementMode::Crouching:
		return MaxWalkSpeedCrouched;
	case EFPSMovementMode::Walking:
	default:
		return WalkSpeed;
	}
}

void UFPSCharacterMovementComponent::SetWantsToSprint(bool bSprint)
{
	if (bSprint && (bIsStaminaExhausted || CurrentStamina <= 5.0f))
	{
		bWantsToSprint = false;
		return;
	}

	bWantsToSprint = bSprint;

	if (bWantsToSprint && IsMovingOnGround() && !IsCrouching() && !bIsSliding)
	{
		CurrentFPSMode = EFPSMovementMode::Sprinting;
	}
	else if (!bIsSliding && !IsCrouching())
	{
		CurrentFPSMode = EFPSMovementMode::Walking;
	}
}

void UFPSCharacterMovementComponent::StartSlide()
{
	if (!IsMovingOnGround() || bIsSliding) return;

	if (Velocity.Size2D() > WalkSpeed * 0.8f)
	{
		bIsSliding = true;
		CurrentFPSMode = EFPSMovementMode::Sliding;
		SlideDirection = Velocity.GetSafeNormal2D();
		Velocity = SlideDirection * SlideImpulse;

		ACharacter* CharOwner = GetCharacterOwner();
		if (CharOwner)
		{
			CharOwner->Crouch();
		}
	}
}

void UFPSCharacterMovementComponent::StopSlide()
{
	bIsSliding = false;
	CurrentFPSMode = bWantsToSprint ? EFPSMovementMode::Sprinting : EFPSMovementMode::Walking;

	ACharacter* CharOwner = GetCharacterOwner();
	if (CharOwner)
	{
		CharOwner->UnCrouch();
	}
}

bool UFPSCharacterMovementComponent::TryMantle()
{
	ACharacter* CharOwner = GetCharacterOwner();
	if (!CharOwner) return false;

	UWorld* World = GetWorld();
	if (!World) return false;

	FVector Start = CharOwner->GetActorLocation();
	FVector Forward = CharOwner->GetActorForwardVector();
	FVector End = Start + (Forward * 80.0f);

	FHitResult WallHit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(CharOwner);

	// Trace forward for wall
	if (World->LineTraceSingleByChannel(WallHit, Start, End, ECC_Visibility, Params))
	{
		// Trace downward to find the top surface
		FVector TopTraceStart = WallHit.Location + (Forward * 20.0f) + FVector(0, 0, MaxMantleHeight);
		FVector TopTraceEnd = TopTraceStart - FVector(0, 0, MaxMantleHeight - MinMantleHeight);

		FHitResult TopHit;
		if (World->LineTraceSingleByChannel(TopHit, TopTraceStart, TopTraceEnd, ECC_Visibility, Params))
		{
			float ClimbHeight = TopHit.Location.Z - Start.Z;
			if (ClimbHeight >= MinMantleHeight && ClimbHeight <= MaxMantleHeight)
			{
				// Boost character upward onto ledge
				Velocity = FVector(Forward.X * 250.0f, Forward.Y * 250.0f, 480.0f);
				CurrentFPSMode = EFPSMovementMode::Mantling;
				return true;
			}
		}
	}

	return false;
}
