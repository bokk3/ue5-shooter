#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "FPSCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class USkeletalMeshComponent;
class UFPSCharacterMovementComponent;
class UFPSHealthComponent;
class UFPSInventoryComponent;
class UFPSHUDWidget;

/**
 * AAA Tactical Character Class (Primary 3rd-Person Perspective)
 * Features dynamic over-the-shoulder tactical camera, camera lag, collision probe test,
 * shoulder swapping, anatomical SK_BaseMale operator body, PBR ripstop uniform,
 * full locomotion gait cycles, weapon socketing, and authoritative ragdoll physics.
 */
UCLASS()
class NOBLEFERMI_API AFPSCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AFPSCharacter(const FObjectInitializer& ObjectInitializer);

	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** Primary 3rd-Person Tactical Camera Boom */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	/** Primary 3rd-Person Tactical Follow Camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	/** First-Person Camera (Kept for secondary inspection / legacy fallback) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> FirstPersonCamera;

	/** First-Person Mesh (Hidden in 3rd person mode) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	TObjectPtr<USkeletalMeshComponent> Mesh1P;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UFPSCharacterMovementComponent> FPSMovementComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UFPSHealthComponent> HealthComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UFPSInventoryComponent> InventoryComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UFPSEquipmentComponent> EquipmentComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UFPSBodyMechanicsComponent> BodyMechanicsComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UFPSHUDWidget> HUDWidgetClass;

	UFUNCTION(BlueprintPure, Category = "FPS Character|UI")
	UFPSHUDWidget* GetActiveHUDWidget() const { return ActiveHUDWidget; }

	UFUNCTION(BlueprintPure, Category = "FPS Character|Components")
	UFPSCharacterMovementComponent* GetFPSMovementComponent() const { return FPSMovementComponent; }

	UFUNCTION(BlueprintPure, Category = "FPS Character|Components")
	UFPSInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

	UFUNCTION(BlueprintPure, Category = "FPS Character|Components")
	class UFPSBodyMechanicsComponent* GetBodyMechanicsComponent() const { return BodyMechanicsComponent; }

	// Tactical Locomotion Animations
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS Character|Animations")
	TObjectPtr<class UAnimSequence> AnimIdleADS;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS Character|Animations")
	TObjectPtr<class UAnimSequence> AnimWalk;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS Character|Animations")
	TObjectPtr<class UAnimSequence> AnimRun;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS Character|Animations")
	TObjectPtr<class UAnimSequence> AnimSprint;

	// Tactical 3rd-Person Over-The-Shoulder (OTS) Camera Settings (Phase 8A Spec)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS Character|Camera")
	float DefaultArmLength = 210.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS Character|Camera")
	float ADSArmLength = 95.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS Character|Camera")
	FVector HipfireShoulderOffset = FVector(0.0f, 52.0f, 22.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS Character|Camera")
	FVector ADSShoulderOffset = FVector(0.0f, 34.0f, 16.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS Character|Camera")
	float DefaultFOV = 90.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS Character|Camera")
	float ADSFOV = 62.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Character|Camera")
	bool bIsRightShoulder = true;

	/** Smoothly interpolated between +1.0f (Right Shoulder) and -1.0f (Left Shoulder) at speed 10.0f */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Character|Camera")
	float ShoulderSideMultiplier = 1.0f;

	/** Hybrid toggle (MiddleMouseButton while aiming): switches between 3rd-Person Tight OTS ADS and 1st-Person True Optic Red-Dot ADS */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Character|Camera")
	bool bUseFirstPersonOpticADS = false;

	UFUNCTION(BlueprintCallable, Category = "FPS Character|Camera")
	void ToggleShoulderSwap();

	UFUNCTION(BlueprintCallable, Category = "FPS Character|Camera")
	void ToggleOpticADSMode();

	void OnMiddleMouseButtonPressed();
	void OnMiddleMouseButtonReleased();

	UFUNCTION(BlueprintPure, Category = "FPS Character|Camera")
	bool IsAiming() const;

	UFUNCTION(BlueprintPure, Category = "FPS Character|Camera")
	bool IsRightShoulder() const { return bIsRightShoulder; }

	// The Fermi Paradox Engine (Dive, Clone, Swap, Shatter Time)
	UFUNCTION(BlueprintCallable, Category = "FPS Character|Fermi Engine")
	void TriggerBallisticDive();

	UFUNCTION(BlueprintCallable, Category = "FPS Character|Fermi Engine")
	void TriggerFermiEchoOrQuantumSwap();

	UFUNCTION(BlueprintPure, Category = "FPS Character|Fermi Engine")
	bool IsBallisticDiving() const;

	UFUNCTION(BlueprintPure, Category = "FPS Character|Fermi Engine")
	bool IsFermiTimeBubbleActive() const;

	UFUNCTION(BlueprintPure, Category = "FPS Character|Fermi Engine")
	float GetFermiTimeBubbleRemaining() const;

	UFUNCTION(BlueprintPure, Category = "FPS Character|Fermi Engine")
	bool IsFermiDecoyActive() const;

	UFUNCTION(BlueprintPure, Category = "FPS Character|Fermi Engine")
	float GetFermiDecoyRemaining() const;

	UFUNCTION(BlueprintPure, Category = "FPS Character|Fermi Engine")
	float GetFermiDecoyCooldownRemaining() const { return FermiDecoyCooldownRemaining; }

	UFUNCTION(BlueprintPure, Category = "FPS Character|Fermi Engine")
	float GetQuantumSwapFlashAlpha() const { return FMath::Clamp(QuantumSwapFlashTimer / 0.35f, 0.0f, 1.0f); }

	// Actions
	UFUNCTION(BlueprintCallable, Category = "FPS Character|Actions")
	void StartCookingLethal();

	UFUNCTION(BlueprintCallable, Category = "FPS Character|Actions")
	void ReleaseLethal();

	UFUNCTION(BlueprintCallable, Category = "FPS Character|Actions")
	void ThrowTactical();

	UFUNCTION(BlueprintCallable, Category = "FPS Character|Actions")
	void StartFire();

	UFUNCTION(BlueprintCallable, Category = "FPS Character|Actions")
	void StopFire();

	UFUNCTION(BlueprintCallable, Category = "FPS Character|Actions")
	void StartADS();

	UFUNCTION(BlueprintCallable, Category = "FPS Character|Actions")
	void StopADS();

	UFUNCTION(BlueprintCallable, Category = "FPS Character|Actions")
	void Reload();

	UFUNCTION(BlueprintCallable, Category = "FPS Character|Actions")
	void StartSprint();

	UFUNCTION(BlueprintCallable, Category = "FPS Character|Actions")
	void StopSprint();

	UFUNCTION(BlueprintCallable, Category = "FPS Character|Actions")
	void StartCrouchOrSlide();

	UFUNCTION(BlueprintCallable, Category = "FPS Character|Actions")
	void StopCrouchOrSlide();

	UFUNCTION(BlueprintCallable, Category = "FPS Character|Actions")
	void OnJumpOrDivePressed();

	// Surface Footstep Audio Assets (Concrete, Wood, Sand/Dirt, Water/Puddle, Metal)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS Character|Audio|Footsteps")
	TArray<TObjectPtr<USoundBase>> FootstepConcreteSounds;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS Character|Audio|Footsteps")
	TArray<TObjectPtr<USoundBase>> FootstepWoodSounds;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS Character|Audio|Footsteps")
	TArray<TObjectPtr<USoundBase>> FootstepSandSounds;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS Character|Audio|Footsteps")
	TArray<TObjectPtr<USoundBase>> FootstepWaterSounds;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS Character|Audio|Footsteps")
	TArray<TObjectPtr<USoundBase>> FootstepMetalSounds;

	UFUNCTION(BlueprintCallable, Category = "FPS Character|Audio|Footsteps")
	void TriggerSurfaceFootstep(float VolumeScale);

	// Death Handling
	UFUNCTION()
	void HandleOnDeath(AActor* KillerActor);

	// Input Handlers
	void MoveForward(float Value);
	void MoveRight(float Value);
	void Turn(float Rate);
	void LookUp(float Rate);

protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_Controller() override;
	void InitializeHUD();

private:
	UPROPERTY()
	TObjectPtr<UFPSHUDWidget> ActiveHUDWidget;

	UPROPERTY()
	TObjectPtr<class AFPSFermiTimeBubble> ActiveTimeBubble;

	UPROPERTY()
	TObjectPtr<class AFPSFermiDecoyActor> ActiveFermiDecoy;

	// Tactical Camera, Dive Posture & Locomotion Updates
	void UpdateThirdPersonCamera(float DeltaTime);
	void UpdateBallisticDivePosture(float DeltaTime);
	void UpdateProceduralCameraBob(float DeltaTime);
	void UpdateWeaponSway(float DeltaTime);
	void UpdateLocomotionAnimation(float DeltaTime);

	UPROPERTY()
	TObjectPtr<class UAnimSequence> CurrentLocomotionAnim;

	FVector2D CurrentMoveInput = FVector2D::ZeroVector;
	FRotator CurrentDiveMeshRotation = FRotator(0.0f, -90.0f, 0.0f);
	float CurrentDiveMeshZOffset = -96.0f;
	float FermiDecoyCooldownRemaining = 0.0f;
	float QuantumSwapFlashTimer = 0.0f;
	float LastCrouchPressTime = -10.0f;
	bool bIsHoldingCrouchKey = false;
	bool bShouldCaptureHUD = false;
	int32 CaptureHUDFrameCounter = 0;

	float BobTimer = 0.0f;
	FVector DefaultCameraPos;
	FRotator LastControlRotation;
	FRotator CurrentWeaponSwayRot;
	bool bIsDead = false;

	float FootstepDistanceTraveled = 0.0f;
	int32 FootstepAlternator = 0;
};
