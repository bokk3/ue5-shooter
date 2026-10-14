#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FPSHUDWidget.generated.h"

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
 * Manages Dynamic Reticle Spread, Animated Hitmarkers, 3D Directional Damage Arcs, and Ammo Status.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class NOBLEFERMI_API UFPSHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "FPS HUD|Feedback")
	void TriggerHitmarker(EFPSHitmarkerType HitType);

	UFUNCTION(BlueprintCallable, Category = "FPS HUD|Feedback")
	void TriggerDamageIndicator(FVector DamageSourceLocation, FVector PlayerLocation, FRotator PlayerCameraRotation);

	UFUNCTION(BlueprintCallable, Category = "FPS HUD|State")
	void UpdateAmmoDisplay(int32 CurrentAmmo, int32 ReserveAmmo, const FText& WeaponName);

	UFUNCTION(BlueprintCallable, Category = "FPS HUD|State")
	void UpdateHealthShield(float HealthPercent, float ShieldPercent);

	UFUNCTION(BlueprintCallable, Category = "FPS HUD|State")
	void UpdateCrosshairSpread(float SpreadDegrees);

	// Blueprint-implemented event hooks for visual styling & widget animations
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
};
