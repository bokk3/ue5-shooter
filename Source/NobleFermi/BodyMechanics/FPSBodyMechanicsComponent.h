#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BodyMechanics/FPSHumanBones.h"
#include "FPSBodyMechanicsComponent.generated.h"

class USkeletalMeshComponent;
class UCharacterMovementComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnBoneHitReactionSignature, FName, HitBone, const FVector&, Impulse, float, FlinchIntensity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLimbTraumaChangedSignature, EFPSBodyDamageZone, Zone, float, TraumaPercent);

/**
 * AAA FPS Procedural Human Body Mechanics Component
 * Simulates human male physical body mechanics:
 * - Bone-specific hit flinch, rotational torque, and camera aim punch
 * - Procedural spine leaning and banking during locomotion
 * - Procedural two-bone foot IK for steps and uneven ground
 * - Anatomical joint limits and physical death ragdoll transition
 * - Limb trauma tracking with mobility and weapon handling degradation
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class NOBLEFERMI_API UFPSBodyMechanicsComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFPSBodyMechanicsComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(BlueprintAssignable, Category = "FPS Body|Events")
	FOnBoneHitReactionSignature OnBoneHitReaction;

	UPROPERTY(BlueprintAssignable, Category = "FPS Body|Events")
	FOnLimbTraumaChangedSignature OnLimbTraumaChanged;

	// Bone Impact & Aim Punch
	UFUNCTION(BlueprintCallable, Category = "FPS Body|Combat")
	void ApplyBoneHitImpulse(FName HitBoneName, const FVector& ShotDirection, float DamageAmount);

	// Foot IK Offsets
	UFUNCTION(BlueprintPure, Category = "FPS Body|IK")
	FVector GetLeftFootIKOffset() const { return CurrentLeftFootIKOffset; }

	UFUNCTION(BlueprintPure, Category = "FPS Body|IK")
	FVector GetRightFootIKOffset() const { return CurrentRightFootIKOffset; }

	UFUNCTION(BlueprintPure, Category = "FPS Body|IK")
	float GetPelvisIKOffset() const { return CurrentPelvisIKOffset; }

	// Procedural Spine Rotations
	UFUNCTION(BlueprintPure, Category = "FPS Body|Locomotion")
	FRotator GetProceduralSpineRotation() const { return CurrentSpineLeanRot; }

	UFUNCTION(BlueprintPure, Category = "FPS Body|Combat")
	FRotator GetCurrentAimPunch() const { return CurrentAimPunchRot; }

	// Trauma & Status
	UFUNCTION(BlueprintPure, Category = "FPS Body|Trauma")
	float GetZoneTraumaPercent(EFPSBodyDamageZone Zone) const;

	UFUNCTION(BlueprintPure, Category = "FPS Body|Trauma")
	bool IsLegInjured() const;

	UFUNCTION(BlueprintPure, Category = "FPS Body|Trauma")
	bool IsArmInjured() const;

	// Physical Ragdoll Transition
	UFUNCTION(BlueprintCallable, Category = "FPS Body|Physics")
	void TriggerDeathRagdoll(const FVector& ImpactImpulse, const FVector& HitLocation, FName HitBoneName);

	// Debug Visualizer
	UFUNCTION(BlueprintCallable, Category = "FPS Body|Debug")
	void DrawDebugSkeletalMechanics(float Duration = 0.0f);

	// Configuration
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Body|Locomotion")
	float MaxSpineLeanDegrees = 8.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Body|Locomotion")
	float SpineLeanInterpSpeed = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Body|Flinch")
	float AimPunchIntensity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Body|Flinch")
	float FlinchRecoverySpeed = 14.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Body|IK")
	bool bEnableFootIK = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Body|IK")
	float FootTraceDistanceCm = 65.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Body|IK")
	float FootIKInterpSpeed = 18.0f;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TObjectPtr<USkeletalMeshComponent> OwnerSkeletalMesh;

	UPROPERTY()
	TObjectPtr<UCharacterMovementComponent> OwnerMovementComponent;

	// Procedural Spine Lean State
	FRotator TargetSpineLeanRot = FRotator::ZeroRotator;
	FRotator CurrentSpineLeanRot = FRotator::ZeroRotator;

	// Flinch & Aim Punch Spring-Damper State
	FRotator TargetAimPunchRot = FRotator::ZeroRotator;
	FRotator CurrentAimPunchRot = FRotator::ZeroRotator;
	FRotator AimPunchVelocity = FRotator::ZeroRotator;

	// Foot IK State
	FVector CurrentLeftFootIKOffset = FVector::ZeroVector;
	FVector CurrentRightFootIKOffset = FVector::ZeroVector;
	float CurrentPelvisIKOffset = 0.0f;

	// Limb Trauma Tracking
	TMap<EFPSBodyDamageZone, float> ZoneTraumaMap;

	// Internal Sub-routines
	void UpdateSpineLeaning(float DeltaTime);
	void UpdateFlinchPhysics(float DeltaTime);
	void UpdateProceduralFootIK(float DeltaTime);
	float PerformFootTrace(FName FootBoneName, FVector& OutHitNormal);
};
