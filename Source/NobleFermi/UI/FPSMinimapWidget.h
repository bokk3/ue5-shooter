#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Radar/FPSRadarSubsystem.h"
#include "FPSMinimapWidget.generated.h"

USTRUCT(BlueprintType)
struct FFPSMinimapIcon
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Minimap")
	FVector2D ScreenPosition = FVector2D::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Minimap")
	float RotationDegrees = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Minimap")
	float Opacity = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Minimap")
	bool bIsClampedToEdge = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Minimap")
	bool bIsEnemy = true;
};

/**
 * AAA FPS Minimap, Acoustic Radar & Compass HUD Widget
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class NOBLEFERMI_API UFPSMinimapWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Tuning")
	float RadarRangeMeters = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Tuning")
	float RadarRadiusPixels = 90.0f;

	UFUNCTION(BlueprintPure, Category = "Minimap")
	float GetCompassHeading() const;

	UFUNCTION(BlueprintImplementableEvent, Category = "Minimap")
	void OnMinimapUpdated(const TArray<FFPSMinimapIcon>& RenderIcons, float CompassHeading, float UAVSweepAngle);

protected:
	virtual void NativeConstruct() override;

private:
	FVector2D WorldToMinimapPosition(const FVector& WorldPos, const FVector& PlayerPos, float PlayerYaw, bool& bOutClamped);
};
