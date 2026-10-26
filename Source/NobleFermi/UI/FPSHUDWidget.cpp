#include "UI/FPSHUDWidget.h"
#include "Map/FPSCoastalOperationsMapActor.h"
#include "Kismet/GameplayStatics.h"
#include "Character/FPSCharacter.h"
#include "Character/FPSCharacterMovementComponent.h"
#include "Combat/FPSHealthComponent.h"
#include "Inventory/FPSInventoryComponent.h"
#include "Weapons/FPSWeaponBase.h"
#include "Radar/FPSRadarSubsystem.h"
#include "Economy/FPSEconomySubsystem.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Widgets/SCompoundWidget.h"
#include "Rendering/DrawElements.h"
#include "Rendering/RenderingCommon.h"
#include "Styling/CoreStyle.h"
#include "Styling/AppStyle.h"

/**
 * AAA Future-Warfare Holographic Tactical HUD (MW4 / Battlefield 6 Aesthetic)
 * Pure Slate C++ custom vertex & vector renderer featuring:
 * - Frameless floating AR visor elements with inertial camera-lag parallax
 * - True circular 36-segment radar minimap with FOV cone, phosphor sweep fan & orbiting North indicator
 * - Vignette-faded horizontal compass tape with cosine edge falloff & chamfered bearing pill
 * - Skewed parallelogram vitals bars with 4-plate Level IV armor cells, damage ghost bar & live ECG waveform
 * - 30-round individual bullet pip matrix, vector carbine schematic & oversized floating digital counters
 * - Contrast-shadowed T-bracket dynamic reticle, radial 4-phase reload progress ring & 3D damage arcs
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
		if (ScreenW <= 40.0f || ScreenH <= 40.0f) return LayerId;

		const FSlateBrush* WhiteBrush = FAppStyle::Get().GetBrush("WhiteBrush");
		const FSlateResourceHandle ResourceHandle = WhiteBrush->GetRenderingResource();
		const FSlateRenderTransform& RenderTransform = AllottedGeometry.ToPaintGeometry().GetAccumulatedRenderTransform();

		// Master HUD Scale Factor (+20% Scale Increase)
		const float HUDScale = 1.20f;

		// Visor Inertial Lag Offset (applied to peripheral HUD elements for 3D helmet-visor feel, 25% shake reduction)
		const float VX = OwningHUD->VisorLagOffset.X;
		const float VY = OwningHUD->VisorLagOffset.Y;
		const float TimeSec = OwningHUD->HUDTimeSeconds;

		// Future-Warfare Color Palette (Preserving Cyan #00E5FF & Amber #FFB300 visor theme)
		const FLinearColor TealPrimary(0.0f, 0.94f, 0.86f, 1.0f);       // #00F0DC Electric BF6 Teal
		const FLinearColor CyanTactical(0.0f, 0.90f, 1.0f, 1.0f);       // #00E5FF Tactical Cyan
		const FLinearColor CobaltArmor(0.20f, 0.58f, 1.0f, 1.0f);       // #3394FF Level IV Plate Blue
		const FLinearColor EmeraldStamina(0.05f, 0.95f, 0.62f, 1.0f);   // #0DF29E Kinetic Sprint Green
		const FLinearColor AmberWarn(1.0f, 0.70f, 0.0f, 1.0f);          // #FFB300 Tactical Amber
		const FLinearColor CrimsonAlert(1.0f, 0.165f, 0.33f, 1.0f);     // #FF2A54 Crimson Threat / Hostile
		const FLinearColor HudWhite(0.95f, 0.99f, 1.0f, 0.96f);         // Crisp HUD White

		// Typography (+20% Font Size Increase)
		const FSlateFontInfo FontMicro    = FCoreStyle::GetDefaultFontStyle("Bold", 8);
		const FSlateFontInfo FontSmall    = FCoreStyle::GetDefaultFontStyle("Bold", 11);
		const FSlateFontInfo FontMedium   = FCoreStyle::GetDefaultFontStyle("Bold", 13);
		const FSlateFontInfo FontBold     = FCoreStyle::GetDefaultFontStyle("Bold", 16);
		const FSlateFontInfo FontCardinal = FCoreStyle::GetDefaultFontStyle("Bold", 13);
		const FSlateFontInfo FontHealth   = FCoreStyle::GetDefaultFontStyle("Bold", 29);
		const FSlateFontInfo FontBigAmmo  = FCoreStyle::GetDefaultFontStyle("Bold", 41);

		// =========================================================================
		// LOW-LEVEL GPU VERTEX & VECTOR DRAWING PRIMITIVES
		// =========================================================================

		// Draw arbitrary 4-corner quad (supports skewed parallelograms, trapezoids & smooth per-corner gradients)
		auto DrawQuad = [&](FVector2f P0, FVector2f P1, FVector2f P2, FVector2f P3,
		                    const FLinearColor& C0, const FLinearColor& C1, const FLinearColor& C2, const FLinearColor& C3,
		                    int32 SubLayer = 0)
		{
			TArray<FSlateVertex> Verts;
			Verts.SetNumUninitialized(4);
			Verts[0] = FSlateVertex::Make<ESlateVertexRounding::Disabled>(RenderTransform, P0, FVector2f(0.0f, 0.0f), C0.ToFColor(true));
			Verts[1] = FSlateVertex::Make<ESlateVertexRounding::Disabled>(RenderTransform, P1, FVector2f(1.0f, 0.0f), C1.ToFColor(true));
			Verts[2] = FSlateVertex::Make<ESlateVertexRounding::Disabled>(RenderTransform, P2, FVector2f(1.0f, 1.0f), C2.ToFColor(true));
			Verts[3] = FSlateVertex::Make<ESlateVertexRounding::Disabled>(RenderTransform, P3, FVector2f(0.0f, 1.0f), C3.ToFColor(true));

			TArray<SlateIndex> Indices = { 0, 1, 2, 0, 2, 3 };
			FSlateDrawElement::MakeCustomVerts(OutDrawElements, LayerId + SubLayer, ResourceHandle, Verts, Indices, nullptr, 0, 0);
		};

		// Draw forward-slanted parallelogram (SkewX > 0 slants top edge to the right like BF6 / MW4 bars)
		auto DrawSkewedRect = [&](float X, float Y, float W, float H, float SkewX,
		                          const FLinearColor& LeftColor, const FLinearColor& RightColor, int32 SubLayer = 0)
		{
			DrawQuad(
				FVector2f(X + SkewX, Y),
				FVector2f(X + W + SkewX, Y),
				FVector2f(X + W, Y + H),
				FVector2f(X, Y + H),
				LeftColor, RightColor, RightColor, LeftColor, SubLayer);
		};

		// Draw filled circular or polygonal triangle fan (for circular radar disc, FOV cones & sweep fans)
		auto DrawSectorFan = [&](float CX, float CY, float Radius, float StartRad, float EndRad,
		                         const FLinearColor& CenterColor, const FLinearColor& EdgeColor, int32 NumSteps = 32, int32 SubLayer = 0)
		{
			TArray<FSlateVertex> Verts;
			Verts.Reserve(NumSteps + 2);
			Verts.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(RenderTransform, FVector2f(CX, CY), FVector2f(0.5f, 0.5f), CenterColor.ToFColor(true)));

			const FColor EdgeFColor = EdgeColor.ToFColor(true);
			for (int32 i = 0; i <= NumSteps; ++i)
			{
				const float Alpha = (float)i / (float)NumSteps;
				const float Angle = FMath::Lerp(StartRad, EndRad, Alpha);
				const FVector2f Pt(CX + FMath::Cos(Angle) * Radius, CY + FMath::Sin(Angle) * Radius);
				Verts.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(RenderTransform, Pt, FVector2f(0.5f, 0.5f), EdgeFColor));
			}

			TArray<SlateIndex> Indices;
			Indices.Reserve(NumSteps * 3);
			for (int32 i = 1; i <= NumSteps; ++i)
			{
				Indices.Add(0);
				Indices.Add(i);
				Indices.Add(i + 1);
			}
			FSlateDrawElement::MakeCustomVerts(OutDrawElements, LayerId + SubLayer, ResourceHandle, Verts, Indices, nullptr, 0, 0);
		};

		auto DrawLine = [&](float X1, float Y1, float X2, float Y2, float Thickness, const FLinearColor& Tint, int32 SubLayer = 2)
		{
			TArray<FVector2f> Points = { FVector2f(X1, Y1), FVector2f(X2, Y2) };
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + SubLayer, AllottedGeometry.ToPaintGeometry(), MoveTemp(Points), ESlateDrawEffect::None, Tint, true, Thickness);
		};

		auto DrawGradientLine = [&](float X1, float Y1, float X2, float Y2, float Thickness, const FLinearColor& C1, const FLinearColor& C2, int32 SubLayer = 2)
		{
			TArray<FVector2f> Points = { FVector2f(X1, Y1), FVector2f(X2, Y2) };
			TArray<FLinearColor> Colors = { C1, C2 };
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + SubLayer, AllottedGeometry.ToPaintGeometry(), MoveTemp(Points), MoveTemp(Colors), ESlateDrawEffect::None, FLinearColor::White, true, Thickness);
		};

		auto DrawArc = [&](float CX, float CY, float Radius, float StartDeg, float EndDeg, float Thickness, const FLinearColor& Tint, int32 Segments = 28, int32 SubLayer = 2)
		{
			TArray<FVector2f> Points;
			Points.Reserve(Segments + 1);
			for (int32 i = 0; i <= Segments; ++i)
			{
				const float Deg = FMath::Lerp(StartDeg, EndDeg, (float)i / (float)Segments);
				const float Rad = FMath::DegreesToRadians(Deg);
				Points.Add(FVector2f(CX + FMath::Cos(Rad) * Radius, CY + FMath::Sin(Rad) * Radius));
			}
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + SubLayer, AllottedGeometry.ToPaintGeometry(), MoveTemp(Points), ESlateDrawEffect::None, Tint, true, Thickness);
		};

		auto DrawDiamond = [&](float CX, float CY, float Radius, float Thickness, const FLinearColor& Tint, bool bFilled = false, int32 SubLayer = 3)
		{
			if (bFilled)
			{
				DrawQuad(
					FVector2f(CX, CY - Radius),
					FVector2f(CX + Radius, CY),
					FVector2f(CX, CY + Radius),
					FVector2f(CX - Radius, CY),
					Tint, Tint, Tint, Tint, SubLayer - 1);
			}
			TArray<FVector2f> Points = {
				FVector2f(CX, CY - Radius),
				FVector2f(CX + Radius, CY),
				FVector2f(CX, CY + Radius),
				FVector2f(CX - Radius, CY),
				FVector2f(CX, CY - Radius)
			};
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + SubLayer, AllottedGeometry.ToPaintGeometry(), MoveTemp(Points), ESlateDrawEffect::None, Tint, true, Thickness);
		};

		// Crisp Floating Text with Drop-Shadow & Subtle Holographic Back-Glow (no opaque box needed!)
		auto DrawGlowText = [&](float X, float Y, const FString& Text, const FSlateFontInfo& Font, const FLinearColor& Tint, float ShadowOpacity = 0.65f, int32 SubLayer = 4)
		{
			if (ShadowOpacity > 0.01f)
			{
				const FLinearColor ShadowColor(0.0f, 0.01f, 0.03f, Tint.A * ShadowOpacity);
				FPaintGeometry ShadowGeom = AllottedGeometry.ToPaintGeometry(FSlateLayoutTransform(FVector2f(X + 1.5f, Y + 1.5f)));
				FSlateDrawElement::MakeText(OutDrawElements, LayerId + SubLayer - 1, ShadowGeom, Text, Font, ESlateDrawEffect::None, ShadowColor);
			}
			FPaintGeometry TextGeom = AllottedGeometry.ToPaintGeometry(FSlateLayoutTransform(FVector2f(X, Y)));
			FSlateDrawElement::MakeText(OutDrawElements, LayerId + SubLayer, TextGeom, Text, Font, ESlateDrawEffect::None, Tint);
		};

		// =========================================================================
		// 0. AR HELMET VISOR PERIPHERAL BRACKETS & TOP-RIGHT TELEMETRY
		// =========================================================================
		{
			const float Margin = 18.0f * HUDScale;
			const float CornerLen = 22.0f * HUDScale;
			const float LineThick = 1.2f * HUDScale;
			const FLinearColor VisorFrameTint(0.0f, 0.90f, 0.85f, 0.18f);

			// Top-Left & Top-Right peripheral visor ticks
			DrawLine(Margin + VX * 0.3f, Margin + VY * 0.3f, Margin + CornerLen + VX * 0.3f, Margin + VY * 0.3f, LineThick, VisorFrameTint, 1);
			DrawLine(Margin + VX * 0.3f, Margin + VY * 0.3f, Margin + VX * 0.3f, Margin + CornerLen + VY * 0.3f, LineThick, VisorFrameTint, 1);

			DrawLine(ScreenW - Margin - CornerLen + VX * 0.3f, Margin + VY * 0.3f, ScreenW - Margin + VX * 0.3f, Margin + VY * 0.3f, LineThick, VisorFrameTint, 1);
			DrawLine(ScreenW - Margin + VX * 0.3f, Margin + VY * 0.3f, ScreenW - Margin + VX * 0.3f, Margin + CornerLen + VY * 0.3f, LineThick, VisorFrameTint, 1);

			// Bottom-Left & Bottom-Right peripheral visor ticks
			DrawLine(Margin + VX * 0.3f, ScreenH - Margin + VY * 0.3f, Margin + CornerLen + VX * 0.3f, ScreenH - Margin + VY * 0.3f, LineThick, VisorFrameTint, 1);
			DrawLine(Margin + VX * 0.3f, ScreenH - Margin - CornerLen + VY * 0.3f, Margin + VX * 0.3f, ScreenH - Margin + VY * 0.3f, LineThick, VisorFrameTint, 1);

			DrawLine(ScreenW - Margin - CornerLen + VX * 0.3f, ScreenH - Margin + VY * 0.3f, ScreenW - Margin + VX * 0.3f, ScreenH - Margin + VY * 0.3f, LineThick, VisorFrameTint, 1);
			DrawLine(ScreenW - Margin + VX * 0.3f, ScreenH - Margin - CornerLen + VY * 0.3f, ScreenW - Margin + VX * 0.3f, ScreenH - Margin + VY * 0.3f, LineThick, VisorFrameTint, 1);

			// Top-Right Floating AR Telemetry Link (Frameless, Scaled 1.20x)
			const float TeleW = 215.0f * HUDScale;
			const float TeleH = 46.0f * HUDScale;
			const float TeleX = ScreenW - TeleW - Margin + VX * 0.45f;
			const float TeleY = Margin + 4.0f + VY * 0.45f;
			DrawSkewedRect(TeleX - 10.0f * HUDScale, TeleY - 3.0f * HUDScale, TeleW, TeleH, -8.0f * HUDScale,
				FLinearColor(0.0f, 0.08f, 0.12f, 0.0f),
				FLinearColor(0.0f, 0.16f, 0.22f, 0.22f), 0);
			DrawLine(TeleX + TeleW - 13.0f * HUDScale, TeleY - 3.0f * HUDScale, TeleX + TeleW - 21.0f * HUDScale, TeleY + TeleH - 3.0f * HUDScale, 2.0f * HUDScale, TealPrimary * FLinearColor(1, 1, 1, 0.65f), 2);

			DrawGlowText(TeleX, TeleY, TEXT("AR-VISOR // TAC-LINK 128HZ"), FontMicro, TealPrimary * FLinearColor(1, 1, 1, 0.80f), 0.5f);
			const float SpeedMetersPerSec = OwningHUD->PlayerSpeed / 100.0f;
			DrawGlowText(TeleX + 18.0f * HUDScale, TeleY + 13.0f * HUDScale,
				FString::Printf(TEXT("VEL: %04.1f M/S  //  SYNC: OK"), SpeedMetersPerSec),
				FontMicro, FLinearColor(0.75f, 0.88f, 0.95f, 0.60f), 0.5f);

			// Real-Time Persistent Economy Wallet
			DrawGlowText(TeleX + 6.0f * HUDScale, TeleY + 26.0f * HUDScale,
				FString::Printf(TEXT("WALLET: %d GOLD  //  %d OPALS"), OwningHUD->CurrentGoldBalance, OwningHUD->CurrentOpalBalance),
				FontMicro, AmberWarn * FLinearColor(1, 1, 1, 0.95f), 0.6f);

			// Bridge 3: Live Bounty Tracker on In-Game HUD (Active Contracts)
			if (OwningHUD->CachedBounties.Num() > 0)
			{
				const float BountyBoxY = TeleY + TeleH + 6.0f * HUDScale;
				const int32 NumToDraw = FMath::Min(OwningHUD->CachedBounties.Num(), 3);
				const float BountyBoxH = (NumToDraw * 13.0f + 16.0f) * HUDScale;
				DrawSkewedRect(TeleX - 10.0f * HUDScale, BountyBoxY - 2.0f * HUDScale, TeleW, BountyBoxH, -6.0f * HUDScale,
					FLinearColor(0.0f, 0.05f, 0.08f, 0.0f),
					FLinearColor(0.01f, 0.10f, 0.15f, 0.35f), 0);
				DrawLine(TeleX + TeleW - 13.0f * HUDScale, BountyBoxY - 2.0f * HUDScale, TeleX + TeleW - 19.0f * HUDScale, BountyBoxY + BountyBoxH - 2.0f * HUDScale, 1.5f * HUDScale, CyanTactical * FLinearColor(1, 1, 1, 0.5f), 2);

				DrawGlowText(TeleX, BountyBoxY, TEXT("ACTIVE CONTRACTS // DAILY"), FontMicro, CyanTactical * FLinearColor(1, 1, 1, 0.85f), 0.5f);

				for (int32 b = 0; b < NumToDraw; ++b)
				{
					const FFPSTacticalBounty& Bounty = OwningHUD->CachedBounties[b];
					const float LineY = BountyBoxY + 13.0f * HUDScale + (b * 12.0f * HUDScale);
					FString BountyLine;
					FLinearColor LineColor;
					if (Bounty.bCompleted)
					{
						BountyLine = FString::Printf(TEXT("[DONE] %s"), *Bounty.Title.ToString().ToUpper());
						LineColor = FLinearColor(0.2f, 1.0f, 0.45f, 0.90f);
					}
					else
					{
						BountyLine = FString::Printf(TEXT("[%d/%d] %s"), Bounty.CurrentProgress, Bounty.TargetProgress, *Bounty.Title.ToString().ToUpper());
						LineColor = FLinearColor(0.85f, 0.92f, 0.98f, 0.75f);
					}
					DrawGlowText(TeleX + 4.0f * HUDScale, LineY, BountyLine, FontMicro, LineColor, 0.4f);
				}
			}
		}

		// =========================================================================
		// 1. TOP-CENTER FRAMELESS HOLOGRAPHIC COMPASS TAPE (MW4 / BF6 Style)
		// =========================================================================
		const float CenterCompassX = ScreenW * 0.5f + VX * 0.38f;
		const float TopY = 20.0f * HUDScale + VY * 0.38f;
		const float HalfW = 240.0f * HUDScale;
		const float TapeH = 30.0f * HUDScale;
		const float PixelsPerDeg = HalfW / 50.0f; // 100-degree field of view along tape

		// Soft frameless horizontal vignette backing (0.0 alpha at edges -> 0.20 at center -> 0.0 at right)
		DrawQuad(
			FVector2f(CenterCompassX - HalfW, TopY),
			FVector2f(CenterCompassX, TopY),
			FVector2f(CenterCompassX, TopY + TapeH),
			FVector2f(CenterCompassX - HalfW, TopY + TapeH),
			FLinearColor(0.0f, 0.04f, 0.07f, 0.0f),
			FLinearColor(0.0f, 0.08f, 0.14f, 0.22f),
			FLinearColor(0.0f, 0.04f, 0.08f, 0.04f),
			FLinearColor(0.0f, 0.04f, 0.07f, 0.0f), 0);

		DrawQuad(
			FVector2f(CenterCompassX, TopY),
			FVector2f(CenterCompassX + HalfW, TopY),
			FVector2f(CenterCompassX + HalfW, TopY + TapeH),
			FVector2f(CenterCompassX, TopY + TapeH),
			FLinearColor(0.0f, 0.08f, 0.14f, 0.22f),
			FLinearColor(0.0f, 0.04f, 0.07f, 0.0f),
			FLinearColor(0.0f, 0.04f, 0.07f, 0.0f),
			FLinearColor(0.0f, 0.04f, 0.08f, 0.04f), 0);

		// Razor-thin top horizon line with edge fade
		DrawGradientLine(CenterCompassX - HalfW, TopY, CenterCompassX, TopY, 1.2f * HUDScale,
			FLinearColor(0.0f, 0.94f, 0.86f, 0.0f), FLinearColor(0.0f, 0.94f, 0.86f, 0.55f), 1);
		DrawGradientLine(CenterCompassX, TopY, CenterCompassX + HalfW, TopY, 1.2f * HUDScale,
			FLinearColor(0.0f, 0.94f, 0.86f, 0.55f), FLinearColor(0.0f, 0.94f, 0.86f, 0.0f), 1);

		// Normalize Camera Yaw [0, 360)
		float NormYaw = FMath::Fmod(OwningHUD->CameraYaw, 360.0f);
		if (NormYaw < 0.0f) NormYaw += 360.0f;

		// Render Ticks & Cardinals with Cosine Edge Vignette
		for (int32 deg = 0; deg < 360; deg += 5)
		{
			const float Delta = FRotator::NormalizeAxis((float)deg - NormYaw);
			if (FMath::Abs(Delta) <= 50.0f)
			{
				const float TickX = CenterCompassX + Delta * PixelsPerDeg;
				// Smooth cosine vignette fade so elements dissolve organically at the ends
				const float EdgeFade = FMath::Cos((Delta / 50.0f) * (PI * 0.5f));

				if (deg % 45 == 0)
				{
					FString Cardinal;
					bool bPrimaryCardinal = false;
					if (deg == 0)        { Cardinal = TEXT("N"); bPrimaryCardinal = true; }
					else if (deg == 45)  { Cardinal = TEXT("NE"); }
					else if (deg == 90)  { Cardinal = TEXT("E"); bPrimaryCardinal = true; }
					else if (deg == 135) { Cardinal = TEXT("SE"); }
					else if (deg == 180) { Cardinal = TEXT("S"); bPrimaryCardinal = true; }
					else if (deg == 225) { Cardinal = TEXT("SW"); }
					else if (deg == 270) { Cardinal = TEXT("W"); bPrimaryCardinal = true; }
					else if (deg == 315) { Cardinal = TEXT("NW"); }

					const FLinearColor CardColor = (deg == 0)
						? FLinearColor(1.0f, 0.70f, 0.0f, 0.98f * EdgeFade) // Amber N
						: (bPrimaryCardinal ? FLinearColor(0.0f, 0.90f, 1.0f, 0.95f * EdgeFade) : FLinearColor(0.85f, 0.95f, 1.0f, 0.82f * EdgeFade));

					DrawLine(TickX, TopY, TickX, TopY + 12.0f * HUDScale, (bPrimaryCardinal ? 2.0f : 1.5f) * HUDScale, CardColor, 2);
					const float TextOffset = (Cardinal.Len() == 1) ? 5.0f * HUDScale : 9.0f * HUDScale;
					DrawGlowText(TickX - TextOffset, TopY + 14.0f * HUDScale, Cardinal, FontCardinal, CardColor, 0.7f);
				}
				else if (deg % 15 == 0)
				{
					const FLinearColor MajColor(0.75f, 0.88f, 0.96f, 0.62f * EdgeFade);
					DrawLine(TickX, TopY, TickX, TopY + 8.5f * HUDScale, 1.2f * HUDScale, MajColor, 2);
					DrawGlowText(TickX - 9.0f * HUDScale, TopY + 11.5f * HUDScale, FString::Printf(TEXT("%03d"), deg), FontMicro, MajColor * FLinearColor(1, 1, 1, 0.85f), 0.5f);
				}
				else
				{
					DrawLine(TickX, TopY, TickX, TopY + 5.5f * HUDScale, 1.0f * HUDScale, FLinearColor(0.5f, 0.72f, 0.85f, 0.35f * EdgeFade), 2);
				}
			}
		}

		// Center Inverted Lubber Chevron (V) & Chamfered Digital Bearing Pill
		DrawLine(CenterCompassX - 5.5f * HUDScale, TopY - 6.5f * HUDScale, CenterCompassX, TopY - 1.0f * HUDScale, 2.0f * HUDScale, TealPrimary, 3);
		DrawLine(CenterCompassX + 5.5f * HUDScale, TopY - 6.5f * HUDScale, CenterCompassX, TopY - 1.0f * HUDScale, 2.0f * HUDScale, TealPrimary, 3);
		DrawLine(CenterCompassX, TopY, CenterCompassX, TopY + 11.0f * HUDScale, 1.8f * HUDScale, TealPrimary, 3);

		// Determine current cardinal string for readout
		const TCHAR* DirNames[] = { TEXT("N"), TEXT("NE"), TEXT("E"), TEXT("SE"), TEXT("S"), TEXT("SW"), TEXT("W"), TEXT("NW") };
		const int32 DirIdx = FMath::RoundToInt(NormYaw / 45.0f) % 8;
		const FString BearingPillText = FString::Printf(TEXT("%03.0f° %s"), NormYaw, DirNames[DirIdx]);

		// Subtle chamfered pill under center of compass
		DrawSkewedRect(CenterCompassX - 32.0f * HUDScale, TopY + TapeH - 1.0f * HUDScale, 64.0f * HUDScale, 16.0f * HUDScale, -4.0f * HUDScale,
			FLinearColor(0.0f, 0.14f, 0.20f, 0.45f), FLinearColor(0.0f, 0.22f, 0.30f, 0.45f), 1);
		DrawLine(CenterCompassX - 32.0f * HUDScale, TopY + TapeH - 1.0f * HUDScale, CenterCompassX + 28.0f * HUDScale, TopY + TapeH - 1.0f * HUDScale, 1.2f * HUDScale, TealPrimary * FLinearColor(1, 1, 1, 0.7f), 2);
		DrawGlowText(CenterCompassX - 24.0f * HUDScale, TopY + TapeH + 0.5f * HUDScale, BearingPillText, FontSmall, HudWhite, 0.8f);

		// Detect Active Theater (0 = Favela // Rocha Velha, 1 = Favela // Baía Brava)
		int32 ActiveTheaterIdx = 0;
		if (UWorld* HudWorld = OwningHUD->GetWorld())
		{
			if (UGameplayStatics::GetActorOfClass(HudWorld, AFPSCoastalOperationsMapActor::StaticClass()) != nullptr)
			{
				ActiveTheaterIdx = 1;
			}
			else if (UGameInstance* GI = HudWorld->GetGameInstance())
			{
				if (UFPSEconomySubsystem* Economy = GI->GetSubsystem<UFPSEconomySubsystem>())
				{
					ActiveTheaterIdx = Economy->GetSelectedTheaterIndex();
				}
			}
		}
		const bool bIsCoastalOps = (ActiveTheaterIdx == 1);

		// Objectives Definition (Flags A, B, C per Theater)
		const struct { FVector Loc; const TCHAR* Name; FLinearColor Color; } Objectives[] = {
			{ bIsCoastalOps ? FVector(0.0f, 4000.0f, 15.0f)   : FVector(-1400.0f, -1000.0f, 175.0f), TEXT("A"), FLinearColor(0.0f, 0.90f, 1.0f, 0.95f) },
			{ bIsCoastalOps ? FVector(0.0f, -500.0f, 425.0f)  : FVector(0.0f, 0.0f, 200.0f),         TEXT("B"), FLinearColor(1.0f, 0.70f, 0.0f, 0.95f) },
			{ bIsCoastalOps ? FVector(0.0f, -5800.0f, 265.0f) : FVector(1400.0f, 1000.0f, 175.0f),   TEXT("C"), FLinearColor(0.0f, 0.95f, 0.78f, 0.95f) }
		};

		for (const auto& Obj : Objectives)
		{
			const FVector Dir = Obj.Loc - OwningHUD->PlayerLocation;
			const float DistMeters = Dir.Size2D() / 100.0f;
			const float ObjBearing = FMath::RadiansToDegrees(FMath::Atan2(Dir.Y, Dir.X));
			const float DeltaObj = FRotator::NormalizeAxis(ObjBearing - NormYaw);
			if (FMath::Abs(DeltaObj) <= 46.0f)
			{
				const float ObjX = CenterCompassX + DeltaObj * PixelsPerDeg;
				const float EdgeFade = FMath::Cos((DeltaObj / 50.0f) * (PI * 0.5f));
				FLinearColor C = Obj.Color;
				C.A *= EdgeFade;
				DrawDiamond(ObjX, TopY - 8.0f * HUDScale, 6.5f * HUDScale, 1.5f * HUDScale, C, true, 3);
				DrawGlowText(ObjX - 4.0f * HUDScale, TopY - 14.0f * HUDScale, Obj.Name, FontMicro, FLinearColor(0.02f, 0.04f, 0.06f, EdgeFade), 0.0f, 4);
			}
		}

		// Gunfire Acoustic Radar Threat Pings on Compass
		UWorld* World = OwningHUD->GetWorld();
		if (World)
		{
			if (UFPSRadarSubsystem* Radar = World->GetSubsystem<UFPSRadarSubsystem>())
			{
				TArray<FFPSRadarPing> Pings = Radar->GetVisiblePingsForTeam(EFPSPlayerTeam::Alpha);
				for (const FFPSRadarPing& Ping : Pings)
				{
					const FVector Dir = Ping.WorldLocation - OwningHUD->PlayerLocation;
					const float PingBearing = FMath::RadiansToDegrees(FMath::Atan2(Dir.Y, Dir.X));
					const float DeltaPing = FRotator::NormalizeAxis(PingBearing - NormYaw);
					const float ClampedDelta = FMath::Clamp(DeltaPing, -48.0f, 48.0f);
					const float PingX = CenterCompassX + ClampedDelta * PixelsPerDeg;
					const float Pulse = 1.0f + 0.35f * FMath::Sin(TimeSec * 14.0f);
					DrawDiamond(PingX, TopY + 7.0f * HUDScale, 6.5f * Pulse * HUDScale, 1.8f * HUDScale, CrimsonAlert, true, 4);
				}
			}
		}

		// =========================================================================
		// 2. TOP-LEFT TRUE CIRCULAR HOLOGRAPHIC MINIMAP (No Square Box!)
		// =========================================================================
		const float MapRadius = 82.0f * HUDScale;
		const float RCX = 26.0f * HUDScale + MapRadius + VX * 0.56f;
		const float RCY = 26.0f * HUDScale + MapRadius + VY * 0.56f;

		// 2.1 Radial Translucent Holographic Disc (Dark center -> Glowing Teal Rim, ZERO square box!)
		DrawSectorFan(RCX, RCY, MapRadius + 4.0f * HUDScale, 0.0f, 2.0f * PI,
			FLinearColor(0.0f, 0.05f, 0.09f, 0.42f),
			FLinearColor(0.0f, 0.18f, 0.26f, 0.12f), 40, 0);

		DrawSectorFan(RCX, RCY, MapRadius, 0.0f, 2.0f * PI,
			FLinearColor(0.01f, 0.04f, 0.07f, 0.36f),
			FLinearColor(0.0f, 0.32f, 0.42f, 0.26f), 40, 0);

		// 2.2 Camera FOV View Cone Wedge (Pointing Upward from Center Player Chevron: -125 deg to -55 deg)
		DrawSectorFan(RCX, RCY, MapRadius - 2.0f * HUDScale,
			FMath::DegreesToRadians(-125.0f), FMath::DegreesToRadians(-55.0f),
			FLinearColor(0.0f, 0.94f, 0.86f, 0.18f),
			FLinearColor(0.0f, 0.94f, 0.86f, 0.01f), 16, 1);

		// Crisp FOV Wedge Boundary Lines
		DrawGradientLine(RCX, RCY,
			RCX + FMath::Cos(FMath::DegreesToRadians(-125.0f)) * (MapRadius - 2.0f * HUDScale),
			RCY + FMath::Sin(FMath::DegreesToRadians(-125.0f)) * (MapRadius - 2.0f * HUDScale),
			1.2f * HUDScale, FLinearColor(0.0f, 0.94f, 0.86f, 0.45f), FLinearColor(0.0f, 0.94f, 0.86f, 0.05f), 2);
		DrawGradientLine(RCX, RCY,
			RCX + FMath::Cos(FMath::DegreesToRadians(-55.0f)) * (MapRadius - 2.0f * HUDScale),
			RCY + FMath::Sin(FMath::DegreesToRadians(-55.0f)) * (MapRadius - 2.0f * HUDScale),
			1.2f * HUDScale, FLinearColor(0.0f, 0.94f, 0.86f, 0.45f), FLinearColor(0.0f, 0.94f, 0.86f, 0.05f), 2);

		// 2.3 Rotating Phosphor Radar Sweep Sector Fan
		{
			const float SweepDeg = FMath::Fmod(TimeSec * 110.0f, 360.0f);
			const float SweepRad = FMath::DegreesToRadians(SweepDeg);
			const float TrailRad = FMath::DegreesToRadians(SweepDeg - 32.0f);
			DrawSectorFan(RCX, RCY, MapRadius - 2.0f * HUDScale, TrailRad, SweepRad,
				FLinearColor(0.0f, 0.94f, 0.86f, 0.12f),
				FLinearColor(0.0f, 0.94f, 0.86f, 0.02f), 10, 1);
			DrawLine(RCX, RCY, RCX + FMath::Cos(SweepRad) * (MapRadius - 2.0f * HUDScale), RCY + FMath::Sin(SweepRad) * (MapRadius - 2.0f * HUDScale),
				1.4f * HUDScale, FLinearColor(0.0f, 0.96f, 0.88f, 0.42f), 2);
		}

		// 2.4 Concentric Range Rings & Subtle Axis Hairlines
		DrawArc(RCX, RCY, MapRadius, 0.0f, 360.0f, 1.8f * HUDScale, FLinearColor(0.0f, 0.94f, 0.86f, 0.55f), 40, 2);
		DrawArc(RCX, RCY, MapRadius + 4.0f * HUDScale, -60.0f, 60.0f, 1.3f * HUDScale, FLinearColor(0.0f, 0.90f, 1.0f, 0.30f), 16, 2);
		DrawArc(RCX, RCY, MapRadius + 4.0f * HUDScale, 120.0f, 240.0f, 1.3f * HUDScale, FLinearColor(0.0f, 0.90f, 1.0f, 0.30f), 16, 2);
		DrawArc(RCX, RCY, MapRadius * 0.62f, 0.0f, 360.0f, 1.1f * HUDScale, FLinearColor(0.0f, 0.85f, 0.85f, 0.18f), 32, 1);
		DrawArc(RCX, RCY, MapRadius * 0.31f, 0.0f, 360.0f, 1.1f * HUDScale, FLinearColor(0.0f, 0.85f, 0.85f, 0.12f), 24, 1);

		// Outer Bezel Degree Micro-Ticks (every 15 degrees)
		for (int32 deg = 0; deg < 360; deg += 15)
		{
			const float Rad = FMath::DegreesToRadians((float)deg - NormYaw - 90.0f);
			const float InnerR = (deg % 90 == 0) ? (MapRadius - 5.5f * HUDScale) : (MapRadius - 3.0f * HUDScale);
			const float OuterR = MapRadius;
			const FLinearColor TickCol = (deg == 0)
				? FLinearColor(1.0f, 0.70f, 0.0f, 0.9f)
				: FLinearColor(0.0f, 0.94f, 0.86f, (deg % 90 == 0) ? 0.65f : 0.28f);
			DrawLine(RCX + FMath::Cos(Rad) * InnerR, RCY + FMath::Sin(Rad) * InnerR,
			         RCX + FMath::Cos(Rad) * OuterR, RCY + FMath::Sin(Rad) * OuterR,
			         (deg % 90 == 0) ? 1.8f * HUDScale : 1.1f * HUDScale, TickCol, 2);
		}

		// Orbiting North ('N') Indicator on Outer Rim of Minimap
		{
			const float NorthRad = FMath::DegreesToRadians(-NormYaw - 90.0f);
			const float NX = RCX + FMath::Cos(NorthRad) * (MapRadius + 11.0f * HUDScale);
			const float NY = RCY + FMath::Sin(NorthRad) * (MapRadius + 11.0f * HUDScale);
			DrawDiamond(NX, NY, 7.5f * HUDScale, 1.3f * HUDScale, FLinearColor(0.02f, 0.08f, 0.12f, 0.85f), true, 3);
			DrawDiamond(NX, NY, 7.5f * HUDScale, 1.3f * HUDScale, FLinearColor(1.0f, 0.70f, 0.0f, 0.90f), false, 3);
			DrawGlowText(NX - 4.0f * HUDScale, NY - 6.0f * HUDScale, TEXT("N"), FontMicro, FLinearColor(1.0f, 0.75f, 0.15f, 1.0f), 0.6f, 4);
		}

		// 2.5 Objectives on Minimap (Flags A, B, C)
		const float YawRad = FMath::DegreesToRadians(NormYaw);
		for (const auto& Obj : Objectives)
		{
			const FVector Delta = Obj.Loc - OwningHUD->PlayerLocation;
			const float Fwd = Delta.X * FMath::Cos(-YawRad) - Delta.Y * FMath::Sin(-YawRad);
			const float Right = Delta.X * FMath::Sin(-YawRad) + Delta.Y * FMath::Cos(-YawRad);

			float ScreenDX = (Right / 4500.0f) * MapRadius;
			float ScreenDY = -(Fwd / 4500.0f) * MapRadius;

			const float Dist = FMath::Sqrt(ScreenDX * ScreenDX + ScreenDY * ScreenDY);
			const float MaxObjR = MapRadius - 10.0f * HUDScale;
			if (Dist > MaxObjR)
			{
				ScreenDX = (ScreenDX / Dist) * MaxObjR;
				ScreenDY = (ScreenDY / Dist) * MaxObjR;
			}

			const float OX = RCX + ScreenDX;
			const float OY = RCY + ScreenDY;
			DrawDiamond(OX, OY, 7.5f * HUDScale, 1.6f * HUDScale, FLinearColor(0.01f, 0.05f, 0.09f, 0.85f), true, 3);
			DrawDiamond(OX, OY, 7.5f * HUDScale, 1.6f * HUDScale, Obj.Color, false, 3);
			DrawGlowText(OX - 3.5f * HUDScale, OY - 5.5f * HUDScale, Obj.Name, FontMicro, HudWhite, 0.7f, 4);
		}

		// 2.6 Acoustic Gunfire Radar Pings on Minimap (with expanding soundwave ring!)
		if (World)
		{
			if (UFPSRadarSubsystem* Radar = World->GetSubsystem<UFPSRadarSubsystem>())
			{
				TArray<FFPSRadarPing> Pings = Radar->GetVisiblePingsForTeam(EFPSPlayerTeam::Alpha);
				for (const FFPSRadarPing& Ping : Pings)
				{
					const FVector Delta = Ping.WorldLocation - OwningHUD->PlayerLocation;
					const float Fwd = Delta.X * FMath::Cos(-YawRad) - Delta.Y * FMath::Sin(-YawRad);
					const float Right = Delta.X * FMath::Sin(-YawRad) + Delta.Y * FMath::Cos(-YawRad);

					float ScreenDX = (Right / 4500.0f) * MapRadius;
					float ScreenDY = -(Fwd / 4500.0f) * MapRadius;

					const float Dist = FMath::Sqrt(ScreenDX * ScreenDX + ScreenDY * ScreenDY);
					if (Dist > MapRadius - 7.0f * HUDScale)
					{
						ScreenDX = (ScreenDX / Dist) * (MapRadius - 7.0f * HUDScale);
						ScreenDY = (ScreenDY / Dist) * (MapRadius - 7.0f * HUDScale);
					}

					const float LifeRatio = FMath::Clamp(Ping.LifetimeRemaining / FMath::Max(0.1f, Ping.InitialLifetime), 0.0f, 1.0f);
					const float PX = RCX + ScreenDX;
					const float PY = RCY + ScreenDY;

					// Expanding ripple ring + glowing hostile diamond
					const float RippleR = (3.0f + (1.0f - LifeRatio) * 14.0f) * HUDScale;
					DrawArc(PX, PY, RippleR, 0.0f, 360.0f, 1.3f * HUDScale, FLinearColor(1.0f, 0.165f, 0.33f, LifeRatio * 0.85f), 16, 3);
					DrawDiamond(PX, PY, 4.5f * HUDScale, 1.6f * HUDScale, FLinearColor(1.0f, 0.2f, 0.25f, LifeRatio), true, 4);
				}
			}
		}

		// 2.7 Center Player Tactical Arrowhead Chevron
		DrawQuad(
			FVector2f(RCX, RCY - 8.0f * HUDScale),
			FVector2f(RCX + 6.5f * HUDScale, RCY + 6.5f * HUDScale),
			FVector2f(RCX, RCY + 2.5f * HUDScale),
			FVector2f(RCX - 6.5f * HUDScale, RCY + 6.5f * HUDScale),
			HudWhite, TealPrimary, TealPrimary, TealPrimary, 4);

		// 2.8 Frameless Floating Sector & Grid Callout Below Circular Radar
		const float CalloutX = RCX - MapRadius + 4.0f * HUDScale;
		const float CalloutY = RCY + MapRadius + 14.0f * HUDScale;
		DrawSkewedRect(CalloutX - 4.0f * HUDScale, CalloutY - 2.0f * HUDScale, 185.0f * HUDScale, 20.0f * HUDScale, -5.0f * HUDScale,
			FLinearColor(0.0f, 0.16f, 0.22f, 0.30f), FLinearColor(0.0f, 0.08f, 0.12f, 0.0f), 0);
		DrawLine(CalloutX - 6.0f * HUDScale, CalloutY - 2.0f * HUDScale, CalloutX - 11.0f * HUDScale, CalloutY + 18.0f * HUDScale, 2.2f * HUDScale, TealPrimary, 2);
		DrawGlowText(CalloutX, CalloutY,
			FString::Printf(TEXT("%s // [%+03.0fm, %+03.0fm]"), bIsCoastalOps ? TEXT("FAVELA // BAÍA BRAVA") : TEXT("FAVELA // ROCHA VELHA"), OwningHUD->PlayerLocation.X / 100.0f, OwningHUD->PlayerLocation.Y / 100.0f),
			FontMicro, FLinearColor(0.78f, 0.92f, 0.98f, 0.82f), 0.7f);

		// =========================================================================
		// 3. BOTTOM-LEFT FLOATING VITALS, 4-PLATE ARMOR & LIVE ECG (BF6 / MW4)
		// =========================================================================
		const float VitW = 310.0f * HUDScale;
		const float VitX = 32.0f * HUDScale + VX * 0.64f;
		const float VitY = ScreenH - 138.0f * HUDScale + VY * 0.64f;
		const float BarSkew = 7.0f * HUDScale; // Forward tactical shear

		// Translucent frameless glass gradient strip (fades completely to 0.0 alpha on the right!)
		DrawSkewedRect(VitX - 12.0f * HUDScale, VitY - 8.0f * HUDScale, VitW, 95.0f * HUDScale, BarSkew,
			FLinearColor(0.0f, 0.09f, 0.14f, 0.28f),
			FLinearColor(0.0f, 0.04f, 0.07f, 0.0f), 0);

		// Glowing Left Spine Accent Bracket
		DrawLine(VitX - 12.0f * HUDScale + BarSkew, VitY - 8.0f * HUDScale, VitX - 12.0f * HUDScale, VitY + 86.0f * HUDScale, 2.8f * HUDScale, TealPrimary, 2);
		DrawLine(VitX - 12.0f * HUDScale + BarSkew, VitY - 8.0f * HUDScale, VitX + 28.0f * HUDScale + BarSkew, VitY - 8.0f * HUDScale, 1.8f * HUDScale, TealPrimary * FLinearColor(1, 1, 1, 0.7f), 2);
		DrawLine(VitX - 12.0f * HUDScale, VitY + 86.0f * HUDScale, VitX + 20.0f * HUDScale, VitY + 86.0f * HUDScale, 1.8f * HUDScale, TealPrimary * FLinearColor(1, 1, 1, 0.7f), 2);

		// 3.0 High-Contrast Tactical Backing Panel for Vitals
		DrawSkewedRect(VitX - 8.0f * HUDScale, VitY - 8.0f * HUDScale, 292.0f * HUDScale, 78.0f * HUDScale, 6.0f * HUDScale,
			FLinearColor(0.01f, 0.03f, 0.06f, 0.78f), FLinearColor(0.02f, 0.04f, 0.08f, 0.65f), 0);

		// 3.1 Segmented 4-Plate Level IV Ceramic Armor Row
		const float ShieldPct = OwningHUD->MaxShield > 0.0f ? FMath::Clamp(OwningHUD->CurrentShield / OwningHUD->MaxShield, 0.0f, 1.0f) : 0.0f;
		const float ShieldGhostPct = OwningHUD->MaxShield > 0.0f ? FMath::Clamp(OwningHUD->DisplayedShieldGhost / OwningHUD->MaxShield, 0.0f, 1.0f) : 0.0f;
		const int32 NumPlates = 4;
		const float PlateW = 49.0f * HUDScale;
		const float PlateGap = 5.0f * HUDScale;
		const float PlateY = VitY + 14.0f * HUDScale;
		const float PlateH = 9.0f * HUDScale;

		DrawGlowText(VitX + 4.0f * HUDScale, VitY - 3.0f * HUDScale, TEXT("LVL-IV CERAMIC PLATES"), FontMicro, CobaltArmor * FLinearColor(1, 1, 1, 0.90f), 0.7f);
		DrawGlowText(VitX + 165.0f * HUDScale, VitY - 3.0f * HUDScale, FString::Printf(TEXT("%03.0f"), OwningHUD->CurrentShield), FontSmall,
			ShieldPct > 0.01f ? CobaltArmor : FLinearColor(0.5f, 0.6f, 0.7f, 0.5f), 0.7f);

		for (int32 p = 0; p < NumPlates; ++p)
		{
			const float PX = VitX + p * (PlateW + PlateGap);
			const float PlateStartPct = (float)p / (float)NumPlates;
			const float PlateEndPct = (float)(p + 1) / (float)NumPlates;
			const float LocalFill = FMath::Clamp((ShieldPct - PlateStartPct) / (PlateEndPct - PlateStartPct), 0.0f, 1.0f);
			const float LocalGhost = FMath::Clamp((ShieldGhostPct - PlateStartPct) / (PlateEndPct - PlateStartPct), 0.0f, 1.0f);

			// Empty Plate Track
			DrawSkewedRect(PX, PlateY, PlateW, PlateH, 4.0f * HUDScale,
				FLinearColor(0.02f, 0.08f, 0.16f, 0.35f), FLinearColor(0.02f, 0.08f, 0.16f, 0.22f), 1);

			// Trailing Shield Damage Ghost
			if (LocalGhost > LocalFill + 0.01f)
			{
				DrawSkewedRect(PX, PlateY, PlateW * LocalGhost, PlateH, 4.0f * HUDScale,
					FLinearColor(0.8f, 0.95f, 1.0f, 0.55f), FLinearColor(0.4f, 0.8f, 1.0f, 0.55f), 1);
			}

			// Active Ceramic Plate Fill
			if (LocalFill > 0.005f)
			{
				DrawSkewedRect(PX, PlateY, PlateW * LocalFill, PlateH, 4.0f * HUDScale,
					FLinearColor(0.12f, 0.48f, 0.98f, 0.92f), FLinearColor(0.28f, 0.78f, 1.0f, 0.96f), 2);
				// Bright top specular edge
				DrawLine(PX + 4.0f * HUDScale, PlateY, PX + 4.0f * HUDScale + PlateW * LocalFill, PlateY, 1.5f * HUDScale, FLinearColor(0.65f, 0.92f, 1.0f, 0.9f), 3);
			}

			// Crisp subtle plate outline
			DrawLine(PX + 4.0f * HUDScale, PlateY, PX + 4.0f * HUDScale + PlateW, PlateY, 1.1f * HUDScale, FLinearColor(0.2f, 0.6f, 1.0f, 0.35f), 2);
			DrawLine(PX, PlateY + PlateH, PX + PlateW, PlateY + PlateH, 1.1f * HUDScale, FLinearColor(0.2f, 0.6f, 1.0f, 0.25f), 2);
		}

		// 3.2 Sleek Skewed Health Bar with Trailing Damage Ghost & Live ECG Waveform
		const float HealthPct = OwningHUD->MaxHealth > 0.0f ? FMath::Clamp(OwningHUD->CurrentHealth / OwningHUD->MaxHealth, 0.0f, 1.0f) : 0.0f;
		const float HealthGhostPct = OwningHUD->MaxHealth > 0.0f ? FMath::Clamp(OwningHUD->DisplayedHealthGhost / OwningHUD->MaxHealth, 0.0f, 1.0f) : 0.0f;
		const float HBarX = VitX;
		const float HBarY = VitY + 34.0f * HUDScale;
		const float HBarW = 211.0f * HUDScale;
		const float HBarH = 12.0f * HUDScale;

		const FLinearColor HealthMainColor = (HealthPct < 0.30f)
			? CrimsonAlert
			: ((HealthPct < 0.60f) ? AmberWarn : TealPrimary);
		const FLinearColor HealthBrightColor = (HealthPct < 0.30f)
			? FLinearColor(1.0f, 0.45f, 0.45f, 1.0f)
			: ((HealthPct < 0.60f) ? FLinearColor(1.0f, 0.90f, 0.4f, 1.0f) : FLinearColor(0.65f, 1.0f, 0.98f, 1.0f));

		// Translucent Health Track
		DrawSkewedRect(HBarX, HBarY, HBarW, HBarH, 5.0f * HUDScale,
			FLinearColor(0.02f, 0.06f, 0.09f, 0.38f), FLinearColor(0.02f, 0.06f, 0.09f, 0.22f), 1);

		// Trailing Damage Ghost Bar
		if (HealthGhostPct > HealthPct + 0.005f)
		{
			DrawSkewedRect(HBarX, HBarY, HBarW * HealthGhostPct, HBarH, 5.0f * HUDScale,
				FLinearColor(1.0f, 0.25f, 0.25f, 0.65f), FLinearColor(1.0f, 0.85f, 0.85f, 0.80f), 1);
		}

		// Primary Health Fill
		if (HealthPct > 0.005f)
		{
			DrawSkewedRect(HBarX, HBarY, HBarW * HealthPct, HBarH, 5.0f * HUDScale,
				HealthMainColor * FLinearColor(0.85f, 0.85f, 0.85f, 0.92f), HealthBrightColor, 2);
			DrawLine(HBarX + 5.0f * HUDScale, HBarY, HBarX + 5.0f * HUDScale + HBarW * HealthPct, HBarY, 1.6f * HUDScale, HudWhite, 3);
		}

		// 20-HP Micro-Subdivision Notches
		for (int32 n = 1; n < 5; ++n)
		{
			const float NX = HBarX + (HBarW * 0.2f * n);
			DrawLine(NX + 5.0f * HUDScale, HBarY, NX, HBarY + HBarH, 1.3f * HUDScale, FLinearColor(0.0f, 0.04f, 0.07f, 0.55f), 3);
		}

		// Oversized Floating Numeric Health Readout + Medical Cross on Right of Bars
		const float NumHPX = VitX + 222.0f * HUDScale;
		const float NumHPY = VitY + 18.0f * HUDScale;
		// Mini glowing medical cross icon '+'
		DrawLine(NumHPX, NumHPY + 5.0f * HUDScale, NumHPX + 9.0f * HUDScale, NumHPY + 5.0f * HUDScale, 2.2f * HUDScale, HealthMainColor, 3);
		DrawLine(NumHPX + 4.5f * HUDScale, NumHPY + 1.0f * HUDScale, NumHPX + 4.5f * HUDScale, NumHPY + 9.5f * HUDScale, 2.2f * HUDScale, HealthMainColor, 3);
		DrawGlowText(NumHPX + 13.0f * HUDScale, NumHPY - 3.0f * HUDScale, TEXT("BPM // BIO"), FontMicro, FLinearColor(0.7f, 0.88f, 0.95f, 0.65f), 0.6f);
		DrawGlowText(NumHPX, NumHPY + 9.0f * HUDScale, FString::Printf(TEXT("%03.0f"), OwningHUD->CurrentHealth), FontHealth, HealthBrightColor, 0.85f);

		// Live Animated ECG Heartbeat Waveform under HP Number
		{
			TArray<FVector2f> EcgPts;
			const int32 NumEcg = 18;
			EcgPts.Reserve(NumEcg);
			const float EcgStartX = NumHPX + 2.0f * HUDScale;
			const float EcgBaseY = VitY + 62.0f * HUDScale;
			const float EcgSpeed = (HealthPct < 0.4f || OwningHUD->bIsStaminaExhausted) ? 9.0f : 4.5f;
			for (int32 i = 0; i < NumEcg; ++i)
			{
				const float NormI = (float)i / (float)(NumEcg - 1);
				const float Phase = FMath::Fmod(NormI * 2.0f - TimeSec * EcgSpeed + 100.0f, 1.0f);
				float Deflection = 0.0f;
				if (Phase > 0.42f && Phase < 0.48f) Deflection = -8.0f * HUDScale;
				else if (Phase >= 0.48f && Phase < 0.54f) Deflection = 6.0f * HUDScale;
				else if (Phase >= 0.54f && Phase < 0.60f) Deflection = -2.5f * HUDScale;
				EcgPts.Add(FVector2f(EcgStartX + NormI * 58.0f * HUDScale, EcgBaseY + Deflection));
			}
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 3, AllottedGeometry.ToPaintGeometry(), MoveTemp(EcgPts), ESlateDrawEffect::None, HealthMainColor * FLinearColor(1, 1, 1, 0.75f), true, 1.4f * HUDScale);
		}

		// 3.3 12-Cell Skewed Stamina Kinetic Meter
		const float StaminaPct = OwningHUD->MaxStamina > 0.0f ? FMath::Clamp(OwningHUD->CurrentStamina / OwningHUD->MaxStamina, 0.0f, 1.0f) : 0.0f;
		const int32 StaminaCells = 12;
		const float SCellW = 14.5f * HUDScale;
		const float SCellGap = 3.2f * HUDScale;
		const float SCellY = VitY + 56.0f * HUDScale;
		const float SCellH = 6.5f * HUDScale;

		const FLinearColor StaminaCol = OwningHUD->bIsStaminaExhausted
			? CrimsonAlert
			: ((StaminaPct < 0.30f) ? AmberWarn : EmeraldStamina);

		for (int32 c = 0; c < StaminaCells; ++c)
		{
			const float CXPos = VitX + c * (SCellW + SCellGap);
			const float CellThreshold = (float)(c + 1) / (float)StaminaCells;
			const bool bCellActive = StaminaPct >= (CellThreshold - 0.04f);

			DrawSkewedRect(CXPos, SCellY, SCellW, SCellH, 3.0f * HUDScale,
				bCellActive ? StaminaCol : FLinearColor(0.02f, 0.07f, 0.10f, 0.28f),
				bCellActive ? (StaminaCol * 1.2f) : FLinearColor(0.02f, 0.07f, 0.10f, 0.20f), 2);
		}

		const FString StaminaStatus = OwningHUD->bIsStaminaExhausted
			? TEXT("SPRINT LOCKED // EXHAUSTED")
			: FString::Printf(TEXT("KINETIC STAMINA // %03.0f%%"), StaminaPct * 100.0f);
		DrawGlowText(VitX, VitY + 68.0f * HUDScale, StaminaStatus, FontMicro,
			OwningHUD->bIsStaminaExhausted ? CrimsonAlert : FLinearColor(0.65f, 0.92f, 0.85f, 0.72f), 0.6f);

		// 3.4 "FERMI DIVE // CHRONO FIELD" Telemetry Bar (Below Stamina in Vitals Panel)
		{
			const float DiveBarY = VitY + 86.0f * HUDScale;
			const float DiveBarW = 248.0f * HUDScale;
			const float DiveBarH = 14.0f * HUDScale;

			DrawSkewedRect(VitX - 6.0f * HUDScale, DiveBarY - 2.0f * HUDScale, DiveBarW + 12.0f * HUDScale, DiveBarH + 4.0f * HUDScale, 4.0f * HUDScale,
				FLinearColor(0.0f, 0.10f, 0.16f, 0.36f), FLinearColor(0.0f, 0.04f, 0.08f, 0.05f), 0);

			if (OwningHUD->bIsFermiTimeBubbleActive || OwningHUD->bIsBallisticDiving)
			{
				const float BubbleRatio = FMath::Clamp(OwningHUD->FermiBubbleTimeRemaining / 2.4f, 0.05f, 1.0f);
				const float Pulse = 0.78f + 0.22f * FMath::Sin(TimeSec * 16.0f);
				DrawSkewedRect(VitX, DiveBarY, DiveBarW * BubbleRatio, DiveBarH, 4.0f * HUDScale,
					TealPrimary * FLinearColor(1, 1, 1, 0.55f * Pulse), CyanTactical * FLinearColor(1, 1, 1, 0.85f * Pulse), 1);
				DrawLine(VitX + 4.0f * HUDScale, DiveBarY, VitX + 4.0f * HUDScale + DiveBarW * BubbleRatio, DiveBarY, 1.8f * HUDScale, HudWhite, 2);

				DrawGlowText(VitX + 6.0f * HUDScale, DiveBarY + 1.0f * HUDScale,
					FString::Printf(TEXT("FERMI DIVE // [ACTIVE // 0.25x TIME DILATION %.1fs]"), OwningHUD->FermiBubbleTimeRemaining),
					FontMicro, HudWhite, 0.85f, 4);
			}
			else if (OwningHUD->FermiDiveCooldownRemaining > 0.01f)
			{
				const float CooldownRatio = 1.0f - FMath::Clamp(OwningHUD->FermiDiveCooldownRemaining / 1.2f, 0.0f, 1.0f);
				DrawSkewedRect(VitX, DiveBarY, DiveBarW * CooldownRatio, DiveBarH, 4.0f * HUDScale,
					AmberWarn * FLinearColor(1, 1, 1, 0.30f), AmberWarn * FLinearColor(1, 1, 1, 0.50f), 1);
				DrawGlowText(VitX + 6.0f * HUDScale, DiveBarY + 1.0f * HUDScale,
					FString::Printf(TEXT("FERMI DIVE // [RECHARGING %.1fs]"), OwningHUD->FermiDiveCooldownRemaining),
					FontMicro, AmberWarn, 0.75f, 4);
			}
			else if (OwningHUD->bCanBallisticDive)
			{
				DrawSkewedRect(VitX, DiveBarY, DiveBarW, DiveBarH, 4.0f * HUDScale,
					FLinearColor(0.0f, 0.45f, 0.48f, 0.35f), FLinearColor(0.0f, 0.75f, 0.72f, 0.22f), 1);
				DrawLine(VitX + 4.0f * HUDScale, DiveBarY, VitX + 4.0f * HUDScale + DiveBarW, DiveBarY, 1.4f * HUDScale, TealPrimary * FLinearColor(1, 1, 1, 0.8f), 2);
				DrawGlowText(VitX + 6.0f * HUDScale, DiveBarY + 1.0f * HUDScale,
					TEXT("FERMI DIVE // [READY - ALT / DIVE]"),
					FontMicro, TealPrimary, 0.80f, 4);
			}
			else
			{
				DrawGlowText(VitX + 6.0f * HUDScale, DiveBarY + 1.0f * HUDScale,
					TEXT("FERMI DIVE // [LOW STAMINA < 25%]"),
					FontMicro, CrimsonAlert, 0.75f, 4);
			}
		}

		// =========================================================================
		// 4. BOTTOM-RIGHT FLOATING WEAPON SCHEMATIC & 30-ROUND PIP MATRIX
		// =========================================================================
		const float AmmoW = 315.0f * HUDScale;
		const float AmmoX = ScreenW - AmmoW - (32.0f * HUDScale) + VX * 0.64f;
		const float AmmoY = ScreenH - 138.0f * HUDScale + VY * 0.64f;

		// Right-to-Left Translucent Glass Gradient Strip (0.0 alpha on left -> 0.28 on right)
		DrawSkewedRect(AmmoX, AmmoY - 8.0f * HUDScale, AmmoW + 12.0f * HUDScale, 95.0f * HUDScale, -BarSkew,
			FLinearColor(0.0f, 0.04f, 0.07f, 0.0f),
			FLinearColor(0.0f, 0.09f, 0.14f, 0.28f), 0);

		// Glowing Right Spine Accent Bracket
		const float RightSpineX = AmmoX + AmmoW + 12.0f * HUDScale;
		DrawLine(RightSpineX - BarSkew, AmmoY - 8.0f * HUDScale, RightSpineX, AmmoY + 86.0f * HUDScale, 2.8f * HUDScale, TealPrimary, 2);
		DrawLine(RightSpineX - BarSkew - 28.0f * HUDScale, AmmoY - 8.0f * HUDScale, RightSpineX - BarSkew, AmmoY - 8.0f * HUDScale, 1.8f * HUDScale, TealPrimary * FLinearColor(1, 1, 1, 0.7f), 2);
		DrawLine(RightSpineX - 20.0f * HUDScale, AmmoY + 86.0f * HUDScale, RightSpineX, AmmoY + 86.0f * HUDScale, 1.8f * HUDScale, TealPrimary * FLinearColor(1, 1, 1, 0.7f), 2);

		// 4.1 Procedural Vector Carbine Silhouette (Top-Right of Weapon Panel)
		{
			const float GunX = AmmoX + 185.0f * HUDScale;
			const float GunY = AmmoY + 4.0f * HUDScale;
			const FLinearColor GunLineCol = OwningHUD->bIsADS ? TealPrimary : FLinearColor(0.55f, 0.88f, 0.98f, 0.72f);

			// Buttstock
			DrawQuad(FVector2f(GunX + 78.0f * HUDScale, GunY + 4.0f * HUDScale), FVector2f(GunX + 104.0f * HUDScale, GunY + 3.0f * HUDScale),
			         FVector2f(GunX + 104.0f * HUDScale, GunY + 15.0f * HUDScale), FVector2f(GunX + 82.0f * HUDScale, GunY + 10.0f * HUDScale),
			         GunLineCol * 0.5f, GunLineCol * 0.5f, GunLineCol * 0.5f, GunLineCol * 0.5f, 2);
			// Upper & Lower Receiver
			DrawSkewedRect(GunX + 38.0f * HUDScale, GunY + 3.0f * HUDScale, 40.0f * HUDScale, 9.0f * HUDScale, -2.0f * HUDScale, GunLineCol * 0.75f, GunLineCol * 0.65f, 2);
			// M-LOK Handguard
			DrawSkewedRect(GunX + 12.0f * HUDScale, GunY + 4.0f * HUDScale, 26.0f * HUDScale, 6.0f * HUDScale, -1.0f * HUDScale, GunLineCol * 0.65f, GunLineCol * 0.75f, 2);
			// Rifled Barrel & Compensator
			DrawLine(GunX - 8.0f * HUDScale, GunY + 6.5f * HUDScale, GunX + 12.0f * HUDScale, GunY + 6.5f * HUDScale, 2.4f * HUDScale, GunLineCol, 3);
			DrawLine(GunX - 11.0f * HUDScale, GunY + 5.5f * HUDScale, GunX - 6.0f * HUDScale, GunY + 5.5f * HUDScale, 3.2f * HUDScale, GunLineCol, 3);
			// Reflex Red-Dot Optic Hood
			DrawLine(GunX + 48.0f * HUDScale, GunY + 3.0f * HUDScale, GunX + 50.0f * HUDScale, GunY - 2.5f * HUDScale, 1.8f * HUDScale, TealPrimary, 3);
			DrawLine(GunX + 50.0f * HUDScale, GunY - 2.5f * HUDScale, GunX + 60.0f * HUDScale, GunY - 2.5f * HUDScale, 1.8f * HUDScale, TealPrimary, 3);
			DrawLine(GunX + 60.0f * HUDScale, GunY - 2.5f * HUDScale, GunX + 61.0f * HUDScale, GunY + 3.0f * HUDScale, 1.8f * HUDScale, TealPrimary, 3);
			// Swept Pistol Grip
			DrawLine(GunX + 68.0f * HUDScale, GunY + 11.0f * HUDScale, GunX + 73.0f * HUDScale, GunY + 22.0f * HUDScale, 3.4f * HUDScale, GunLineCol * 0.7f, 2);
			// Curved 30-Round STANAG Magazine
			DrawLine(GunX + 46.0f * HUDScale, GunY + 12.0f * HUDScale, GunX + 41.0f * HUDScale, GunY + 25.0f * HUDScale, 4.4f * HUDScale,
				OwningHUD->AmmoInClip <= 5 ? CrimsonAlert : (GunLineCol * 0.85f), 2);
		}

		// 4.2 Oversized Floating Digital Ammo Readout (Left Side of Weapon Panel)
		const bool bLowAmmo = (OwningHUD->AmmoInClip <= 5);
		const FLinearColor ClipTextColor = bLowAmmo
			? (FMath::Sin(TimeSec * 12.0f) > 0.0f ? CrimsonAlert : AmberWarn)
			: HudWhite;

		DrawGlowText(AmmoX + 14.0f * HUDScale, AmmoY - 4.0f * HUDScale, FString::Printf(TEXT("%02d"), OwningHUD->AmmoInClip), FontBigAmmo, ClipTextColor, 0.9f);
		DrawLine(AmmoX + 84.0f * HUDScale, AmmoY + 6.0f * HUDScale, AmmoX + 76.0f * HUDScale, AmmoY + 38.0f * HUDScale, 2.0f * HUDScale, TealPrimary * FLinearColor(1, 1, 1, 0.55f), 2);
		DrawGlowText(AmmoX + 88.0f * HUDScale, AmmoY + 4.0f * HUDScale, TEXT("RESERVE"), FontMicro, FLinearColor(0.6f, 0.82f, 0.92f, 0.65f), 0.5f);
		DrawGlowText(AmmoX + 88.0f * HUDScale, AmmoY + 15.0f * HUDScale, FString::Printf(TEXT("%03d"), OwningHUD->AmmoReserve), FontBold, FLinearColor(0.78f, 0.92f, 0.98f, 0.90f), 0.8f);

		// 4.3 30-Round Individual Bullet Pip Matrix
		const int32 MagCap = FMath::Clamp(OwningHUD->MagazineCapacity, 1, 40);
		const float PipStartX = AmmoX + 16.0f * HUDScale;
		const float PipY = AmmoY + 48.0f * HUDScale;
		const float TotalPipW = 280.0f * HUDScale;
		const float PipStep = TotalPipW / (float)MagCap;
		const float PipW = FMath::Max(2.8f * HUDScale, PipStep - 2.8f * HUDScale);
		const float PipH = 11.5f * HUDScale;

		for (int32 b = 0; b < MagCap; ++b)
		{
			const float BX = PipStartX + b * PipStep;
			const bool bLoaded = (b < OwningHUD->AmmoInClip);
			const bool bTracerRound = (b < 5); // First 5 rounds at bottom of mag are low-ammo tracers

			FLinearColor PipCol;
			if (!bLoaded)
			{
				PipCol = FLinearColor(0.05f, 0.12f, 0.18f, 0.25f);
			}
			else if (bTracerRound && OwningHUD->AmmoInClip <= 8)
			{
				PipCol = CrimsonAlert;
			}
			else if (bTracerRound)
			{
				PipCol = AmberWarn * FLinearColor(1, 1, 1, 0.92f);
			}
			else
			{
				PipCol = (b == OwningHUD->AmmoInClip - 1) ? HudWhite : TealPrimary * FLinearColor(1, 1, 1, 0.90f);
			}

			DrawSkewedRect(BX, PipY, PipW, PipH, -2.5f * HUDScale, PipCol, PipCol * 1.15f, 2);
		}

		// 4.4 Weapon Title, 3-Mode Fire Selector & Tactical Equipment Slots
		const FString WeaponTitle = OwningHUD->ActiveWeaponName.IsEmpty()
			? TEXT("AK-47 TACTICAL // 7.62x39MM")
			: OwningHUD->ActiveWeaponName.ToString().ToUpper();
		DrawGlowText(AmmoX + 16.0f * HUDScale, AmmoY + 65.0f * HUDScale, WeaponTitle, FontSmall, TealPrimary, 0.75f);

		// Fire Selector Pills: [ AUTO ]  BURST  SEMI
		const TCHAR* Modes[] = { TEXT("AUTO"), TEXT("BURST"), TEXT("SEMI") };
		for (int32 m = 0; m < 3; ++m)
		{
			const float MX = AmmoX + 188.0f * HUDScale + m * (38.0f * HUDScale);
			const float MY = AmmoY + 65.0f * HUDScale;
			const bool bActiveMode = (OwningHUD->ActiveFireModeIndex == m);
			if (bActiveMode)
			{
				DrawSkewedRect(MX - 4.0f * HUDScale, MY - 1.0f * HUDScale, 36.0f * HUDScale, 14.0f * HUDScale, -3.0f * HUDScale,
					FLinearColor(0.0f, 0.94f, 0.86f, 0.28f), FLinearColor(0.0f, 0.94f, 0.86f, 0.14f), 1);
				DrawLine(MX - 4.0f * HUDScale, MY + 13.0f * HUDScale, MX + 32.0f * HUDScale, MY + 13.0f * HUDScale, 1.6f * HUDScale, TealPrimary, 2);
			}
			DrawGlowText(MX, MY, Modes[m], FontMicro,
				bActiveMode ? HudWhite : FLinearColor(0.5f, 0.65f, 0.75f, 0.45f), bActiveMode ? 0.8f : 0.0f);
		}

		// Lethal & Fermi Echo / Quantum Swap Tactical Micro-Badges Above Weapon Panel
		FString EchoStatusStr;
		FLinearColor EchoCol = TealPrimary;
		if (OwningHUD->bIsFermiDecoyActive)
		{
			EchoStatusStr = FString::Printf(TEXT("[G] FRAG  //  [Q] QUANTUM SWAP READY (%.1fs)"), OwningHUD->FermiDecoyTimeRemaining);
			EchoCol = EmeraldStamina;
		}
		else if (OwningHUD->FermiDecoyCooldownRemaining > 0.05f)
		{
			EchoStatusStr = FString::Printf(TEXT("[G] FRAG  //  [Q] ECHO COOLDOWN (%.1fs)"), OwningHUD->FermiDecoyCooldownRemaining);
			EchoCol = AmberWarn;
		}
		else
		{
			EchoStatusStr = TEXT("[G] FRAG  //  [Q] DEPLOY FERMI ECHO");
			EchoCol = TealPrimary;
		}
		DrawGlowText(AmmoX + 82.0f * HUDScale, AmmoY - 22.0f * HUDScale, EchoStatusStr, FontMicro, EchoCol, 0.75f);

		// =========================================================================
		// 4.8 ACTIVE FERMI TIME-DILATION VIGNETTE & DIGITAL CHRONOMETER TICKER
		// =========================================================================
		if (OwningHUD->bIsFermiTimeBubbleActive || OwningHUD->bIsBallisticDiving)
		{
			const float Pulse = 0.65f + 0.35f * FMath::Sin(TimeSec * 14.0f);
			const FLinearColor EdgeCyan(0.0f, 0.92f, 0.86f, 0.16f * Pulse);
			const FLinearColor InnerClear(0.0f, 0.92f, 0.86f, 0.0f);
			const float VigThickness = 48.0f * HUDScale;

			// Top, Bottom, Left, Right subtle cyan peripheral vignette strips
			DrawQuad(FVector2f(0.0f, 0.0f), FVector2f(ScreenW, 0.0f), FVector2f(ScreenW, VigThickness), FVector2f(0.0f, VigThickness),
				EdgeCyan, EdgeCyan, InnerClear, InnerClear, 1);
			DrawQuad(FVector2f(0.0f, ScreenH - VigThickness), FVector2f(ScreenW, ScreenH - VigThickness), FVector2f(ScreenW, ScreenH), FVector2f(0.0f, ScreenH),
				InnerClear, InnerClear, EdgeCyan, EdgeCyan, 1);
			DrawQuad(FVector2f(0.0f, 0.0f), FVector2f(VigThickness, 0.0f), FVector2f(VigThickness, ScreenH), FVector2f(0.0f, ScreenH),
				EdgeCyan, InnerClear, InnerClear, EdgeCyan, 1);
			DrawQuad(FVector2f(ScreenW - VigThickness, 0.0f), FVector2f(ScreenW, 0.0f), FVector2f(ScreenW, ScreenH), FVector2f(ScreenW - VigThickness, ScreenH),
				InnerClear, EdgeCyan, EdgeCyan, InnerClear, 1);

			// Digital Chronometer Ticker below top compass tape
			const float ChronoX = ScreenW * 0.5f - 118.0f * HUDScale + VX * 0.35f;
			const float ChronoY = 64.0f * HUDScale + VY * 0.35f;
			DrawSkewedRect(ChronoX - 8.0f * HUDScale, ChronoY - 3.0f * HUDScale, 240.0f * HUDScale, 20.0f * HUDScale, -4.0f * HUDScale,
				FLinearColor(0.0f, 0.25f, 0.30f, 0.42f), FLinearColor(0.0f, 0.45f, 0.52f, 0.22f), 2);
			DrawLine(ChronoX - 8.0f * HUDScale, ChronoY + 17.0f * HUDScale, ChronoX + 232.0f * HUDScale, ChronoY + 17.0f * HUDScale, 1.8f * HUDScale, TealPrimary, 3);
			DrawGlowText(ChronoX, ChronoY,
				FString::Printf(TEXT("CHRONO FIELD // 0.25x c  [%.2fs]"), OwningHUD->FermiBubbleTimeRemaining),
				FontSmall, TealPrimary, 0.9f, 4);
		}

		// Quantum Phase-Swap Visor Flash
		if (OwningHUD->QuantumSwapFlashAlpha > 0.01f)
		{
			const FLinearColor SwapCol(0.0f, 0.98f, 0.88f, OwningHUD->QuantumSwapFlashAlpha * 0.22f);
			DrawQuad(FVector2f(0.0f, 0.0f), FVector2f(ScreenW, 0.0f), FVector2f(ScreenW, ScreenH), FVector2f(0.0f, ScreenH),
				SwapCol, SwapCol, SwapCol, SwapCol, 5);
			DrawGlowText(ScreenW * 0.5f - 96.0f, 90.0f, TEXT(">> QUANTUM PHASE-SWAP COMPLETE <<"), FontSmall, EmeraldStamina, 0.9f, 6);
		}

		// =========================================================================
		// 5. CENTER MULTI-LAYER RETICLE, RADIAL RELOAD RING & DIRECTIONAL THREAT RADAR
		// =========================================================================
		const float CX = ScreenW * 0.5f;
		const float CY = ScreenH * 0.5f;

		// 5.1 Dynamic Multi-Layer Precision Reticle (Persists with Precision Holographic ADS Center-Dot)
		const float CrosshairAlpha = 1.0f; // 100% visible in third-person OTS to guarantee instant target acquisition
		if (!OwningHUD->bIsReloading)
		{
			const float HipGap = FMath::Clamp(OwningHUD->CurrentSpreadRadius * HUDScale, 8.0f * HUDScale, 50.0f * HUDScale);
			const float AdsGap = 4.5f * HUDScale;
			const float Gap = FMath::Lerp(HipGap, AdsGap, OwningHUD->ADSAlpha);
			const float Len = FMath::Lerp(9.0f * HUDScale, 5.0f * HUDScale, OwningHUD->ADSAlpha);
			const float TBracket = FMath::Lerp(3.0f * HUDScale, 1.8f * HUDScale, OwningHUD->ADSAlpha);

			const FLinearColor DarkOutline(0.0f, 0.02f, 0.04f, 0.75f);
			const FLinearColor BrightCore = FMath::Lerp(FLinearColor(0.92f, 0.99f, 1.0f, 0.92f), FLinearColor(0.0f, 0.95f, 1.0f, 1.0f), OwningHUD->ADSAlpha);
			const FLinearColor TealTip(0.0f, 0.94f, 0.86f, 0.95f);

			// Dark Contrast Backing Lines
			DrawLine(CX, CY - Gap - Len - 1.0f, CX, CY - Gap + 1.0f, 3.5f * HUDScale, DarkOutline, 2);
			DrawLine(CX, CY + Gap - 1.0f, CX, CY + Gap + Len + 1.0f, 3.5f * HUDScale, DarkOutline, 2);
			DrawLine(CX - Gap - Len - 1.0f, CY, CX - Gap + 1.0f, CY, 3.5f * HUDScale, DarkOutline, 2);
			DrawLine(CX + Gap - 1.0f, CY, CX + Gap + Len + 1.0f, CY, 3.5f * HUDScale, DarkOutline, 2);

			// Crisp Bright Core Pips
			DrawLine(CX, CY - Gap - Len, CX, CY - Gap, 1.6f * HUDScale, BrightCore, 3);
			DrawLine(CX, CY + Gap, CX, CY + Gap + Len, 1.6f * HUDScale, BrightCore, 3);
			DrawLine(CX - Gap - Len, CY, CX - Gap, CY, 1.6f * HUDScale, BrightCore, 3);
			DrawLine(CX + Gap, CY, CX + Gap + Len, CY, 1.6f * HUDScale, BrightCore, 3);

			// Inner Tactical T-Brackets on Pip Tips
			DrawLine(CX - TBracket, CY - Gap, CX + TBracket, CY - Gap, 1.4f * HUDScale, TealTip, 3);
			DrawLine(CX - TBracket, CY + Gap, CX + TBracket, CY + Gap, 1.4f * HUDScale, TealTip, 3);
			DrawLine(CX - Gap, CY - TBracket, CX - Gap, CY + TBracket, 1.4f * HUDScale, TealTip, 3);
			DrawLine(CX + Gap, CY - TBracket, CX + Gap, CY + TBracket, 1.4f * HUDScale, TealTip, 3);

			// Precision Center Holographic Micro-Dot (Sharp 2.2px cyan dot in ADS)
			const float DotSize = FMath::Lerp(1.8f * HUDScale, 2.4f * HUDScale, OwningHUD->ADSAlpha);
			DrawDiamond(CX, CY, DotSize, DotSize * 0.55f, TealTip, true, 3);

			// ADS Precision Aim Ring
			if (OwningHUD->ADSAlpha > 0.05f)
			{
				DrawArc(CX, CY, 14.0f * HUDScale, 0.0f, 360.0f, 1.2f * HUDScale, FLinearColor(0.0f, 0.94f, 0.86f, 0.40f * OwningHUD->ADSAlpha), 32, 2);
			}
		}

		// 5.2 Center Radial 4-Phase Reload Progress Ring
		if (OwningHUD->bIsReloading)
		{
			const float RRadius = 26.0f * HUDScale;
			const float Prog = FMath::Clamp(OwningHUD->ReloadProgress, 0.0f, 1.0f);

			// Background Ring Track
			DrawArc(CX, CY, RRadius, 0.0f, 360.0f, 2.4f * HUDScale, FLinearColor(0.0f, 0.12f, 0.18f, 0.45f), 36, 2);
			// Active Sweeping Progress Arc (-90 deg top clockwise)
			DrawArc(CX, CY, RRadius, -90.0f, -90.0f + Prog * 360.0f, 3.2f * HUDScale, AmberWarn, 36, 3);

			// 4-Phase Stage Label
			const TCHAR* PhaseStr = (Prog < 0.20f) ? TEXT("PHASE 1 // CANT")
			                      : (Prog < 0.42f) ? TEXT("PHASE 2 // MAG DROP")
			                      : (Prog < 0.76f) ? TEXT("PHASE 3 // MAG LOCK")
			                                       : TEXT("PHASE 4 // BOLT SNAP");
			DrawGlowText(CX - 52.0f * HUDScale, CY + RRadius + 9.0f * HUDScale, PhaseStr, FontMicro, AmberWarn, 0.85f);
		}
		else if (OwningHUD->AmmoInClip <= 5)
		{
			const float PulseAlpha = 0.55f + 0.40f * FMath::Sin(TimeSec * 10.0f);
			DrawGlowText(CX - 42.0f * HUDScale, CY + 38.0f * HUDScale,
				OwningHUD->AmmoInClip == 0 ? TEXT("[ R ] EMPTY // RELOAD") : TEXT("[ R ] LOW AMMO"),
				FontMicro, FLinearColor(1.0f, 0.35f, 0.15f, PulseAlpha), 0.8f);
		}

		// 5.3 Animated Kinetic Hitmarker
		// Differentiated Visual Feedback:
		// - White X: Standard limb/flesh hit
		// - Cobalt Blue X + Shield Icon: Hit absorbed by enemy Level IV Ceramic Armor Plate
		// - Crimson Red Double-X (Expanding): Critical Headshot / Confirmed Elimination
		if (OwningHUD->HitmarkerTimer > 0.0f)
		{
			const float NormHit = FMath::Clamp(OwningHUD->HitmarkerTimer / 0.25f, 0.0f, 1.0f);
			const bool bKill = (OwningHUD->CurrentHitmarker == EFPSHitmarkerType::Kill);
			const bool bHeadshot = (OwningHUD->CurrentHitmarker == EFPSHitmarkerType::Headshot);
			const bool bShield = (OwningHUD->CurrentHitmarker == EFPSHitmarkerType::ShieldBreak);

			const float Expand = (1.0f - NormHit) * (bKill ? 12.0f : (bHeadshot ? 8.0f : 5.0f)) * HUDScale;
			const float InnerR = (5.0f + Expand * 0.4f) * HUDScale;
			const float OuterR = (13.0f + Expand) * HUDScale;

			FLinearColor HitCol = FLinearColor(0.96f, 0.98f, 1.0f, NormHit); // Standard White X
			if (bShield)
			{
				HitCol = FLinearColor(0.0f, 0.65f, 1.0f, NormHit); // Cobalt Blue #00A8FF
			}
			else if (bHeadshot || bKill)
			{
				HitCol = FLinearColor(1.0f, 0.12f, 0.20f, NormHit); // Crimson Red #FF2A54
			}

			// Dark Shadow Underlay
			const FLinearColor HitShadow(0.0f, 0.0f, 0.0f, NormHit * 0.65f);
			DrawLine(CX - OuterR, CY - OuterR, CX - InnerR, CY - InnerR, 3.8f * HUDScale, HitShadow, 3);
			DrawLine(CX + OuterR, CY - OuterR, CX + InnerR, CY - InnerR, 3.8f * HUDScale, HitShadow, 3);
			DrawLine(CX - OuterR, CY + OuterR, CX - InnerR, CY + InnerR, 3.8f * HUDScale, HitShadow, 3);
			DrawLine(CX + OuterR, CY + OuterR, CX + InnerR, CY + InnerR, 3.8f * HUDScale, HitShadow, 3);

			// Primary Hitmarker Cross
			DrawLine(CX - OuterR, CY - OuterR, CX - InnerR, CY - InnerR, 2.2f * HUDScale, HitCol, 4);
			DrawLine(CX + OuterR, CY - OuterR, CX + InnerR, CY - InnerR, 2.2f * HUDScale, HitCol, 4);
			DrawLine(CX - OuterR, CY + OuterR, CX - InnerR, CY + InnerR, 2.2f * HUDScale, HitCol, 4);
			DrawLine(CX + OuterR, CY + OuterR, CX + InnerR, CY + InnerR, 2.2f * HUDScale, HitCol, 4);

			// Cobalt Blue Shield Icon: 4-Corner Level IV Armor Plate Brackets & Outline
			if (bShield)
			{
				const float ShR = OuterR + 5.0f * HUDScale;
				const float ShW = 7.0f * HUDScale;
				DrawLine(CX - ShR, CY - ShR, CX - ShR + ShW, CY - ShR, 2.2f * HUDScale, HitCol, 4);
				DrawLine(CX - ShR, CY - ShR, CX - ShR, CY - ShR + ShW, 2.2f * HUDScale, HitCol, 4);
				DrawLine(CX + ShR, CY - ShR, CX + ShR - ShW, CY - ShR, 2.2f * HUDScale, HitCol, 4);
				DrawLine(CX + ShR, CY - ShR, CX + ShR, CY - ShR + ShW, 2.2f * HUDScale, HitCol, 4);
				DrawLine(CX - ShR, CY + ShR, CX - ShR + ShW, CY + ShR, 2.2f * HUDScale, HitCol, 4);
				DrawLine(CX - ShR, CY + ShR, CX - ShR, CY + ShR - ShW, 2.2f * HUDScale, HitCol, 4);
				DrawLine(CX + ShR, CY + ShR, CX + ShR - ShW, CY + ShR, 2.2f * HUDScale, HitCol, 4);
				DrawLine(CX + ShR, CY + ShR, CX + ShR, CY + ShR - ShW, 2.2f * HUDScale, HitCol, 4);

				DrawDiamond(CX, CY, ShR + 2.0f * HUDScale, 1.4f * HUDScale, HitCol * FLinearColor(1, 1, 1, 0.75f), false, 4);
			}

			// Crimson Red Double-X (Expanding): Critical Headshot / Confirmed Elimination
			if (bHeadshot || bKill)
			{
				const float OuterX_Inner = OuterR + 3.0f * HUDScale;
				const float OuterX_Outer = OuterR + (8.0f + Expand * 0.5f) * HUDScale;
				DrawLine(CX - OuterX_Outer, CY - OuterX_Outer, CX - OuterX_Inner, CY - OuterX_Inner, 2.4f * HUDScale, HitCol, 4);
				DrawLine(CX + OuterX_Outer, CY - OuterX_Outer, CX + OuterX_Inner, CY - OuterX_Inner, 2.4f * HUDScale, HitCol, 4);
				DrawLine(CX - OuterX_Outer, CY + OuterX_Outer, CX - OuterX_Inner, CY + OuterX_Inner, 2.4f * HUDScale, HitCol, 4);
				DrawLine(CX + OuterX_Outer, CY + OuterX_Outer, CX + OuterX_Inner, CY + OuterX_Inner, 2.4f * HUDScale, HitCol, 4);

				DrawDiamond(CX, CY, OuterR * 0.95f, 2.0f * HUDScale, FLinearColor(1.0f, 0.45f, 0.15f, NormHit * 0.90f), false, 4);
			}
		}

		// 5.4 Killstreak Combo Counter (2x DOUBLE, 3x TRIPLE, UNSTOPPABLE)
		if (OwningHUD->CurrentComboCount >= 2 && OwningHUD->ComboDecayTimer > 0.0f)
		{
			const float ComboRatio = FMath::Clamp(OwningHUD->ComboDecayTimer / FMath::Max(0.1f, OwningHUD->MaxComboWindow), 0.0f, 1.0f);
			const float BarW = 150.0f * HUDScale;
			const float BarH = 20.0f * HUDScale;
			const float ComboX = CX - (BarW * 0.5f);
			const float ComboY = CY + 60.0f * HUDScale;

			FString ComboStr;
			FLinearColor ComboCol;
			if (OwningHUD->CurrentComboCount == 2)
			{
				ComboStr = TEXT("2x DOUBLE");
				ComboCol = AmberWarn;
			}
			else if (OwningHUD->CurrentComboCount == 3)
			{
				ComboStr = TEXT("3x TRIPLE");
				ComboCol = FLinearColor(1.0f, 0.45f, 0.0f, 1.0f);
			}
			else if (OwningHUD->CurrentComboCount == 4)
			{
				ComboStr = TEXT("4x QUAD FEED");
				ComboCol = CrimsonAlert;
			}
			else
			{
				ComboStr = FString::Printf(TEXT("👑 %dx UNSTOPPABLE"), OwningHUD->CurrentComboCount);
				ComboCol = FLinearColor(1.0f, 0.15f, 0.85f, 1.0f);
			}

			// Background Glass Panel
			DrawSkewedRect(ComboX, ComboY, BarW, BarH, -4.0f * HUDScale,
				FLinearColor(0.02f, 0.04f, 0.07f, 0.70f),
				FLinearColor(0.04f, 0.08f, 0.14f, 0.80f), 2);

			// Decaying Combo Timer Meter Bar
			const float MeterW = BarW * ComboRatio;
			if (MeterW > 2.0f)
			{
				DrawLine(ComboX, ComboY + BarH - 2.0f * HUDScale, ComboX + MeterW, ComboY + BarH - 2.0f * HUDScale, 2.4f * HUDScale, ComboCol, 3);
			}

			// Combo Title Text
			DrawGlowText(ComboX + 16.0f * HUDScale, ComboY + 2.0f * HUDScale, ComboStr, FontMicro, ComboCol, 0.85f, 4);
		}

		// 5.5 Tactical Style & Combat Accolade Ticker
		if (OwningHUD->ActiveAccolades.Num() > 0)
		{
			const float AccBaseY = CY + 84.0f * HUDScale + (OwningHUD->CurrentComboCount >= 2 ? 22.0f * HUDScale : 0.0f);
			const int32 NumAcc = FMath::Min(OwningHUD->ActiveAccolades.Num(), 3);
			for (int32 a = 0; a < NumAcc; ++a)
			{
				const FFPSCombatAccolade& Acc = OwningHUD->ActiveAccolades[OwningHUD->ActiveAccolades.Num() - 1 - a];
				const float Elapsed = Acc.InitialLifetime - Acc.LifetimeRemaining;
				// Pop-in scale interpolation
				const float PopScale = (Elapsed < 0.12f)
					? FMath::Lerp(0.40f, 1.08f, Elapsed / 0.12f)
					: (Elapsed < 0.22f ? FMath::Lerp(1.08f, 1.0f, (Elapsed - 0.12f) / 0.10f) : 1.0f);
				const float FadeAlpha = FMath::Clamp(Acc.LifetimeRemaining / 0.35f, 0.0f, 1.0f);
				const float AccY = AccBaseY + (a * 22.0f * HUDScale);

				const float EstimatedTextW = (Acc.Title.Len() * 6.8f + 24.0f) * HUDScale;
				const float PillW = FMath::Max(180.0f * HUDScale, EstimatedTextW) * PopScale;
				const float PillX = CX - (PillW * 0.5f);

				// Tactical Glass Pill & Accent Underline
				DrawLine(PillX, AccY + 14.0f * HUDScale, PillX + PillW, AccY + 14.0f * HUDScale, 1.4f * HUDScale, Acc.AccentColor * FLinearColor(1, 1, 1, FadeAlpha * 0.65f), 3);
				DrawLine(PillX, AccY + 4.0f * HUDScale, PillX + 8.0f * HUDScale, AccY + 14.0f * HUDScale, 1.4f * HUDScale, Acc.AccentColor * FLinearColor(1, 1, 1, FadeAlpha * 0.85f), 3);
				DrawLine(PillX + PillW, AccY + 4.0f * HUDScale, PillX + PillW - 8.0f * HUDScale, AccY + 14.0f * HUDScale, 1.4f * HUDScale, Acc.AccentColor * FLinearColor(1, 1, 1, FadeAlpha * 0.85f), 3);

				// Accolade Text
				DrawGlowText(PillX + 12.0f * HUDScale, AccY, Acc.Title, FontMicro, Acc.AccentColor * FLinearColor(1, 1, 1, FadeAlpha), 0.85f, 4);
			}
		}

		// 5.4 Legacy 3D Directional Damage Indicator Arc
		if (OwningHUD->DamageIndicatorTimer > 0.0f)
		{
			const float DmgAlpha = FMath::Clamp(OwningHUD->DamageIndicatorTimer / 1.2f, 0.0f, 1.0f);
			const float CenterDeg = OwningHUD->DamageIndicatorAngle - 90.0f;
			const float DmgRadius = 85.0f * HUDScale;
			DrawArc(CX, CY, DmgRadius, CenterDeg - 26.0f, CenterDeg + 26.0f, 4.4f * HUDScale, FLinearColor(1.0f, 0.10f, 0.18f, DmgAlpha * 0.85f), 18, 3);
			DrawArc(CX, CY, DmgRadius + 4.0f * HUDScale, CenterDeg - 14.0f, CenterDeg + 14.0f, 2.2f * HUDScale, FLinearColor(1.0f, 0.55f, 0.55f, DmgAlpha * 0.95f), 12, 4);
		}

		// 5.6 Center-Screen Directional Hit & Visual Audio Semicircle + Chevron Radar (Deaf Accessibility)
		if (OwningHUD->bEnableVisualAudioRadar)
		{
			const float ThreatRadius = 78.0f * HUDScale;

			for (const FFPSThreatIndicator& Ind : OwningHUD->ActiveThreatIndicators)
		{
			const float LifeRatio = FMath::Clamp(Ind.LifetimeRemaining / FMath::Max(0.01f, Ind.InitialLifetime), 0.0f, 1.0f);
			if (LifeRatio <= 0.001f) continue;

			FLinearColor BaseColor;
			switch (Ind.Type)
			{
			case EThreatIndicatorType::Damage:
				BaseColor = CrimsonAlert; // Crimson #FF2A54
				break;
			case EThreatIndicatorType::Gunfire:
				BaseColor = AmberWarn;    // Amber #FFB300
				break;
			case EThreatIndicatorType::Footstep:
			default:
				BaseColor = CyanTactical; // Cyan #00E5FF
				break;
			}

			const float ThetaDeg = Ind.RelativeAngleDeg;
			const float HalfSpanDeg = (Ind.Type == EThreatIndicatorType::Damage) ? 52.0f : 44.0f;
			const int32 ArcSteps = 20;
			const float ArcThickness = (Ind.Type == EThreatIndicatorType::Damage ? 4.4f : 3.4f) * HUDScale * FMath::Clamp(Ind.Intensity, 0.8f, 1.8f);

			// Draw multi-segment smooth curved arc with cosine alpha tapering toward ends
			for (int32 s = 0; s < ArcSteps; ++s)
			{
				const float StepT0 = (float)s / (float)ArcSteps;
				const float StepT1 = (float)(s + 1) / (float)ArcSteps;

				const float Deg0 = ThetaDeg + FMath::Lerp(-HalfSpanDeg, HalfSpanDeg, StepT0);
				const float Deg1 = ThetaDeg + FMath::Lerp(-HalfSpanDeg, HalfSpanDeg, StepT1);

				const float Rad0 = FMath::DegreesToRadians(Deg0);
				const float Rad1 = FMath::DegreesToRadians(Deg1);

				// Screen coordinates: 0° is straight ahead (top: Y = -R, X = 0), +90° is right (X = +R, Y = 0)
				const float X0 = CX + ThreatRadius * FMath::Sin(Rad0);
				const float Y0 = CY - ThreatRadius * FMath::Cos(Rad0);
				const float X1 = CX + ThreatRadius * FMath::Sin(Rad1);
				const float Y1 = CY - ThreatRadius * FMath::Cos(Rad1);

				// Cosine alpha taper: 1.0 at center, 0.0 at arc tips
				const float CenterNorm0 = (StepT0 - 0.5f) * 2.0f; // [-1, 1]
				const float CenterNorm1 = (StepT1 - 0.5f) * 2.0f;
				const float Taper0 = FMath::Max(0.0f, FMath::Cos(CenterNorm0 * PI * 0.5f));
				const float Taper1 = FMath::Max(0.0f, FMath::Cos(CenterNorm1 * PI * 0.5f));

				const float Alpha0 = LifeRatio * Taper0 * 0.95f;
				const float Alpha1 = LifeRatio * Taper1 * 0.95f;

				// Dark contrast drop-shadow underlay
				DrawGradientLine(X0, Y0, X1, Y1, ArcThickness + 1.8f * HUDScale,
					FLinearColor(0.0f, 0.01f, 0.02f, Alpha0 * 0.65f),
					FLinearColor(0.0f, 0.01f, 0.02f, Alpha1 * 0.65f), 3);

				// Glowing core arc
				DrawGradientLine(X0, Y0, X1, Y1, ArcThickness,
					BaseColor * FLinearColor(1, 1, 1, Alpha0),
					BaseColor * FLinearColor(1, 1, 1, Alpha1), 4);
			}

			// Draw sharp outward-pointing chevron/arrow just outside the peak of the semicircle
			{
				const float RadPeak = FMath::DegreesToRadians(ThetaDeg);
				const float SinP = FMath::Sin(RadPeak);
				const float CosP = FMath::Cos(RadPeak);

				const float ChevDist = ThreatRadius + 8.0f * HUDScale;
				const float ChevLen = 14.0f * HUDScale;
				const float ChevSpread = 10.0f * HUDScale;

				// Peak tip pointing outward
				const float TipX = CX + (ChevDist + ChevLen) * SinP;
				const float TipY = CY - (ChevDist + ChevLen) * CosP;

				// Base center
				const float BaseX = CX + ChevDist * SinP;
				const float BaseY = CY - ChevDist * CosP;

				// Perpendicular unit vector in screen coordinates: (cos(theta), sin(theta))
				const float PerpX = CosP;
				const float PerpY = SinP;

				const float LeftX = BaseX - PerpX * ChevSpread;
				const float LeftY = BaseY - PerpY * ChevSpread;
				const float RightX = BaseX + PerpX * ChevSpread;
				const float RightY = BaseY + PerpY * ChevSpread;

				const float ChevAlpha = LifeRatio * 0.95f;
				const FLinearColor ChevColor = BaseColor * FLinearColor(1, 1, 1, ChevAlpha);
				const FLinearColor ChevShadow(0.0f, 0.01f, 0.02f, ChevAlpha * 0.65f);
				const float ChevThick = 2.4f * HUDScale;

				// Drop-shadow chevron
				DrawLine(LeftX, LeftY, TipX, TipY, ChevThick + 1.6f * HUDScale, ChevShadow, 3);
				DrawLine(RightX, RightY, TipX, TipY, ChevThick + 1.6f * HUDScale, ChevShadow, 3);

				// Colored chevron
				DrawLine(LeftX, LeftY, TipX, TipY, ChevThick, ChevColor, 5);
				DrawLine(RightX, RightY, TipX, TipY, ChevThick, ChevColor, 5);

				// Inner notch accent
				const float NotchX = BaseX + SinP * (ChevLen * 0.35f);
				const float NotchY = BaseY - CosP * (ChevLen * 0.35f);
				DrawLine(LeftX, LeftY, NotchX, NotchY, ChevThick * 0.8f, ChevColor * FLinearColor(1, 1, 1, 0.75f), 5);
				DrawLine(RightX, RightY, NotchX, NotchY, ChevThick * 0.8f, ChevColor * FLinearColor(1, 1, 1, 0.75f), 5);
			}
		}
		}

		// 5.7 3D-Projected "[ ! BAITED ]" Diamond Indicator Above Baited Enemies
		for (const FFPSBaitedTargetMarker& Marker : OwningHUD->ActiveBaitedMarkers)
		{
			const float MX = FMath::Clamp(Marker.ScreenPos.X, 60.0f * HUDScale, ScreenW - 60.0f * HUDScale);
			const float MY = FMath::Clamp(Marker.ScreenPos.Y, 55.0f * HUDScale, ScreenH - 80.0f * HUDScale);
			const float Pulse = 0.82f + 0.18f * FMath::Sin(TimeSec * 16.0f);
			const FLinearColor BaitCol = FLinearColor(1.0f, 0.70f, 0.0f, 0.96f * Pulse);
			const FLinearColor BaitAlert = FLinearColor(0.0f, 0.96f, 0.65f, 0.95f);

			// Pulsing Outer & Inner Tactical Diamond
			DrawDiamond(MX, MY, 9.5f * Pulse * HUDScale, 2.0f * HUDScale, BaitCol, false, 5);
			DrawDiamond(MX, MY, 4.5f * HUDScale, 1.5f * HUDScale, BaitAlert, true, 5);

			// Floating Pill & "[ ! BAITED ]" Readout Above Diamond
			const float PillW = 96.0f * HUDScale;
			const float PillH = 16.0f * HUDScale;
			const float PillX = MX - PillW * 0.5f;
			const float PillY = MY - 28.0f * HUDScale;

			DrawSkewedRect(PillX, PillY, PillW, PillH, -3.0f * HUDScale,
				FLinearColor(0.02f, 0.06f, 0.09f, 0.78f),
				FLinearColor(0.04f, 0.12f, 0.16f, 0.82f), 4);
			DrawLine(PillX, PillY + PillH, PillX + PillW * FMath::Clamp(Marker.RemainingRatio, 0.05f, 1.0f), PillY + PillH,
				2.2f * HUDScale, BaitAlert, 5);
			DrawGlowText(PillX + 10.0f * HUDScale, PillY + 1.5f * HUDScale, TEXT("[ ! BAITED ]"), FontMicro, BaitCol, 0.9f, 6);
		}

		return LayerId + 8;
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

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UFPSEconomySubsystem* Economy = GI->GetSubsystem<UFPSEconomySubsystem>())
		{
			CurrentGoldBalance = Economy->GetGoldBalance();
			CurrentOpalBalance = Economy->GetOpalBalance();
			CachedBounties = Economy->GetActiveBounties();
			Economy->OnCurrencyChanged.AddDynamic(this, &UFPSHUDWidget::HandleCurrencyChanged);
			Economy->OnBountyCompleted.AddDynamic(this, &UFPSHUDWidget::HandleBountyCompleted);
		}
	}
}

void UFPSHUDWidget::HandleCurrencyChanged(int32 NewGold, int32 NewOpals)
{
	CurrentGoldBalance = NewGold;
	CurrentOpalBalance = NewOpals;
}

void UFPSHUDWidget::HandleBountyCompleted(const FFPSTacticalBounty& CompletedBounty)
{
	// Trigger accolade banner on HUD when a Daily Contract completes mid-match
	TriggerAccolade(TEXT("CONTRACT COMPLETE // +GOLD & OPALS!"), CompletedBounty.GoldReward, FLinearColor(1.0f, 0.84f, 0.0f));
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

	HUDTimeSeconds += InDeltaTime;

	// Keep HUD persistent economy & active contracts telemetry synchronized
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UFPSEconomySubsystem* Economy = GI->GetSubsystem<UFPSEconomySubsystem>())
		{
			CurrentGoldBalance = Economy->GetGoldBalance();
			CurrentOpalBalance = Economy->GetOpalBalance();
			CachedBounties = Economy->GetActiveBounties();
		}
	}

	if (HitmarkerTimer > 0.0f)
	{
		HitmarkerTimer = FMath::Max(0.0f, HitmarkerTimer - InDeltaTime);
	}

	if (DamageIndicatorTimer > 0.0f)
	{
		DamageIndicatorTimer = FMath::Max(0.0f, DamageIndicatorTimer - InDeltaTime);
	}

	if (!OwningCharacter.IsValid())
	{
		if (APlayerController* PC = GetOwningPlayer())
		{
			if (AFPSCharacter* FPSChar = Cast<AFPSCharacter>(PC->GetPawn()))
			{
				SetOwningCharacter(FPSChar);
			}
		}
	}

	if (OwningCharacter.IsValid())
	{
		AFPSCharacter* Char = OwningCharacter.Get();

		// 1. Vitals & Smooth Trailing Damage Ghost Bars
		if (Char->HealthComponent)
		{
			CurrentHealth = Char->HealthComponent->GetCurrentHealth();
			MaxHealth = Char->HealthComponent->MaxHealth;
			CurrentShield = Char->HealthComponent->GetCurrentShield();
			MaxShield = Char->HealthComponent->MaxShield;

			if (CurrentHealth > DisplayedHealthGhost)
			{
				DisplayedHealthGhost = CurrentHealth;
			}
			else
			{
				DisplayedHealthGhost = FMath::FInterpTo(DisplayedHealthGhost, CurrentHealth, InDeltaTime, 3.2f);
			}

			if (CurrentShield > DisplayedShieldGhost)
			{
				DisplayedShieldGhost = CurrentShield;
			}
			else
			{
				DisplayedShieldGhost = FMath::FInterpTo(DisplayedShieldGhost, CurrentShield, InDeltaTime, 3.8f);
			}
		}

		// 2. Stamina & Fermi Ballistic Dive System
		if (Char->FPSMovementComponent)
		{
			CurrentStamina = Char->FPSMovementComponent->GetCurrentStamina();
			MaxStamina = Char->FPSMovementComponent->GetMaxStamina();
			bIsStaminaExhausted = Char->FPSMovementComponent->IsStaminaExhausted();
			bIsBallisticDiving = Char->FPSMovementComponent->IsBallisticDiving();
			FermiDiveCooldownRemaining = Char->FPSMovementComponent->GetDiveCooldownRemaining();
			bCanBallisticDive = Char->FPSMovementComponent->CanStartBallisticDive();
		}

		bIsFermiTimeBubbleActive = Char->IsFermiTimeBubbleActive();
		FermiBubbleTimeRemaining = Char->GetFermiTimeBubbleRemaining();
		bIsFermiDecoyActive = Char->IsFermiDecoyActive();
		FermiDecoyTimeRemaining = Char->GetFermiDecoyRemaining();
		FermiDecoyCooldownRemaining = Char->GetFermiDecoyCooldownRemaining();
		QuantumSwapFlashAlpha = Char->GetQuantumSwapFlashAlpha();
		bUseFirstPersonOpticADS = Char->bUseFirstPersonOpticADS;

		// 3. Weapon, Individual Pip Matrix & Reload Progress
		if (Char->InventoryComponent)
		{
			if (AFPSWeaponBase* Weapon = Char->InventoryComponent->GetActiveWeapon())
			{
				AmmoInClip = Weapon->GetCurrentAmmo();
				MagazineCapacity = Weapon->GetMagazineCapacity();
				AmmoReserve = Weapon->GetReserveAmmo();
				ActiveWeaponName = Weapon->WeaponDisplayName;
				bIsReloading = Weapon->IsReloading();
				ReloadProgress = Weapon->GetReloadProgress();
				bIsADS = Weapon->IsAimingDownSights();
				ADSAlpha = Weapon->GetADSAlpha();

				switch (Weapon->FireMode)
				{
				case EFPSFireMode::FullAuto:
					FireModeString = TEXT("FULL AUTO // 750 RPM");
					ActiveFireModeIndex = 0;
					break;
				case EFPSFireMode::Burst:
					FireModeString = TEXT("3-ROUND BURST");
					ActiveFireModeIndex = 1;
					break;
				case EFPSFireMode::SemiAuto:
				default:
					FireModeString = TEXT("SEMI-AUTO");
					ActiveFireModeIndex = 2;
					break;
				}
			}
		}

		// 4. Pose, Velocity & Helmet Visor Inertial Lag Parallax
		PlayerLocation = Char->GetActorLocation();
		PlayerSpeed = Char->GetVelocity().Size2D();

		const FRotator ControlRot = Char->GetControlRotation();
		CameraYaw = ControlRot.Yaw;
		CameraPitch = ControlRot.Pitch;

		const float DeltaYaw = FRotator::NormalizeAxis(CameraYaw - LastCameraYaw);
		const float DeltaPitch = FRotator::NormalizeAxis(CameraPitch - LastCameraPitch);
		LastCameraYaw = CameraYaw;
		LastCameraPitch = CameraPitch;

		// Subtle breathing + movement + angular turn sway on the AR visor
		// 25% Shake & Lag Reduction (0.75x)
		const float WalkBobY = (PlayerSpeed > 20.0f) ? FMath::Sin(HUDTimeSeconds * 9.5f) * 1.65f : FMath::Sin(HUDTimeSeconds * 2.0f) * 0.60f;
		const float TargetLagX = FMath::Clamp(-DeltaYaw * 2.1f, -10.5f, 10.5f);
		const float TargetLagY = FMath::Clamp(DeltaPitch * 1.8f + WalkBobY, -9.0f, 9.0f);

		VisorLagOffset.X = FMath::FInterpTo(VisorLagOffset.X, TargetLagX, InDeltaTime, 10.0f);
		VisorLagOffset.Y = FMath::FInterpTo(VisorLagOffset.Y, TargetLagY, InDeltaTime, 10.0f);
	}

	// Update 3D-to-2D Projected "[ ! BAITED ]" Diamond Indicators above baited enemies
	ActiveBaitedMarkers.Reset();
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = GetOwningPlayer())
		{
			int32 ViewportX = 1920, ViewportY = 1080;
			PC->GetViewportSize(ViewportX, ViewportY);
			const FVector2f LocalSize = MyGeometry.GetLocalSize();
			const float ScaleX = (ViewportX > 0 && LocalSize.X > 10.0f) ? (LocalSize.X / (float)ViewportX) : 1.0f;
			const float ScaleY = (ViewportY > 0 && LocalSize.Y > 10.0f) ? (LocalSize.Y / (float)ViewportY) : 1.0f;

			for (TActorIterator<AFPSCharacter> It(World); It; ++It)
			{
				AFPSCharacter* BotChar = *It;
				if (!BotChar || BotChar == OwningCharacter.Get() || !BotChar->IsBaitedByDecoy()) continue;

				const FVector HeadWorldPos = BotChar->GetActorLocation() + FVector(0.0f, 0.0f, 115.0f);
				FVector2D ProjectedScreen;
				if (PC->ProjectWorldLocationToScreen(HeadWorldPos, ProjectedScreen, true))
				{
					FFPSBaitedTargetMarker Marker;
					Marker.ScreenPos = FVector2f((float)ProjectedScreen.X * ScaleX, (float)ProjectedScreen.Y * ScaleY);
					Marker.RemainingRatio = BotChar->GetBaitedRemainingRatio();
					Marker.Callsign = BotChar->GetBotCallsign();
					ActiveBaitedMarkers.Add(Marker);
				}
			}
		}
	}

	// Update Active Threat Indicators (Deaf Accessibility Radar)
	for (int32 i = ActiveThreatIndicators.Num() - 1; i >= 0; --i)
	{
		ActiveThreatIndicators[i].LifetimeRemaining -= InDeltaTime;
		if (ActiveThreatIndicators[i].LifetimeRemaining <= 0.0f)
		{
			ActiveThreatIndicators.RemoveAt(i);
			continue;
		}

		// Recalculate relative angle dynamically to current camera heading
		const FVector CamLoc = OwningCharacter.IsValid() ? OwningCharacter->GetActorLocation() : PlayerLocation;
		const FRotator CamRot = FRotator(0.0f, CameraYaw, 0.0f);
		const FVector Fwd = CamRot.Vector();
		const FVector Right = FRotationMatrix(CamRot).GetUnitAxis(EAxis::Y);
		const FVector ToSource = (ActiveThreatIndicators[i].WorldSourceLocation - CamLoc).GetSafeNormal2D();
		if (!ToSource.IsNearlyZero())
		{
			const float FwdDot = FVector::DotProduct(ToSource, Fwd);
			const float RightDot = FVector::DotProduct(ToSource, Right);
			ActiveThreatIndicators[i].RelativeAngleDeg = FMath::RadiansToDegrees(FMath::Atan2(RightDot, FwdDot));
		}
	}

	// Update Accolades
	for (int32 i = ActiveAccolades.Num() - 1; i >= 0; --i)
	{
		ActiveAccolades[i].LifetimeRemaining -= InDeltaTime;
		if (ActiveAccolades[i].LifetimeRemaining <= 0.0f)
		{
			ActiveAccolades.RemoveAt(i);
		}
	}

	// Update Killstreak Combo Decay
	if (ComboDecayTimer > 0.0f)
	{
		ComboDecayTimer = FMath::Max(0.0f, ComboDecayTimer - InDeltaTime);
		if (ComboDecayTimer <= 0.0f)
		{
			CurrentComboCount = 0;
		}
	}
}

void UFPSHUDWidget::TriggerHitmarker(EFPSHitmarkerType HitType)
{
	CurrentHitmarker = HitType;
	HitmarkerTimer = 0.25f;
	OnShowHitmarker(HitType);
}

void UFPSHUDWidget::TriggerAccolade(const FString& Title, int32 ScoreBonus, const FLinearColor& AccentColor)
{
	FFPSCombatAccolade NewAcc;
	NewAcc.Title = Title;
	NewAcc.ScoreBonus = ScoreBonus;
	NewAcc.AccentColor = AccentColor;
	NewAcc.LifetimeRemaining = 2.4f;
	NewAcc.InitialLifetime = 2.4f;

	if (ActiveAccolades.Num() >= 3)
	{
		ActiveAccolades.RemoveAt(0);
	}
	ActiveAccolades.Add(NewAcc);

	if (OwningCharacter.IsValid())
	{
		if (AFPSPlayerState* PS = OwningCharacter->GetPlayerState<AFPSPlayerState>())
		{
			PS->AddCombatScore(ScoreBonus);
		}
	}
}

void UFPSHUDWidget::RegisterKillForCombo()
{
	if (ComboDecayTimer > 0.0f)
	{
		CurrentComboCount++;
	}
	else
	{
		CurrentComboCount = 1;
	}
	ComboDecayTimer = MaxComboWindow;

	FString ComboTitle;
	int32 ComboReward = 0;
	FLinearColor AccentCol = FLinearColor(1.0f, 0.702f, 0.0f);

	if (CurrentComboCount == 2)
	{
		ComboTitle = TEXT("🔥 2x DOUBLE KILL (+50 GOLD)");
		ComboReward = 50;
		AccentCol = FLinearColor(1.0f, 0.702f, 0.0f);
	}
	else if (CurrentComboCount == 3)
	{
		ComboTitle = TEXT("⚡ 3x TRIPLE KILL (+100 GOLD)");
		ComboReward = 100;
		AccentCol = FLinearColor(1.0f, 0.45f, 0.0f);
	}
	else if (CurrentComboCount == 4)
	{
		ComboTitle = TEXT("💀 4x QUAD FEED (+150 GOLD)");
		ComboReward = 150;
		AccentCol = FLinearColor(1.0f, 0.15f, 0.25f);
	}
	else if (CurrentComboCount >= 5)
	{
		ComboTitle = FString::Printf(TEXT("👑 %dx UNSTOPPABLE (+250 GOLD)"), CurrentComboCount);
		ComboReward = 250;
		AccentCol = FLinearColor(1.0f, 0.10f, 0.90f);
	}

	if (ComboReward > 0)
	{
		if (OwningCharacter.IsValid())
		{
			OwningCharacter->AwardCombatAccolade(ComboTitle, ComboReward, AccentCol);
		}
		else
		{
			TriggerAccolade(ComboTitle, ComboReward, AccentCol);
			if (UGameInstance* GI = GetGameInstance())
			{
				if (UFPSEconomySubsystem* Economy = GI->GetSubsystem<UFPSEconomySubsystem>())
				{
					Economy->AddGold(ComboReward, ComboTitle);
				}
			}
		}
	}
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

	DamageIndicatorAngle = AngleDeg;
	DamageIndicatorTimer = 1.2f;

	AddDirectionalIndicator(DamageSourceLocation, 1.2f, EThreatIndicatorType::Damage);

	OnShowDamageIndicator(AngleDeg);
}

void UFPSHUDWidget::AddDirectionalIndicator(FVector WorldSourceLocation, float Intensity, EThreatIndicatorType Type)
{
	float Duration = 1.2f;
	if (Type == EThreatIndicatorType::Footstep)
	{
		Duration = 0.6f;
	}
	else if (Type == EThreatIndicatorType::Gunfire)
	{
		Duration = 0.9f;
	}

	const FVector CamLoc = OwningCharacter.IsValid() ? OwningCharacter->GetActorLocation() : PlayerLocation;
	const FRotator CamRot = FRotator(0.0f, CameraYaw, 0.0f);
	const FVector Fwd = CamRot.Vector();
	const FVector Right = FRotationMatrix(CamRot).GetUnitAxis(EAxis::Y);
	const FVector ToSource = (WorldSourceLocation - CamLoc).GetSafeNormal2D();

	float ThreatAngleDeg = 0.0f;
	if (!ToSource.IsNearlyZero())
	{
		const float FwdDot = FVector::DotProduct(ToSource, Fwd);
		const float RightDot = FVector::DotProduct(ToSource, Right);
		ThreatAngleDeg = FMath::RadiansToDegrees(FMath::Atan2(RightDot, FwdDot));
	}

	// Merge with existing indicator of the same type within 18 degrees
	bool bMerged = false;
	for (FFPSThreatIndicator& Existing : ActiveThreatIndicators)
	{
		if (Existing.Type == Type)
		{
			const float AngleDelta = FMath::Abs(FRotator::NormalizeAxis(Existing.RelativeAngleDeg - ThreatAngleDeg));
			if (AngleDelta < 18.0f)
			{
				Existing.LifetimeRemaining = Duration;
				Existing.InitialLifetime = Duration;
				Existing.Intensity = FMath::Clamp(Existing.Intensity + Intensity * 0.4f, 0.6f, 2.5f);
				Existing.WorldSourceLocation = FMath::Lerp(Existing.WorldSourceLocation, WorldSourceLocation, 0.5f);
				Existing.RelativeAngleDeg = ThreatAngleDeg;
				bMerged = true;
				break;
			}
		}
	}

	if (!bMerged)
	{
		FFPSThreatIndicator NewInd;
		NewInd.WorldSourceLocation = WorldSourceLocation;
		NewInd.Intensity = FMath::Clamp(Intensity, 0.5f, 2.5f);
		NewInd.Type = Type;
		NewInd.LifetimeRemaining = Duration;
		NewInd.InitialLifetime = Duration;
		NewInd.RelativeAngleDeg = ThreatAngleDeg;
		ActiveThreatIndicators.Add(NewInd);
	}
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
