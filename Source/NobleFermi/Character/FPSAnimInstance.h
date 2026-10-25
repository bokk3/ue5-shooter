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
 * 4. Procedural Left-Hand Two-Bone IK solver (AnimationCore::SolveTwoBoneIK) locking hand_l to Carbine / SM_AK47
 * 5. Full thoracic spine pitch alignment (+-89 deg), procedural flinch / aim punch, and Menu Low-Ready Carbine stance.
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

	// Right-Hand Weapon Grip Target Data
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Weapon IK")
	FVector RightHandIKLocation = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Weapon IK")
	FRotator RightHandIKRotation = FRotator::ZeroRotator;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Weapon IK")
	FVector RightElbowPoleVector = FVector::ZeroVector;

	// Procedural Gait & Breathing Phase
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Locomotion")
	float LocomotionPhase = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Locomotion")
	float BreathingPhase = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Locomotion")
	float CurrentADSAlpha = 0.0f;

	// Menu Preview Turntable Stance Controls (0 = Low-Ready Carbine, 1 = High-Ready ADS, 2 = Patrol Walk)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Menu Preview")
	bool bIsMenuPreview = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Menu Preview")
	int32 MenuPoseIndex = 0;

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
	// 21-Bone Anatomical Skeletal Indices
	int32 Pelvis_Index = INDEX_NONE;
	int32 Spine01_Index = INDEX_NONE;
	int32 Spine02_Index = INDEX_NONE;
	int32 Spine03_Index = INDEX_NONE;
	int32 Neck01_Index = INDEX_NONE;
	int32 Head_Index = INDEX_NONE;

	int32 ClavicleL_Index = INDEX_NONE;
	int32 ClavicleR_Index = INDEX_NONE;
	int32 UpperArmL_Index = INDEX_NONE;
	int32 UpperArmR_Index = INDEX_NONE;
	int32 LowerArmL_Index = INDEX_NONE;
	int32 LowerArmR_Index = INDEX_NONE;
	int32 HandL_Index = INDEX_NONE;
	int32 HandR_Index = INDEX_NONE;

	int32 ThighL_Index = INDEX_NONE;
	int32 ThighR_Index = INDEX_NONE;
	int32 CalfL_Index = INDEX_NONE;
	int32 CalfR_Index = INDEX_NONE;
	int32 FootL_Index = INDEX_NONE;
	int32 FootR_Index = INDEX_NONE;

	TArray<int32> LeftFingerBoneIndices;
	TArray<int32> RightFingerBoneIndices;
	TArray<FTransform> RestCompBases;
	bool bBoneIndicesCached = false;

	void CacheBoneIndices();
	void UpdateIKTargetFromWeapon(AFPSWeaponBase* Weapon);
	void ApplyLowerBodyLocomotion(TArray<FTransform>& SpaceBases);
	void ApplySpineAimPitchAndTwist(TArray<FTransform>& SpaceBases);
	void SolveRightArmRifleIK(TArray<FTransform>& SpaceBases);
	void SolveLeftHandTwoBoneIK(TArray<FTransform>& SpaceBases);
};
