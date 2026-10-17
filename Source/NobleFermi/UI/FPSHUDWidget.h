#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Economy/FPSEconomySubsystem.h"
#include "FPSHUDWidget.generated.h"

class AFPSCharacter;
class SFPSHUDWidget;

UENUM(BlueprintType)
enum class EFPSHitmarkerType : uint8
{
	Standard,
	Headshot,
	ShieldBreak,
	Kill
};

UENUM(BlueprintType)
enum class EThreatIndicatorType : uint8
{
	Damage,
	Gunfire,
	Footstep
};

USTRUCT(BlueprintType)
struct FFPSThreatIndicator
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Threat")
	FVector WorldSourceLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Threat")
	float Intensity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Threat")
	EThreatIndicatorType Type = EThreatIndicatorType::Damage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Threat")
	float LifetimeRemaining = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Threat")
	float InitialLifetime = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Threat")
	float RelativeAngleDeg = 0.0f;
};

USTRUCT(BlueprintType)
struct FFPSCombatAccolade
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Accolade")
	FString Title;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Accolade")
	int32 ScoreBonus = 100;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Accolade")
	FLinearColor AccentColor = FLinearColor(1.0f, 0.702f, 0.0f);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Accolade")
	float LifetimeRemaining = 2.4f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Accolade")
	float InitialLifetime = 2.4f;
};

USTRUCT(BlueprintType)
struct FFPSBaitedTargetMarker
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Baited")
	FVector2f ScreenPos = FVector2f::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Baited")
	float RemainingRatio = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Baited")
	FString Callsign = TEXT("BRAVO-OP");
};

/**
 * AAA First-Person Shooter In-Game Tactical HUD
 * Slate-powered military overlay featuring:
 * - Dynamic Minimap (tactical coordinates, objectives, acoustic gunfire radar blips)
 * - Top Radar Compass (bearing ticker, cardinal directions, threat markers)
 * - Operator Vitals (health bar, Level IV armor shield)
 * - Stamina System (tactical sprint meter, recovery)
 * - Ammo Counter (digital count, reserve, fire mode, reload progress)
 * - Dynamic Crosshair & Hitmarkers
 * - Center-Screen Directional Hit & Visual Audio Semicircle + Chevron Radar (Deaf Accessibility)
 */
UCLASS(BlueprintType, Blueprintable)
class NOBLEFERMI_API UFPSHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFPSHUDWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

	void SetOwningCharacter(AFPSCharacter* InCharacter);

	UFUNCTION(BlueprintPure, Category = "FPS HUD")
	AFPSCharacter* GetOwningCharacter() const { return OwningCharacter.Get(); }

	UFUNCTION(BlueprintCallable, Category = "FPS HUD|Feedback")
	void TriggerHitmarker(EFPSHitmarkerType HitType);

	UFUNCTION(BlueprintCallable, Category = "FPS HUD|Feedback")
	void TriggerDamageIndicator(FVector DamageSourceLocation, FVector InPlayerLocation, FRotator PlayerCameraRotation);

	UFUNCTION(BlueprintCallable, Category = "FPS HUD|Feedback")
	void AddDirectionalIndicator(FVector WorldSourceLocation, float Intensity, EThreatIndicatorType Type);

	UFUNCTION(BlueprintCallable, Category = "FPS HUD|Feedback")
	void TriggerAccolade(const FString& Title, int32 ScoreBonus, const FLinearColor& AccentColor = FLinearColor(1.0f, 0.702f, 0.0f));

	UFUNCTION(BlueprintCallable, Category = "FPS HUD|Combos")
	void RegisterKillForCombo();

	UFUNCTION(BlueprintCallable, Category = "FPS HUD|State")
	void UpdateAmmoDisplay(int32 CurrentAmmo, int32 ReserveAmmo, const FText& WeaponName);

	UFUNCTION(BlueprintCallable, Category = "FPS HUD|State")
	void UpdateHealthShield(float HealthPercent, float ShieldPercent);

	UFUNCTION(BlueprintCallable, Category = "FPS HUD|State")
	void UpdateCrosshairSpread(float SpreadDegrees);

	// Blueprint-implemented event hooks for optional visual styling
	UFUNCTION(BlueprintImplementableEvent, Category = "FPS HUD")
	void OnShowHitmarker(EFPSHitmarkerType HitType);

	UFUNCTION(BlueprintImplementableEvent, Category = "FPS HUD")
	void OnShowDamageIndicator(float AngleDegrees);

	UFUNCTION(BlueprintImplementableEvent, Category = "FPS HUD")
	void OnAmmoUpdated(int32 CurrentAmmo, int32 ReserveAmmo, const FText& WeaponName);

	UFUNCTION(BlueprintImplementableEvent, Category = "FPS HUD")
	void OnVitalsUpdated(float HealthPercent, float ShieldPercent);

	UFUNCTION(BlueprintImplementableEvent, Category = "FPS HUD")
	void OnSpreadUpdated(float SpreadRadiusPixels);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS HUD|Settings")
	float HUDScale = 1.20f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS HUD|Settings")
	bool bEnableVisualAudioRadar = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS HUD|Accolades")
	TArray<FFPSCombatAccolade> ActiveAccolades;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS HUD|Combos")
	int32 CurrentComboCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS HUD|Combos")
	float ComboDecayTimer = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS HUD|Combos")
	float MaxComboWindow = 4.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS HUD|Reticle")
	float ReticlePixelMultiplier = 12.0f;

	// Telemetry data consumed by the native Slate HUD
	float CurrentHealth = 100.0f;
	float MaxHealth = 100.0f;
	float DisplayedHealthGhost = 100.0f;
	float CurrentShield = 100.0f;
	float MaxShield = 100.0f;
	float DisplayedShieldGhost = 100.0f;
	float CurrentStamina = 100.0f;
	float MaxStamina = 100.0f;
	bool bIsStaminaExhausted = false;

	int32 AmmoInClip = 30;
	int32 MagazineCapacity = 30;
	int32 AmmoReserve = 180;
	FText ActiveWeaponName;
	FString FireModeString = TEXT("FULL AUTO");
	uint8 ActiveFireModeIndex = 0; // 0=Auto, 1=Burst, 2=Semi
	bool bIsReloading = false;
	float ReloadProgress = 0.0f;
	bool bIsADS = false;
	float ADSAlpha = 0.0f;

	float CurrentSpreadRadius = 12.0f;
	float HitmarkerTimer = 0.0f;
	EFPSHitmarkerType CurrentHitmarker = EFPSHitmarkerType::Standard;

	float DamageIndicatorTimer = 0.0f;
	float DamageIndicatorAngle = 0.0f;

	float CameraYaw = 0.0f;
	float CameraPitch = 0.0f;
	float LastCameraYaw = 0.0f;
	float LastCameraPitch = 0.0f;
	FVector2f VisorLagOffset = FVector2f::ZeroVector;
	FVector PlayerLocation = FVector::ZeroVector;
	float PlayerSpeed = 0.0f;
	float HUDTimeSeconds = 0.0f;

	// Fermi Paradox Engine Telemetry (Chrono Dive & Acoustic Echo / Quantum Swap)
	bool bIsBallisticDiving = false;
	bool bIsFermiTimeBubbleActive = false;
	float FermiBubbleTimeRemaining = 0.0f;
	float FermiDiveCooldownRemaining = 0.0f;
	bool bCanBallisticDive = true;

	bool bIsFermiDecoyActive = false;
	float FermiDecoyTimeRemaining = 0.0f;
	float FermiDecoyCooldownRemaining = 0.0f;
	float QuantumSwapFlashAlpha = 0.0f;
	bool bUseFirstPersonOpticADS = false;

	// Center-Screen Directional Hit & Visual Audio Semicircle Radar (Deaf Accessibility)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS HUD|Threats")
	TArray<FFPSThreatIndicator> ActiveThreatIndicators;

	// 3D-to-2D Projected "[ ! BAITED ]" Diamond Indicators above baited enemies
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS HUD|Baited")
	TArray<FFPSBaitedTargetMarker> ActiveBaitedMarkers;

	// Economy persistent currency cache
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS HUD|Economy")
	int32 CurrentGoldBalance = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS HUD|Economy")
	int32 CurrentOpalBalance = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS HUD|Bounties")
	TArray<FFPSTacticalBounty> CachedBounties;

	UFUNCTION()
	void HandleCurrencyChanged(int32 NewGold, int32 NewOpals);

	UFUNCTION()
	void HandleBountyCompleted(const FFPSTacticalBounty& CompletedBounty);

private:
	UPROPERTY()
	TWeakObjectPtr<AFPSCharacter> OwningCharacter;

	TSharedPtr<SFPSHUDWidget> SlateHUDWidget;
};

