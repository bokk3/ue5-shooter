#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "FPSAnimInstance.generated.h"

class AFPSCharacter;
class UFPSCharacterMovementComponent;
class UFPSBodyMechanicsComponent;
class AFPSWeaponBase;

/**
 * AAA Tactical Locomotion Animation Instance
 * Implements:
 * 1. Authoritative movement telemetry aligned with UFPSCharacterMovementComponent velocity tiers
 * 2. Directional locomotion blend space support (Walk, Run, Sprint, Backwards, Crouch Walk)
 * 3. Layered blend per bone at spine_01 for independent upper-body aim/low-ready stances
 * 4. Procedural Left-Hand Two-Bone IK solver (AnimationCore::SolveTwoBoneIK) locking hand_l to SM_AK47
 * 5. Full thoracic spine pitch alignment (+-89 deg) and procedural flinch / aim punch.
 */
UCLASS()
class NOBLEFERMI_API UFPSAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	UFPSAnimInstance();

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativePostEvaluateAnimation() override;

	// Ground Locomotion Telemetry Aligned with UFPSCharacterMovementComponent
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Locomotion")
	float GroundSpeed = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Locomotion")
	float Direction = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Locomotion")
	bool bIsMoving = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Locomotion")
	bool bIsAiming = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Locomotion")
	bool bIsSprinting = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Locomotion")
	bool bIsCrouching = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Locomotion")
	bool bIsFalling = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Locomotion")
	bool bIsSliding = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Locomotion")
	bool bIsBallisticDiving = false;

	// Authoritative Movement Speed Thresholds (from UFPSCharacterMovementComponent)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS Locomotion|Thresholds")
	float WalkSpeedThreshold = 450.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS Locomotion|Thresholds")
	float CrouchSpeedThreshold = 260.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS Locomotion|Thresholds")
	float SprintSpeedThreshold = 750.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS Locomotion|Thresholds")
	float SlideSpeedThreshold = 950.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS Locomotion|Thresholds")
	float DiveSpeedThreshold = 820.0f;

	// Thoracic Aim Pitch & Leaning
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Procedural Aim")
	float AimPitch = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Procedural Aim")
	float AimYaw = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Procedural Aim")
	float ShoulderSideMultiplier = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Procedural Aim")
	float UpperBodyBlendWeight = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Procedural Aim")
	FRotator ProceduralSpineLean = FRotator::ZeroRotator;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Procedural Aim")
	FRotator AimPunchRotation = FRotator::ZeroRotator;

	// Left-Hand Two-Bone IK Solver Data
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Left Hand IK")
	FVector LeftHandIKLocation = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Left Hand IK")
	FRotator LeftHandIKRotation = FRotator::ZeroRotator;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Left Hand IK")
	FVector LeftElbowPoleVector = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Left Hand IK")
	bool bLeftHandIKValid = false;

	// Foot IK Offsets
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Foot IK")
	FVector LeftFootIKOffset = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Foot IK")
	FVector RightFootIKOffset = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Foot IK")
	float PelvisIKOffset = 0.0f;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "FPS Owner References")
	TObjectPtr<AFPSCharacter> OwningCharacter;

	UPROPERTY(BlueprintReadOnly, Category = "FPS Owner References")
	TObjectPtr<UFPSCharacterMovementComponent> OwningMovement;

	UPROPERTY(BlueprintReadOnly, Category = "FPS Owner References")
	TObjectPtr<UFPSBodyMechanicsComponent> OwningBodyMechanics;

private:
	int32 UpperArmL_Index = INDEX_NONE;
	int32 LowerArmL_Index = INDEX_NONE;
	int32 HandL_Index = INDEX_NONE;
	int32 Spine01_Index = INDEX_NONE;
	int32 Spine02_Index = INDEX_NONE;
	int32 Spine03_Index = INDEX_NONE;
	int32 Neck01_Index = INDEX_NONE;
	int32 Head_Index = INDEX_NONE;
	TArray<int32> LeftFingerBoneIndices;
	bool bBoneIndicesCached = false;

	void CacheBoneIndices();
	void UpdateIKTargetFromWeapon(AFPSWeaponBase* Weapon);
	void SolveLeftHandTwoBoneIK(TArray<FTransform>& SpaceBases);
	void ApplySpineAimPitch(TArray<FTransform>& SpaceBases);
};
