#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "FPSAnimInstance.generated.h"

class AFPSCharacter;
class UFPSCharacterMovementComponent;
class UFPSBodyMechanicsComponent;

/**
 * AAA Tactical Locomotion Animation Instance
 * Exposes authoritative movement telemetry, procedural spine leaning,
 * 2-bone foot IK, and aim punch flinch to character skeletal meshes.
 */
UCLASS()
class NOBLEFERMI_API UFPSAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	UFPSAnimInstance();

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	// Ground Locomotion Telemetry
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	float GroundSpeed = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	float Direction = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	bool bIsMoving = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	bool bIsAiming = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	bool bIsSprinting = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	bool bIsCrouching = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	bool bIsFalling = false;

	// Procedural Spine & Flinch Mechanics
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Procedural Mechanics")
	FRotator ProceduralSpineLean = FRotator::ZeroRotator;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Procedural Mechanics")
	FRotator AimPunchRotation = FRotator::ZeroRotator;

	// Foot IK Offsets
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Foot IK")
	FVector LeftFootIKOffset = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Foot IK")
	FVector RightFootIKOffset = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Foot IK")
	float PelvisIKOffset = 0.0f;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Owner References")
	TObjectPtr<AFPSCharacter> OwningCharacter;

	UPROPERTY(BlueprintReadOnly, Category = "Owner References")
	TObjectPtr<UFPSCharacterMovementComponent> OwningMovement;

	UPROPERTY(BlueprintReadOnly, Category = "Owner References")
	TObjectPtr<UFPSBodyMechanicsComponent> OwningBodyMechanics;
};
