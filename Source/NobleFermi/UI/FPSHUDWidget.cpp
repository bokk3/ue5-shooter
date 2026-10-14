#include "UI/FPSHUDWidget.h"

void UFPSHUDWidget::TriggerHitmarker(EFPSHitmarkerType HitType)
{
	OnShowHitmarker(HitType);
}

void UFPSHUDWidget::TriggerDamageIndicator(FVector DamageSourceLocation, FVector PlayerLocation, FRotator PlayerCameraRotation)
{
	// Calculate relative angle to the damage source in camera space
	FVector DirToAttacker = (DamageSourceLocation - PlayerLocation).GetSafeNormal2D();
	FVector Forward2D = PlayerCameraRotation.Vector().GetSafeNormal2D();
	FVector Right2D = FRotationMatrix(PlayerCameraRotation).GetUnitAxis(EAxis::Y).GetSafeNormal2D();

	float ForwardDot = FVector::DotProduct(Forward2D, DirToAttacker);
	float RightDot = FVector::DotProduct(Right2D, DirToAttacker);

	float AngleRad = FMath::Atan2(RightDot, ForwardDot);
	float AngleDeg = FMath::RadiansToDegrees(AngleRad);

	OnShowDamageIndicator(AngleDeg);
}

void UFPSHUDWidget::UpdateAmmoDisplay(int32 CurrentAmmo, int32 ReserveAmmo, const FText& WeaponName)
{
	OnAmmoUpdated(CurrentAmmo, ReserveAmmo, WeaponName);
}

void UFPSHUDWidget::UpdateHealthShield(float HealthPercent, float ShieldPercent)
{
	OnVitalsUpdated(HealthPercent, ShieldPercent);
}

void UFPSHUDWidget::UpdateCrosshairSpread(float SpreadDegrees)
{
	float PixelRadius = SpreadDegrees * ReticlePixelMultiplier;
	OnSpreadUpdated(PixelRadius);
}
