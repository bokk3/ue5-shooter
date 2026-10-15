#include "UI/FPSHUDWidget.h"
#include "Character/FPSCharacter.h"
#include "Character/FPSCharacterMovementComponent.h"
#include "Combat/FPSHealthComponent.h"
#include "Inventory/FPSInventoryComponent.h"
#include "Weapons/FPSWeaponBase.h"
#include "Radar/FPSRadarSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Widgets/SCompoundWidget.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Styling/AppStyle.h"

/**
 * Native Slate Tactical HUD Widget
 * Renders Minimap, Top Radar Compass, Health/Armor, Stamina, Ammo Counter, and Dynamic Crosshair
 */
class SFPSHUDWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFPSHUDWidget) {}
		SLATE_ARGUMENT(UFPSHUDWidget*, OwningHUD)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		OwningHUD = InArgs._OwningHUD;
		SetCanTick(false);
	}

	virtual int32 OnPaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override
	{
		if (!OwningHUD) return LayerId;

		const FVector2f ScreenSize = AllottedGeometry.GetLocalSize();
		const float ScreenW = ScreenSize.X;
		const float ScreenH = ScreenSize.Y;
		if (ScreenW <= 20.0f || ScreenH <= 20.0f) return LayerId;

		const FSlateBrush* WhiteBrush = FAppStyle::Get().GetBrush("WhiteBrush");

		// Fonts
		const FSlateFontInfo FontSmall = FCoreStyle::GetDefaultFontStyle("Bold", 8);
		const FSlateFontInfo FontMedium = FCoreStyle::GetDefaultFontStyle("Bold", 11);
		const FSlateFontInfo FontBold = FCoreStyle::GetDefaultFontStyle("Bold", 13);
		const FSlateFontInfo FontBigAmmo = FCoreStyle::GetDefaultFontStyle("Bold", 30);
		const FSlateFontInfo FontCardinal = FCoreStyle::GetDefaultFontStyle("Bold", 10);

		// Drawing Lambdas
		auto DrawBox = [&](float X, float Y, float W, float H, const FLinearColor& Tint)
		{
			FPaintGeometry Geom = AllottedGeometry.ToPaintGeometry(FVector2f(W, H), FSlateLayoutTransform(FVector2f(X, Y)));
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId, Geom, WhiteBrush, ESlateDrawEffect::None, Tint);
		};

		auto DrawBorder = [&](float X, float Y, float W, float H, float Thickness, const FLinearColor& Tint)
		{
			DrawBox(X, Y, W, Thickness, Tint);
			DrawBox(X, Y + H - Thickness, W, Thickness, Tint);
			DrawBox(X, Y, Thickness, H, Tint);
			DrawBox(X + W - Thickness, Y, Thickness, H, Tint);
		};

		auto DrawText = [&](float X, float Y, const FString& Text, const FSlateFontInfo& Font, const FLinearColor& Tint)
		{
			FPaintGeometry Geom = AllottedGeometry.ToPaintGeometry(FSlateLayoutTransform(FVector2f(X, Y)));
			FSlateDrawElement::MakeText(OutDrawElements, LayerId, Geom, Text, Font, ESlateDrawEffect::None, Tint);
		};

		auto DrawLine = [&](float X1, float Y1, float X2, float Y2, float Thickness, const FLinearColor& Tint)
		{
			TArray<FVector2f> Points;
			Points.Add(FVector2f(X1, Y1));
			Points.Add(FVector2f(X2, Y2));
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), Points, ESlateDrawEffect::None, Tint, true, Thickness);
		};

		auto DrawDiamond = [&](float CX, float CY, float Radius, const FLinearColor& Tint)
		{
			TArray<FVector2f> Points;
			Points.Add(FVector2f(CX, CY - Radius));
			Points.Add(FVector2f(CX + Radius, CY));
			Points.Add(FVector2f(CX, CY + Radius));
			Points.Add(FVector2f(CX - Radius, CY));
			Points.Add(FVector2f(CX, CY - Radius));
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), Points, ESlateDrawEffect::None, Tint, true, 1.5f);
		};

		// =========================================================================
		// 1. TOP RADAR COMPASS
		// =========================================================================
		const float CenterCompassX = ScreenW * 0.5f;
		const float TopY = 16.0f;
		const float CompassW = 440.0f;
		const float CompassH = 34.0f;
		const float HalfW = 190.0f;
		const float PixelsPerDeg = HalfW / 45.0f;

		// Background & Border
		DrawBox(CenterCompassX - CompassW * 0.5f, TopY, CompassW, CompassH, FLinearColor(0.02f, 0.03f, 0.05f, 0.75f));
		DrawBorder(CenterCompassX - CompassW * 0.5f, TopY, CompassW, CompassH, 1.0f, FLinearColor(0.12f, 0.65f, 0.85f, 0.35f));

		// Compass Bearing Calculations
		float Yaw = OwningHUD->CameraYaw;
		float NormYaw = FMath::Fmod(Yaw, 360.0f);
		if (NormYaw < 0.0f) NormYaw += 360.0f;

		for (int32 deg = 0; deg < 360; deg += 5)
		{
			float Delta = FRotator::NormalizeAxis((float)deg - NormYaw);
			if (FMath::Abs(Delta) <= 45.0f)
			{
				float TickX = CenterCompassX + Delta * PixelsPerDeg;
				if (deg % 45 == 0)
				{
					FString Cardinal;
					if (deg == 0) Cardinal = TEXT("N");
					else if (deg == 45) Cardinal = TEXT("NE");
					else if (deg == 90) Cardinal = TEXT("E");
					else if (deg == 135) Cardinal = TEXT("SE");
					else if (deg == 180) Cardinal = TEXT("S");
					else if (deg == 225) Cardinal = TEXT("SW");
					else if (deg == 270) Cardinal = TEXT("W");
					else if (deg == 315) Cardinal = TEXT("NW");

					DrawLine(TickX, TopY + 2.0f, TickX, TopY + 12.0f, 1.5f, FLinearColor(1.0f, 0.85f, 0.2f, 0.9f));
					DrawText(TickX - 5.0f, TopY + 14.0f, Cardinal, FontCardinal, FLinearColor(1.0f, 0.85f, 0.2f, 0.95f));
				}
				else if (deg % 15 == 0)
				{
					DrawLine(TickX, TopY + 4.0f, TickX, TopY + 12.0f, 1.0f, FLinearColor(0.7f, 0.75f, 0.8f, 0.6f));
					DrawText(TickX - 7.0f, TopY + 15.0f, FString::Printf(TEXT("%02d"), deg / 10), FontSmall, FLinearColor(0.7f, 0.75f, 0.8f, 0.6f));
				}
				else
				{
					DrawLine(TickX, TopY + 7.0f, TickX, TopY + 12.0f, 1.0f, FLinearColor(0.5f, 0.55f, 0.6f, 0.4f));
				}
			}
		}

		// Center Indicator Mark
		DrawLine(CenterCompassX - 4.0f, TopY, CenterCompassX + 4.0f, TopY, 2.0f, FLinearColor(0.15f, 0.85f, 1.0f));
		DrawLine(CenterCompassX, TopY, CenterCompassX, TopY + 7.0f, 2.0f, FLinearColor(0.15f, 0.85f, 1.0f));

		// Heading Readout Pill
		FString HeadingStr = FString::Printf(TEXT("[ %03.0f° ]"), NormYaw);
		DrawText(CenterCompassX - 22.0f, TopY + CompassH + 3.0f, HeadingStr, FontSmall, FLinearColor(0.15f, 0.85f, 1.0f, 0.85f));

		// Gunfire Acoustic Radar Threat Pings on Compass
		UWorld* World = OwningHUD->GetWorld();
		if (World)
		{
			UFPSRadarSubsystem* Radar = World->GetSubsystem<UFPSRadarSubsystem>();
			if (Radar)
			{
				TArray<FFPSRadarPing> Pings = Radar->GetVisiblePingsForTeam(EFPSPlayerTeam::Alpha);
				for (const FFPSRadarPing& Ping : Pings)
				{
					FVector Dir = Ping.WorldLocation - OwningHUD->PlayerLocation;
					float PingBearing = FMath::RadiansToDegrees(FMath::Atan2(Dir.Y, Dir.X));
					float DeltaPing = FRotator::NormalizeAxis(PingBearing - NormYaw);
					if (FMath::Abs(DeltaPing) <= 45.0f)
					{
						float PingX = CenterCompassX + DeltaPing * PixelsPerDeg;
						float Pulse = 1.0f + 0.3f * FMath::Sin(Ping.LifetimeRemaining * 10.0f);
						DrawDiamond(PingX, TopY + 11.0f, 5.0f * Pulse, FLinearColor(1.0f, 0.15f, 0.15f, 0.95f));
						DrawBox(PingX - 1.5f, TopY + 9.5f, 3.0f, 3.0f, FLinearColor(1.0f, 0.8f, 0.8f, 1.0f));
					}
					else
					{
						float EdgeX = DeltaPing > 0 ? (CenterCompassX + HalfW - 6.0f) : (CenterCompassX - HalfW + 6.0f);
						DrawDiamond(EdgeX, TopY + 11.0f, 3.5f, FLinearColor(1.0f, 0.2f, 0.2f, 0.7f));
					}
				}
			}
		}

		// Objectives on Compass (Flags A, B, C)
		const struct { FVector Loc; const TCHAR* Name; } Objectives[] = {
			{ FVector(-1200.0f, -800.0f, 0.0f), TEXT("A") },
			{ FVector(0.0f, 0.0f, 0.0f), TEXT("B") },
			{ FVector(1200.0f, 800.0f, 0.0f), TEXT("C") }
		};

		for (const auto& Obj : Objectives)
		{
			FVector Dir = Obj.Loc - OwningHUD->PlayerLocation;
			float ObjBearing = FMath::RadiansToDegrees(FMath::Atan2(Dir.Y, Dir.X));
			float DeltaObj = FRotator::NormalizeAxis(ObjBearing - NormYaw);
			if (FMath::Abs(DeltaObj) <= 45.0f)
			{
				float ObjX = CenterCompassX + DeltaObj * PixelsPerDeg;
				DrawDiamond(ObjX, TopY + 11.0f, 5.0f, FLinearColor(0.2f, 0.75f, 1.0f, 0.85f));
				DrawText(ObjX - 3.0f, TopY + 14.0f, Obj.Name, FontSmall, FLinearColor(0.2f, 0.75f, 1.0f, 0.9f));
			}
		}

		// =========================================================================
		// 2. TACTICAL MINIMAP (Top-Left)
		// =========================================================================
		const float MapX = 24.0f;
		const float MapY = 24.0f;
		const float MapW = 180.0f;
		const float MapH = 180.0f;
		const float RCX = MapX + MapW * 0.5f;
		const float RCY = MapY + MapH * 0.5f + 8.0f;
		const float MapRadius = 64.0f;

		// Map Background & Chamfered Frame
		DrawBox(MapX, MapY, MapW, MapH, FLinearColor(0.02f, 0.03f, 0.05f, 0.78f));
		DrawBorder(MapX, MapY, MapW, MapH, 1.0f, FLinearColor(0.12f, 0.65f, 0.85f, 0.4f));

		// Cyan Corner Ticks
		DrawBox(MapX, MapY, 8.0f, 2.0f, FLinearColor(0.15f, 0.85f, 1.0f));
		DrawBox(MapX, MapY, 2.0f, 8.0f, FLinearColor(0.15f, 0.85f, 1.0f));
		DrawBox(MapX + MapW - 8.0f, MapY, 8.0f, 2.0f, FLinearColor(0.15f, 0.85f, 1.0f));
		DrawBox(MapX + MapW - 2.0f, MapY, 2.0f, 8.0f, FLinearColor(0.15f, 0.85f, 1.0f));

		DrawText(MapX + 8.0f, MapY + 6.0f, TEXT("TAC-MAP // RUST 60M"), FontSmall, FLinearColor(0.15f, 0.85f, 1.0f, 0.85f));

		// Concentric Range Rings (25m and 50m)
		const int32 NumSegments = 24;
		TArray<FVector2f> RingPoints50;
		TArray<FVector2f> RingPoints25;
		for (int32 i = 0; i <= NumSegments; ++i)
		{
			float Angle = ((float)i / (float)NumSegments) * 2.0f * PI;
			RingPoints50.Add(FVector2f(RCX + FMath::Cos(Angle) * MapRadius, RCY + FMath::Sin(Angle) * MapRadius));
			RingPoints25.Add(FVector2f(RCX + FMath::Cos(Angle) * (MapRadius * 0.5f), RCY + FMath::Sin(Angle) * (MapRadius * 0.5f)));
		}
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), RingPoints50, ESlateDrawEffect::None, FLinearColor(0.12f, 0.65f, 0.85f, 0.25f), true, 1.0f);
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), RingPoints25, ESlateDrawEffect::None, FLinearColor(0.12f, 0.65f, 0.85f, 0.15f), true, 1.0f);

		// Rotating Radar Sweep Line
		if (World)
		{
			float SweepAngle = FMath::Fmod(World->GetTimeSeconds() * 120.0f, 360.0f);
			float SweepRad = FMath::DegreesToRadians(SweepAngle);
			DrawLine(RCX, RCY, RCX + FMath::Cos(SweepRad) * MapRadius, RCY + FMath::Sin(SweepRad) * MapRadius, 1.0f, FLinearColor(0.15f, 0.85f, 1.0f, 0.35f));
		}

		// Objectives on Minimap
		const float YawRad = FMath::DegreesToRadians(NormYaw);
		for (const auto& Obj : Objectives)
		{
			FVector Delta = Obj.Loc - OwningHUD->PlayerLocation;
			float Fwd = Delta.X * FMath::Cos(-YawRad) - Delta.Y * FMath::Sin(-YawRad);
			float Right = Delta.X * FMath::Sin(-YawRad) + Delta.Y * FMath::Cos(-YawRad);

			float ScreenDX = (Right / 5000.0f) * MapRadius;
			float ScreenDY = -(Fwd / 5000.0f) * MapRadius;

			// Clamp to radar radius
			float Dist = FMath::Sqrt(ScreenDX * ScreenDX + ScreenDY * ScreenDY);
			if (Dist > MapRadius - 4.0f)
			{
				ScreenDX = (ScreenDX / Dist) * (MapRadius - 4.0f);
				ScreenDY = (ScreenDY / Dist) * (MapRadius - 4.0f);
			}

			DrawDiamond(RCX + ScreenDX, RCY + ScreenDY, 4.0f, FLinearColor(0.2f, 0.75f, 1.0f, 0.85f));
			DrawText(RCX + ScreenDX - 3.0f, RCY + ScreenDY - 5.0f, Obj.Name, FontSmall, FLinearColor(1.0f, 1.0f, 1.0f, 0.95f));
		}

		// Acoustic Gunfire Radar Pings on Minimap
		if (World)
		{
			UFPSRadarSubsystem* Radar = World->GetSubsystem<UFPSRadarSubsystem>();
			if (Radar)
			{
				TArray<FFPSRadarPing> Pings = Radar->GetVisiblePingsForTeam(EFPSPlayerTeam::Alpha);
				for (const FFPSRadarPing& Ping : Pings)
				{
					FVector Delta = Ping.WorldLocation - OwningHUD->PlayerLocation;
					float Fwd = Delta.X * FMath::Cos(-YawRad) - Delta.Y * FMath::Sin(-YawRad);
					float Right = Delta.X * FMath::Sin(-YawRad) + Delta.Y * FMath::Cos(-YawRad);

					float ScreenDX = (Right / 5000.0f) * MapRadius;
					float ScreenDY = -(Fwd / 5000.0f) * MapRadius;

					float Dist = FMath::Sqrt(ScreenDX * ScreenDX + ScreenDY * ScreenDY);
					if (Dist > MapRadius - 3.0f)
					{
						ScreenDX = (ScreenDX / Dist) * (MapRadius - 3.0f);
						ScreenDY = (ScreenDY / Dist) * (MapRadius - 3.0f);
					}

					float Alpha = FMath::Clamp(Ping.LifetimeRemaining / Ping.InitialLifetime, 0.0f, 1.0f);
					DrawBox(RCX + ScreenDX - 2.5f, RCY + ScreenDY - 2.5f, 5.0f, 5.0f, FLinearColor(1.0f, 0.15f, 0.15f, Alpha));
				}
			}
		}

		// Center Player Chevron (Forward pointing UP)
		DrawLine(RCX - 5.0f, RCY + 5.0f, RCX, RCY - 5.0f, 2.0f, FLinearColor(0.15f, 0.85f, 1.0f));
		DrawLine(RCX + 5.0f, RCY + 5.0f, RCX, RCY - 5.0f, 2.0f, FLinearColor(0.15f, 0.85f, 1.0f));
		DrawLine(RCX - 3.0f, RCY + 2.0f, RCX + 3.0f, RCY + 2.0f, 1.5f, FLinearColor(0.15f, 0.85f, 1.0f));

		// Position Readout
		FString CoordStr = FString::Printf(TEXT("GRID: [%.0f, %.0f]"), OwningHUD->PlayerLocation.X / 100.0f, OwningHUD->PlayerLocation.Y / 100.0f);
		DrawText(MapX + 8.0f, MapY + MapH - 16.0f, CoordStr, FontSmall, FLinearColor(0.5f, 0.6f, 0.7f, 0.6f));

		// =========================================================================
		// 3. OPERATOR VITALS & LEVEL IV SHIELD & STAMINA (Bottom-Left)
		// =========================================================================
		const float VitX = 24.0f;
		const float VitY = ScreenH - 120.0f;
		const float VitW = 270.0f;
		const float VitH = 96.0f;

		DrawBox(VitX, VitY, VitW, VitH, FLinearColor(0.02f, 0.03f, 0.05f, 0.75f));
		DrawBorder(VitX, VitY, VitW, VitH, 1.0f, FLinearColor(0.12f, 0.65f, 0.85f, 0.35f));

		// Vitals Header
		DrawText(VitX + 10.0f, VitY + 6.0f, TEXT("OPERATOR // TF-141 [ONLINE]"), FontSmall, FLinearColor(0.15f, 0.85f, 1.0f, 0.85f));

		// 1. Stamina Meter
		float StaminaPct = OwningHUD->MaxStamina > 0.0f ? FMath::Clamp(OwningHUD->CurrentStamina / OwningHUD->MaxStamina, 0.0f, 1.0f) : 0.0f;
		DrawBox(VitX + 10.0f, VitY + 22.0f, 180.0f, 8.0f, FLinearColor(0.05f, 0.08f, 0.1f, 0.8f));
		FLinearColor StaminaColor = OwningHUD->bIsStaminaExhausted ? FLinearColor(1.0f, 0.2f, 0.2f) : (StaminaPct < 0.3f ? FLinearColor(1.0f, 0.75f, 0.1f) : FLinearColor(0.2f, 0.88f, 0.45f));
		DrawBox(VitX + 10.0f, VitY + 22.0f, 180.0f * StaminaPct, 8.0f, StaminaColor);
		FString StaminaLabel = OwningHUD->bIsStaminaExhausted ? TEXT("EXHAUSTED") : FString::Printf(TEXT("STAMINA: %.0f%%"), StaminaPct * 100.0f);
		DrawText(VitX + 196.0f, VitY + 21.0f, StaminaLabel, FontSmall, StaminaColor);

		// 2. Health Bar
		float HealthPct = OwningHUD->MaxHealth > 0.0f ? FMath::Clamp(OwningHUD->CurrentHealth / OwningHUD->MaxHealth, 0.0f, 1.0f) : 0.0f;
		DrawBox(VitX + 10.0f, VitY + 42.0f, 180.0f, 12.0f, FLinearColor(0.05f, 0.08f, 0.1f, 0.8f));
		FLinearColor HealthColor = HealthPct < 0.3f ? FLinearColor(1.0f, 0.15f, 0.15f) : (HealthPct < 0.6f ? FLinearColor(1.0f, 0.75f, 0.15f) : FLinearColor(0.12f, 0.82f, 0.92f));
		DrawBox(VitX + 10.0f, VitY + 42.0f, 180.0f * HealthPct, 12.0f, HealthColor);
		FString HealthLabel = FString::Printf(TEXT("%.0f / %.0f HP"), OwningHUD->CurrentHealth, OwningHUD->MaxHealth);
		DrawText(VitX + 196.0f, VitY + 42.0f, HealthLabel, FontBold, HealthColor);

		// 3. Level IV Ceramic Armor Shield
		float ShieldPct = OwningHUD->MaxShield > 0.0f ? FMath::Clamp(OwningHUD->CurrentShield / OwningHUD->MaxShield, 0.0f, 1.0f) : 0.0f;
		DrawBox(VitX + 10.0f, VitY + 66.0f, 180.0f, 10.0f, FLinearColor(0.05f, 0.08f, 0.1f, 0.8f));
		FLinearColor ShieldColor = FLinearColor(0.2f, 0.55f, 1.0f);
		DrawBox(VitX + 10.0f, VitY + 66.0f, 180.0f * ShieldPct, 10.0f, ShieldColor);
		FString ShieldLabel = FString::Printf(TEXT("ARMOR: %.0f [LVL IV]"), OwningHUD->CurrentShield);
		DrawText(VitX + 196.0f, VitY + 65.0f, ShieldLabel, FontSmall, ShieldColor);

		// =========================================================================
		// 4. AMMO & WEAPON LOADOUT (Bottom-Right)
		// =========================================================================
		const float AmmoX = ScreenW - 294.0f;
		const float AmmoW = 270.0f;
		const float AmmoH = 96.0f;

		DrawBox(AmmoX, VitY, AmmoW, AmmoH, FLinearColor(0.02f, 0.03f, 0.05f, 0.75f));
		DrawBorder(AmmoX, VitY, AmmoW, AmmoH, 1.0f, FLinearColor(0.12f, 0.65f, 0.85f, 0.35f));

		// Weapon Name & Fire Mode
		FString WeaponStr = OwningHUD->ActiveWeaponName.IsEmpty() ? TEXT("NF-98 CARBINE // 5.56 NATO") : OwningHUD->ActiveWeaponName.ToString();
		DrawText(AmmoX + 10.0f, VitY + 6.0f, WeaponStr, FontSmall, FLinearColor(0.15f, 0.85f, 1.0f, 0.85f));
		DrawText(AmmoX + 10.0f, VitY + 22.0f, OwningHUD->FireModeString, FontSmall, FLinearColor(0.7f, 0.75f, 0.8f, 0.65f));

		// Digital Ammo Readout
		FString ClipStr = FString::Printf(TEXT("%02d"), OwningHUD->AmmoInClip);
		FLinearColor AmmoColor = OwningHUD->AmmoInClip <= 5 ? FLinearColor(1.0f, 0.25f, 0.15f) : FLinearColor(0.95f, 0.98f, 1.0f);
		DrawText(AmmoX + 12.0f, VitY + 36.0f, ClipStr, FontBigAmmo, AmmoColor);
		DrawText(AmmoX + 66.0f, VitY + 44.0f, TEXT("/"), FontMedium, FLinearColor(0.45f, 0.5f, 0.55f, 0.7f));
		FString ReserveStr = FString::Printf(TEXT("%d"), OwningHUD->AmmoReserve);
		DrawText(AmmoX + 80.0f, VitY + 46.0f, ReserveStr, FontMedium, FLinearColor(0.7f, 0.75f, 0.8f, 0.8f));

		// Tactical State Line
		if (OwningHUD->bIsReloading)
		{
			DrawText(AmmoX + 10.0f, VitY + 74.0f, TEXT("[>> RELOADING...]"), FontBold, FLinearColor(1.0f, 0.8f, 0.1f));
		}
		else if (OwningHUD->bIsADS)
		{
			DrawText(AmmoX + 10.0f, VitY + 74.0f, TEXT("[OPTIC: REFLEX RED-DOT ACTIVE]"), FontSmall, FLinearColor(0.15f, 0.85f, 1.0f, 0.8f));
		}
		else
		{
			DrawText(AmmoX + 10.0f, VitY + 74.0f, TEXT("CALIBER: 5.56x45 NATO [MATCH]"), FontSmall, FLinearColor(0.5f, 0.55f, 0.6f, 0.6f));
		}

		// =========================================================================
		// 5. DYNAMIC CENTER RETICLE & HITMARKER
		// =========================================================================
		const float CX = ScreenW * 0.5f;
		const float CY = ScreenH * 0.5f;

		if (!OwningHUD->bIsADS)
		{
			float Gap = FMath::Clamp(OwningHUD->CurrentSpreadRadius, 8.0f, 40.0f);
			float Len = 8.0f;
			FLinearColor CrosshairTint(1.0f, 1.0f, 1.0f, 0.75f);

			DrawLine(CX, CY - Gap - Len, CX, CY - Gap, 1.5f, CrosshairTint);
			DrawLine(CX, CY + Gap, CX, CY + Gap + Len, 1.5f, CrosshairTint);
			DrawLine(CX - Gap - Len, CY, CX - Gap, CY, 1.5f, CrosshairTint);
			DrawLine(CX + Gap, CY, CX + Gap + Len, CY, 1.5f, CrosshairTint);
			DrawBox(CX - 1.0f, CY - 1.0f, 2.0f, 2.0f, FLinearColor(1.0f, 1.0f, 1.0f, 0.5f));
		}

		if (OwningHUD->HitmarkerTimer > 0.0f)
		{
			float Alpha = FMath::Clamp(OwningHUD->HitmarkerTimer / 0.25f, 0.0f, 1.0f);
			FLinearColor HColor = (OwningHUD->CurrentHitmarker == EFPSHitmarkerType::Headshot) ? FLinearColor(1.0f, 0.1f, 0.1f, Alpha) : FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
			DrawLine(CX - 12.0f, CY - 12.0f, CX - 5.0f, CY - 5.0f, 2.0f, HColor);
			DrawLine(CX + 12.0f, CY - 12.0f, CX + 5.0f, CY - 5.0f, 2.0f, HColor);
			DrawLine(CX - 12.0f, CY + 12.0f, CX - 5.0f, CY + 5.0f, 2.0f, HColor);
			DrawLine(CX + 12.0f, CY + 12.0f, CX + 5.0f, CY + 5.0f, 2.0f, HColor);
		}

		return LayerId + 10;
	}

private:
	UFPSHUDWidget* OwningHUD = nullptr;
};

// =============================================================================
// UFPSHUDWidget Implementation
// =============================================================================

UFPSHUDWidget::UFPSHUDWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(false);
}

void UFPSHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UFPSHUDWidget::SetOwningCharacter(AFPSCharacter* InCharacter)
{
	OwningCharacter = InCharacter;
}

TSharedRef<SWidget> UFPSHUDWidget::RebuildWidget()
{
	SlateHUDWidget = SNew(SFPSHUDWidget)
		.OwningHUD(this);

	return SlateHUDWidget.ToSharedRef();
}

void UFPSHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (HitmarkerTimer > 0.0f)
	{
		HitmarkerTimer = FMath::Max(0.0f, HitmarkerTimer - InDeltaTime);
	}

	if (!OwningCharacter.IsValid())
	{
		APlayerController* PC = GetOwningPlayer();
		if (PC)
		{
			AFPSCharacter* FPSChar = Cast<AFPSCharacter>(PC->GetPawn());
			if (FPSChar)
			{
				SetOwningCharacter(FPSChar);
			}
		}
	}

	if (OwningCharacter.IsValid())
	{
		AFPSCharacter* Char = OwningCharacter.Get();

		// Vitals (Health & Level IV Shield)
		if (Char->HealthComponent)
		{
			CurrentHealth = Char->HealthComponent->GetCurrentHealth();
			MaxHealth = Char->HealthComponent->MaxHealth;
			CurrentShield = Char->HealthComponent->GetCurrentShield();
			MaxShield = Char->HealthComponent->MaxShield;
		}

		// Stamina System
		if (Char->FPSMovementComponent)
		{
			CurrentStamina = Char->FPSMovementComponent->GetCurrentStamina();
			MaxStamina = Char->FPSMovementComponent->GetMaxStamina();
			bIsStaminaExhausted = Char->FPSMovementComponent->IsStaminaExhausted();
		}

		// Weapon & Ammo
		if (Char->InventoryComponent)
		{
			AFPSWeaponBase* Weapon = Char->InventoryComponent->GetActiveWeapon();
			if (Weapon)
			{
				AmmoInClip = Weapon->GetCurrentAmmo();
				AmmoReserve = Weapon->GetReserveAmmo();
				ActiveWeaponName = Weapon->WeaponDisplayName;
				bIsReloading = Weapon->IsReloading();
				bIsADS = Weapon->IsAimingDownSights();

				switch (Weapon->FireMode)
				{
				case EFPSFireMode::FullAuto:
					FireModeString = TEXT("FULL AUTO - 750 RPM");
					break;
				case EFPSFireMode::Burst:
					FireModeString = TEXT("3-ROUND BURST");
					break;
				case EFPSFireMode::SemiAuto:
				default:
					FireModeString = TEXT("SEMI-AUTO");
					break;
				}
			}
		}

		// Pose & Orientation
		PlayerLocation = Char->GetActorLocation();
		CameraYaw = Char->GetControlRotation().Yaw;
	}
}

void UFPSHUDWidget::TriggerHitmarker(EFPSHitmarkerType HitType)
{
	CurrentHitmarker = HitType;
	HitmarkerTimer = 0.25f;
	OnShowHitmarker(HitType);
}

void UFPSHUDWidget::TriggerDamageIndicator(FVector DamageSourceLocation, FVector InPlayerLocation, FRotator PlayerCameraRotation)
{
	FVector DirToAttacker = (DamageSourceLocation - InPlayerLocation).GetSafeNormal2D();
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
	AmmoInClip = CurrentAmmo;
	AmmoReserve = ReserveAmmo;
	ActiveWeaponName = WeaponName;
	OnAmmoUpdated(CurrentAmmo, ReserveAmmo, WeaponName);
}

void UFPSHUDWidget::UpdateHealthShield(float HealthPercent, float ShieldPercent)
{
	OnVitalsUpdated(HealthPercent, ShieldPercent);
}

void UFPSHUDWidget::UpdateCrosshairSpread(float SpreadDegrees)
{
	CurrentSpreadRadius = SpreadDegrees * ReticlePixelMultiplier;
	OnSpreadUpdated(CurrentSpreadRadius);
}
