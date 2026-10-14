#include "UI/FPSMinimapWidget.h"
#include "Radar/FPSRadarSubsystem.h"
#include "Multiplayer/FPSPlayerState.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"

void UFPSMinimapWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UFPSMinimapWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	APlayerController* PC = GetOwningPlayer();
	if (!PC) return;

	APawn* Pawn = PC->GetPawn();
	if (!Pawn) return;

	UWorld* World = GetWorld();
	if (!World) return;

	UFPSRadarSubsystem* RadarSubsystem = World->GetSubsystem<UFPSRadarSubsystem>();
	if (!RadarSubsystem) return;

	AFPSPlayerState* PS = PC->GetPlayerState<AFPSPlayerState>();
	EFPSPlayerTeam MyTeam = PS ? PS->GetPlayerTeam() : EFPSPlayerTeam::Alpha;

	FVector PlayerLoc = Pawn->GetActorLocation();
	float PlayerYaw = PC->GetControlRotation().Yaw;

	TArray<FFPSRadarPing> Pings = RadarSubsystem->GetVisiblePingsForTeam(MyTeam);
	TArray<FFPSMinimapIcon> RenderIcons;

	for (const FFPSRadarPing& Ping : Pings)
	{
		bool bClamped = false;
		FVector2D IconPos = WorldToMinimapPosition(Ping.WorldLocation, PlayerLoc, PlayerYaw, bClamped);

		FFPSMinimapIcon Icon;
		Icon.ScreenPosition = IconPos;
		Icon.Opacity = FMath::Clamp(Ping.LifetimeRemaining / FMath::Max(0.1f, Ping.InitialLifetime), 0.0f, 1.0f);
		Icon.bIsClampedToEdge = bClamped;
		Icon.bIsEnemy = true;

		RenderIcons.Add(Icon);
	}

	float CompassHeading = GetCompassHeading();
	static float SweepAngle = 0.0f;
	SweepAngle = FMath::Fmod(SweepAngle + (InDeltaTime * 180.0f), 360.0f);

	OnMinimapUpdated(RenderIcons, CompassHeading, SweepAngle);
}

FVector2D UFPSMinimapWidget::WorldToMinimapPosition(const FVector& TargetWorldPos, const FVector& PlayerPos, float PlayerYaw, bool& bOutClamped)
{
	// 2D Relative Offset in centimeters
	FVector Delta = TargetWorldPos - PlayerPos;
	FVector2D LocalDelta(Delta.X, Delta.Y);

	// Rotate delta by -PlayerYaw to orient up = forward
	float YawRad = FMath::DegreesToRadians(-PlayerYaw);
	float RotatedX = LocalDelta.X * FMath::Cos(YawRad) - LocalDelta.Y * FMath::Sin(YawRad);
	float RotatedY = LocalDelta.X * FMath::Sin(YawRad) + LocalDelta.Y * FMath::Cos(YawRad);

	// Invert Y for screen space (forward = -Y)
	FVector2D ScreenDelta(RotatedY, -RotatedX);

	// Scale to pixels based on range
	float MaxRangeCm = RadarRangeMeters * 100.0f;
	FVector2D PixelDelta = (ScreenDelta / MaxRangeCm) * RadarRadiusPixels;

	// Clamp to radar circle edge if out of range
	float DistFromCenter = PixelDelta.Size();
	bOutClamped = false;

	if (DistFromCenter > RadarRadiusPixels)
	{
		PixelDelta = PixelDelta.GetSafeNormal() * RadarRadiusPixels;
		bOutClamped = true;
	}

	return PixelDelta;
}

float UFPSMinimapWidget::GetCompassHeading() const
{
	APlayerController* PC = GetOwningPlayer();
	if (PC)
	{
		float Yaw = PC->GetControlRotation().Yaw;
		return FRotator::ClampAxis(Yaw);
	}
	return 0.0f;
}
