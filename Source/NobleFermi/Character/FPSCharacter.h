#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Multiplayer/FPSPlayerState.h"
#include "FPSCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class USkeletalMeshComponent;
class UStaticMeshComponent;
class UProceduralMeshComponent;
class UFPSCharacterMovementComponent;
class UFPSHealthComponent;
class UFPSInventoryComponent;
class UFPSHUDWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCharacterCombatAccoladeSignature, const FString&, AccoladeName, int32, GoldReward);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBountyEventSignature, FName, EventTag, int32, Count);

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

	/** 7-Zone Socket-Driven 3D Tactical Gear & Armor Rig Component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Character|Gear")
	TObjectPtr<class UFPSOperatorGearComponent> OperatorGearComponent;

	/** 3rd-Person Level IV Ceramic Plate Carrier Procedural Mesh attached to spine_03 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Character|Gear")
	TObjectPtr<UProceduralMeshComponent> PlateCarrierProcMesh;

	/** 3rd-Person Tactical Ballistic Helmet Procedural Mesh attached to head */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Character|Gear")
	TObjectPtr<UProceduralMeshComponent> BallisticHelmetProcMesh;

	UFUNCTION(BlueprintCallable, Category = "FPS Character|Gear")
	void BuildProceduralTacticalGear();

	/** 3rd-Person Level IV Ceramic Plate Carrier attached to spine_03 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Character|Gear")
	TObjectPtr<UStaticMeshComponent> ChestCarrierMesh;

	/** 3rd-Person Tactical Ballistic Helmet attached to head */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Character|Gear")
	TObjectPtr<UStaticMeshComponent> BallisticHelmetMesh;

	/** Physical Ceramic Plate Shard ejected on Level IV Armor Break */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Character|Gear")
	TObjectPtr<UStaticMeshComponent> EjectedPlateShardMesh;

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
	UFPSHealthComponent* GetHealthComponent() const { return HealthComponent; }

	UFUNCTION(BlueprintPure, Category = "FPS Character|Weapons")
	class AFPSWeaponBase* GetCurrentWeapon() const;

	UFUNCTION(BlueprintPure, Category = "FPS Character|Components")
	class UFPSBodyMechanicsComponent* GetBodyMechanicsComponent() const { return BodyMechanicsComponent; }

	// Tactical Locomotion Animation Blueprint & Assets
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS Character|Animations")
	TSubclassOf<class UAnimInstance> AnimBlueprintClass;

	UFUNCTION(BlueprintPure, Category = "FPS Character|Animations")
	class UFPSAnimInstance* GetFPSAnimInstance() const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS Character|Animations")
	TObjectPtr<class UAnimSequence> AnimIdleADS;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS Character|Animations")
	TObjectPtr<class UAnimSequence> AnimLowReady;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS Character|Animations")
	TObjectPtr<class UAnimSequence> AnimWalk;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS Character|Animations")
	TObjectPtr<class UAnimSequence> AnimRun;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS Character|Animations")
	TObjectPtr<class UAnimSequence> AnimSprint;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS Character|Animations")
	TObjectPtr<class UAnimSequence> AnimJumpLoop;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS Character|Animations")
	TObjectPtr<class UAnimSequence> AnimCrouchIdle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS Character|Animations")
	TObjectPtr<class UAnimSequence> AnimCrouchWalk;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS Character|Animations")
	TObjectPtr<class UAnimSequence> AnimSlide;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS Character|Animations")
	TObjectPtr<class UAnimSequence> AnimWalkBackwards;

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

	UFUNCTION(BlueprintPure, Category = "FPS Character|State")
	bool IsDead() const { return bIsDead; }

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

	// Damage Handling (Threat Radar Directional Hit)
	UFUNCTION()
	void HandleOnDamageTaken(float Damage, AActor* Attacker, const FHitResult& HitResult, bool bIsHeadshot, bool bShieldBroken);

	// Live Escape Menu Input Toggle
	UFUNCTION(BlueprintCallable, Category = "FPS Character|UI")
	void ToggleEscapeMenu();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Character|UI")
	TSubclassOf<class UFPSEscapeMenuWidget> EscapeMenuWidgetClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Character|UI")
	TObjectPtr<class UFPSEscapeMenuWidget> ActiveEscapeMenuWidget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Character|UI")
	bool bIsEscapeMenuOpen = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Character|Settings")
	float MouseSensitivityMultiplier = 1.0f;

	UFUNCTION(BlueprintCallable, Category = "FPS Character|Actions")
	void NotifyEnemyHit(AActor* Victim, const FHitResult& HitInfo, bool bIsHeadshot, bool bShieldBroken, bool bKilled);

	UFUNCTION(BlueprintCallable, Category = "FPS Character|Actions")
	void HandleBulletGraze(float Distance);

	UPROPERTY(BlueprintAssignable, Category = "FPS Character|Accolades")
	FOnCharacterCombatAccoladeSignature OnCombatAccoladeEarned;

	UFUNCTION(BlueprintCallable, Category = "FPS Character|Accolades")
	void AwardCombatAccolade(const FString& AccoladeName, int32 GoldReward, const FLinearColor& AccentColor = FLinearColor(1.0f, 0.702f, 0.0f));

	UPROPERTY(BlueprintAssignable, Category = "FPS Character|Bounties")
	FOnBountyEventSignature OnBountyEvent;

	UFUNCTION(BlueprintCallable, Category = "FPS Character|Bounties")
	void NotifyBountyEvent(FName EventTag, int32 Count = 1);

	UFUNCTION(BlueprintCallable, Category = "FPS Character|Cosmetics")
	void ApplyLoadoutCosmetics(int32 CamoIndex, int32 HeadgearIndex, int32 ArmorIndex, int32 WeaponFinishIndex);

	UFUNCTION(NetMulticast, Unreliable, BlueprintCallable, Category = "FPS Character|Trauma")
	void MulticastTriggerArmorPlateShatter(FVector ShotDirection, FVector ImpactPoint);

	UFUNCTION(NetMulticast, Unreliable, BlueprintCallable, Category = "FPS Character|Trauma")
	void MulticastPopBallisticHelmet(FVector ShotDirection);

	// Responsive Tactical Bot AI & Acoustic Decoy-Bait API
	UFUNCTION(BlueprintCallable, Category = "FPS Character|Tactical Bot")
	void InitializeAsTacticalBot(int32 InBotIndex, const FString& InCallsign, EFPSPlayerTeam InTeam);

	UFUNCTION(BlueprintPure, Category = "FPS Character|Tactical Bot")
	bool IsTacticalBot() const { return bIsTacticalBot; }

	UFUNCTION(BlueprintPure, Category = "FPS Character|Tactical Bot")
	EFPSPlayerTeam GetBotTeam() const { return BotTeam; }

	UFUNCTION(BlueprintPure, Category = "FPS Character|Tactical Bot")
	FString GetBotCallsign() const { return BotCallsign; }

	UFUNCTION(BlueprintPure, Category = "FPS Character|Tactical Bot")
	FVector GetBotEyeLocation() const { return GetActorLocation() + FVector(0.0f, 0.0f, 64.0f); }

	UFUNCTION(BlueprintPure, Category = "FPS Character|Tactical Bot")
	FRotator GetBotAimRotation() const { return BotAimRotation; }

	UFUNCTION(BlueprintCallable, Category = "FPS Character|Tactical Bot")
	void NotifyBaitedByDecoy(class AFPSFermiDecoyActor* InDecoy, float BaitDuration = 2.5f);

	UFUNCTION(BlueprintPure, Category = "FPS Character|Tactical Bot")
	bool IsBaitedByDecoy() const;

	UFUNCTION(BlueprintPure, Category = "FPS Character|Tactical Bot")
	float GetBaitedRemainingRatio() const { return FMath::Clamp(BaitedRemainingTimer / 2.5f, 0.0f, 1.0f); }

	UFUNCTION(BlueprintCallable, Category = "FPS Character|Settings")
	void SetFieldOfView(float NewFOV);

	UFUNCTION(BlueprintCallable, Category = "FPS Character|Customization")
	void SetOperatorCamo(uint8 CamoIndex);

	UFUNCTION(BlueprintCallable, Category = "FPS Character|Combat")
	void RedeployOperator();

	// Input Handlers
	void MoveForward(float Value);
	void MoveRight(float Value);
	void Turn(float Rate);
	void LookUp(float Rate);

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
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

	// Tactical Camera, Dive Posture, Gear Physics & Bot AI Updates
	void UpdateThirdPersonCamera(float DeltaTime);
	void UpdateBallisticDivePosture(float DeltaTime);
	void UpdateProceduralCameraBob(float DeltaTime);
	void UpdateWeaponSway(float DeltaTime);
	void UpdateLocomotionAnimation(float DeltaTime);
	void UpdatePhysicalArmorAndHelmetDebris(float DeltaTime);
	void TickTacticalBotAI(float DeltaTime);

	UPROPERTY()
	TObjectPtr<class UAnimSequence> CurrentLocomotionAnim;

	FVector2D CurrentMoveInput = FVector2D::ZeroVector;
	FRotator CurrentDiveMeshRotation = FRotator(0.0f, -90.0f, 0.0f);
	float CurrentDiveMeshZOffset = -96.0f;
	float FermiDecoyCooldownRemaining = 0.0f;
	float QuantumSwapFlashTimer = 0.0f;
	float FOVPunchTimer = 0.0f;
	float QuantumSwapKillWindowRemaining = 0.0f;
	float LastCrouchPressTime = -10.0f;
	bool bIsHoldingCrouchKey = false;
	bool bShouldCaptureHUD = false;
	int32 CaptureHUDFrameCounter = 0;

	// 3rd-Person Armor Plate Shatter & Helmet Pop Physics State
	bool bIsArmorPlateShattered = false;
	bool bIsHelmetPopped = false;
	float ShardFlightTimer = 0.0f;
	FVector ShardVelocity = FVector::ZeroVector;
	FRotator ShardAngularVelocity = FRotator::ZeroRotator;
	float HelmetFlightTimer = 0.0f;
	FVector HelmetVelocity = FVector::ZeroVector;
	FRotator HelmetAngularVelocity = FRotator::ZeroRotator;
	float LimbTraumaFlinchTimer = 0.0f;
	FRotator LimbTraumaRotOffset = FRotator::ZeroRotator;

	// Tactical Bot AI State
	bool bIsTacticalBot = false;
	int32 BotLaneIndex = 0;
	int32 BotWaypointIndex = 0;
	FString BotCallsign = TEXT("BRAVO-OP");
	EFPSPlayerTeam BotTeam = EFPSPlayerTeam::Bravo;
	FRotator BotAimRotation = FRotator::ZeroRotator;
	TWeakObjectPtr<class AFPSFermiDecoyActor> BaitedDecoyTarget;
	float BaitedRemainingTimer = 0.0f;
	float BotBurstFireTimer = 0.0f;
	float BotBurstCooldownTimer = 0.0f;
	float BotStrafeTimer = 0.0f;
	float BotStrafeSign = 1.0f;
	float BotWaypointStuckTimer = 0.0f;
	bool bBotIsFiringBurst = false;

	float BobTimer = 0.0f;
	FVector DefaultCameraPos;
	FRotator LastControlRotation;
	FRotator CurrentWeaponSwayRot;
	bool bIsDead = false;

	float FootstepDistanceTraveled = 0.0f;
	int32 FootstepAlternator = 0;
};
