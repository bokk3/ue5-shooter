#include "UI/MainMenuWidget.h"
#include "UI/SettingsMenuWidget.h"
#include "Character/FPSCharacterPreviewActor.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Layout/SBackgroundBlur.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Misc/CommandLine.h"
#include "Economy/FPSEconomySubsystem.h"
#include "Rendering/DrawElements.h"
#include "Widgets/SCompoundWidget.h"

namespace MWTheme
{
	// High-Contrast AAA Modern Warfare Colors
	static const FLinearColor CyanAccent      = FLinearColor(0.0f, 0.898f, 1.0f, 1.0f);     // #00E5FF Tactical Cyan
	static const FLinearColor AmberAccent     = FLinearColor(1.0f, 0.702f, 0.0f, 1.0f);     // #FFB300 Afterburner Amber
	static const FLinearColor CrimsonAlert    = FLinearColor(1.0f, 0.165f, 0.329f, 1.0f);    // #FF2A54 Crimson Alert
	static const FLinearColor EmeraldReady    = FLinearColor(0.0f, 0.902f, 0.463f, 1.0f);    // #00E676 Emerald Ready

	// Semi-Transparent Dark Glass Panels
	static const FLinearColor DarkGlass       = FLinearColor(0.03f, 0.06f, 0.10f, 0.68f);    // Glass Panel Base (0.68 alpha)
	static const FLinearColor DarkGlassCard   = FLinearColor(0.04f, 0.08f, 0.14f, 0.72f);    // Inner Card Glass
	static const FLinearColor DarkGlassHover  = FLinearColor(0.07f, 0.14f, 0.22f, 0.80f);    // Card Hover/Active Glass
	static const FLinearColor GlassBorder     = FLinearColor(0.14f, 0.22f, 0.32f, 0.60f);    // Crisp 1px glass border
	static const FLinearColor ActiveBorder    = FLinearColor(0.0f, 0.898f, 1.0f, 0.90f);     // 1-2px glowing Cyan border

	// Typography Colors
	static const FLinearColor TextPrimary     = FLinearColor(0.94f, 0.96f, 0.98f, 1.0f);    // High-contrast clean white
	static const FLinearColor TextSecondary   = FLinearColor(0.72f, 0.78f, 0.86f, 1.0f);    // Subtle white
	static const FLinearColor TextMuted       = FLinearColor(0.48f, 0.55f, 0.65f, 1.0f);    // Tactical muted grey
}

class SFPSFortuneWheelWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFPSFortuneWheelWidget)
		: _WheelAngle(0.0f)
		, _BurstTimer(0.0f)
	{}
		SLATE_ATTRIBUTE(float, WheelAngle)
		SLATE_ATTRIBUTE(float, BurstTimer)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		WheelAngle = InArgs._WheelAngle;
		BurstTimer = InArgs._BurstTimer;
	}

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
	{
		const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
		const FVector2D Center = LocalSize * 0.5f;
		const float Radius = FMath::Min(Center.X, Center.Y) - 16.0f;
		if (Radius <= 10.0f) return LayerId;

		const float CurrentAngleDeg = WheelAngle.Get(0.0f);

		// 1. Draw outer rim (circle)
		const int32 NumCircleSegments = 48;
		TArray<FVector2D> RimPoints;
		RimPoints.Reserve(NumCircleSegments + 1);
		for (int32 i = 0; i <= NumCircleSegments; ++i)
		{
			const float Theta = (float(i) / float(NumCircleSegments)) * (2.0f * UE_PI);
			RimPoints.Add(Center + FVector2D(FMath::Cos(Theta), FMath::Sin(Theta)) * Radius);
		}
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), RimPoints, ESlateDrawEffect::None, FLinearColor(0.2f, 0.35f, 0.45f, 0.8f), false, 3.0f);

		// 2. Draw 8 spokes
		const int32 NumSlices = 8;
		const float SliceAngleDeg = 360.0f / float(NumSlices);
		for (int32 i = 0; i < NumSlices; ++i)
		{
			const float SpokeAngleRad = FMath::DegreesToRadians(CurrentAngleDeg + i * SliceAngleDeg);
			const FVector2D Dir(FMath::Cos(SpokeAngleRad), FMath::Sin(SpokeAngleRad));

			TArray<FVector2D> SpokePoints;
			SpokePoints.Add(Center);
			SpokePoints.Add(Center + Dir * Radius);

			const FLinearColor SpokeColor = (i % 2 == 0) ? MWTheme::CyanAccent.CopyWithNewOpacity(0.7f) : MWTheme::AmberAccent.CopyWithNewOpacity(0.7f);
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(), SpokePoints, ESlateDrawEffect::None, SpokeColor, false, 2.0f);

			// Outer rim peg
			TArray<FVector2D> PegPoints;
			const FVector2D PegPos = Center + Dir * Radius;
			PegPoints.Add(PegPos - FVector2D(3.0f, 3.0f));
			PegPoints.Add(PegPos + FVector2D(3.0f, 3.0f));
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, AllottedGeometry.ToPaintGeometry(), PegPoints, ESlateDrawEffect::None, FLinearColor::White, false, 4.0f);
		}

		// 3. Draw inner concentric ring
		TArray<FVector2D> InnerRingPoints;
		InnerRingPoints.Reserve(NumCircleSegments + 1);
		const float InnerRadius = Radius * 0.45f;
		for (int32 i = 0; i <= NumCircleSegments; ++i)
		{
			const float Theta = (float(i) / float(NumCircleSegments)) * (2.0f * UE_PI);
			InnerRingPoints.Add(Center + FVector2D(FMath::Cos(Theta), FMath::Sin(Theta)) * InnerRadius);
		}
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(), InnerRingPoints, ESlateDrawEffect::None, FLinearColor(0.15f, 0.25f, 0.35f, 0.6f), false, 1.5f);

		// 4. Draw Center Hub
		TArray<FVector2D> HubPoints;
		HubPoints.Reserve(25);
		const float HubRadius = 22.0f;
		for (int32 i = 0; i <= 24; ++i)
		{
			const float Theta = (float(i) / 24.0f) * (2.0f * UE_PI);
			HubPoints.Add(Center + FVector2D(FMath::Cos(Theta), FMath::Sin(Theta)) * HubRadius);
		}
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 3, AllottedGeometry.ToPaintGeometry(), HubPoints, ESlateDrawEffect::None, MWTheme::CyanAccent, false, 3.0f);

		// 5. Top Needle Indicator (pointing down at center top)
		const FVector2D NeedleTip = Center + FVector2D(0.0f, -Radius + 8.0f);
		const FVector2D NeedleLeft = Center + FVector2D(-12.0f, -Radius - 14.0f);
		const FVector2D NeedleRight = Center + FVector2D(12.0f, -Radius - 14.0f);

		TArray<FVector2D> NeedlePoints;
		NeedlePoints.Add(NeedleLeft);
		NeedlePoints.Add(NeedleTip);
		NeedlePoints.Add(NeedleRight);
		NeedlePoints.Add(NeedleLeft);
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 4, AllottedGeometry.ToPaintGeometry(), NeedlePoints, ESlateDrawEffect::None, MWTheme::AmberAccent, false, 3.0f);

		// 6. Jackpot / High-Tier Particle Spark Burst Lines
		const float Burst = BurstTimer.Get(0.0f);
		if (Burst > 0.0f)
		{
			const float Progress = 1.0f - FMath::Clamp(Burst / 1.5f, 0.0f, 1.0f);
			const float Alpha = FMath::Clamp(1.0f - Progress, 0.0f, 1.0f);
			const int32 NumSparks = 20;
			for (int32 s = 0; s < NumSparks; ++s)
			{
				const float SparkAngle = (float(s) / float(NumSparks)) * (2.0f * UE_PI) + Progress * 0.8f;
				const FVector2D Dir(FMath::Cos(SparkAngle), FMath::Sin(SparkAngle));
				const float StartR = Radius * (0.25f + Progress * 0.65f);
				const float EndR = StartR + 18.0f * (1.0f - Progress * 0.4f);

				TArray<FVector2D> SparkLine;
				SparkLine.Add(Center + Dir * StartR);
				SparkLine.Add(Center + Dir * EndR);

				const FLinearColor SparkColor = (s % 2 == 0)
					? MWTheme::AmberAccent.CopyWithNewOpacity(Alpha)
					: MWTheme::CyanAccent.CopyWithNewOpacity(Alpha);
				FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 5, AllottedGeometry.ToPaintGeometry(), SparkLine, ESlateDrawEffect::None, SparkColor, false, 2.5f);
			}
		}

		return LayerId + 6;
	}

	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(320.0f, 320.0f); }

private:
	TAttribute<float> WheelAngle;
	TAttribute<float> BurstTimer;
};

class SFPSDartboardWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFPSDartboardWidget)
		: _ReticlePos(FVector2D(0.5f, 0.5f))
		, _LastHitPos(FVector2D(0.5f, 0.5f))
		, _HasLastHit(false)
		, _BurstTimer(0.0f)
	{}
		SLATE_ATTRIBUTE(FVector2D, ReticlePos)
		SLATE_ATTRIBUTE(FVector2D, LastHitPos)
		SLATE_ATTRIBUTE(bool, HasLastHit)
		SLATE_ATTRIBUTE(float, BurstTimer)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		ReticlePos = InArgs._ReticlePos;
		LastHitPos = InArgs._LastHitPos;
		HasLastHit = InArgs._HasLastHit;
		BurstTimer = InArgs._BurstTimer;
	}

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
	{
		const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
		const FVector2D Center = LocalSize * 0.5f;
		const float Radius = FMath::Min(Center.X, Center.Y) - 16.0f;
		if (Radius <= 10.0f) return LayerId;

		auto DrawCircle = [&](float RingRadius, const FLinearColor& Color, float Thickness)
		{
			const int32 Segments = 48;
			TArray<FVector2D> Pts;
			Pts.Reserve(Segments + 1);
			for (int32 i = 0; i <= Segments; ++i)
			{
				const float Theta = (float(i) / float(Segments)) * (2.0f * UE_PI);
				Pts.Add(Center + FVector2D(FMath::Cos(Theta), FMath::Sin(Theta)) * RingRadius);
			}
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), Pts, ESlateDrawEffect::None, Color, false, Thickness);
		};

		// 1. Concentric rings
		DrawCircle(Radius, FLinearColor(0.25f, 0.35f, 0.45f, 0.9f), 2.5f);        // Outer perimeter
		DrawCircle(Radius * 0.78f, FLinearColor(0.18f, 0.28f, 0.38f, 0.7f), 1.5f); // Triple ring
		DrawCircle(Radius * 0.52f, FLinearColor(0.18f, 0.28f, 0.38f, 0.7f), 1.5f); // Double ring
		DrawCircle(Radius * 0.24f, MWTheme::AmberAccent.CopyWithNewOpacity(0.85f), 2.0f); // Outer Bull
		DrawCircle(Radius * 0.08f, MWTheme::CrimsonAlert, 3.0f);                          // Inner Bullseye

		// 2. Spokes (12 radial divisions)
		for (int32 i = 0; i < 12; ++i)
		{
			const float Theta = (float(i) / 12.0f) * (2.0f * UE_PI);
			const FVector2D Dir(FMath::Cos(Theta), FMath::Sin(Theta));
			TArray<FVector2D> Spoke;
			Spoke.Add(Center + Dir * (Radius * 0.08f));
			Spoke.Add(Center + Dir * Radius);
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(), Spoke, ESlateDrawEffect::None, FLinearColor(0.15f, 0.22f, 0.30f, 0.5f), false, 1.0f);
		}

		// 3. Last Dart Impact & Bullseye Spark Burst
		if (HasLastHit.Get(false))
		{
			const FVector2D LastHit = LastHitPos.Get(FVector2D(0.5f, 0.5f));
			const FVector2D LastHitPx(LastHit.X * LocalSize.X, LastHit.Y * LocalSize.Y);

			TArray<FVector2D> HitMark1;
			HitMark1.Add(LastHitPx - FVector2D(8.0f, 8.0f));
			HitMark1.Add(LastHitPx + FVector2D(8.0f, 8.0f));
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 3, AllottedGeometry.ToPaintGeometry(), HitMark1, ESlateDrawEffect::None, MWTheme::EmeraldReady, false, 3.0f);

			TArray<FVector2D> HitMark2;
			HitMark2.Add(LastHitPx - FVector2D(-8.0f, 8.0f));
			HitMark2.Add(LastHitPx + FVector2D(-8.0f, 8.0f));
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 3, AllottedGeometry.ToPaintGeometry(), HitMark2, ESlateDrawEffect::None, MWTheme::EmeraldReady, false, 3.0f);

			const float Burst = BurstTimer.Get(0.0f);
			if (Burst > 0.0f)
			{
				const float Progress = 1.0f - FMath::Clamp(Burst / 1.5f, 0.0f, 1.0f);
				const float Alpha = FMath::Clamp(1.0f - Progress, 0.0f, 1.0f);
				const int32 NumRays = 16;
				for (int32 r = 0; r < NumRays; ++r)
				{
					const float RayAngle = (float(r) / float(NumRays)) * (2.0f * UE_PI);
					const FVector2D RayDir(FMath::Cos(RayAngle), FMath::Sin(RayAngle));
					const float InnerDist = 10.0f + Progress * 55.0f;
					const float OuterDist = InnerDist + 16.0f * (1.0f - Progress * 0.3f);

					TArray<FVector2D> RayPts;
					RayPts.Add(LastHitPx + RayDir * InnerDist);
					RayPts.Add(LastHitPx + RayDir * OuterDist);
					const FLinearColor RayColor = (r % 2 == 0)
						? MWTheme::EmeraldReady.CopyWithNewOpacity(Alpha)
						: MWTheme::AmberAccent.CopyWithNewOpacity(Alpha);
					FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 5, AllottedGeometry.ToPaintGeometry(), RayPts, ESlateDrawEffect::None, RayColor, false, 2.5f);
				}
			}
		}

		// 4. Moving Tactical Crosshair Reticle
		const FVector2D Reticle = ReticlePos.Get(FVector2D(0.5f, 0.5f));
		const FVector2D ReticlePx(Reticle.X * LocalSize.X, Reticle.Y * LocalSize.Y);
		const float CrosshairRadius = 14.0f;

		// Crosshair circle
		{
			const int32 ReticleSegs = 24;
			TArray<FVector2D> ReticleCircle;
			ReticleCircle.Reserve(ReticleSegs + 1);
			for (int32 i = 0; i <= ReticleSegs; ++i)
			{
				const float Theta = (float(i) / float(ReticleSegs)) * (2.0f * UE_PI);
				ReticleCircle.Add(ReticlePx + FVector2D(FMath::Cos(Theta), FMath::Sin(Theta)) * CrosshairRadius);
			}
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 4, AllottedGeometry.ToPaintGeometry(), ReticleCircle, ESlateDrawEffect::None, MWTheme::CyanAccent, false, 2.0f);
		}

		// Crosshair ticks
		TArray<FVector2D> HorizTick;
		HorizTick.Add(ReticlePx - FVector2D(CrosshairRadius + 6.0f, 0.0f));
		HorizTick.Add(ReticlePx + FVector2D(CrosshairRadius + 6.0f, 0.0f));
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 4, AllottedGeometry.ToPaintGeometry(), HorizTick, ESlateDrawEffect::None, MWTheme::CyanAccent, false, 2.0f);

		TArray<FVector2D> VertTick;
		VertTick.Add(ReticlePx - FVector2D(0.0f, CrosshairRadius + 6.0f));
		VertTick.Add(ReticlePx + FVector2D(0.0f, CrosshairRadius + 6.0f));
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 4, AllottedGeometry.ToPaintGeometry(), VertTick, ESlateDrawEffect::None, MWTheme::CyanAccent, false, 2.0f);

		return LayerId + 6;
	}

	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(320.0f, 320.0f); }

private:
	TAttribute<FVector2D> ReticlePos;
	TAttribute<FVector2D> LastHitPos;
	TAttribute<bool> HasLastHit;
	TAttribute<float> BurstTimer;
};

class SFPSBountyProgressBarWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFPSBountyProgressBarWidget)
		: _Fraction(0.0f)
		, _bCompleted(false)
	{}
		SLATE_ATTRIBUTE(float, Fraction)
		SLATE_ATTRIBUTE(bool, bCompleted)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		Fraction = InArgs._Fraction;
		bCompleted = InArgs._bCompleted;
	}

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
	{
		const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
		if (LocalSize.X <= 4.0f || LocalSize.Y <= 2.0f) return LayerId;

		const float Pct = FMath::Clamp(Fraction.Get(0.0f), 0.0f, 1.0f);
		const bool bDone = bCompleted.Get(false);
		const FSlateBrush* WhiteBrush = FAppStyle::Get().GetBrush("WhiteBrush");

		// 1. Track Background
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(FVector2f(LocalSize), FSlateLayoutTransform()),
			WhiteBrush,
			ESlateDrawEffect::None,
			FLinearColor(0.02f, 0.04f, 0.07f, 0.90f)
		);

		// 2. Filled Progress Bar
		const float FillWidth = FMath::Max(0.0f, (LocalSize.X - 2.0f) * Pct);
		if (FillWidth > 0.5f)
		{
			const FLinearColor FillColor = bDone ? MWTheme::EmeraldReady : MWTheme::CyanAccent;
			FSlateDrawElement::MakeBox(
				OutDrawElements,
				LayerId + 1,
				AllottedGeometry.ToPaintGeometry(FVector2f(FillWidth, LocalSize.Y - 2.0f), FSlateLayoutTransform(FVector2f(1.0f, 1.0f))),
				WhiteBrush,
				ESlateDrawEffect::None,
				FillColor
			);
		}

		// 3. Crisp Vector Perimeter & Tick Marks
		TArray<FVector2D> BorderPts;
		BorderPts.Add(FVector2D(0.0f, 0.0f));
		BorderPts.Add(FVector2D(LocalSize.X, 0.0f));
		BorderPts.Add(FVector2D(LocalSize.X, LocalSize.Y));
		BorderPts.Add(FVector2D(0.0f, LocalSize.Y));
		BorderPts.Add(FVector2D(0.0f, 0.0f));
		const FLinearColor BorderColor = bDone ? MWTheme::EmeraldReady.CopyWithNewOpacity(0.85f) : MWTheme::GlassBorder;
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, AllottedGeometry.ToPaintGeometry(), BorderPts, ESlateDrawEffect::None, BorderColor, false, 1.0f);

		return LayerId + 3;
	}

	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(180.0f, 8.0f); }

private:
	TAttribute<float> Fraction;
	TAttribute<bool> bCompleted;
};

UMainMenuWidget::UMainMenuWidget()
{
	bIsBackHandler = false;
}

void UMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (MenuMusicTrack)
	{
		UGameplayStatics::PlaySound2D(this, MenuMusicTrack);
	}

	UpdateCurrencyHeader();
}

void UMainMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (WheelBurstTimer > 0.0f)
	{
		WheelBurstTimer = FMath::Max(0.0f, WheelBurstTimer - InDeltaTime);
	}
	if (DartBurstTimer > 0.0f)
	{
		DartBurstTimer = FMath::Max(0.0f, DartBurstTimer - InDeltaTime);
	}

	// 1. Fortune Wheel Rotational Physics
	if (bIsWheelSpinning)
	{
		WheelAngularVelocity *= FMath::Pow(0.96f, InDeltaTime * 60.0f);
		WheelCurrentAngle += WheelAngularVelocity * InDeltaTime;
		WheelCurrentAngle = FMath::Fmod(WheelCurrentAngle, 360.0f);
		WheelSpinElapsed += InDeltaTime;

		if (WheelAngularVelocity < 15.0f && WheelSpinElapsed > 2.0f)
		{
			bIsWheelSpinning = false;
			WheelAngularVelocity = 0.0f;

			if (UGameInstance* GI = GetGameInstance())
			{
				if (UFPSEconomySubsystem* Economy = GI->GetSubsystem<UFPSEconomySubsystem>())
				{
					FText RewardMsg;
					int32 RewardAmount = 0;
					EWheelRewardType RewardType;
					Economy->AwardWheelSliceReward(WheelTargetSlice, RewardMsg, RewardAmount, RewardType);
					if (RewardType == EWheelRewardType::Opals || RewardAmount >= 1250)
					{
						WheelBurstTimer = 1.5f;
					}
					if (WheelRewardText.IsValid())
					{
						WheelRewardText->SetText(RewardMsg);
					}
					UpdateCurrencyHeader();
					RefreshDailyRewardsUI();
				}
			}
		}
	}

	// 2. Tactical Dartboard Reticle Lissajous Oscillation
	DartOscillationTime += InDeltaTime;
	DartReticlePos.X = 0.5f + 0.38f * FMath::Sin(DartOscillationTime * 2.3f);
	DartReticlePos.Y = 0.5f + 0.32f * FMath::Cos(DartOscillationTime * 3.1f);
}

void UMainMenuWidget::SetPreviewActor(AFPSCharacterPreviewActor* InPreviewActor)
{
	PreviewActor = InPreviewActor;
	if (PreviewActor)
	{
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UFPSEconomySubsystem* Economy = GI->GetSubsystem<UFPSEconomySubsystem>())
			{
				PreviewActor->SetCamo(static_cast<EOperatorCamo>(FMath::Clamp(Economy->GetEquippedCamoIndex(), 0, 3)));
				PreviewActor->SetHeadgear(static_cast<EOperatorHeadgear>(FMath::Clamp(Economy->GetEquippedHeadgearIndex(), 0, 3)));
				PreviewActor->SetArmorVest(static_cast<EOperatorArmorVest>(FMath::Clamp(Economy->GetEquippedArmorIndex(), 0, 2)));
				PreviewActor->SetWeaponFinish(static_cast<EOperatorWeaponFinish>(FMath::Clamp(Economy->GetEquippedWeaponFinishIndex(), 0, 3)));
			}
		}
	}
}

TSharedRef<SWidget> UMainMenuWidget::RebuildWidget()
{
	return SNew(SOverlay)
		// Layer 0: Root Vignette / Scanline overlay (no center fill - 100% transparent center for 3D world!)
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor(0.01f, 0.02f, 0.03f, 0.12f))
			.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
			.Padding(0.0f)
		]
		// Layer 1: Top Glowing Accent Stripe (Tactical Cyan #00E5FF)
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Top)
		[
			SNew(SBox)
			.HeightOverride(2.0f)
			[
				SNew(SBorder)
				.BorderBackgroundColor(MWTheme::CyanAccent)
				.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
			]
		]
		// Layer 2: Main Tactical HUD Container
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		.Padding(24.0f, 16.0f, 24.0f, 12.0f)
		[
			SNew(SVerticalBox)
			// Zone 1: Top Navigation Bar
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 14.0f)
			[
				SNew(SBackgroundBlur)
				.BlurStrength(14.0f)
				.bApplyAlphaToBlur(true)
				.Padding(0.0f)
				[
					SNew(SBorder)
					.BorderBackgroundColor(MWTheme::DarkGlass)
					.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
					.Padding(18.0f, 10.0f)
					[
						BuildHeader()
					]
				]
			]
			// Content Switcher for Tab switching
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SAssignNew(ContentSwitcher, SWidgetSwitcher)
				.WidgetIndex(0)
				+ SWidgetSwitcher::Slot() [ BuildTacticalLobbyView() ]    // 0: Modern Warfare 3-Zone Command Lobby
				+ SWidgetSwitcher::Slot() [ BuildGunsmithView() ]         // 1: Gunsmith Loadouts
				+ SWidgetSwitcher::Slot() [ BuildServerBrowserView() ]     // 2: VPS Server Browser
				+ SWidgetSwitcher::Slot() [ BuildSettingsView() ]          // 3: Settings Menu
				+ SWidgetSwitcher::Slot() [ BuildDailyRewardsView() ]      // 4: Daily Rewards & Armory Vault
			]
			// Zone 4: Translucent Footer Status Bar
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 12.0f, 0.0f, 0.0f)
			[
				SNew(SBackgroundBlur)
				.BlurStrength(14.0f)
				.bApplyAlphaToBlur(true)
				.Padding(0.0f)
				[
					SNew(SBorder)
					.BorderBackgroundColor(MWTheme::DarkGlass)
					.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
					.Padding(16.0f, 6.0f)
					[
						BuildFooter()
					]
				]
			]
		];
}

TSharedRef<SWidget> UMainMenuWidget::BuildHeader()
{
	auto MakeTabBtn = [this](const FString& TabText, EFPSMenuSection Section, TSharedPtr<SBorder>& OutIndicator) -> TSharedRef<SWidget>
	{
		return SNew(SBox)
			.HeightOverride(42.0f)
			.Padding(6.0f, 0.0f)
			[
				SNew(SButton)
				.ButtonColorAndOpacity(FLinearColor::Transparent)
				.ContentPadding(0.0f)
				.OnClicked_Lambda([this, Section]() { return HandleTabClicked(Section); })
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.FillHeight(1.0f)
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Center)
					.Padding(16.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TabText))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
						.ColorAndOpacity(MWTheme::TextPrimary)
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SAssignNew(OutIndicator, SBorder)
						.BorderBackgroundColor(Section == EFPSMenuSection::Play ? MWTheme::CyanAccent : FLinearColor::Transparent)
						.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
						.Padding(0.0f)
						[
							SNew(SBox).HeightOverride(2.0f)
						]
					]
				]
			];
	};

	return SNew(SHorizontalBox)
		// Left: Title & Subtitle with glowing tactical bracket
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, 10.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("// ")))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 18))
				.ColorAndOpacity(MWTheme::CyanAccent)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("NOBLE FERMI")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 20))
						.ColorAndOpacity(MWTheme::AmberAccent)
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(8.0f, 0.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("// TACTICAL OPS")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 20))
						.ColorAndOpacity(MWTheme::CyanAccent)
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 1.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("TACTICAL MILITARY WARFARE // TASK FORCE ECHO")))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
					.ColorAndOpacity(MWTheme::TextMuted)
				]
			]
		]
		// Center: Modern Warfare Tabs (PLAY, OPERATOR, GUNSMITH, DAILY VAULT, SETTINGS)
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth() [ MakeTabBtn(TEXT("PLAY"), EFPSMenuSection::Play, TabIndicatorPlay) ]
			+ SHorizontalBox::Slot().AutoWidth() [ MakeTabBtn(TEXT("OPERATOR"), EFPSMenuSection::Customization, TabIndicatorOperator) ]
			+ SHorizontalBox::Slot().AutoWidth() [ MakeTabBtn(TEXT("GUNSMITH"), EFPSMenuSection::Loadout, TabIndicatorGunsmith) ]
			+ SHorizontalBox::Slot().AutoWidth() [ MakeTabBtn(TEXT("DAILY VAULT"), EFPSMenuSection::DailyRewards, TabIndicatorDailyRewards) ]
			+ SHorizontalBox::Slot().AutoWidth() [ MakeTabBtn(TEXT("SETTINGS"), EFPSMenuSection::Settings, TabIndicatorSettings) ]
		]
		// Right: Persistent Balances & Live Telemetry Badge
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Center)
		[
			SNew(SHorizontalBox)
			// Gold Currency Badge
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, 8.0f, 0.0f)
			[
				SNew(SBorder)
				.BorderBackgroundColor(MWTheme::DarkGlassCard)
				.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
				.Padding(10.0f, 6.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 6.0f, 0.0f)
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("🪙"))).Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SAssignNew(GoldBalanceText, STextBlock)
						.Text(FText::FromString(TEXT("1,500 GOLD")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
						.ColorAndOpacity(MWTheme::AmberAccent)
					]
				]
			]
			// Opal Currency Badge
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, 12.0f, 0.0f)
			[
				SNew(SBorder)
				.BorderBackgroundColor(MWTheme::DarkGlassCard)
				.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
				.Padding(10.0f, 6.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 6.0f, 0.0f)
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("💎"))).Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SAssignNew(OpalBalanceText, STextBlock)
						.Text(FText::FromString(TEXT("50 OPALS")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
						.ColorAndOpacity(MWTheme::CyanAccent)
					]
				]
			]
			// Live Telemetry Badge
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SBorder)
				.BorderBackgroundColor(MWTheme::DarkGlassCard)
				.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
				.Padding(12.0f, 7.0f)
				[
					SNew(SHorizontalBox)
					// Emerald live pip
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(0.0f, 0.0f, 8.0f, 0.0f)
					[
						SNew(SBox)
						.WidthOverride(8.0f)
						.HeightOverride(8.0f)
						[
							SNew(SBorder)
							.BorderBackgroundColor(MWTheme::EmeraldReady)
							.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
						]
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("128 HZ TICK // IOSTORE VERIFIED // REGION: EU-WEST")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
						.ColorAndOpacity(MWTheme::TextPrimary)
					]
				]
			]
		];
}

TSharedRef<SWidget> UMainMenuWidget::BuildTacticalLobbyView()
{
	return SNew(SHorizontalBox)
		// Zone 2: Left Blade (Deployment & Mode Cards) ~380px
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(0.0f, 0.0f, 16.0f, 0.0f)
		[
			SNew(SBox)
			.WidthOverride(380.0f)
			[
				SNew(SBackgroundBlur)
				.BlurStrength(14.0f)
				.bApplyAlphaToBlur(true)
				.Padding(0.0f)
				[
					SNew(SBorder)
					.BorderBackgroundColor(MWTheme::DarkGlass)
					.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
					.Padding(16.0f)
					[
						BuildLeftBlade()
					]
				]
			]
		]
		// Zone 3: Center Viewport (Live 3D Operator Showcase) - Completely unobstructed 50%+ width!
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			BuildCenterViewportHUD()
		]
		// Zone 4: Right Blade (Live Operator & Camo Inspector) ~380px
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(16.0f, 0.0f, 0.0f, 0.0f)
		[
			SNew(SBox)
			.WidthOverride(380.0f)
			[
				SNew(SBackgroundBlur)
				.BlurStrength(14.0f)
				.bApplyAlphaToBlur(true)
				.Padding(0.0f)
				[
					SNew(SBorder)
					.BorderBackgroundColor(MWTheme::DarkGlass)
					.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
					.Padding(16.0f)
					[
						BuildRightBlade()
					]
				]
			]
		];
}

TSharedRef<SWidget> UMainMenuWidget::BuildLeftBlade()
{
	auto MakeTacticalModeCard = [this](int32 ModeIdx, const FString& Tag, const FLinearColor& TagColor, const FString& Title, const FString& Meta, const FString& Desc, TSharedPtr<SBorder>& OutBorder) -> TSharedRef<SWidget>
	{
		return SNew(SBox)
			.Padding(0.0f, 0.0f, 0.0f, 8.0f)
			[
				SNew(SButton)
				.ButtonColorAndOpacity(FLinearColor::Transparent)
				.ContentPadding(0.0f)
				.OnClicked_Lambda([this, ModeIdx]() { return HandleModeCardClicked(ModeIdx); })
				[
					SAssignNew(OutBorder, SBorder)
					.BorderBackgroundColor(SelectedModeIndex == ModeIdx ? MWTheme::ActiveBorder : MWTheme::GlassBorder)
					.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
					.Padding(1.5f)
					[
						SNew(SBorder)
						.BorderBackgroundColor(MWTheme::DarkGlassCard)
						.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
						.Padding(12.0f, 10.0f)
						[
							SNew(SVerticalBox)
							// Header row: Status tag + Index
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0.0f, 0.0f, 0.0f, 4.0f)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
								.AutoWidth()
								.VAlign(VAlign_Center)
								[
									SNew(SBorder)
									.BorderBackgroundColor(TagColor.CopyWithNewOpacity(0.20f))
									.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
									.Padding(6.0f, 2.0f)
									[
										SNew(STextBlock)
										.Text(FText::FromString(Tag))
										.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
										.ColorAndOpacity(TagColor)
									]
								]
								+ SHorizontalBox::Slot()
								.FillWidth(1.0f)
								.HAlign(HAlign_Right)
								.VAlign(VAlign_Center)
								[
									SNew(STextBlock)
									.Text(FText::FromString(FString::Printf(TEXT("// 0%d"), ModeIdx + 1)))
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
									.ColorAndOpacity(MWTheme::TextMuted)
								]
							]
							// Title
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(STextBlock)
								.Text(FText::FromString(Title))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
								.ColorAndOpacity(MWTheme::TextPrimary)
							]
							// Metadata
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0.0f, 3.0f, 0.0f, 2.0f)
							[
								SNew(STextBlock)
								.Text(FText::FromString(Meta))
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
								.ColorAndOpacity(MWTheme::CyanAccent)
							]
							// Description
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(STextBlock)
								.Text(FText::FromString(Desc))
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
								.ColorAndOpacity(MWTheme::TextSecondary)
								.AutoWrapText(true)
							]
						]
					]
				]
			];
	};

	return SNew(SScrollBox)
		+ SScrollBox::Slot()
		[
			SNew(SVerticalBox)
			// Header
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 10.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("/// DEPLOYMENT SECTOR")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
					.ColorAndOpacity(MWTheme::CyanAccent)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 2.0f, 0.0f, 4.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("SELECT COMBAT THEATER & OBJECTIVE")))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
					.ColorAndOpacity(MWTheme::TextMuted)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SBox)
					.HeightOverride(1.0f)
					[
						SNew(SBorder)
						.BorderBackgroundColor(MWTheme::CyanAccent.CopyWithNewOpacity(0.5f))
						.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
					]
				]
			]
			// Mode Cards
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				MakeTacticalModeCard(
					0,
					TEXT("[ ACTIVE ROTATION ]"),
					MWTheme::EmeraldReady,
					TEXT("RUST TACTICAL 60M ARENA"),
					TEXT("MAP: RUST // 6v6 CQB // 128 HZ TICK // TDM 75"),
					TEXT("High-tempo industrial CQB arena. Fast engagements, container cover, vertical crane sightlines."),
					ModeCardBorder0
				)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				MakeTacticalModeCard(
					1,
					TEXT("[ ANOMALY TESTBED ]"),
					MWTheme::CyanAccent,
					TEXT("FERMI TIME-DILATION LAB"),
					TEXT("MAP: RESEARCH COMPLEX // 4v4 TACTICAL // CHRONO FIELD"),
					TEXT("Localized time-dilation bubbles altering projectile flight ballistics, weapon cycles, and movement."),
					ModeCardBorder1
				)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				MakeTacticalModeCard(
					2,
					TEXT("[ COMBAT TRAINING ]"),
					MWTheme::AmberAccent,
					TEXT("BOT SKIRMISH"),
					TEXT("PVE OFFLINE // AUTHORITATIVE BOT RIG // DIFFICULTY: HARD"),
					TEXT("Offline tactical drills against authoritative AI operators for zeroing weapon handling and recoil."),
					ModeCardBorder2
				)
			]
			// Prominent Glowing Amber Hero Button (DEPLOY TO ARENA)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 8.0f, 0.0f, 10.0f)
			[
				SNew(SBox)
				.HeightOverride(52.0f)
				[
					SNew(SButton)
					.ButtonColorAndOpacity(FLinearColor::Transparent)
					.ContentPadding(0.0f)
					.OnClicked_Lambda([this]() { return HandleQuickplayClicked(); })
					[
						SNew(SBorder)
						.BorderBackgroundColor(MWTheme::AmberAccent)
						.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
						.Padding(2.0f)
						[
							SNew(SBorder)
							.BorderBackgroundColor(MWTheme::AmberAccent)
							.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							.Padding(12.0f, 4.0f)
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot()
								.AutoHeight()
								.HAlign(HAlign_Center)
								[
									SNew(SHorizontalBox)
									+ SHorizontalBox::Slot()
									.AutoWidth()
									.VAlign(VAlign_Center)
									[
										SNew(STextBlock)
										.Text(FText::FromString(TEXT("DEPLOY TO ARENA")))
										.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
										.ColorAndOpacity(FLinearColor(0.04f, 0.05f, 0.07f, 1.0f))
									]
									+ SHorizontalBox::Slot()
									.AutoWidth()
									.VAlign(VAlign_Center)
									.Padding(8.0f, 0.0f, 0.0f, 0.0f)
									[
										SNew(STextBlock)
										.Text(FText::FromString(TEXT("►")))
										.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
										.ColorAndOpacity(FLinearColor(0.04f, 0.05f, 0.07f, 1.0f))
									]
								]
								+ SVerticalBox::Slot()
								.AutoHeight()
								.HAlign(HAlign_Center)
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("128 HZ TICK // AUTHORITATIVE SERVER")))
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
									.ColorAndOpacity(FLinearColor(0.20f, 0.16f, 0.05f, 1.0f))
								]
							]
						]
					]
				]
			]
			// Secondary Action: Host Listen Server
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 6.0f)
			[
				SNew(SBox)
				.HeightOverride(36.0f)
				[
					SNew(SButton)
					.ButtonColorAndOpacity(FLinearColor::Transparent)
					.ContentPadding(0.0f)
					.OnClicked_Lambda([this]() { return HandleHostListenClicked(); })
					[
						SNew(SBorder)
						.BorderBackgroundColor(MWTheme::GlassBorder)
						.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
						.Padding(1.0f)
						[
							SNew(SBorder)
							.BorderBackgroundColor(MWTheme::DarkGlassCard)
							.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("HOST 128-HZ LISTEN ROOM")))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
								.ColorAndOpacity(MWTheme::CyanAccent)
							]
						]
					]
				]
			]
			// Direct Connect Section
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 4.0f, 0.0f, 8.0f)
			[
				SNew(SBorder)
				.BorderBackgroundColor(MWTheme::GlassBorder)
				.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
				.Padding(1.0f)
				[
					SNew(SBorder)
					.BorderBackgroundColor(MWTheme::DarkGlassCard)
					.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
					.Padding(10.0f, 8.0f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 0.0f, 0.0f, 4.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("DIRECT VPS DEDICATED IP:")))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
							.ColorAndOpacity(MWTheme::TextMuted)
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.FillWidth(1.0f)
							.Padding(0.0f, 0.0f, 6.0f, 0.0f)
							[
								SAssignNew(DirectConnectInput, SEditableTextBox)
								.Text(FText::FromString(TEXT("127.0.0.1:7777")))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
								.HintText(FText::FromString(TEXT("IP:PORT")))
							]
							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(SBox)
								.WidthOverride(70.0f)
								.HeightOverride(30.0f)
								[
									SNew(SButton)
									.ButtonColorAndOpacity(MWTheme::CyanAccent)
									.OnClicked_Lambda([this]() { return HandleDirectConnectClicked(); })
									.HAlign(HAlign_Center)
									.VAlign(VAlign_Center)
									[
										SNew(STextBlock)
										.Text(FText::FromString(TEXT("JOIN")))
										.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
										.ColorAndOpacity(FLinearColor(0.02f, 0.04f, 0.06f, 1.0f))
									]
								]
							]
						]
					]
				]
			]
			// Active Daily Contracts // Bounty Board Panel
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 4.0f, 0.0f, 8.0f)
			[
				BuildBountyBoardPanel()
			]
			// Quit to Desktop Button in Crimson Alert (#FF2A54)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 4.0f, 0.0f, 0.0f)
			[
				SNew(SBox)
				.HeightOverride(36.0f)
				[
					SNew(SButton)
					.ButtonColorAndOpacity(FLinearColor::Transparent)
					.ContentPadding(0.0f)
					.OnClicked_Lambda([this]() { return HandleQuitClicked(); })
					[
						SNew(SBorder)
						.BorderBackgroundColor(MWTheme::CrimsonAlert.CopyWithNewOpacity(0.6f))
						.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
						.Padding(1.0f)
						[
							SNew(SBorder)
							.BorderBackgroundColor(FLinearColor(0.35f, 0.06f, 0.12f, 0.85f))
							.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("QUIT TO DESKTOP")))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
								.ColorAndOpacity(FLinearColor(1.0f, 0.8f, 0.8f, 1.0f))
							]
						]
					]
				]
			]
		];
}

TSharedRef<SWidget> UMainMenuWidget::BuildBountyBoardPanel()
{
	auto MakeBountyRow = [this](int32 SlotIdx, const FString& FallbackTitle, const FString& FallbackDesc, int32 DefaultTarget, int32 DefaultGold, int32 DefaultOpals) -> TSharedRef<SWidget>
	{
		return SNew(SBox)
			.Padding(0.0f, 0.0f, 0.0f, 6.0f)
			[
				SNew(SBorder)
				.BorderBackgroundColor_Lambda([this, SlotIdx]()
				{
					if (UGameInstance* GI = GetGameInstance())
					{
						if (UFPSEconomySubsystem* Economy = GI->GetSubsystem<UFPSEconomySubsystem>())
						{
							const TArray<FFPSTacticalBounty>& Bounties = Economy->GetActiveBounties();
							if (Bounties.IsValidIndex(SlotIdx) && Bounties[SlotIdx].bCompleted)
							{
								return MWTheme::EmeraldReady;
							}
						}
					}
					return MWTheme::GlassBorder;
				})
				.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
				.Padding(1.0f)
				[
					SNew(SBorder)
					.BorderBackgroundColor(MWTheme::DarkGlassCard)
					.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
					.Padding(8.0f, 6.0f)
					[
						SNew(SVerticalBox)
						// Title + Reward Badges
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.FillWidth(1.0f)
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text_Lambda([this, SlotIdx, FallbackTitle]()
								{
									if (UGameInstance* GI = GetGameInstance())
									{
										if (UFPSEconomySubsystem* Economy = GI->GetSubsystem<UFPSEconomySubsystem>())
										{
											const TArray<FFPSTacticalBounty>& Bounties = Economy->GetActiveBounties();
											if (Bounties.IsValidIndex(SlotIdx))
											{
												return Bounties[SlotIdx].Title;
											}
										}
									}
									return FText::FromString(FallbackTitle);
								})
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
								.ColorAndOpacity(MWTheme::TextPrimary)
							]
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text_Lambda([this, SlotIdx, DefaultGold, DefaultOpals]()
								{
									int32 GoldAmt = DefaultGold;
									int32 OpalAmt = DefaultOpals;
									if (UGameInstance* GI = GetGameInstance())
									{
										if (UFPSEconomySubsystem* Economy = GI->GetSubsystem<UFPSEconomySubsystem>())
										{
											const TArray<FFPSTacticalBounty>& Bounties = Economy->GetActiveBounties();
											if (Bounties.IsValidIndex(SlotIdx))
											{
												GoldAmt = Bounties[SlotIdx].GoldReward;
												OpalAmt = Bounties[SlotIdx].OpalReward;
											}
										}
									}
									if (OpalAmt > 0)
									{
										return FText::FromString(FString::Printf(TEXT("+%dG + %d💎"), GoldAmt, OpalAmt));
									}
									return FText::FromString(FString::Printf(TEXT("+%d GOLD"), GoldAmt));
								})
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
								.ColorAndOpacity(MWTheme::AmberAccent)
							]
						]
						// Objective Description
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 2.0f, 0.0f, 4.0f)
						[
							SNew(STextBlock)
							.Text_Lambda([this, SlotIdx, FallbackDesc]()
							{
								if (UGameInstance* GI = GetGameInstance())
								{
									if (UFPSEconomySubsystem* Economy = GI->GetSubsystem<UFPSEconomySubsystem>())
									{
										const TArray<FFPSTacticalBounty>& Bounties = Economy->GetActiveBounties();
										if (Bounties.IsValidIndex(SlotIdx))
										{
											return Bounties[SlotIdx].Description;
										}
									}
								}
								return FText::FromString(FallbackDesc);
							})
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
							.ColorAndOpacity(MWTheme::TextSecondary)
							.AutoWrapText(true)
						]
						// Vector Progress Bar + Numeric Counter + Action/Claim State
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.FillWidth(1.0f)
							.VAlign(VAlign_Center)
							.Padding(0.0f, 0.0f, 8.0f, 0.0f)
							[
								SNew(SBox)
								.HeightOverride(8.0f)
								[
									SNew(SFPSBountyProgressBarWidget)
									.Fraction_Lambda([this, SlotIdx, DefaultTarget]()
									{
										if (UGameInstance* GI = GetGameInstance())
										{
											if (UFPSEconomySubsystem* Economy = GI->GetSubsystem<UFPSEconomySubsystem>())
											{
												const TArray<FFPSTacticalBounty>& Bounties = Economy->GetActiveBounties();
												if (Bounties.IsValidIndex(SlotIdx) && Bounties[SlotIdx].TargetProgress > 0)
												{
													return float(Bounties[SlotIdx].CurrentProgress) / float(Bounties[SlotIdx].TargetProgress);
												}
											}
										}
										return 0.0f;
									})
									.bCompleted_Lambda([this, SlotIdx]()
									{
										if (UGameInstance* GI = GetGameInstance())
										{
											if (UFPSEconomySubsystem* Economy = GI->GetSubsystem<UFPSEconomySubsystem>())
											{
												const TArray<FFPSTacticalBounty>& Bounties = Economy->GetActiveBounties();
												if (Bounties.IsValidIndex(SlotIdx))
												{
													return Bounties[SlotIdx].bCompleted;
												}
											}
										}
										return false;
									})
								]
							]
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							.Padding(0.0f, 0.0f, 6.0f, 0.0f)
							[
								SNew(STextBlock)
								.Text_Lambda([this, SlotIdx, DefaultTarget]()
								{
									int32 Cur = 0;
									int32 Tgt = DefaultTarget;
									bool bDone = false;
									if (UGameInstance* GI = GetGameInstance())
									{
										if (UFPSEconomySubsystem* Economy = GI->GetSubsystem<UFPSEconomySubsystem>())
										{
											const TArray<FFPSTacticalBounty>& Bounties = Economy->GetActiveBounties();
											if (Bounties.IsValidIndex(SlotIdx))
											{
												Cur = Bounties[SlotIdx].CurrentProgress;
												Tgt = Bounties[SlotIdx].TargetProgress;
												bDone = Bounties[SlotIdx].bCompleted;
											}
										}
									}
									return FText::FromString(bDone ? FString::Printf(TEXT("COMPLETED (%d/%d)"), Cur, Tgt) : FString::Printf(TEXT("%d / %d"), Cur, Tgt));
								})
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
								.ColorAndOpacity_Lambda([this, SlotIdx]()
								{
									if (UGameInstance* GI = GetGameInstance())
									{
										if (UFPSEconomySubsystem* Economy = GI->GetSubsystem<UFPSEconomySubsystem>())
										{
											const TArray<FFPSTacticalBounty>& Bounties = Economy->GetActiveBounties();
											if (Bounties.IsValidIndex(SlotIdx) && Bounties[SlotIdx].bCompleted)
											{
												return MWTheme::EmeraldReady;
											}
										}
									}
									return MWTheme::CyanAccent;
								})
							]
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							[
								SNew(SButton)
								.ButtonColorAndOpacity(MWTheme::DarkGlass)
								.ContentPadding(FMargin(6.0f, 2.0f))
								.ToolTipText(FText::FromString(TEXT("Simulate / Claim tactical contract progress")))
								.OnClicked_Lambda([this, SlotIdx]() { return HandleSimulateBountyProgressClicked(SlotIdx); })
								[
									SNew(STextBlock)
									.Text_Lambda([this, SlotIdx]()
									{
										if (UGameInstance* GI = GetGameInstance())
										{
											if (UFPSEconomySubsystem* Economy = GI->GetSubsystem<UFPSEconomySubsystem>())
											{
												const TArray<FFPSTacticalBounty>& Bounties = Economy->GetActiveBounties();
												if (Bounties.IsValidIndex(SlotIdx) && Bounties[SlotIdx].bCompleted)
												{
													return FText::FromString(TEXT("CLAIMED ✓"));
												}
											}
										}
										return FText::FromString(TEXT("+1 TEST"));
									})
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 7))
									.ColorAndOpacity(MWTheme::EmeraldReady)
								]
							]
						]
					]
				]
			];
	};

	return SNew(SBorder)
		.BorderBackgroundColor(MWTheme::GlassBorder)
		.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
		.Padding(1.0f)
		[
			SNew(SBorder)
			.BorderBackgroundColor(MWTheme::DarkGlass)
			.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
			.Padding(10.0f, 8.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 6.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("ACTIVE DAILY CONTRACTS // BOUNTY BOARD")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
						.ColorAndOpacity(MWTheme::AmberAccent)
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(SButton)
						.ButtonColorAndOpacity(MWTheme::DarkGlassCard)
						.ContentPadding(FMargin(6.0f, 2.0f))
						.OnClicked_Lambda([this]() { return HandleRerollBountiesClicked(); })
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("↻ REROLL: 250 GOLD")))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 7))
							.ColorAndOpacity(MWTheme::CyanAccent)
						]
					]
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					MakeBountyRow(0, TEXT("CHRONO-HUNTER"), TEXT("Get 3 Eliminations while mid-air in Chrono-Dive"), 3, 1200, 0)
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					MakeBountyRow(1, TEXT("QUANTUM DECEPTION"), TEXT("Swap with a Fermi Decoy and eliminate a hostile within 4 seconds"), 2, 1500, 5)
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					MakeBountyRow(2, TEXT("UNTOUCHABLE"), TEXT("Dodge 10 supersonic slugs in Fermi Dilation or shatter 3 enemy armor plates"), 10, 1000, 3)
				]
			]
		];
}

TSharedRef<SWidget> UMainMenuWidget::BuildCenterViewportHUD()
{
	return SNew(SVerticalBox)
		// Empty fill slot: Leaves the center 50%+ completely unobstructed for AFPSCharacterPreviewActor!
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SNew(SBox)
		]
		// Minimal floating acrylic telemetry pill at bottom of center viewport
		+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Center)
		.Padding(0.0f, 0.0f, 0.0f, 10.0f)
		[
			SNew(SBackgroundBlur)
			.BlurStrength(14.0f)
			.bApplyAlphaToBlur(true)
			.Padding(0.0f)
			[
				SNew(SBorder)
				.BorderBackgroundColor(MWTheme::DarkGlass)
				.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
				.Padding(1.0f)
				[
					SNew(SBorder)
					.BorderBackgroundColor(MWTheme::DarkGlassCard)
					.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
					.Padding(16.0f, 8.0f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(0.0f, 0.0f, 8.0f, 0.0f)
						[
							SNew(SBox)
							.WidthOverride(8.0f)
							.HeightOverride(8.0f)
							[
								SNew(SBorder)
								.BorderBackgroundColor(MWTheme::CyanAccent)
								.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
							]
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SAssignNew(CenterPillText, STextBlock)
							.Text(FText::FromString(TEXT("OPERATOR: TASK FORCE ECHO // LOADOUT: NF-98 CARBINE // 360° TURNTABLE")))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
							.ColorAndOpacity(MWTheme::TextPrimary)
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(14.0f, 0.0f, 0.0f, 0.0f)
						[
							SNew(SButton)
							.ButtonColorAndOpacity(MWTheme::DarkGlass)
							.OnClicked_Lambda([this]() { return HandleToggleTurntableRotation(); })
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("⟳ TURNTABLE")))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
								.ColorAndOpacity(MWTheme::CyanAccent)
							]
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(6.0f, 0.0f, 0.0f, 0.0f)
						[
							SNew(SButton)
							.ButtonColorAndOpacity(MWTheme::DarkGlass)
							.OnClicked_Lambda([this]() { return HandleResetTurntableFacing(); })
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("0° RESET")))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
								.ColorAndOpacity(MWTheme::TextSecondary)
							]
						]
					]
				]
			]
		];
}

TSharedRef<SWidget> UMainMenuWidget::BuildRightBlade()
{
	auto MakeCamoSwatchButton = [this](uint8 CamoIdx, const FString& ItemID, const FString& Name, const FString& Subtitle, const FLinearColor& SwatchColor, int32 GoldPrice, int32 OpalPrice) -> TSharedRef<SWidget>
	{
		UFPSEconomySubsystem* Economy = nullptr;
		if (UGameInstance* GI = GetGameInstance())
		{
			Economy = GI->GetSubsystem<UFPSEconomySubsystem>();
		}
		const bool bIsUnlocked = (Economy == nullptr || Economy->IsItemUnlocked(ItemID));

		return SNew(SBox)
			.HeightOverride(42.0f)
			.Padding(0.0f, 0.0f, 0.0f, 5.0f)
			[
				SNew(SBorder)
				.BorderBackgroundColor(MWTheme::GlassBorder)
				.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
				.Padding(1.0f)
				[
					SNew(SBorder)
					.BorderBackgroundColor(MWTheme::DarkGlassCard)
					.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
					.Padding(10.0f, 4.0f)
					[
						SNew(SHorizontalBox)
						// Clickable Preview Area (Swatch Chip + Name & Subtitle for Live 360° Turntable Preview)
						+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						.VAlign(VAlign_Center)
						[
							SNew(SButton)
							.ButtonColorAndOpacity(FLinearColor::Transparent)
							.ContentPadding(0.0f)
							.OnClicked_Lambda([this, CamoIdx]()
							{
								HandleCamoSelected(CamoIdx);
								return FReply::Handled();
							})
							[
								SNew(SHorizontalBox)
								// Color Swatch Chip
								+ SHorizontalBox::Slot()
								.AutoWidth()
								.VAlign(VAlign_Center)
								.Padding(0.0f, 0.0f, 10.0f, 0.0f)
								[
									SNew(SBox)
									.WidthOverride(16.0f)
									.HeightOverride(16.0f)
									[
										SNew(SBorder)
										.BorderBackgroundColor(SwatchColor)
										.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
									]
								]
								// Title & Subtitle
								+ SHorizontalBox::Slot()
								.FillWidth(1.0f)
								.VAlign(VAlign_Center)
								[
									SNew(SVerticalBox)
									+ SVerticalBox::Slot()
									.AutoHeight()
									[
										SNew(STextBlock)
										.Text(FText::FromString(Name))
										.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
										.ColorAndOpacity(MWTheme::TextPrimary)
									]
									+ SVerticalBox::Slot()
									.AutoHeight()
									[
										SNew(STextBlock)
										.Text(FText::FromString(Subtitle))
										.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
										.ColorAndOpacity(MWTheme::TextMuted)
									]
								]
							]
						]
						// Action / Purchase Buttons
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							bIsUnlocked
								? StaticCastSharedRef<SWidget>(
									SNew(SButton)
									.ButtonColorAndOpacity(MWTheme::DarkGlass)
									.ContentPadding(FMargin(8.0f, 2.0f))
									.OnClicked_Lambda([this, CamoIdx]() { return HandleCamoSelected(CamoIdx); })
									[
										SNew(STextBlock)
										.Text(FText::FromString(TEXT("APPLY >")))
										.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
										.ColorAndOpacity(MWTheme::CyanAccent)
									]
								)
								: StaticCastSharedRef<SWidget>(
									SNew(SHorizontalBox)
									+ SHorizontalBox::Slot()
									.AutoWidth()
									.Padding(0.0f, 0.0f, 4.0f, 0.0f)
									[
										SNew(SButton)
										.ButtonColorAndOpacity(MWTheme::DarkGlass)
										.ContentPadding(FMargin(6.0f, 2.0f))
										.ToolTipText(FText::FromString(TEXT("Unlock permanently with Gold")))
										.OnClicked_Lambda([this, ItemID, CamoIdx]()
										{
											HandleUnlockCustomizationItem(ItemID, false);
											HandleCamoSelected(CamoIdx);
											return FReply::Handled();
										})
										[
											SNew(STextBlock)
											.Text(FText::FromString(FString::Printf(TEXT("🪙 %d"), GoldPrice)))
											.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
											.ColorAndOpacity(MWTheme::AmberAccent)
										]
									]
									+ SHorizontalBox::Slot()
									.AutoWidth()
									[
										SNew(SButton)
										.ButtonColorAndOpacity(MWTheme::DarkGlass)
										.ContentPadding(FMargin(6.0f, 2.0f))
										.ToolTipText(FText::FromString(TEXT("Unlock permanently with Opals")))
										.OnClicked_Lambda([this, ItemID, CamoIdx]()
										{
											HandleUnlockCustomizationItem(ItemID, true);
											HandleCamoSelected(CamoIdx);
											return FReply::Handled();
										})
										[
											SNew(STextBlock)
											.Text(FText::FromString(FString::Printf(TEXT("💎 %d"), OpalPrice)))
											.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
											.ColorAndOpacity(MWTheme::CyanAccent)
										]
									]
								)
						]
					]
				]
			];
	};

	auto MakePurchasableItemBtn = [this](const FString& ItemID, const FString& DisplayName, int32 GoldPrice, int32 OpalPrice, TFunction<FReply()> OnSelect) -> TSharedRef<SWidget>
	{
		UFPSEconomySubsystem* Economy = nullptr;
		if (UGameInstance* GI = GetGameInstance())
		{
			Economy = GI->GetSubsystem<UFPSEconomySubsystem>();
		}
		const bool bIsUnlocked = (Economy == nullptr || Economy->IsItemUnlocked(ItemID));

		return SNew(SBox)
			.HeightOverride(34.0f)
			.Padding(0.0f, 0.0f, 0.0f, 4.0f)
			[
				SNew(SBorder)
				.BorderBackgroundColor(MWTheme::GlassBorder)
				.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
				.Padding(1.0f)
				[
					SNew(SBorder)
					.BorderBackgroundColor(MWTheme::DarkGlassCard)
					.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
					.Padding(8.0f, 0.0f)
					.VAlign(VAlign_Center)
					[
						SNew(SHorizontalBox)
						// Clickable Preview Area (Click to preview on 360° turntable)
						+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						.VAlign(VAlign_Center)
						[
							SNew(SButton)
							.ButtonColorAndOpacity(FLinearColor::Transparent)
							.ContentPadding(0.0f)
							.OnClicked_Lambda(OnSelect)
							[
								SNew(STextBlock)
								.Text(FText::FromString(DisplayName))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
								.ColorAndOpacity(MWTheme::TextSecondary)
							]
						]
						// Action / Purchase Buttons
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							bIsUnlocked
								? StaticCastSharedRef<SWidget>(
									SNew(SButton)
									.ButtonColorAndOpacity(MWTheme::DarkGlass)
									.ContentPadding(FMargin(8.0f, 2.0f))
									.OnClicked_Lambda(OnSelect)
									[
										SNew(STextBlock)
										.Text(FText::FromString(TEXT("EQUIP")))
										.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
										.ColorAndOpacity(MWTheme::CyanAccent)
									]
								)
								: StaticCastSharedRef<SWidget>(
									SNew(SHorizontalBox)
									+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 4.0f, 0.0f)
									[
										SNew(SButton)
										.ButtonColorAndOpacity(MWTheme::DarkGlass)
										.ContentPadding(FMargin(6.0f, 2.0f))
										.ToolTipText(FText::FromString(TEXT("Unlock permanently with Gold")))
										.OnClicked_Lambda([this, ItemID, OnSelect]()
										{
											HandleUnlockCustomizationItem(ItemID, false);
											OnSelect();
											return FReply::Handled();
										})
										[
											SNew(STextBlock)
											.Text(FText::FromString(FString::Printf(TEXT("🪙 %d"), GoldPrice)))
											.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
											.ColorAndOpacity(MWTheme::AmberAccent)
										]
									]
									+ SHorizontalBox::Slot().AutoWidth()
									[
										SNew(SButton)
										.ButtonColorAndOpacity(MWTheme::DarkGlass)
										.ContentPadding(FMargin(6.0f, 2.0f))
										.ToolTipText(FText::FromString(TEXT("Unlock permanently with Opals")))
										.OnClicked_Lambda([this, ItemID, OnSelect]()
										{
											HandleUnlockCustomizationItem(ItemID, true);
											OnSelect();
											return FReply::Handled();
										})
										[
											SNew(STextBlock)
											.Text(FText::FromString(FString::Printf(TEXT("💎 %d"), OpalPrice)))
											.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
											.ColorAndOpacity(MWTheme::CyanAccent)
										]
									]
								)
						]
					]
				]
			];
	};

	return SNew(SScrollBox)
		+ SScrollBox::Slot()
		[
			SNew(SVerticalBox)
			// Header
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 10.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("/// OPERATOR & LOADOUT")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
					.ColorAndOpacity(MWTheme::CyanAccent)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 2.0f, 0.0f, 4.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("LIVE 3D MODEL TELEMETRY & CAMOUFLAGE")))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
					.ColorAndOpacity(MWTheme::TextMuted)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SBox)
					.HeightOverride(1.0f)
					[
						SNew(SBorder)
						.BorderBackgroundColor(MWTheme::CyanAccent.CopyWithNewOpacity(0.5f))
						.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
					]
				]
			]
			// 1. Semi-Transparent Loadout Card (NF-98 Carbine, Lvl IV Plates, Fermi Echo Rig)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 12.0f)
			[
				SNew(SBorder)
				.BorderBackgroundColor(MWTheme::GlassBorder)
				.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
				.Padding(1.0f)
				[
					SNew(SBorder)
					.BorderBackgroundColor(MWTheme::DarkGlassCard)
					.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
					.Padding(12.0f, 10.0f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 0.0f, 0.0f, 6.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("// ACTIVE LOADOUT CONFIGURATION")))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
							.ColorAndOpacity(MWTheme::AmberAccent)
						]
						// Primary: NF-98 Carbine
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 2.0f)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("PRIMARY: NF-98 CARBINE")))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
								.ColorAndOpacity(MWTheme::TextPrimary)
							]
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("7.62x39MM // SUPPRESSED // 40-RND // HOLO SIGHT")))
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
								.ColorAndOpacity(MWTheme::CyanAccent)
							]
						]
						// Armor: LVL IV Ceramic Plates
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 4.0f)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("ARMOR: LVL IV CERAMIC PLATES")))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
								.ColorAndOpacity(MWTheme::TextPrimary)
							]
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("FULL THORACIC PROTECTION // 250 EHP // TRAUMA ABSORPTION")))
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
								.ColorAndOpacity(MWTheme::EmeraldReady)
							]
						]
						// Rig: Fermi Echo Rig
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 2.0f)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("RIG: FERMI ECHO RIG")))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
								.ColorAndOpacity(MWTheme::TextPrimary)
							]
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("MODULAR KEVLAR HARNESS // 6x MAGS // HYDRATION BLADDER")))
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
								.ColorAndOpacity(MWTheme::TextSecondary)
							]
						]
					]
				]
			]
			// 2. Live Camo Swatch Inspector (DIGITAL, WOODLAND, DESERT, BLACKOUT)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 4.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("// LIVE CAMOUFLAGE SWATCHES")))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
				.ColorAndOpacity(MWTheme::AmberAccent)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 6.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("CYCLE SWATCH TO UPDATE 3D MODEL IN REAL TIME:")))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
				.ColorAndOpacity(MWTheme::TextMuted)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				MakeCamoSwatchButton(0, TEXT("camo_digital"), TEXT("DIGITAL"), TEXT("Urban Digital Metro // Gray & Charcoal"), FLinearColor(0.28f, 0.32f, 0.38f), 0, 0)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				MakeCamoSwatchButton(1, TEXT("camo_woodland"), TEXT("WOODLAND"), TEXT("Woodland Tactical // Olive Drab & Khaki"), FLinearColor(0.18f, 0.26f, 0.14f), 2000, 30)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				MakeCamoSwatchButton(2, TEXT("camo_desert"), TEXT("DESERT"), TEXT("Desert Spec Ops // Coyote Tan & FDE"), FLinearColor(0.50f, 0.44f, 0.32f), 2500, 40)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				MakeCamoSwatchButton(3, TEXT("camo_blackout"), TEXT("BLACKOUT"), TEXT("Stealth Blackout // Midnight Matte"), FLinearColor(0.06f, 0.06f, 0.08f), 3500, 60)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 2.0f, 0.0f, 10.0f)
			[
				SAssignNew(CamoStatusText, STextBlock)
				.Text(FText::FromString(TEXT("ACTIVE SWATCH: DIGITAL // URBAN CHARCOAL")))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
				.ColorAndOpacity(MWTheme::CyanAccent)
			]
			// 2.5 NF-98 Weapon Finish Selector (4 Finishes with Dual-Currency Purchasing & Live 360° Preview)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 4.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("// NF-98 RIFLE FINISHES")))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
				.ColorAndOpacity(MWTheme::AmberAccent)
			]
			+ SVerticalBox::Slot().AutoHeight() [ MakePurchasableItemBtn(TEXT("weapon_steel"), TEXT("FACTORY BLUED STEEL"), 0, 0, [this]() { return HandleWeaponFinishSelected(0); }) ]
			+ SVerticalBox::Slot().AutoHeight() [ MakePurchasableItemBtn(TEXT("weapon_cerakote"), TEXT("DESERT TAN CERAKOTE"), 1800, 30, [this]() { return HandleWeaponFinishSelected(1); }) ]
			+ SVerticalBox::Slot().AutoHeight() [ MakePurchasableItemBtn(TEXT("weapon_carbon"), TEXT("CARBON FIBER SPEC-OPS"), 3200, 50, [this]() { return HandleWeaponFinishSelected(2); }) ]
			+ SVerticalBox::Slot().AutoHeight() [ MakePurchasableItemBtn(TEXT("weapon_damascene"), TEXT("24K GOLD & OPAL DAMASCENE"), 6500, 90, [this]() { return HandleWeaponFinishSelected(3); }) ]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 2.0f, 0.0f, 10.0f)
			[
				SAssignNew(WeaponFinishStatusText, STextBlock)
				.Text(FText::FromString(TEXT("RIFLE FINISH: FACTORY BLUED STEEL")))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
				.ColorAndOpacity(MWTheme::CyanAccent)
			]
			// 3. Stance & Locomotion Controls
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 4.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("// OPERATOR STANCE")))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
				.ColorAndOpacity(MWTheme::AmberAccent)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 4.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(0.0f, 0.0f, 3.0f, 0.0f)
				[
					SNew(SButton)
					.ButtonColorAndOpacity(MWTheme::DarkGlassCard)
					.OnClicked_Lambda([this]()
					{
						if (PreviewActor) PreviewActor->SetPreviewStance(0);
						if (StanceStatusText.IsValid()) StanceStatusText->SetText(FText::FromString(TEXT("STANCE: AIMED ADS (SM_AK47)")));
						return FReply::Handled();
					})
					.HAlign(HAlign_Center)
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("ADS"))).Font(FCoreStyle::GetDefaultFontStyle("Bold", 8)).ColorAndOpacity(MWTheme::CyanAccent)
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.0f, 0.0f)
				[
					SNew(SButton)
					.ButtonColorAndOpacity(MWTheme::DarkGlassCard)
					.OnClicked_Lambda([this]()
					{
						if (PreviewActor) PreviewActor->SetPreviewStance(1);
						if (StanceStatusText.IsValid()) StanceStatusText->SetText(FText::FromString(TEXT("STANCE: LOW READY PATROL")));
						return FReply::Handled();
					})
					.HAlign(HAlign_Center)
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("LOW READY"))).Font(FCoreStyle::GetDefaultFontStyle("Bold", 8)).ColorAndOpacity(MWTheme::TextPrimary)
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(3.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SButton)
					.ButtonColorAndOpacity(MWTheme::DarkGlassCard)
					.OnClicked_Lambda([this]()
					{
						if (PreviewActor) PreviewActor->SetPreviewStance(2);
						if (StanceStatusText.IsValid()) StanceStatusText->SetText(FText::FromString(TEXT("STANCE: TACTICAL WALK CYCLE")));
						return FReply::Handled();
					})
					.HAlign(HAlign_Center)
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("WALK"))).Font(FCoreStyle::GetDefaultFontStyle("Bold", 8)).ColorAndOpacity(MWTheme::TextPrimary)
					]
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 10.0f)
			[
				SAssignNew(StanceStatusText, STextBlock)
				.Text(FText::FromString(TEXT("STANCE: AIMED ADS (SM_AK47)")))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
				.ColorAndOpacity(MWTheme::CyanAccent)
			]
			// 4. Headgear & Vest Quick Toggles
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 4.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("// TACTICAL GEAR VARIANTS")))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
				.ColorAndOpacity(MWTheme::AmberAccent)
			]
			+ SVerticalBox::Slot().AutoHeight() [ MakePurchasableItemBtn(TEXT("headgear_fast"), TEXT("FAST BALLISTIC HELMET"), 0, 0, [this]() { return HandleHeadgearSelected(0); }) ]
			+ SVerticalBox::Slot().AutoHeight() [ MakePurchasableItemBtn(TEXT("headgear_quadnods"), TEXT("GPNVG-18 QUAD-NODS (PHOSPHOR)"), 3000, 50, [this]() { return HandleHeadgearSelected(1); }) ]
			+ SVerticalBox::Slot().AutoHeight() [ MakePurchasableItemBtn(TEXT("headgear_balaclava"), TEXT("SPEC OPS BALACLAVA"), 1800, 25, [this]() { return HandleHeadgearSelected(2); }) ]
			+ SVerticalBox::Slot().AutoHeight() [ MakePurchasableItemBtn(TEXT("headgear_cap"), TEXT("TACTICAL CAP & PELTOR"), 1200, 20, [this]() { return HandleHeadgearSelected(3); }) ]
			+ SVerticalBox::Slot().AutoHeight() [ MakePurchasableItemBtn(TEXT("vest_level4"), TEXT("LEVEL IV HEAVY CARRIER"), 0, 0, [this]() { return HandleVestSelected(0); }) ]
			+ SVerticalBox::Slot().AutoHeight() [ MakePurchasableItemBtn(TEXT("vest_jpc"), TEXT("LIGHTWEIGHT JPC VEST"), 2200, 35, [this]() { return HandleVestSelected(1); }) ]
			+ SVerticalBox::Slot().AutoHeight() [ MakePurchasableItemBtn(TEXT("vest_recon"), TEXT("RECON TACTICAL RIG"), 2800, 45, [this]() { return HandleVestSelected(2); }) ]
			// 5. Biometrics & Skeletal Telemetry
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 6.0f, 0.0f, 0.0f)
			[
				SNew(SBorder)
				.BorderBackgroundColor(MWTheme::GlassBorder)
				.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
				.Padding(1.0f)
				[
					SNew(SBorder)
					.BorderBackgroundColor(MWTheme::DarkGlassCard)
					.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
					.Padding(10.0f, 8.0f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 4.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("// BIOMETRICS & RIGGING")))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
							.ColorAndOpacity(MWTheme::TextMuted)
						]
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("• MODEL: HUMAN MALE 180 CM // 82 KG\n"
								"• MESH: SK_BASEMALE (M_OPERATOR_BASEBODY)\n"
								"• WEAPON: SM_AK47 RIGGED TO WEAPON_R\n"
								"• IK: TWO-BONE LEFT-HAND & FOOT IK READY\n"
								"• NETWORKING: 128 HZ TICK REPLICATION")))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
							.ColorAndOpacity(MWTheme::TextSecondary)
						]
					]
				]
			]
		];
}

TSharedRef<SWidget> UMainMenuWidget::BuildGunsmithView()
{
	auto MakeArchetypeBtn = [this](int32 Idx, const FString& Name, const FString& Weapon, const FString& Role) -> TSharedRef<SWidget>
	{
		return SNew(SBox)
			.HeightOverride(52.0f)
			.Padding(0.0f, 0.0f, 0.0f, 6.0f)
			[
				SNew(SButton)
				.ButtonColorAndOpacity(FLinearColor::Transparent)
				.ContentPadding(0.0f)
				.OnClicked_Lambda([this, Idx]() { return HandleSelectArchetype(Idx); })
				[
					SNew(SBorder)
					.BorderBackgroundColor(MWTheme::GlassBorder)
					.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
					.Padding(1.0f)
					[
						SNew(SBorder)
						.BorderBackgroundColor(MWTheme::DarkGlassCard)
						.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
						.Padding(12.0f, 6.0f)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.FillWidth(1.0f)
							.VAlign(VAlign_Center)
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot()
								.AutoHeight()
								[
									SNew(STextBlock)
									.Text(FText::FromString(Name))
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
									.ColorAndOpacity(MWTheme::TextPrimary)
								]
								+ SVerticalBox::Slot()
								.AutoHeight()
								.Padding(0.0f, 2.0f, 0.0f, 0.0f)
								[
									SNew(STextBlock)
									.Text(FText::FromString(Weapon + TEXT(" // ") + Role))
									.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
									.ColorAndOpacity(MWTheme::TextMuted)
								]
							]
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("SELECT >")))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
								.ColorAndOpacity(MWTheme::CyanAccent)
							]
						]
					]
				]
			];
	};

	return SNew(SBackgroundBlur)
		.BlurStrength(14.0f)
		.bApplyAlphaToBlur(true)
		.Padding(0.0f)
		[
			SNew(SBorder)
			.BorderBackgroundColor(MWTheme::DarkGlass)
			.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
			.Padding(18.0f)
			[
				SNew(SHorizontalBox)
				// Left Archetypes List
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.0f, 0.0f, 20.0f, 0.0f)
				[
					SNew(SBox)
					.WidthOverride(360.0f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 0.0f, 0.0f, 10.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("/// GUNSMITH COMBAT LOADOUTS")))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
							.ColorAndOpacity(MWTheme::CyanAccent)
						]
						+ SVerticalBox::Slot().AutoHeight() [ MakeArchetypeBtn(0, TEXT("CLASS 1: ASSAULT"), TEXT("M4A1 Full-Auto Rifle"), TEXT("Jack of all Trades")) ]
						+ SVerticalBox::Slot().AutoHeight() [ MakeArchetypeBtn(1, TEXT("CLASS 2: SPEC OPS STEALTH"), TEXT("MP5SD Silenced SMG"), TEXT("Ghost / Flanker")) ]
						+ SVerticalBox::Slot().AutoHeight() [ MakeArchetypeBtn(2, TEXT("CLASS 3: HEAVY GUNNER"), TEXT("L86 LSW 100-Round Drum"), TEXT("Continuous Suppression")) ]
						+ SVerticalBox::Slot().AutoHeight() [ MakeArchetypeBtn(3, TEXT("CLASS 4: RECON SNIPER"), TEXT("INTERVENTION .50"), TEXT("One-Shot Lethality")) ]
						+ SVerticalBox::Slot().AutoHeight() [ MakeArchetypeBtn(4, TEXT("CLASS 5: RUN & GUN RUSHER"), TEXT("VECTOR .45 ACP"), TEXT("Extreme Rate of Fire")) ]
					]
				]
				// Right Loadout & Attachment Details
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SNew(SBorder)
					.BorderBackgroundColor(MWTheme::DarkGlassCard)
					.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
					.Padding(20.0f)
					[
						SNew(SScrollBox)
						+ SScrollBox::Slot()
						[
							SAssignNew(SelectedLoadoutDetails, STextBlock)
							.Text(FText::FromString(TEXT("SELECTED: CLASS 1 (ASSAULT)\n\n"
								"PRIMARY: M4A1 RIFLE (Attachments: Red Dot Sight, Compensator, 40-Rnd Mag, Stippled Grip)\n"
								"SECONDARY: USP .45 Tactical (Tactical Knife)\n"
								"LETHAL: Frag Grenade (4.0s cookable fuse, cooking tick)\n"
								"TACTICAL: Flashbang (Disorientation + LOS blinding)\n"
								"PERKS: Sleight of Hand Pro | Stopping Power | Ninja Pro\n\n"
								"RECOIL STATS: Kick 1.2° | ADS Speed 210ms | Effective Range 45m | Muzzle Velocity 880 m/s")))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
							.ColorAndOpacity(MWTheme::TextPrimary)
						]
					]
				]
			]
		];
}

TSharedRef<SWidget> UMainMenuWidget::BuildServerBrowserView()
{
	return SNew(SBackgroundBlur)
		.BlurStrength(14.0f)
		.bApplyAlphaToBlur(true)
		.Padding(0.0f)
		[
			SNew(SBorder)
			.BorderBackgroundColor(MWTheme::DarkGlass)
			.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
			.Padding(18.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 14.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("/// MULTIPLAYER HOSTING & VPS DIRECT CONNECT")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
					.ColorAndOpacity(MWTheme::CyanAccent)
				]
				// Server List Preview
				+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				[
					SNew(SBorder)
					.BorderBackgroundColor(MWTheme::DarkGlassCard)
					.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
					.Padding(14.0f)
					[
						SNew(SScrollBox)
						+ SScrollBox::Slot()
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0.0f, 0.0f, 0.0f, 8.0f)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("AVAILABLE REGIONAL SERVERS (128-TICK DEDICATED VPS NODES)")))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
								.ColorAndOpacity(MWTheme::AmberAccent)
							]
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0.0f, 4.0f)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("• [EU CENTRAL] Frankfurt #1 — 12/12 Players | Map: Rust Tactical | Ping: 18ms")))
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
								.ColorAndOpacity(MWTheme::TextPrimary)
							]
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0.0f, 4.0f)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("• [US EAST] Virginia #2 — 8/12 Players | Map: Terminal 24/7 | Ping: 82ms")))
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
								.ColorAndOpacity(MWTheme::TextPrimary)
							]
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0.0f, 4.0f)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("• [LOCAL VPS] 127.0.0.1:7777 — Standalone Dedicated Server Binary Ready")))
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
								.ColorAndOpacity(MWTheme::EmeraldReady)
							]
						]
					]
				]
			]
		];
}

TSharedRef<SWidget> UMainMenuWidget::BuildSettingsView()
{
	return SNew(SBackgroundBlur)
		.BlurStrength(14.0f)
		.bApplyAlphaToBlur(true)
		.Padding(0.0f)
		[
			SNew(SBorder)
			.BorderBackgroundColor(MWTheme::DarkGlass)
			.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
			.Padding(18.0f)
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 0.0f, 0.0f, 12.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("/// OPERATOR PREFERENCES & CLIENT SETTINGS")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
						.ColorAndOpacity(MWTheme::CyanAccent)
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 0.0f, 0.0f, 12.0f)
					[
						SNew(SBorder)
						.BorderBackgroundColor(MWTheme::DarkGlassCard)
						.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
						.Padding(18.0f)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("GAMEPLAY & CONTROLS:")))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
								.ColorAndOpacity(MWTheme::AmberAccent)
							]
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0.0f, 6.0f, 0.0f, 0.0f)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("• Field of View (FOV): 105° Vertical | ADS Relative Scaling Enabled\n"
									"• Mouse Sensitivity: 1.85 (Raw Input, Zero Acceleration)\n"
									"• Invert Y Axis: Disabled\n"
									"• Weapon Mount Activation: ADS + Melee Key\n"
									"• Tactical Sprint Behavior: Double Tap W / Shift")))
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
								.ColorAndOpacity(MWTheme::TextPrimary)
							]
						]
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SBorder)
						.BorderBackgroundColor(MWTheme::DarkGlassCard)
						.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
						.Padding(18.0f)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("AUDIO & ACOUSTIC SIMULATION:")))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
								.ColorAndOpacity(MWTheme::AmberAccent)
							]
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0.0f, 6.0f, 0.0f, 0.0f)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("• Master Volume: 100% | Effects: 100% | Music: 45%\n"
									"• Acoustic Occlusion: Geometric raycast attenuation enabled\n"
									"• Concussion Tinnitus: High-frequency ear ringing on close explosion\n"
									"• Hitmaker Sound: Classic MW2 high-clarity metallic chirp")))
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
								.ColorAndOpacity(MWTheme::TextPrimary)
							]
						]
					]
				]
			]
		];
}

TSharedRef<SWidget> UMainMenuWidget::BuildFooter()
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SAssignNew(NetworkStatusText, STextBlock)
			.Text(FText::FromString(TEXT("SYSTEM: READY // ENGINE: UE 5.8 // BUILD: STANDALONE WIN64 // NET: 128-HZ")))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
			.ColorAndOpacity(MWTheme::TextMuted)
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("NOBLE FERMI // MODERN WARFARE TACTICAL LOBBY")))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
			.ColorAndOpacity(MWTheme::TextMuted)
		];
}

void UMainMenuWidget::NavigateToSection(EFPSMenuSection Section)
{
	CurrentSection = Section;
	if (ContentSwitcher.IsValid())
	{
		int32 TargetIndex = 0;
		switch (Section)
		{
		case EFPSMenuSection::Home:
		case EFPSMenuSection::Play:
		case EFPSMenuSection::Customization:
			TargetIndex = 0; // Modern Warfare 3-Zone Lobby!
			break;
		case EFPSMenuSection::Loadout:
			TargetIndex = 1; // Gunsmith View
			break;
		case EFPSMenuSection::ServerBrowser:
			TargetIndex = 2; // Server Browser View
			break;
		case EFPSMenuSection::Settings:
			TargetIndex = 3; // Settings View
			break;
		case EFPSMenuSection::DailyRewards:
			TargetIndex = 4; // Daily Rewards & Armory Vault
			RefreshDailyRewardsUI();
			break;
		default:
			TargetIndex = 0;
			break;
		}
		ContentSwitcher->SetActiveWidgetIndex(TargetIndex);
	}

	UpdateTabVisuals();

	if (TransitionSound)
	{
		UGameplayStatics::PlaySound2D(this, TransitionSound);
	}

	OnSectionChanged.Broadcast(Section);
}

void UMainMenuWidget::UpdateModeCardVisuals()
{
	if (ModeCardBorder0.IsValid())
	{
		ModeCardBorder0->SetBorderBackgroundColor(SelectedModeIndex == 0 ? MWTheme::ActiveBorder : MWTheme::GlassBorder);
	}
	if (ModeCardBorder1.IsValid())
	{
		ModeCardBorder1->SetBorderBackgroundColor(SelectedModeIndex == 1 ? MWTheme::ActiveBorder : MWTheme::GlassBorder);
	}
	if (ModeCardBorder2.IsValid())
	{
		ModeCardBorder2->SetBorderBackgroundColor(SelectedModeIndex == 2 ? MWTheme::ActiveBorder : MWTheme::GlassBorder);
	}
}

void UMainMenuWidget::UpdateTabVisuals()
{
	const bool bIsPlayOrLobby = (CurrentSection == EFPSMenuSection::Play || CurrentSection == EFPSMenuSection::Home);
	const bool bIsOperator = (CurrentSection == EFPSMenuSection::Customization);
	const bool bIsGunsmith = (CurrentSection == EFPSMenuSection::Loadout);
	const bool bIsDailyRewards = (CurrentSection == EFPSMenuSection::DailyRewards);
	const bool bIsSettings = (CurrentSection == EFPSMenuSection::Settings);

	if (TabIndicatorPlay.IsValid())
	{
		TabIndicatorPlay->SetBorderBackgroundColor(bIsPlayOrLobby ? MWTheme::CyanAccent : FLinearColor::Transparent);
	}
	if (TabIndicatorOperator.IsValid())
	{
		TabIndicatorOperator->SetBorderBackgroundColor(bIsOperator ? MWTheme::CyanAccent : FLinearColor::Transparent);
	}
	if (TabIndicatorGunsmith.IsValid())
	{
		TabIndicatorGunsmith->SetBorderBackgroundColor(bIsGunsmith ? MWTheme::CyanAccent : FLinearColor::Transparent);
	}
	if (TabIndicatorDailyRewards.IsValid())
	{
		TabIndicatorDailyRewards->SetBorderBackgroundColor(bIsDailyRewards ? MWTheme::CyanAccent : FLinearColor::Transparent);
	}
	if (TabIndicatorSettings.IsValid())
	{
		TabIndicatorSettings->SetBorderBackgroundColor(bIsSettings ? MWTheme::CyanAccent : FLinearColor::Transparent);
	}
}

void UMainMenuWidget::StartMatchmaking()
{
	if (TransitionSound)
	{
		UGameplayStatics::PlaySound2D(this, TransitionSound);
	}

	OnMatchmakingTriggered();
	HostListenRoom();
}

void UMainMenuWidget::HostListenRoom()
{
	APlayerController* PC = GetOwningPlayer();
	if (PC)
	{
		FInputModeGameOnly GameOnlyInput;
		PC->SetInputMode(GameOnlyInput);
		PC->bShowMouseCursor = false;
	}

	UWorld* World = GetWorld();
	if (World)
	{
		UGameplayStatics::OpenLevel(World, FName(TEXT("/Engine/Maps/Entry")), true, TEXT("listen?game=/Script/NobleFermi.FPSMultiplayerGameMode"));
	}
}

void UMainMenuWidget::ConnectToDedicatedServer(const FString& ServerAddress)
{
	APlayerController* PC = GetOwningPlayer();
	if (PC)
	{
		FInputModeGameOnly GameOnlyInput;
		PC->SetInputMode(GameOnlyInput);
		PC->bShowMouseCursor = false;
		PC->ClientTravel(ServerAddress, TRAVEL_Absolute);
	}
}

void UMainMenuWidget::OpenSettings()
{
	NavigateToSection(EFPSMenuSection::Settings);
}

void UMainMenuWidget::RequestQuit()
{
	OnShowQuitConfirmation();

	APlayerController* PC = GetOwningPlayer();
	UKismetSystemLibrary::QuitGame(GetWorld(), PC, EQuitPreference::Quit, false);
}

FReply UMainMenuWidget::HandleTabClicked(EFPSMenuSection Section)
{
	NavigateToSection(Section);
	return FReply::Handled();
}

FReply UMainMenuWidget::HandleModeCardClicked(int32 ModeIndex)
{
	SelectedModeIndex = FMath::Clamp(ModeIndex, 0, 2);
	UpdateModeCardVisuals();
	if (TransitionSound)
	{
		UGameplayStatics::PlaySound2D(this, TransitionSound);
	}
	return FReply::Handled();
}

FReply UMainMenuWidget::HandleQuickplayClicked()
{
	StartMatchmaking();
	return FReply::Handled();
}

FReply UMainMenuWidget::HandleHostListenClicked()
{
	HostListenRoom();
	return FReply::Handled();
}

FReply UMainMenuWidget::HandleDirectConnectClicked()
{
	FString TargetAddress = TEXT("127.0.0.1:7777");
	if (DirectConnectInput.IsValid())
	{
		TargetAddress = DirectConnectInput->GetText().ToString().TrimStartAndEnd();
	}
	ConnectToDedicatedServer(TargetAddress);
	return FReply::Handled();
}

FReply UMainMenuWidget::HandleQuitClicked()
{
	RequestQuit();
	return FReply::Handled();
}

FReply UMainMenuWidget::HandleSelectArchetype(int32 ArchetypeIndex)
{
	if (!SelectedLoadoutDetails.IsValid())
	{
		return FReply::Handled();
	}

	switch (ArchetypeIndex)
	{
	case 0: // Assault
		SelectedLoadoutDetails->SetText(FText::FromString(TEXT("SELECTED: CLASS 1 (ASSAULT)\n\n"
			"PRIMARY: M4A1 RIFLE (Attachments: Red Dot Sight, Compensator, 40-Rnd Mag, Stippled Grip)\n"
			"SECONDARY: USP .45 Tactical (Tactical Knife)\n"
			"LETHAL: Frag Grenade (4.0s cookable fuse, cooking tick)\n"
			"TACTICAL: Flashbang (Disorientation + LOS blinding)\n"
			"PERKS: Sleight of Hand Pro | Stopping Power | Ninja Pro\n\n"
			"RECOIL STATS: Kick 1.2° | ADS Speed 210ms | Effective Range 45m | Muzzle Velocity 880 m/s")));
		break;
	case 1: // Spec Ops
		SelectedLoadoutDetails->SetText(FText::FromString(TEXT("SELECTED: CLASS 2 (SPEC OPS STEALTH)\n\n"
			"PRIMARY: MP5SD SUBMACHINE (Attachments: Integrated Suppressor, Subsonic Ammo, Red Laser, Skeleton Stock)\n"
			"SECONDARY: Silenced M9 Pistol\n"
			"LETHAL: Semtex (Sticky surface contact fuse)\n"
			"TACTICAL: Smoke Grenade (Thermal block)\n"
			"PERKS: Ghost Pro | Cold-Blooded | Dead Silence\n\n"
			"RECOIL STATS: Kick 0.8° | ADS Speed 150ms | Effective Range 28m | Muzzle Velocity 420 m/s (SILENCED)")));
		break;
	case 2: // Heavy Gunner
		SelectedLoadoutDetails->SetText(FText::FromString(TEXT("SELECTED: CLASS 3 (HEAVY GUNNER)\n\n"
			"PRIMARY: L86 LSW LMG (Attachments: Heavy Fluted Barrel, Deployable Bipod, 100-Rnd Drum, Combat Optic)\n"
			"SECONDARY: Desert Eagle .50 AE\n"
			"LETHAL: Frag Grenade (Resupplied via Scavenger)\n"
			"TACTICAL: Concussion Grenade (Movement suppression)\n"
			"PERKS: Scavenger Pro | Stopping Power | Steady Aim Pro\n\n"
			"RECOIL STATS: Kick 2.1° | ADS Speed 380ms | Effective Range 65m | Continuous Fire Suppression: 100 Rnds")));
		break;
	case 3: // Recon Sniper
		SelectedLoadoutDetails->SetText(FText::FromString(TEXT("SELECTED: CLASS 4 (RECON SNIPER)\n\n"
			"PRIMARY: INTERVENTION .50 (Attachments: Thermal 10x Scope, Match Fluted Barrel, Recoil Pad, Extended Mag)\n"
			"SECONDARY: Raffica 3-Round Burst Machine Pistol\n"
			"LETHAL: Claymore Anti-Personnel Laser Mine\n"
			"TACTICAL: Tactical Insertion (Deployable Respawn Beacon)\n"
			"PERKS: Sleight of Hand Pro | Stopping Power Pro | Ninja Pro\n\n"
			"LETHALITY: One-Shot Torso & Head (0 - 150m) | ADS Speed 320ms | Bullet Speed 1150 m/s")));
		break;
	case 4: // Run & Gun
		SelectedLoadoutDetails->SetText(FText::FromString(TEXT("SELECTED: CLASS 5 (RUN & GUN RUSHER)\n\n"
			"PRIMARY: VECTOR .45 ACP (Attachments: Rapid Fire Mod, Folding Lightweight Stock, Extended Drum Mag)\n"
			"SECONDARY: Model 1887 Akimbo Shotguns\n"
			"LETHAL: Semtex Sticky Grenade\n"
			"TACTICAL: Stun Grenade\n"
			"PERKS: Marathon Pro (Unlimited Sprint) | Lightweight Pro (Faster Walk) | Commando Pro\n\n"
			"FIRE RATE: 1200 RPM | ADS Speed 120ms | Sprint-Out 100ms | Extreme Close-Quarters Dominance")));
		break;
	default:
		break;
	}

	return FReply::Handled();
}

FReply UMainMenuWidget::HandleCamoSelected(uint8 CamoIdx)
{
	if (PreviewActor)
	{
		PreviewActor->SetCamo(static_cast<EOperatorCamo>(CamoIdx));
	}

	static const TCHAR* CamoIDs[] = { TEXT("camo_digital"), TEXT("camo_woodland"), TEXT("camo_desert"), TEXT("camo_blackout") };
	if (CamoIdx < 4)
	{
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UFPSEconomySubsystem* Economy = GI->GetSubsystem<UFPSEconomySubsystem>())
			{
				if (Economy->IsItemUnlocked(CamoIDs[CamoIdx]))
				{
					Economy->SetEquippedCamoIndex(CamoIdx);
				}
			}
		}
	}

	if (CamoStatusText.IsValid())
	{
		switch (CamoIdx)
		{
		case 0: CamoStatusText->SetText(FText::FromString(TEXT("ACTIVE SWATCH: DIGITAL // URBAN CHARCOAL"))); break;
		case 1: CamoStatusText->SetText(FText::FromString(TEXT("ACTIVE SWATCH: WOODLAND // RANGER OLIVE"))); break;
		case 2: CamoStatusText->SetText(FText::FromString(TEXT("ACTIVE SWATCH: DESERT // COYOTE TAN"))); break;
		case 3: CamoStatusText->SetText(FText::FromString(TEXT("ACTIVE SWATCH: BLACKOUT // MIDNIGHT MATTE"))); break;
		}
	}
	if (CenterPillText.IsValid())
	{
		FString CamoName = TEXT("DIGITAL");
		if (CamoIdx == 1) CamoName = TEXT("WOODLAND");
		else if (CamoIdx == 2) CamoName = TEXT("DESERT");
		else if (CamoIdx == 3) CamoName = TEXT("BLACKOUT");
		CenterPillText->SetText(FText::FromString(FString::Printf(TEXT("OPERATOR: TASK FORCE ECHO // CAMO: %s // 360° TURNTABLE"), *CamoName)));
	}
	if (TransitionSound) UGameplayStatics::PlaySound2D(this, TransitionSound);
	return FReply::Handled();
}

FReply UMainMenuWidget::HandleHeadgearSelected(uint8 HeadgearIdx)
{
	if (PreviewActor)
	{
		PreviewActor->SetHeadgear(static_cast<EOperatorHeadgear>(HeadgearIdx));
	}

	static const TCHAR* HeadgearIDs[] = { TEXT("headgear_fast"), TEXT("headgear_quadnods"), TEXT("headgear_balaclava"), TEXT("headgear_cap") };
	if (HeadgearIdx < 4)
	{
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UFPSEconomySubsystem* Economy = GI->GetSubsystem<UFPSEconomySubsystem>())
			{
				if (Economy->IsItemUnlocked(HeadgearIDs[HeadgearIdx]))
				{
					Economy->SetEquippedHeadgearIndex(HeadgearIdx);
				}
			}
		}
	}

	if (HeadgearStatusText.IsValid())
	{
		switch (HeadgearIdx)
		{
		case 0: HeadgearStatusText->SetText(FText::FromString(TEXT("HEADGEAR: FAST BALLISTIC HELMET"))); break;
		case 1: HeadgearStatusText->SetText(FText::FromString(TEXT("HEADGEAR: GPNVG-18 QUAD-NODS (PHOSPHOR GLOW)"))); break;
		case 2: HeadgearStatusText->SetText(FText::FromString(TEXT("HEADGEAR: SPEC OPS BALACLAVA"))); break;
		case 3: HeadgearStatusText->SetText(FText::FromString(TEXT("HEADGEAR: TACTICAL CAP & PELTOR HEADSET"))); break;
		}
	}
	if (TransitionSound) UGameplayStatics::PlaySound2D(this, TransitionSound);
	return FReply::Handled();
}

FReply UMainMenuWidget::HandleVestSelected(uint8 VestIdx)
{
	if (PreviewActor)
	{
		PreviewActor->SetArmorVest(static_cast<EOperatorArmorVest>(VestIdx));
	}

	static const TCHAR* VestIDs[] = { TEXT("vest_level4"), TEXT("vest_jpc"), TEXT("vest_recon") };
	if (VestIdx < 3)
	{
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UFPSEconomySubsystem* Economy = GI->GetSubsystem<UFPSEconomySubsystem>())
			{
				if (Economy->IsItemUnlocked(VestIDs[VestIdx]))
				{
					Economy->SetEquippedArmorIndex(VestIdx);
				}
			}
		}
	}

	if (VestStatusText.IsValid())
	{
		switch (VestIdx)
		{
		case 0: VestStatusText->SetText(FText::FromString(TEXT("ARMOR: LEVEL IV HEAVY CARRIER (MAX PROTECTION)"))); break;
		case 1: VestStatusText->SetText(FText::FromString(TEXT("ARMOR: LIGHTWEIGHT JPC VEST (AGILE MOBILITY)"))); break;
		case 2: VestStatusText->SetText(FText::FromString(TEXT("ARMOR: RECON TACTICAL RIG (MINIMALIST)"))); break;
		}
	}
	if (TransitionSound) UGameplayStatics::PlaySound2D(this, TransitionSound);
	return FReply::Handled();
}

FReply UMainMenuWidget::HandleWeaponFinishSelected(uint8 FinishIdx)
{
	if (PreviewActor)
	{
		PreviewActor->SetWeaponFinish(static_cast<EOperatorWeaponFinish>(FinishIdx));
	}

	static const TCHAR* WeaponIDs[] = { TEXT("weapon_steel"), TEXT("weapon_cerakote"), TEXT("weapon_carbon"), TEXT("weapon_damascene") };
	if (FinishIdx < 4)
	{
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UFPSEconomySubsystem* Economy = GI->GetSubsystem<UFPSEconomySubsystem>())
			{
				if (Economy->IsItemUnlocked(WeaponIDs[FinishIdx]))
				{
					Economy->SetEquippedWeaponFinishIndex(FinishIdx);
				}
			}
		}
	}

	if (WeaponFinishStatusText.IsValid())
	{
		switch (FinishIdx)
		{
		case 0: WeaponFinishStatusText->SetText(FText::FromString(TEXT("RIFLE FINISH: FACTORY BLUED STEEL"))); break;
		case 1: WeaponFinishStatusText->SetText(FText::FromString(TEXT("RIFLE FINISH: DESERT TAN CERAKOTE"))); break;
		case 2: WeaponFinishStatusText->SetText(FText::FromString(TEXT("RIFLE FINISH: CARBON FIBER SPEC-OPS"))); break;
		case 3: WeaponFinishStatusText->SetText(FText::FromString(TEXT("RIFLE FINISH: 24K GOLD & OPAL DAMASCENE"))); break;
		}
	}
	if (CenterPillText.IsValid())
	{
		FString FinishName = TEXT("FACTORY STEEL");
		if (FinishIdx == 1) FinishName = TEXT("DESERT CERAKOTE");
		else if (FinishIdx == 2) FinishName = TEXT("CARBON SPEC-OPS");
		else if (FinishIdx == 3) FinishName = TEXT("24K GOLD & OPAL");
		CenterPillText->SetText(FText::FromString(FString::Printf(TEXT("OPERATOR: TASK FORCE ECHO // NF-98: %s // 360° TURNTABLE"), *FinishName)));
	}
	if (TransitionSound) UGameplayStatics::PlaySound2D(this, TransitionSound);
	return FReply::Handled();
}

FReply UMainMenuWidget::HandleToggleAimingStance()
{
	if (PreviewActor)
	{
		const int32 NextStance = (PreviewActor->GetPreviewStance() + 1) % 3;
		PreviewActor->SetPreviewStance(NextStance);
		if (StanceStatusText.IsValid())
		{
			switch (NextStance)
			{
			case 0:
				StanceStatusText->SetText(FText::FromString(TEXT("STANCE: AIMED ADS (SM_AK47)")));
				break;
			case 1:
				StanceStatusText->SetText(FText::FromString(TEXT("STANCE: LOW READY PATROL")));
				break;
			case 2:
				StanceStatusText->SetText(FText::FromString(TEXT("STANCE: TACTICAL WALK CYCLE")));
				break;
			}
		}
	}
	if (TransitionSound) UGameplayStatics::PlaySound2D(this, TransitionSound);
	return FReply::Handled();
}

FReply UMainMenuWidget::HandleToggleTurntableRotation()
{
	if (PreviewActor)
	{
		PreviewActor->ToggleRotation();
		if (RotationSpeedText.IsValid())
		{
			RotationSpeedText->SetText(FText::FromString(PreviewActor->IsRotating() ?
				FString::Printf(TEXT("ROTATION: ACTIVE (%.1f DEG/SEC)"), PreviewActor->RotationSpeed) :
				TEXT("ROTATION: PAUSED (MANUAL FACING)")));
		}
	}
	if (TransitionSound) UGameplayStatics::PlaySound2D(this, TransitionSound);
	return FReply::Handled();
}

FReply UMainMenuWidget::HandleSetRotationSpeed(float Speed)
{
	if (PreviewActor)
	{
		PreviewActor->bIsRotating = true;
		PreviewActor->SetRotationSpeed(Speed);
		if (RotationSpeedText.IsValid())
		{
			RotationSpeedText->SetText(FText::FromString(
				FString::Printf(TEXT("ROTATION: ACTIVE (%.1f DEG/SEC)"), Speed)));
		}
	}
	if (TransitionSound) UGameplayStatics::PlaySound2D(this, TransitionSound);
	return FReply::Handled();
}

FReply UMainMenuWidget::HandleResetTurntableFacing()
{
	if (PreviewActor)
	{
		PreviewActor->ResetFacing();
	}
	if (TransitionSound) UGameplayStatics::PlaySound2D(this, TransitionSound);
	return FReply::Handled();
}

TSharedRef<SWidget> UMainMenuWidget::BuildDailyRewardsView()
{
	auto MakeDayCard = [this](int32 DayNum, int32 GoldReward, int32 OpalReward, bool bIsJackpot) -> TSharedRef<SWidget>
	{
		UFPSEconomySubsystem* Economy = nullptr;
		if (UGameInstance* GI = GetGameInstance())
		{
			Economy = GI->GetSubsystem<UFPSEconomySubsystem>();
		}

		const int32 CurrentDay = Economy ? Economy->GetConsecutiveLoginDays() : 1;
		const bool bClaimedToday = Economy ? Economy->HasClaimedDailyRewardToday() : false;

		FString StatusText;
		FLinearColor StatusColor = MWTheme::TextMuted;
		FLinearColor BorderColor = MWTheme::GlassBorder;

		if (DayNum < CurrentDay)
		{
			StatusText = TEXT("CLAIMED ✓");
			StatusColor = MWTheme::EmeraldReady;
		}
		else if (DayNum == CurrentDay)
		{
			if (bClaimedToday)
			{
				StatusText = TEXT("CLAIMED ✓");
				StatusColor = MWTheme::EmeraldReady;
			}
			else
			{
				StatusText = TEXT("READY TO CLAIM!");
				StatusColor = MWTheme::CyanAccent;
				BorderColor = MWTheme::ActiveBorder;
			}
		}
		else
		{
			StatusText = TEXT("LOCKED");
			StatusColor = MWTheme::TextMuted;
		}

		FString RewardLabel = FString::Printf(TEXT("+%s GOLD"), *FText::AsNumber(GoldReward).ToString());
		if (OpalReward > 0)
		{
			RewardLabel += FString::Printf(TEXT(" & +%d OPALS"), OpalReward);
		}

		return SNew(SBox)
			.HeightOverride(42.0f)
			.Padding(0.0f, 0.0f, 0.0f, 4.0f)
			[
				SNew(SBorder)
				.BorderBackgroundColor(BorderColor)
				.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
				.Padding(1.0f)
				[
					SNew(SBorder)
					.BorderBackgroundColor(bIsJackpot ? FLinearColor(0.12f, 0.09f, 0.03f, 0.85f) : MWTheme::DarkGlassCard)
					.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
					.Padding(10.0f, 4.0f)
					.VAlign(VAlign_Center)
					[
						SNew(SHorizontalBox)
						// Day index
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(0.0f, 0.0f, 8.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(FString::Printf(TEXT("DAY %02d"), DayNum)))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
							.ColorAndOpacity(bIsJackpot ? MWTheme::AmberAccent : MWTheme::TextPrimary)
						]
						// Reward amount
						+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(FText::FromString(RewardLabel))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
							.ColorAndOpacity(bIsJackpot ? MWTheme::AmberAccent : MWTheme::CyanAccent)
						]
						// Status Badge
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(FText::FromString(StatusText))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
							.ColorAndOpacity(StatusColor)
						]
					]
				]
			];
	};

	return SNew(SHorizontalBox)
		// Column 1: 7-Day Login Streak Vault (Left Blade, ~360px)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(0.0f, 0.0f, 16.0f, 0.0f)
		[
			SNew(SBox)
			.WidthOverride(360.0f)
			[
				SNew(SBackgroundBlur)
				.BlurStrength(14.0f)
				.bApplyAlphaToBlur(true)
				.Padding(0.0f)
				[
					SNew(SBorder)
					.BorderBackgroundColor(MWTheme::DarkGlass)
					.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
					.Padding(16.0f)
					[
						SNew(SVerticalBox)
						// Section Header
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 0.0f, 0.0f, 4.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("/// 7-DAY OPERATOR STREAK")))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
							.ColorAndOpacity(MWTheme::AmberAccent)
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 0.0f, 0.0f, 6.0f)
						[
							SAssignNew(StreakStatusText, STextBlock)
							.Text(FText::FromString(TEXT("ACTIVE STREAK: DAY 1 OF 7 // READY TO CLAIM")))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
							.ColorAndOpacity(MWTheme::TextSecondary)
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 0.0f, 0.0f, 10.0f)
						[
							SNew(SBox)
							.HeightOverride(1.0f)
							[
								SNew(SBorder)
								.BorderBackgroundColor(MWTheme::AmberAccent.CopyWithNewOpacity(0.5f))
								.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
							]
						]
						// 7 Day Rows
						+ SVerticalBox::Slot().AutoHeight() [ MakeDayCard(1, 250, 0, false) ]
						+ SVerticalBox::Slot().AutoHeight() [ MakeDayCard(2, 500, 0, false) ]
						+ SVerticalBox::Slot().AutoHeight() [ MakeDayCard(3, 750, 0, false) ]
						+ SVerticalBox::Slot().AutoHeight() [ MakeDayCard(4, 1000, 0, false) ]
						+ SVerticalBox::Slot().AutoHeight() [ MakeDayCard(5, 1250, 0, false) ]
						+ SVerticalBox::Slot().AutoHeight() [ MakeDayCard(6, 1500, 0, false) ]
						+ SVerticalBox::Slot().AutoHeight() [ MakeDayCard(7, 2000, 15, true) ]
						// Action Button: Claim Reward
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 12.0f, 0.0f, 6.0f)
						[
							SNew(SBox)
							.HeightOverride(38.0f)
							[
								SNew(SButton)
								.ButtonColorAndOpacity(FLinearColor::Transparent)
								.ContentPadding(0.0f)
								.OnClicked_Lambda([this]() { return HandleClaimDailyStreakClicked(); })
								[
									SNew(SBorder)
									.BorderBackgroundColor(MWTheme::EmeraldReady)
									.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
									.Padding(1.0f)
									[
										SNew(SBorder)
										.BorderBackgroundColor(MWTheme::EmeraldReady.CopyWithNewOpacity(0.25f))
										.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
										.HAlign(HAlign_Center)
										.VAlign(VAlign_Center)
										[
											SNew(STextBlock)
											.Text(FText::FromString(TEXT("🎁 CLAIM TODAY'S STREAK REWARD")))
											.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
											.ColorAndOpacity(MWTheme::TextPrimary)
										]
									]
								]
							]
						]
						// Sandbox Simulation Button
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 4.0f, 0.0f, 0.0f)
						[
							SNew(SBox)
							.HeightOverride(32.0f)
							[
								SNew(SButton)
								.ButtonColorAndOpacity(FLinearColor::Transparent)
								.ContentPadding(0.0f)
								.OnClicked_Lambda([this]() { return HandleSimulateNextDayClicked(); })
								[
									SNew(SBorder)
									.BorderBackgroundColor(MWTheme::GlassBorder)
									.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
									.Padding(1.0f)
									[
										SNew(SBorder)
										.BorderBackgroundColor(MWTheme::DarkGlassCard)
										.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
										.HAlign(HAlign_Center)
										.VAlign(VAlign_Center)
										[
											SNew(STextBlock)
											.Text(FText::FromString(TEXT("⚡ DEV: SIMULATE NEXT DAY")))
											.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
											.ColorAndOpacity(MWTheme::CyanAccent)
										]
									]
								]
							]
						]
					]
				]
			]
		]
		// Column 2: Minigames Center (FillWidth)
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			SNew(SBackgroundBlur)
			.BlurStrength(14.0f)
			.bApplyAlphaToBlur(true)
			.Padding(0.0f)
			[
				SNew(SBorder)
				.BorderBackgroundColor(MWTheme::DarkGlass)
				.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
				.Padding(16.0f)
				[
					SNew(SVerticalBox)
					// Minigame Switcher Header
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 0.0f, 0.0f, 10.0f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						.Padding(0.0f, 0.0f, 6.0f, 0.0f)
						[
							SNew(SBox)
							.HeightOverride(36.0f)
							[
								SNew(SButton)
								.ButtonColorAndOpacity(MWTheme::DarkGlassCard)
								.HAlign(HAlign_Center)
								.VAlign(VAlign_Center)
								.OnClicked_Lambda([this]() { return HandleSwitchMinigame(0); })
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("🎡 TACTICAL FORTUNE WHEEL")))
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
									.ColorAndOpacity(MWTheme::AmberAccent)
								]
							]
						]
						+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						.Padding(6.0f, 0.0f, 0.0f, 0.0f)
						[
							SNew(SBox)
							.HeightOverride(36.0f)
							[
								SNew(SButton)
								.ButtonColorAndOpacity(MWTheme::DarkGlassCard)
								.HAlign(HAlign_Center)
								.VAlign(VAlign_Center)
								.OnClicked_Lambda([this]() { return HandleSwitchMinigame(1); })
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("🎯 TACTICAL DARTBOARD")))
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
									.ColorAndOpacity(MWTheme::CyanAccent)
								]
							]
						]
					]
					// Switcher for Minigame 0 (Wheel) & 1 (Dartboard)
					+ SVerticalBox::Slot()
					.FillHeight(1.0f)
					[
						SAssignNew(MinigameSwitcher, SWidgetSwitcher)
						.WidgetIndex(0)
						// Slot 0: Fortune Wheel
						+ SWidgetSwitcher::Slot()
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot()
							.AutoHeight()
							.HAlign(HAlign_Center)
							.Padding(0.0f, 0.0f, 0.0f, 4.0f)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("/// DAILY SPIN WHEEL // 1 FREE SPIN PER DAY (OR 250 GOLD)")))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
								.ColorAndOpacity(MWTheme::TextSecondary)
							]
							// Wheel Render Widget
							+ SVerticalBox::Slot()
							.AutoHeight()
							.HAlign(HAlign_Center)
							.Padding(0.0f, 6.0f)
							[
								SNew(SBox)
								.WidthOverride(320.0f)
								.HeightOverride(320.0f)
								[
									SNew(SFPSFortuneWheelWidget)
									.WheelAngle_Lambda([this]() { return WheelCurrentAngle; })
									.BurstTimer_Lambda([this]() { return WheelBurstTimer; })
								]
							]
							// Action Button: Spin
							+ SVerticalBox::Slot()
							.AutoHeight()
							.HAlign(HAlign_Center)
							.Padding(0.0f, 6.0f)
							[
								SNew(SBox)
								.WidthOverride(260.0f)
								.HeightOverride(38.0f)
								[
									SNew(SButton)
									.ButtonColorAndOpacity(FLinearColor::Transparent)
									.ContentPadding(0.0f)
									.OnClicked_Lambda([this]() { return HandleSpinWheelClicked(); })
									[
										SNew(SBorder)
										.BorderBackgroundColor(MWTheme::AmberAccent)
										.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
										.Padding(1.0f)
										[
											SNew(SBorder)
											.BorderBackgroundColor(MWTheme::AmberAccent.CopyWithNewOpacity(0.25f))
											.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
											.HAlign(HAlign_Center)
											.VAlign(VAlign_Center)
											[
												SNew(STextBlock)
												.Text(FText::FromString(TEXT("🎡 SPIN WHEEL (FREE / 250G)")))
												.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
												.ColorAndOpacity(MWTheme::TextPrimary)
											]
										]
									]
								]
							]
							// Reward Readout
							+ SVerticalBox::Slot()
							.AutoHeight()
							.HAlign(HAlign_Center)
							.Padding(0.0f, 2.0f, 0.0f, 6.0f)
							[
								SAssignNew(WheelRewardText, STextBlock)
								.Text(FText::FromString(TEXT("SPIN TO WIN GOLD, RARE OPALS, OR DART TOKENS")))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
								.ColorAndOpacity(MWTheme::AmberAccent)
							]
							// Legend
							+ SVerticalBox::Slot()
							.AutoHeight()
							.HAlign(HAlign_Center)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("ODDS: 250G (26%) | 500G (24%) | 750G (18%) | 1,250G (12%) | 2,500G (6%) | +1 DART (9%) | 💎 5 OPALS (4%) | 💎 25 OPALS (1%)")))
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
								.ColorAndOpacity(MWTheme::TextMuted)
							]
						]
						// Slot 1: Dartboard
						+ SWidgetSwitcher::Slot()
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot()
							.AutoHeight()
							.HAlign(HAlign_Center)
							.Padding(0.0f, 0.0f, 0.0f, 2.0f)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("/// TACTICAL DARTBOARD RANGE // AIM WITH OSCILLATING RETICLE")))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
								.ColorAndOpacity(MWTheme::TextSecondary)
							]
							+ SVerticalBox::Slot()
							.AutoHeight()
							.HAlign(HAlign_Center)
							.Padding(0.0f, 0.0f, 0.0f, 4.0f)
							[
								SAssignNew(DartTokensText, STextBlock)
								.Text(FText::FromString(TEXT("DART TOKENS: 3 AVAILABLE")))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
								.ColorAndOpacity(MWTheme::CyanAccent)
							]
							// Dartboard Render Widget
							+ SVerticalBox::Slot()
							.AutoHeight()
							.HAlign(HAlign_Center)
							.Padding(0.0f, 6.0f)
							[
								SNew(SBox)
								.WidthOverride(320.0f)
								.HeightOverride(320.0f)
								[
									SNew(SFPSDartboardWidget)
									.ReticlePos_Lambda([this]() { return DartReticlePos; })
									.LastHitPos_Lambda([this]() { return LastDartHitPos; })
									.HasLastHit_Lambda([this]() { return bHasLastDartHit; })
									.BurstTimer_Lambda([this]() { return DartBurstTimer; })
								]
							]
							// Action Button: Throw Dart
							+ SVerticalBox::Slot()
							.AutoHeight()
							.HAlign(HAlign_Center)
							.Padding(0.0f, 6.0f)
							[
								SNew(SBox)
								.WidthOverride(260.0f)
								.HeightOverride(38.0f)
								[
									SNew(SButton)
									.ButtonColorAndOpacity(FLinearColor::Transparent)
									.ContentPadding(0.0f)
									.OnClicked_Lambda([this]() { return HandleThrowDartClicked(); })
									[
										SNew(SBorder)
										.BorderBackgroundColor(MWTheme::CyanAccent)
										.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
										.Padding(1.0f)
										[
											SNew(SBorder)
											.BorderBackgroundColor(MWTheme::CyanAccent.CopyWithNewOpacity(0.25f))
											.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
											.HAlign(HAlign_Center)
											.VAlign(VAlign_Center)
											[
												SNew(STextBlock)
												.Text(FText::FromString(TEXT("🎯 THROW TACTICAL DART")))
												.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
												.ColorAndOpacity(MWTheme::TextPrimary)
											]
										]
									]
								]
							]
							// Outcome Readout
							+ SVerticalBox::Slot()
							.AutoHeight()
							.HAlign(HAlign_Center)
							.Padding(0.0f, 2.0f, 0.0f, 6.0f)
							[
								SAssignNew(DartRewardText, STextBlock)
								.Text(FText::FromString(TEXT("AIM FOR DEAD-CENTER BULLSEYE (<6% RADIUS) FOR +1,800 GOLD & +10 OPALS!")))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
								.ColorAndOpacity(MWTheme::CyanAccent)
							]
							// Scoring Legend
							+ SVerticalBox::Slot()
							.AutoHeight()
							.HAlign(HAlign_Center)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("SCORING: BULLSEYE (<6%): +1,800G & +10💎 | INNER RING (<38%): +600-1,000G | OUTER RING (<100%): +150-350G")))
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
								.ColorAndOpacity(MWTheme::TextMuted)
							]
						]
					]
				]
			]
		]
		// Column 3: Opal Supply Drops & Active Daily Contracts (Right Blade, ~360px)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(16.0f, 0.0f, 0.0f, 0.0f)
		[
			SNew(SBox)
			.WidthOverride(360.0f)
			[
				SNew(SBackgroundBlur)
				.BlurStrength(14.0f)
				.bApplyAlphaToBlur(true)
				.Padding(0.0f)
				[
					SNew(SBorder)
					.BorderBackgroundColor(MWTheme::DarkGlass)
					.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
					.Padding(16.0f)
					[
						SNew(SScrollBox)
						+ SScrollBox::Slot()
						[
							SNew(SVerticalBox)
							// Active Daily Contracts // Bounty Board Panel at top of Daily Vault Right Blade
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0.0f, 0.0f, 0.0f, 12.0f)
							[
								BuildBountyBoardPanel()
							]
							// Header
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0.0f, 0.0f, 0.0f, 4.0f)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("/// CRYSTALLINE OPAL STORE")))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
								.ColorAndOpacity(MWTheme::CyanAccent)
							]
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0.0f, 0.0f, 0.0f, 6.0f)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("ACQUIRE RARE OPALS TO UNLOCK HIGH-TIER OPERATOR ITEMS")))
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
								.ColorAndOpacity(MWTheme::TextMuted)
							]
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0.0f, 0.0f, 0.0f, 10.0f)
							[
								SNew(SBox)
								.HeightOverride(1.0f)
								[
									SNew(SBorder)
									.BorderBackgroundColor(MWTheme::CyanAccent.CopyWithNewOpacity(0.5f))
									.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
								]
							]
							// Pack 1: Field Recon Drop
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0.0f, 0.0f, 0.0f, 8.0f)
							[
								SNew(SBorder)
								.BorderBackgroundColor(MWTheme::GlassBorder)
								.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
								.Padding(1.0f)
								[
									SNew(SBorder)
									.BorderBackgroundColor(MWTheme::DarkGlassCard)
									.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
									.Padding(10.0f)
									[
										SNew(SVerticalBox)
										+ SVerticalBox::Slot().AutoHeight()
										[
											SNew(STextBlock).Text(FText::FromString(TEXT("FIELD RECON SUPPLY DROP"))).Font(FCoreStyle::GetDefaultFontStyle("Bold", 9)).ColorAndOpacity(MWTheme::TextPrimary)
										]
										+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 6.0f)
										[
											SNew(STextBlock).Text(FText::FromString(TEXT("+100 CRYSTALLINE OPALS // $4.99 VALUE"))).Font(FCoreStyle::GetDefaultFontStyle("Bold", 8)).ColorAndOpacity(MWTheme::CyanAccent)
										]
										+ SVerticalBox::Slot().AutoHeight()
										[
											SNew(SButton)
											.ButtonColorAndOpacity(MWTheme::DarkGlass)
											.HAlign(HAlign_Center)
											.OnClicked_Lambda([this]() { return HandleClaimOpalDrop(0); })
											[
												SNew(STextBlock).Text(FText::FromString(TEXT("ACQUIRE (+100 💎)"))).Font(FCoreStyle::GetDefaultFontStyle("Bold", 8)).ColorAndOpacity(MWTheme::CyanAccent)
											]
										]
									]
								]
							]
							// Pack 2: Special Forces Cache
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0.0f, 0.0f, 0.0f, 8.0f)
							[
								SNew(SBorder)
								.BorderBackgroundColor(MWTheme::GlassBorder)
								.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
								.Padding(1.0f)
								[
									SNew(SBorder)
									.BorderBackgroundColor(MWTheme::DarkGlassCard)
									.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
									.Padding(10.0f)
									[
										SNew(SVerticalBox)
										+ SVerticalBox::Slot().AutoHeight()
										[
											SNew(STextBlock).Text(FText::FromString(TEXT("SPECIAL FORCES CACHE"))).Font(FCoreStyle::GetDefaultFontStyle("Bold", 9)).ColorAndOpacity(MWTheme::TextPrimary)
										]
										+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 6.0f)
										[
											SNew(STextBlock).Text(FText::FromString(TEXT("+300 CRYSTALLINE OPALS // $12.99 VALUE"))).Font(FCoreStyle::GetDefaultFontStyle("Bold", 8)).ColorAndOpacity(MWTheme::CyanAccent)
										]
										+ SVerticalBox::Slot().AutoHeight()
										[
											SNew(SButton)
											.ButtonColorAndOpacity(MWTheme::DarkGlass)
											.HAlign(HAlign_Center)
											.OnClicked_Lambda([this]() { return HandleClaimOpalDrop(1); })
											[
												SNew(STextBlock).Text(FText::FromString(TEXT("ACQUIRE (+300 💎)"))).Font(FCoreStyle::GetDefaultFontStyle("Bold", 8)).ColorAndOpacity(MWTheme::CyanAccent)
											]
										]
									]
								]
							]
							// Pack 3: Black Ops Vault
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0.0f, 0.0f, 0.0f, 10.0f)
							[
								SNew(SBorder)
								.BorderBackgroundColor(MWTheme::AmberAccent)
								.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
								.Padding(1.0f)
								[
									SNew(SBorder)
									.BorderBackgroundColor(MWTheme::DarkGlassCard)
									.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
									.Padding(10.0f)
									[
										SNew(SVerticalBox)
										+ SVerticalBox::Slot().AutoHeight()
										[
											SNew(STextBlock).Text(FText::FromString(TEXT("BLACK OPS VAULT CRATE"))).Font(FCoreStyle::GetDefaultFontStyle("Bold", 9)).ColorAndOpacity(MWTheme::AmberAccent)
										]
										+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 6.0f)
										[
											SNew(STextBlock).Text(FText::FromString(TEXT("+750 CRYSTALLINE OPALS // $24.99 VALUE"))).Font(FCoreStyle::GetDefaultFontStyle("Bold", 8)).ColorAndOpacity(MWTheme::AmberAccent)
										]
										+ SVerticalBox::Slot().AutoHeight()
										[
											SNew(SButton)
											.ButtonColorAndOpacity(MWTheme::DarkGlass)
											.HAlign(HAlign_Center)
											.OnClicked_Lambda([this]() { return HandleClaimOpalDrop(2); })
											[
												SNew(STextBlock).Text(FText::FromString(TEXT("ACQUIRE (+750 💎)"))).Font(FCoreStyle::GetDefaultFontStyle("Bold", 8)).ColorAndOpacity(MWTheme::AmberAccent)
											]
										]
									]
								]
							]
							// Fair Play Guarantee
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SBorder)
								.BorderBackgroundColor(MWTheme::GlassBorder)
								.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
								.Padding(8.0f)
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("FAIR-PLAY GUARANTEE:\n"
										"• All items can be unlocked permanently with standard Gold\n"
										"• Free spins & daily login streaks reward crystalline Opals\n"
										"• Zero pay-to-win mechanics or stat advantages\n"
										"• Profile saved to NobleFermiProfile.json")))
									.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
									.ColorAndOpacity(MWTheme::TextSecondary)
								]
							]
						]
					]
				]
			]
		];
}

FReply UMainMenuWidget::HandleSpinWheelClicked()
{
	if (bIsWheelSpinning)
	{
		return FReply::Handled();
	}

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UFPSEconomySubsystem* Economy = GI->GetSubsystem<UFPSEconomySubsystem>())
		{
			if (!Economy->CanSpinWheel())
			{
				if (WheelRewardText.IsValid())
				{
					WheelRewardText->SetText(FText::FromString(TEXT("INSUFFICIENT GOLD (250G NEEDED FOR EXTRA SPIN)!")));
				}
				return FReply::Handled();
			}

			WheelTargetSlice = Economy->RollWheelSliceIndex();
			bIsWheelSpinning = true;
			WheelSpinElapsed = 0.0f;
			WheelSpinDuration = 3.5f;
			WheelAngularVelocity = 720.0f + FMath::RandRange(100.0f, 300.0f);

			if (WheelRewardText.IsValid())
			{
				WheelRewardText->SetText(FText::FromString(TEXT("SPINNING TACTICAL WHEEL...")));
			}

			if (TransitionSound)
			{
				UGameplayStatics::PlaySound2D(this, TransitionSound);
			}
		}
	}
	return FReply::Handled();
}

FReply UMainMenuWidget::HandleThrowDartClicked()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UFPSEconomySubsystem* Economy = GI->GetSubsystem<UFPSEconomySubsystem>())
		{
			if (!Economy->CanThrowDart())
			{
				if (DartRewardText.IsValid())
				{
					DartRewardText->SetText(FText::FromString(TEXT("OUT OF DART TOKENS! WIN TOKENS ON THE WHEEL OR DAILY STREAKS.")));
				}
				return FReply::Handled();
			}

			LastDartHitPos = DartReticlePos;
			bHasLastDartHit = true;

			const FVector2D Delta = DartReticlePos - FVector2D(0.5f, 0.5f);
			const float DistNormalized = FMath::Clamp(Delta.Size() / 0.5f, 0.0f, 1.0f);

			FText OutMessage;
			int32 OutGold = 0;
			int32 OutOpals = 0;
			bool bBullseye = false;
			Economy->AwardDartThrowReward(DistNormalized, OutMessage, OutGold, OutOpals, bBullseye);
			if (bBullseye)
			{
				DartBurstTimer = 1.5f;
			}

			if (DartRewardText.IsValid())
			{
				DartRewardText->SetText(OutMessage);
			}

			UpdateCurrencyHeader();
			RefreshDailyRewardsUI();

			if (TransitionSound)
			{
				UGameplayStatics::PlaySound2D(this, TransitionSound);
			}
		}
	}
	return FReply::Handled();
}

FReply UMainMenuWidget::HandleClaimDailyStreakClicked()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UFPSEconomySubsystem* Economy = GI->GetSubsystem<UFPSEconomySubsystem>())
		{
			int32 AwardedGold = 0;
			int32 AwardedOpals = 0;
			if (Economy->ClaimDailyLoginReward(AwardedGold, AwardedOpals))
			{
				UpdateCurrencyHeader();
				RefreshDailyRewardsUI();
				if (TransitionSound)
				{
					UGameplayStatics::PlaySound2D(this, TransitionSound);
				}
			}
			else
			{
				if (StreakStatusText.IsValid())
				{
					StreakStatusText->SetText(FText::FromString(TEXT("DAILY REWARD ALREADY CLAIMED TODAY!")));
				}
			}
		}
	}
	return FReply::Handled();
}

FReply UMainMenuWidget::HandleSimulateNextDayClicked()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UFPSEconomySubsystem* Economy = GI->GetSubsystem<UFPSEconomySubsystem>())
		{
			Economy->SimulateNextDay();
			UpdateCurrencyHeader();
			RefreshDailyRewardsUI();
			if (TransitionSound)
			{
				UGameplayStatics::PlaySound2D(this, TransitionSound);
			}
		}
	}
	return FReply::Handled();
}

FReply UMainMenuWidget::HandleSwitchMinigame(int32 MinigameIndex)
{
	if (MinigameSwitcher.IsValid())
	{
		MinigameSwitcher->SetActiveWidgetIndex(FMath::Clamp(MinigameIndex, 0, 1));
	}
	if (TransitionSound)
	{
		UGameplayStatics::PlaySound2D(this, TransitionSound);
	}
	return FReply::Handled();
}

FReply UMainMenuWidget::HandleClaimOpalDrop(int32 PackIndex)
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UFPSEconomySubsystem* Economy = GI->GetSubsystem<UFPSEconomySubsystem>())
		{
			FText Msg;
			int32 AddedOpals = 0;
			Economy->ClaimOpalSupplyDrop(PackIndex, Msg, AddedOpals);
			UpdateCurrencyHeader();
			RefreshDailyRewardsUI();
			if (TransitionSound)
			{
				UGameplayStatics::PlaySound2D(this, TransitionSound);
			}
		}
	}
	return FReply::Handled();
}

FReply UMainMenuWidget::HandleUnlockCustomizationItem(const FString& ItemID, bool bUseOpals)
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UFPSEconomySubsystem* Economy = GI->GetSubsystem<UFPSEconomySubsystem>())
		{
			bool bSuccess = false;
			if (bUseOpals)
			{
				bSuccess = Economy->UnlockItemWithOpals(ItemID);
			}
			else
			{
				bSuccess = Economy->UnlockItemWithGold(ItemID);
			}

			if (bSuccess)
			{
				UpdateCurrencyHeader();
				RefreshDailyRewardsUI();
				if (TransitionSound)
				{
					UGameplayStatics::PlaySound2D(this, TransitionSound);
				}
			}
			else
			{
				if (CamoStatusText.IsValid())
				{
					CamoStatusText->SetText(FText::FromString(TEXT("TRANSACTION FAILED: INSUFFICIENT FUNDS!")));
				}
			}
		}
	}
	return FReply::Handled();
}

FReply UMainMenuWidget::HandleRerollBountiesClicked()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UFPSEconomySubsystem* Economy = GI->GetSubsystem<UFPSEconomySubsystem>())
		{
			if (Economy->RerollDailyBounties())
			{
				UpdateCurrencyHeader();
				RefreshDailyRewardsUI();
				if (TransitionSound)
				{
					UGameplayStatics::PlaySound2D(this, TransitionSound);
				}
			}
		}
	}
	return FReply::Handled();
}

FReply UMainMenuWidget::HandleSimulateBountyProgressClicked(int32 BountyIdx)
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UFPSEconomySubsystem* Economy = GI->GetSubsystem<UFPSEconomySubsystem>())
		{
			const TArray<FFPSTacticalBounty>& Bounties = Economy->GetActiveBounties();
			if (Bounties.IsValidIndex(BountyIdx))
			{
				const FName TagToRecord = Bounties[BountyIdx].EventTag;
				Economy->RecordBountyProgress(TagToRecord, 1);
				UpdateCurrencyHeader();
				RefreshDailyRewardsUI();
				if (TransitionSound)
				{
					UGameplayStatics::PlaySound2D(this, TransitionSound);
				}
			}
		}
	}
	return FReply::Handled();
}

void UMainMenuWidget::UpdateCurrencyHeader()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UFPSEconomySubsystem* Economy = GI->GetSubsystem<UFPSEconomySubsystem>())
		{
			if (GoldBalanceText.IsValid())
			{
				GoldBalanceText->SetText(FText::FromString(FString::Printf(TEXT("%s GOLD"), *FText::AsNumber(Economy->GetGoldBalance()).ToString())));
			}
			if (OpalBalanceText.IsValid())
			{
				OpalBalanceText->SetText(FText::FromString(FString::Printf(TEXT("%s OPALS"), *FText::AsNumber(Economy->GetOpalBalance()).ToString())));
			}
		}
	}
}

void UMainMenuWidget::RefreshDailyRewardsUI()
{
	UpdateCurrencyHeader();

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UFPSEconomySubsystem* Economy = GI->GetSubsystem<UFPSEconomySubsystem>())
		{
			if (StreakStatusText.IsValid())
			{
				const int32 CurrentDay = Economy->GetConsecutiveLoginDays();
				const bool bClaimed = Economy->HasClaimedDailyRewardToday();
				StreakStatusText->SetText(FText::FromString(FString::Printf(
					TEXT("ACTIVE STREAK: DAY %d OF 7 // %s"),
					CurrentDay,
					bClaimed ? TEXT("TODAY'S REWARD CLAIMED") : TEXT("READY TO CLAIM")
				)));
			}
			if (DartTokensText.IsValid())
			{
				DartTokensText->SetText(FText::FromString(FString::Printf(
					TEXT("DART TOKENS: %d AVAILABLE"),
					Economy->GetDartTokens()
				)));
			}
		}
	}
}
