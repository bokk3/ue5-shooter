#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
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

/**
 * AAA First-Person Shooter In-Game Tactical HUD
 * Slate-powered military overlay featuring:
 * - Dynamic Minimap (tactical coordinates, objectives, acoustic gunfire radar blips)
 * - Top Radar Compass (bearing ticker, cardinal directions, threat markers)
 * - Operator Vitals (health bar, Level IV armor shield)
 * - Stamina System (tactical sprint meter, recovery)
 * - Ammo Counter (digital count, reserve, fire mode, reload progress)
 * - Dynamic Crosshair & Hitmarkers
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS HUD|Reticle")
	float ReticlePixelMultiplier = 12.0f;

	// Telemetry data consumed by the native Slate HUD
	float CurrentHealth = 100.0f;
	float MaxHealth = 100.0f;
	float CurrentShield = 100.0f;
	float MaxShield = 100.0f;
	float CurrentStamina = 100.0f;
	float MaxStamina = 100.0f;
	bool bIsStaminaExhausted = false;

	int32 AmmoInClip = 30;
	int32 AmmoReserve = 180;
	FText ActiveWeaponName;
	FString FireModeString = TEXT("FULL AUTO");
	bool bIsReloading = false;
	bool bIsADS = false;

	float CurrentSpreadRadius = 12.0f;
	float HitmarkerTimer = 0.0f;
	EFPSHitmarkerType CurrentHitmarker = EFPSHitmarkerType::Standard;

	float CameraYaw = 0.0f;
	FVector PlayerLocation = FVector::ZeroVector;

private:
	UPROPERTY()
	TWeakObjectPtr<AFPSCharacter> OwningCharacter;

	TSharedPtr<SFPSHUDWidget> SlateHUDWidget;
};
