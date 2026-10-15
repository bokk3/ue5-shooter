#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "FPSCharacterMovementComponent.generated.h"

UENUM(BlueprintType)
enum class EFPSMovementMode : uint8
{
	None,
	Walking,
	Sprinting,
	Sliding,
	Crouching,
	Mantling
};

/**
 * AAA FPS Custom Character Movement Component
 * Supports Tactical Sprinting, Momentum Sliding, Smooth Crouching, and Procedural Mantling.
 */
UCLASS()
class NOBLEFERMI_API UFPSCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	UFPSCharacterMovementComponent();

	virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;
	virtual float GetMaxSpeed() const override;

	// Movement State Queries
	UFUNCTION(BlueprintPure, Category = "FPS Movement")
	EFPSMovementMode GetFPSMovementMode() const { return CurrentFPSMode; }

	UFUNCTION(BlueprintPure, Category = "FPS Movement")
	bool IsTacticalSprinting() const { return CurrentFPSMode == EFPSMovementMode::Sprinting; }

	UFUNCTION(BlueprintPure, Category = "FPS Movement")
	bool IsSliding() const { return CurrentFPSMode == EFPSMovementMode::Sliding; }

	// Input Intents
	UFUNCTION(BlueprintCallable, Category = "FPS Movement")
	void SetWantsToSprint(bool bSprint);

	UFUNCTION(BlueprintCallable, Category = "FPS Movement")
	void StartSlide();

	UFUNCTION(BlueprintCallable, Category = "FPS Movement")
	void StopSlide();

	UFUNCTION(BlueprintCallable, Category = "FPS Movement")
	bool TryMantle();

	// Speeds and Modifiers
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Movement|Speeds")
	float WalkSpeed = 450.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Movement|Speeds")
	float SprintSpeed = 750.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Movement|Speeds")
	float SlideImpulse = 950.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Movement|Speeds")
	float MinSlideSpeed = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Movement|Speeds")
	float SlideFriction = 0.6f;

	// Mantling Settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Movement|Mantle")
	float MaxMantleHeight = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Movement|Mantle")
	float MinMantleHeight = 50.0f;

	// Stamina System
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Movement|Stamina")
	float MaxStamina = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Movement|Stamina")
	float CurrentStamina = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Movement|Stamina")
	float StaminaSprintDrainRate = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Movement|Stamina")
	float StaminaRegenRate = 28.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Movement|Stamina")
	float StaminaRegenDelay = 0.8f;

	UFUNCTION(BlueprintPure, Category = "FPS Movement|Stamina")
	float GetCurrentStamina() const { return CurrentStamina; }

	UFUNCTION(BlueprintPure, Category = "FPS Movement|Stamina")
	float GetMaxStamina() const { return MaxStamina; }

	UFUNCTION(BlueprintPure, Category = "FPS Movement|Stamina")
	float GetStaminaPercent() const { return MaxStamina > 0.0f ? FMath::Clamp(CurrentStamina / MaxStamina, 0.0f, 1.0f) : 0.0f; }

	UFUNCTION(BlueprintPure, Category = "FPS Movement|Stamina")
	bool IsStaminaExhausted() const { return bIsStaminaExhausted; }

private:
	UPROPERTY()
	EFPSMovementMode CurrentFPSMode = EFPSMovementMode::Walking;

	bool bWantsToSprint = false;
	bool bIsSliding = false;
	bool bIsStaminaExhausted = false;
	float TimeSinceLastSprint = 0.0f;
	FVector SlideDirection = FVector::ZeroVector;
};
