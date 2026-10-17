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

	if (DiveCooldownRemaining > 0.0f)
	{
		DiveCooldownRemaining = FMath::Max(0.0f, DiveCooldownRemaining - DeltaSeconds);
	}

	if (bIsBallisticDiving)
	{
		DiveAirborneTime += DeltaSeconds;
		if (!IsMovingOnGround())
		{
			bHasLeftGroundDuringDive = true;
		}
		else if (bHasLeftGroundDuringDive || DiveAirborneTime > 0.15f)
		{
			// Landed from airborne ballistic dive -> transition to 0.45s kinetic ground slide
			if (!bIsDiveLandingSlide)
			{
				bIsDiveLandingSlide = true;
				DiveLandingSlideTimer = DiveLandingSlideDuration;
				bMaintainHorizontalGroundVelocity = true;

				// Project directional momentum smoothly along floor slope/ramp normal (e.g. Rust Oil Derrick 25° ramps)
				FVector SlideDir = BallisticDiveDirection;
				if (CurrentFloor.IsWalkableFloor() && CurrentFloor.HitResult.ImpactNormal.Z < 0.999f && CurrentFloor.HitResult.ImpactNormal.Z > 0.1f)
				{
					SlideDir = FVector::VectorPlaneProject(BallisticDiveDirection, CurrentFloor.HitResult.ImpactNormal).GetSafeNormal();
				}
				Velocity = SlideDir * (DiveHorizontalLaunchSpeed * 0.72f);
			}
			else
			{
				DiveLandingSlideTimer -= DeltaSeconds;

				// Smooth non-snagging deceleration preserving momentum on slopes
				if (CurrentFloor.IsWalkableFloor() && CurrentFloor.HitResult.ImpactNormal.Z < 0.999f && CurrentFloor.HitResult.ImpactNormal.Z > 0.1f)
				{
					const FVector SlopeDir = FVector::VectorPlaneProject(BallisticDiveDirection, CurrentFloor.HitResult.ImpactNormal).GetSafeNormal();
					const float DownhillDot = FVector::DotProduct(SlopeDir, FVector::DownVector);
					if (DownhillDot > 0.1f)
					{
						// Retain momentum when sliding down ramps
						Velocity = FMath::VInterpTo(Velocity, SlopeDir * (DiveHorizontalLaunchSpeed * 0.60f), DeltaSeconds, 1.8f);
					}
					else
					{
						Velocity = FMath::VInterpTo(Velocity, FVector::ZeroVector, DeltaSeconds, 3.2f);
					}
				}
				else
				{
					Velocity = FMath::VInterpTo(Velocity, FVector::ZeroVector, DeltaSeconds, 3.8f);
				}

				if (DiveLandingSlideTimer <= 0.0f)
				{
					StopBallisticDive();
				}
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
	case EFPSMovementMode::BallisticDiving:
		return DiveHorizontalLaunchSpeed;
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
	if (bSprint && (bIsStaminaExhausted || CurrentStamina <= 5.0f || bIsBallisticDiving))
	{
		bWantsToSprint = false;
		return;
	}

	bWantsToSprint = bSprint;

	if (bWantsToSprint && IsMovingOnGround() && !IsCrouching() && !bIsSliding && !bIsBallisticDiving)
	{
		CurrentFPSMode = EFPSMovementMode::Sprinting;
	}
	else if (!bIsSliding && !IsCrouching() && !bIsBallisticDiving)
	{
		CurrentFPSMode = EFPSMovementMode::Walking;
	}
}

void UFPSCharacterMovementComponent::StartSlide()
{
	if (!IsMovingOnGround() || bIsSliding || bIsBallisticDiving) return;

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

bool UFPSCharacterMovementComponent::TryStartBallisticDive(const FVector& DesiredWorldDirection)
{
	if (!CanStartBallisticDive())
	{
		return false;
	}

	ACharacter* CharOwner = GetCharacterOwner();
	if (!CharOwner)
	{
		return false;
	}

	if (bIsSliding)
	{
		StopSlide();
	}
	else if (IsCrouching())
	{
		CharOwner->UnCrouch();
	}

	// Deduct stamina cost (35.0f, requires >= 25.0f)
	CurrentStamina = FMath::Max(0.0f, CurrentStamina - DiveStaminaCost);
	TimeSinceLastSprint = 0.0f;
	if (CurrentStamina <= 0.0f)
	{
		bIsStaminaExhausted = true;
	}

	FVector DiveDir = DesiredWorldDirection.GetSafeNormal2D();
	if (DiveDir.IsNearlyZero())
	{
		DiveDir = Velocity.GetSafeNormal2D();
	}
	if (DiveDir.IsNearlyZero())
	{
		DiveDir = CharOwner->GetActorForwardVector().GetSafeNormal2D();
	}

	BallisticDiveDirection = DiveDir;
	bIsBallisticDiving = true;
	bHasLeftGroundDuringDive = false;
	bIsDiveLandingSlide = false;
	DiveAirborneTime = 0.0f;
	DiveLandingSlideTimer = 0.0f;
	DiveCooldownRemaining = DiveCooldownDuration;
	CurrentFPSMode = EFPSMovementMode::BallisticDiving;

	// Launch character into omnidirectional parabolic arc (820 cm/s horizontal, 340 cm/s upward)
	SetMovementMode(MOVE_Falling);
	Velocity = (BallisticDiveDirection * DiveHorizontalLaunchSpeed) + FVector(0.0f, 0.0f, DiveVerticalLaunchSpeed);
	return true;
}

void UFPSCharacterMovementComponent::StopBallisticDive()
{
	bIsBallisticDiving = false;
	bHasLeftGroundDuringDive = false;
	bIsDiveLandingSlide = false;
	bMaintainHorizontalGroundVelocity = false;
	DiveAirborneTime = 0.0f;
	DiveLandingSlideTimer = 0.0f;
	CurrentFPSMode = bWantsToSprint ? EFPSMovementMode::Sprinting : EFPSMovementMode::Walking;
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
