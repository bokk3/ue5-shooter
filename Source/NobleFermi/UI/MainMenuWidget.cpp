#include "UI/MainMenuWidget.h"
#include "Game/FPSMenuGameMode.h"
#include "Game/FPSMenuHangarStage.h"

#include "UI/SettingsMenuWidget.h"

#include "Character/FPSCharacterPreviewActor.h"

#include "GameFramework/PlayerController.h"

#include "GameFramework/PlayerState.h"

#include "Kismet/GameplayStatics.h"

#include "Kismet/KismetSystemLibrary.h"

#include "GameFramework/GameUserSettings.h"

#include "Misc/ConfigCacheIni.h"



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

#include "Auth/FPSAuthSubsystem.h"

#include "Rendering/DrawElements.h"

#include "Widgets/SCompoundWidget.h"



namespace MWTheme
{
	// Official Noble Fermi Website Palette (web/index.html — noble-fermi.pages.dev)
	static const FLinearColor ObsidianVoid    = FLinearColor(0.012f, 0.027f, 0.071f, 0.88f); // #030712 Obsidian Void
	static const FLinearColor CyanAccent      = FLinearColor(0.0f, 0.898f, 1.0f, 1.0f);      // #00E5FF Tactical Cyan
	static const FLinearColor AmberAccent     = FLinearColor(1.0f, 0.702f, 0.0f, 1.0f);      // #FFB300 Afterburner Amber
	static const FLinearColor CrimsonAlert    = FLinearColor(1.0f, 0.165f, 0.329f, 1.0f);    // #FF2A54 Crimson Alert
	static const FLinearColor EmeraldReady    = FLinearColor(0.0f, 0.902f, 0.463f, 1.0f);    // #00E676 Emerald Ready

	// Wet-Glass Acrylic & .hud-card Surfaces (matching website rgba(6, 16, 29, 0.72) & rgba(0, 229, 255, 0.28))
	static const FLinearColor DarkGlass       = FLinearColor(0.020f, 0.043f, 0.086f, 0.76f); // #050B16 Wet Glass Panel
	static const FLinearColor DarkGlassCard   = FLinearColor(0.024f, 0.063f, 0.114f, 0.82f); // #06101D .hud-card Interior
	static const FLinearColor DarkGlassHover  = FLinearColor(0.045f, 0.110f, 0.190f, 0.88f); // Hover / Active Glass
	static const FLinearColor GlassBorder     = FLinearColor(0.0f, 0.898f, 1.0f, 0.28f);     // rgba(0, 229, 255, 0.28) .hud-card border
	static const FLinearColor ActiveBorder    = FLinearColor(0.0f, 0.898f, 1.0f, 0.95f);     // #00E5FF Active glowing border

	// Website Typography Colors
	static const FLinearColor TextPrimary     = FLinearColor(0.95f, 0.97f, 0.99f, 1.0f);     // #F3F7FC Crisp White
	static const FLinearColor TextSecondary   = FLinearColor(0.74f, 0.82f, 0.90f, 1.0f);     // #BDD1E6 Slate Silver
	static const FLinearColor TextMuted       = FLinearColor(0.46f, 0.56f, 0.68f, 1.0f);     // #758FAD Telemetry Muted

	static TSharedPtr<SBorder> TabIndicatorServerBrowser;
	static TSharedPtr<SWidgetSwitcher> OperatorCategorySwitcher;
	static int32 ActiveOperatorCategoryTab = 0;
	static TSharedPtr<SBorder> OperatorCatBorder0;
	static TSharedPtr<SBorder> OperatorCatBorder1;
	static TSharedPtr<SBorder> OperatorCatBorder2;
	static TSharedPtr<SBorder> OperatorCatBorder3;
	static TSharedPtr<SEditableTextBox> VerificationCodeInput;
	static TSharedPtr<STextBlock> VerificationCallsignLabel;
	static FString PendingVerificationCallsign;
	static FString PendingVerificationToken;
}

/**
 * SFPSNobleFermiEmblemWidget
 * Custom Slate vector renderer for the official Noble Fermi Hexagonal Ballistic Aegis Shield Emblem
 * (matching web/assets/favicon.svg: Outer Amber Hexagon, Inner Cyan Hexagon, Tactical Visor Brow,
 * Interlocking "NF" Monogram, and Relativistic Chrono Ring Slash).
 */
class SFPSNobleFermiEmblemWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFPSNobleFermiEmblemWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		ChildSlot
		[
			SNew(SBox)
			.WidthOverride(40.0f)
			.HeightOverride(40.0f)
		];
	}

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
	{
		const FVector2D Size = AllottedGeometry.GetLocalSize();
		const auto MapPt = [&Size](float X, float Y) -> FVector2D
		{
			return FVector2D((X / 64.0f) * Size.X, (Y / 64.0f) * Size.Y);
		};

		// Dark Obsidian Backing
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(Size, FSlateLayoutTransform()),
			FAppStyle::Get().GetBrush("WhiteBrush"),
			ESlateDrawEffect::None,
			FLinearColor(0.012f, 0.027f, 0.071f, 0.95f)
		);

		// 1. Outer Hexagonal Ballistic Shield (Afterburner Amber #FFB300)
		TArray<FVector2D> OuterHex = {
			MapPt(32.0f, 3.0f), MapPt(57.0f, 15.0f), MapPt(57.0f, 44.0f),
			MapPt(32.0f, 61.0f), MapPt(7.0f, 44.0f), MapPt(7.0f, 15.0f), MapPt(32.0f, 3.0f)
		};
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(), OuterHex, ESlateDrawEffect::None, MWTheme::AmberAccent, true, 2.2f);

		// 2. Inner Hexagonal Bevel (Tactical Cyan #00E5FF)
		TArray<FVector2D> InnerHex = {
			MapPt(32.0f, 7.0f), MapPt(53.0f, 17.5f), MapPt(53.0f, 42.0f),
			MapPt(32.0f, 56.5f), MapPt(11.0f, 42.0f), MapPt(11.0f, 17.5f), MapPt(32.0f, 7.0f)
		};
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, AllottedGeometry.ToPaintGeometry(), InnerHex, ESlateDrawEffect::None, MWTheme::CyanAccent.CopyWithNewOpacity(0.85f), true, 1.3f);

		// 3. Tactical Helmet Visor Brow Chevron (Cyan)
		TArray<FVector2D> VisorBrow = {
			MapPt(15.0f, 17.0f), MapPt(32.0f, 22.0f), MapPt(49.0f, 17.0f)
		};
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 3, AllottedGeometry.ToPaintGeometry(), VisorBrow, ESlateDrawEffect::None, MWTheme::CyanAccent, true, 2.0f);

		// 4. Interlocking "N" Monogram (Crisp Metallic White)
		TArray<FVector2D> LetterN = {
			MapPt(18.0f, 43.0f), MapPt(18.0f, 24.0f), MapPt(29.0f, 40.0f), MapPt(29.0f, 24.0f)
		};
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 4, AllottedGeometry.ToPaintGeometry(), LetterN, ESlateDrawEffect::None, MWTheme::TextPrimary, true, 2.6f);

		// 5. Interlocking "F" Monogram (Afterburner Amber #FFB300)
		TArray<FVector2D> LetterFStem = {
			MapPt(35.0f, 43.0f), MapPt(35.0f, 24.0f), MapPt(47.0f, 24.0f)
		};
		TArray<FVector2D> LetterFBar = {
			MapPt(35.0f, 33.5f), MapPt(44.5f, 33.5f)
		};
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 4, AllottedGeometry.ToPaintGeometry(), LetterFStem, ESlateDrawEffect::None, MWTheme::AmberAccent, true, 2.6f);
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 4, AllottedGeometry.ToPaintGeometry(), LetterFBar, ESlateDrawEffect::None, MWTheme::AmberAccent, true, 2.4f);

		// 6. Relativistic Chrono Ring Slash (Diagonal Cyan Slash + Amber Singularity Core)
		TArray<FVector2D> ChronoSlash = {
			MapPt(9.0f, 46.0f), MapPt(32.0f, 34.0f), MapPt(55.0f, 21.0f)
		};
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 5, AllottedGeometry.ToPaintGeometry(), ChronoSlash, ESlateDrawEffect::None, MWTheme::CyanAccent, true, 1.8f);

		const FVector2D CoreCenter = MapPt(32.0f, 34.0f) - FVector2D(2.5f, 2.5f);
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId + 6,
			AllottedGeometry.ToPaintGeometry(FVector2D(5.0f, 5.0f), FSlateLayoutTransform(CoreCenter)),
			FAppStyle::Get().GetBrush("WhiteBrush"),
			ESlateDrawEffect::None,
			MWTheme::AmberAccent
		);

		return LayerId + 7;
	}
};

/**
 * SFPSNobleFermiWordmarkWidget
 * Custom Slate vector renderer for the Battle-Scarred "NOBLE FERMI" Heavy Wordmark
 * (matching web/assets/logo.svg: 4-Corner Tactical HUD Brackets, Brushed Steel "NOBLE",
 * Afterburner Amber "FERMI", Diagonal Cyan Hacksaw Shear Line, and 7.62mm Molten Amber Bullet Crater).
 */
class SFPSNobleFermiWordmarkWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFPSNobleFermiWordmarkWidget)
		: _Compact(false)
	{}
		SLATE_ARGUMENT(bool, Compact)
	SLATE_END_ARGS()

	bool bCompact = false;

	void Construct(const FArguments& InArgs)
	{
		bCompact = InArgs._Compact;
		ChildSlot
		[
			SNew(SBox)
			.Padding(bCompact ? FMargin(10.0f, 4.0f) : FMargin(14.0f, 8.0f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Bottom)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("NOBLE ")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", bCompact ? 16 : 20))
						.ColorAndOpacity(MWTheme::TextPrimary)
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Bottom)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("FERMI")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", bCompact ? 16 : 20))
						.ColorAndOpacity(MWTheme::AmberAccent)
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 1.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("RELATIVISTIC TACTICAL COMBAT // 128-TICK")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", bCompact ? 7 : 8))
					.ColorAndOpacity(MWTheme::CyanAccent)
				]
			]
		];
	}

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
	{
		const FVector2D Size = AllottedGeometry.GetLocalSize();
		const int32 BaseLayer = SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

		// 1. 4-Corner Tactical HUD Brackets (Cyan Top-Left & Bottom-Right, Amber Top-Right & Bottom-Left)
		const float B = 7.0f;
		TArray<FVector2D> TL = { FVector2D(1.0f, B), FVector2D(1.0f, 1.0f), FVector2D(B, 1.0f) };
		TArray<FVector2D> BR = { FVector2D(Size.X - B, Size.Y - 1.0f), FVector2D(Size.X - 1.0f, Size.Y - 1.0f), FVector2D(Size.X - 1.0f, Size.Y - B) };
		TArray<FVector2D> TR = { FVector2D(Size.X - B, 1.0f), FVector2D(Size.X - 1.0f, 1.0f), FVector2D(Size.X - 1.0f, B) };
		TArray<FVector2D> BL = { FVector2D(1.0f, Size.Y - B), FVector2D(1.0f, Size.Y - 1.0f), FVector2D(B, Size.Y - 1.0f) };

		FSlateDrawElement::MakeLines(OutDrawElements, BaseLayer + 1, AllottedGeometry.ToPaintGeometry(), TL, ESlateDrawEffect::None, MWTheme::CyanAccent, true, 1.5f);
		FSlateDrawElement::MakeLines(OutDrawElements, BaseLayer + 1, AllottedGeometry.ToPaintGeometry(), BR, ESlateDrawEffect::None, MWTheme::CyanAccent, true, 1.5f);
		FSlateDrawElement::MakeLines(OutDrawElements, BaseLayer + 1, AllottedGeometry.ToPaintGeometry(), TR, ESlateDrawEffect::None, MWTheme::AmberAccent, true, 1.5f);
		FSlateDrawElement::MakeLines(OutDrawElements, BaseLayer + 1, AllottedGeometry.ToPaintGeometry(), BL, ESlateDrawEffect::None, MWTheme::AmberAccent, true, 1.5f);

		// 2. Diagonal Cyan Hacksaw Shear Line across the wordmark (from web/assets/logo.svg)
		TArray<FVector2D> HacksawLine = {
			FVector2D(8.0f, Size.Y * 0.56f),
			FVector2D(Size.X - 8.0f, Size.Y * 0.28f)
		};
		FSlateDrawElement::MakeLines(
			OutDrawElements,
			BaseLayer + 2,
			AllottedGeometry.ToPaintGeometry(),
			HacksawLine,
			ESlateDrawEffect::None,
			MWTheme::CyanAccent.CopyWithNewOpacity(0.55f),
			true,
			1.2f
		);

		// 3. 7.62mm Molten Amber Bullet Crater & Radial Stress Fractures on "FERMI"
		const FVector2D CraterCenter(Size.X * 0.72f, Size.Y * 0.36f);
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			BaseLayer + 3,
			AllottedGeometry.ToPaintGeometry(FVector2D(5.0f, 5.0f), FSlateLayoutTransform(CraterCenter - FVector2D(2.5f, 2.5f))),
			FAppStyle::Get().GetBrush("WhiteBrush"),
			ESlateDrawEffect::None,
			MWTheme::AmberAccent
		);

		TArray<FVector2D> Crack1 = { CraterCenter, CraterCenter + FVector2D(-7.0f, -6.0f) };
		TArray<FVector2D> Crack2 = { CraterCenter, CraterCenter + FVector2D(8.0f, -5.0f) };
		TArray<FVector2D> Crack3 = { CraterCenter, CraterCenter + FVector2D(5.0f, 7.0f) };
		FSlateDrawElement::MakeLines(OutDrawElements, BaseLayer + 3, AllottedGeometry.ToPaintGeometry(), Crack1, ESlateDrawEffect::None, MWTheme::AmberAccent.CopyWithNewOpacity(0.85f), true, 1.1f);
		FSlateDrawElement::MakeLines(OutDrawElements, BaseLayer + 3, AllottedGeometry.ToPaintGeometry(), Crack2, ESlateDrawEffect::None, MWTheme::CyanAccent.CopyWithNewOpacity(0.85f), true, 1.1f);
		FSlateDrawElement::MakeLines(OutDrawElements, BaseLayer + 3, AllottedGeometry.ToPaintGeometry(), Crack3, ESlateDrawEffect::None, MWTheme::AmberAccent.CopyWithNewOpacity(0.85f), true, 1.1f);

		return BaseLayer + 4;
	}
};



/**
 * SFPSChamferedHudFrameWidget
 * Renders the dark wet-glass HUD frame with 45-degree chamfered corner brackets (Cyan & Amber)
 * and subtle tactical border lines matching Docs/Menu_Mockup_Reference.jpg.
 */
class SFPSChamferedHudFrameWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFPSChamferedHudFrameWidget)
		: _Padding(FMargin(18.0f, 16.0f))
	{}
		SLATE_DEFAULT_SLOT(FArguments, Content)
		SLATE_ARGUMENT(FMargin, Padding)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		ChildSlot
		[
			SNew(SBackgroundBlur)
			.BlurStrength(16.0f)
			.bApplyAlphaToBlur(true)
			.Padding(0.0f)
			[
				SNew(SBorder)
				.BorderBackgroundColor(FLinearColor(0.016f, 0.038f, 0.075f, 0.82f))
				.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
				.Padding(InArgs._Padding)
				[
					InArgs._Content.Widget
				]
			]
		];
	}

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
	{
		const FVector2D Size = AllottedGeometry.GetLocalSize();
		const int32 BaseLayer = SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

		const float Cut = 10.0f;
		// 1. Chamfered Octagonal Outer Perimeter Line (Tactical Cyan 32% alpha)
		TArray<FVector2D> Perimeter = {
			FVector2D(Cut, 1.0f),
			FVector2D(Size.X - Cut, 1.0f),
			FVector2D(Size.X - 1.0f, Cut),
			FVector2D(Size.X - 1.0f, Size.Y - Cut),
			FVector2D(Size.X - Cut, Size.Y - 1.0f),
			FVector2D(Cut, Size.Y - 1.0f),
			FVector2D(1.0f, Size.Y - Cut),
			FVector2D(1.0f, Cut),
			FVector2D(Cut, 1.0f)
		};
		FSlateDrawElement::MakeLines(
			OutDrawElements,
			BaseLayer + 1,
			AllottedGeometry.ToPaintGeometry(),
			Perimeter,
			ESlateDrawEffect::None,
			MWTheme::CyanAccent.CopyWithNewOpacity(0.34f),
			true,
			1.2f
		);

		// 2. Glowing Chamfered Corner Brackets (Amber + Cyan accents matching mockup)
		const float Arm = 22.0f;
		TArray<FVector2D> TopLeftBracket = {
			FVector2D(1.0f, Cut + Arm), FVector2D(1.0f, Cut), FVector2D(Cut, 1.0f), FVector2D(Cut + Arm, 1.0f)
		};
		TArray<FVector2D> TopRightBracket = {
			FVector2D(Size.X - Cut - Arm, 1.0f), FVector2D(Size.X - Cut, 1.0f), FVector2D(Size.X - 1.0f, Cut), FVector2D(Size.X - 1.0f, Cut + Arm)
		};
		TArray<FVector2D> BottomLeftBracket = {
			FVector2D(1.0f, Size.Y - Cut - Arm), FVector2D(1.0f, Size.Y - Cut), FVector2D(Cut, Size.Y - 1.0f), FVector2D(Cut + Arm, Size.Y - 1.0f)
		};
		TArray<FVector2D> BottomRightBracket = {
			FVector2D(Size.X - Cut - Arm, Size.Y - 1.0f), FVector2D(Size.X - Cut, Size.Y - 1.0f), FVector2D(Size.X - 1.0f, Size.Y - Cut), FVector2D(Size.X - 1.0f, Size.Y - Cut - Arm)
		};

		FSlateDrawElement::MakeLines(OutDrawElements, BaseLayer + 2, AllottedGeometry.ToPaintGeometry(), TopLeftBracket, ESlateDrawEffect::None, MWTheme::AmberAccent, true, 2.0f);
		FSlateDrawElement::MakeLines(OutDrawElements, BaseLayer + 2, AllottedGeometry.ToPaintGeometry(), TopRightBracket, ESlateDrawEffect::None, MWTheme::AmberAccent, true, 2.0f);
		FSlateDrawElement::MakeLines(OutDrawElements, BaseLayer + 2, AllottedGeometry.ToPaintGeometry(), BottomLeftBracket, ESlateDrawEffect::None, MWTheme::CyanAccent, true, 2.0f);
		FSlateDrawElement::MakeLines(OutDrawElements, BaseLayer + 2, AllottedGeometry.ToPaintGeometry(), BottomRightBracket, ESlateDrawEffect::None, MWTheme::AmberAccent, true, 2.0f);

		return BaseLayer + 3;
	}
};

/**
 * SFPSMenuVectorIconWidget
 * Renders the 6 crisp vector line-art icons for the Left Navigation Panel (01..06)
 * matching Docs/Menu_Mockup_Reference.jpg.
 */
class SFPSMenuVectorIconWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFPSMenuVectorIconWidget)
		: _IconIndex(0)
	{}
		SLATE_ARGUMENT(int32, IconIndex)
	SLATE_END_ARGS()

	int32 IconIndex = 0;

	void Construct(const FArguments& InArgs)
	{
		IconIndex = InArgs._IconIndex;
		ChildSlot
		[
			SNew(SBox)
			.WidthOverride(28.0f)
			.HeightOverride(28.0f)
		];
	}

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
	{
		const FVector2D Size = AllottedGeometry.GetLocalSize();
		const FVector2D C = Size * 0.5f;
		const FLinearColor StrokeCol = FLinearColor(0.76f, 0.86f, 0.96f, 0.92f);
		const auto Pt = [&Size](float X, float Y) -> FVector2D { return FVector2D((X / 28.0f) * Size.X, (Y / 28.0f) * Size.Y); };

		if (IconIndex == 0) // 01: Crosshair / Target Reticle (DEPLOYMENT)
		{
			TArray<FVector2D> OuterCircle;
			for (int32 i = 0; i <= 20; ++i)
			{
				const float Ang = (float(i) / 20.0f) * 2.0f * PI;
				OuterCircle.Add(C + FVector2D(FMath::Cos(Ang), FMath::Sin(Ang)) * 9.0f);
			}
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), OuterCircle, ESlateDrawEffect::None, StrokeCol, true, 1.5f);

			TArray<FVector2D> InnerCircle;
			for (int32 i = 0; i <= 12; ++i)
			{
				const float Ang = (float(i) / 12.0f) * 2.0f * PI;
				InnerCircle.Add(C + FVector2D(FMath::Cos(Ang), FMath::Sin(Ang)) * 3.5f);
			}
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), InnerCircle, ESlateDrawEffect::None, MWTheme::CyanAccent, true, 1.3f);

			TArray<FVector2D> TickT = { Pt(14, 2), Pt(14, 7) };
			TArray<FVector2D> TickB = { Pt(14, 21), Pt(14, 26) };
			TArray<FVector2D> TickL = { Pt(2, 14), Pt(7, 14) };
			TArray<FVector2D> TickR = { Pt(21, 14), Pt(26, 14) };
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), TickT, ESlateDrawEffect::None, StrokeCol, true, 1.5f);
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), TickB, ESlateDrawEffect::None, StrokeCol, true, 1.5f);
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), TickL, ESlateDrawEffect::None, StrokeCol, true, 1.5f);
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), TickR, ESlateDrawEffect::None, StrokeCol, true, 1.5f);
		}
		else if (IconIndex == 1) // 02: Squad / Operator Busts (CHARACTER & LOADOUT)
		{
			// Center head + shoulders
			TArray<FVector2D> HeadC = { Pt(14, 6), Pt(17, 9), Pt(14, 12), Pt(11, 9), Pt(14, 6) };
			TArray<FVector2D> BodyC = { Pt(7, 23), Pt(9, 15), Pt(19, 15), Pt(21, 23) };
			// Left flank operator
			TArray<FVector2D> HeadL = { Pt(7, 8), Pt(9, 10), Pt(7, 12), Pt(5, 10), Pt(7, 8) };
			TArray<FVector2D> BodyL = { Pt(2, 21), Pt(4, 15), Pt(8, 15) };
			// Right flank operator
			TArray<FVector2D> HeadR = { Pt(21, 8), Pt(23, 10), Pt(21, 12), Pt(19, 10), Pt(21, 8) };
			TArray<FVector2D> BodyR = { Pt(20, 15), Pt(24, 15), Pt(26, 21) };

			FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), HeadC, ESlateDrawEffect::None, StrokeCol, true, 1.5f);
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), BodyC, ESlateDrawEffect::None, StrokeCol, true, 1.5f);
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), HeadL, ESlateDrawEffect::None, StrokeCol.CopyWithNewOpacity(0.7f), true, 1.3f);
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), BodyL, ESlateDrawEffect::None, StrokeCol.CopyWithNewOpacity(0.7f), true, 1.3f);
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), HeadR, ESlateDrawEffect::None, StrokeCol.CopyWithNewOpacity(0.7f), true, 1.3f);
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), BodyR, ESlateDrawEffect::None, StrokeCol.CopyWithNewOpacity(0.7f), true, 1.3f);
		}
		else if (IconIndex == 2) // 03: Isometric Supply Cube (ARSENAL)
		{
			TArray<FVector2D> OuterHex = { Pt(14, 3), Pt(24, 8.5f), Pt(24, 19.5f), Pt(14, 25), Pt(4, 19.5f), Pt(4, 8.5f), Pt(14, 3) };
			TArray<FVector2D> InnerY1 = { Pt(4, 8.5f), Pt(14, 14), Pt(24, 8.5f) };
			TArray<FVector2D> InnerY2 = { Pt(14, 14), Pt(14, 25) };
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), OuterHex, ESlateDrawEffect::None, StrokeCol, true, 1.5f);
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), InnerY1, ESlateDrawEffect::None, StrokeCol, true, 1.4f);
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), InnerY2, ESlateDrawEffect::None, StrokeCol, true, 1.4f);
		}
		else if (IconIndex == 3) // 04: Folded Tri-Fold Map (TACTICAL MAP)
		{
			TArray<FVector2D> MapOutline = { Pt(3, 6), Pt(10, 4), Pt(18, 7), Pt(25, 5), Pt(25, 22), Pt(18, 24), Pt(10, 21), Pt(3, 23), Pt(3, 6) };
			TArray<FVector2D> Fold1 = { Pt(10, 4), Pt(10, 21) };
			TArray<FVector2D> Fold2 = { Pt(18, 7), Pt(18, 24) };
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), MapOutline, ESlateDrawEffect::None, StrokeCol, true, 1.5f);
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), Fold1, ESlateDrawEffect::None, StrokeCol.CopyWithNewOpacity(0.75f), true, 1.2f);
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), Fold2, ESlateDrawEffect::None, StrokeCol.CopyWithNewOpacity(0.75f), true, 1.2f);
		}
		else if (IconIndex == 4) // 05: Ascending Telemetry Bars (OPERATIONS)
		{
			TArray<FVector2D> BaseLine = { Pt(3, 24), Pt(25, 24) };
			TArray<FVector2D> B1 = { Pt(5, 24), Pt(5, 17), Pt(8, 17), Pt(8, 24) };
			TArray<FVector2D> B2 = { Pt(10, 24), Pt(10, 13), Pt(13, 13), Pt(13, 24) };
			TArray<FVector2D> B3 = { Pt(15, 24), Pt(15, 9), Pt(18, 9), Pt(18, 24) };
			TArray<FVector2D> B4 = { Pt(20, 24), Pt(20, 5), Pt(23, 5), Pt(23, 24) };
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), BaseLine, ESlateDrawEffect::None, StrokeCol, true, 1.5f);
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), B1, ESlateDrawEffect::None, StrokeCol, true, 1.4f);
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), B2, ESlateDrawEffect::None, StrokeCol, true, 1.4f);
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), B3, ESlateDrawEffect::None, StrokeCol, true, 1.4f);
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), B4, ESlateDrawEffect::None, MWTheme::CyanAccent, true, 1.4f);
		}
		else // 06: Mechanical Gear Cog (SYSTEM OPTIONS)
		{
			TArray<FVector2D> GearOuter;
			for (int32 i = 0; i <= 16; ++i)
			{
				const float Ang = (float(i) / 16.0f) * 2.0f * PI;
				const float R = (i % 2 == 0) ? 10.5f : 8.0f;
				GearOuter.Add(C + FVector2D(FMath::Cos(Ang), FMath::Sin(Ang)) * R);
			}
			TArray<FVector2D> GearInner;
			for (int32 i = 0; i <= 12; ++i)
			{
				const float Ang = (float(i) / 12.0f) * 2.0f * PI;
				GearInner.Add(C + FVector2D(FMath::Cos(Ang), FMath::Sin(Ang)) * 4.0f);
			}
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), GearOuter, ESlateDrawEffect::None, StrokeCol, true, 1.5f);
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), GearInner, ESlateDrawEffect::None, MWTheme::CyanAccent, true, 1.4f);
		}

		return LayerId + 1;
	}
};

/**
 * SFPSTacticalRadarGlobeWidget
 * Renders the circular tactical radar globe in the Right Panel's TACTICAL OVERVIEW section
 * (N/E/S/W compass ticks, concentric radar rings, sweep line, terrain contours, and unit blips).
 */
class SFPSTacticalRadarGlobeWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFPSTacticalRadarGlobeWidget)
		: _TheaterIndex(0)
	{}
		SLATE_ATTRIBUTE(int32, TheaterIndex)
	SLATE_END_ARGS()

	TAttribute<int32> TheaterIndex;

	void Construct(const FArguments& InArgs)
	{
		TheaterIndex = InArgs._TheaterIndex;
		ChildSlot
		[
			SNew(SBox)
			.WidthOverride(132.0f)
			.HeightOverride(132.0f)
		];
	}

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
	{
		const FVector2D Size = AllottedGeometry.GetLocalSize();
		const FVector2D C = Size * 0.5f;
		const float MaxR = FMath::Min(Size.X, Size.Y) * 0.45f;
		const int32 ActiveTheater = TheaterIndex.Get(0);

		// 1. Dark Radar Backing Box
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(Size, FSlateLayoutTransform()),
			FAppStyle::Get().GetBrush("WhiteBrush"),
			ESlateDrawEffect::None,
			FLinearColor(0.01f, 0.025f, 0.05f, 0.90f)
		);

		// 2. Concentric Radar Rings (Outer, Mid, Inner)
		const float Radii[3] = { MaxR, MaxR * 0.66f, MaxR * 0.33f };
		for (int32 rIdx = 0; rIdx < 3; ++rIdx)
		{
			TArray<FVector2D> RingPts;
			for (int32 i = 0; i <= 32; ++i)
			{
				const float Ang = (float(i) / 32.0f) * 2.0f * PI;
				RingPts.Add(C + FVector2D(FMath::Cos(Ang), FMath::Sin(Ang)) * Radii[rIdx]);
			}
			FSlateDrawElement::MakeLines(
				OutDrawElements,
				LayerId + 1,
				AllottedGeometry.ToPaintGeometry(),
				RingPts,
				ESlateDrawEffect::None,
				MWTheme::CyanAccent.CopyWithNewOpacity(rIdx == 0 ? 0.75f : 0.25f),
				true,
				rIdx == 0 ? 1.6f : 1.0f
			);
		}

		// 3. Crosshair Axes (Horizontal & Vertical)
		TArray<FVector2D> AxisH = { FVector2D(C.X - MaxR, C.Y), FVector2D(C.X + MaxR, C.Y) };
		TArray<FVector2D> AxisV = { FVector2D(C.X, C.Y - MaxR), FVector2D(C.X, C.Y + MaxR) };
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, AllottedGeometry.ToPaintGeometry(), AxisH, ESlateDrawEffect::None, MWTheme::CyanAccent.CopyWithNewOpacity(0.22f), true, 1.0f);
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, AllottedGeometry.ToPaintGeometry(), AxisV, ESlateDrawEffect::None, MWTheme::CyanAccent.CopyWithNewOpacity(0.22f), true, 1.0f);

		// 4. Terrain Contour Lines inside the Radar Globe
		if (ActiveTheater == 1) // Coastal Operations: Dike spine + Dune arcs
		{
			TArray<FVector2D> DikeLine = { C + FVector2D(-MaxR * 0.85f, -4.0f), C + FVector2D(MaxR * 0.85f, 2.0f) };
			TArray<FVector2D> DuneCurve = { C + FVector2D(-MaxR * 0.65f, 16.0f), C + FVector2D(0.0f, 10.0f), C + FVector2D(MaxR * 0.65f, 20.0f) };
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 3, AllottedGeometry.ToPaintGeometry(), DikeLine, ESlateDrawEffect::None, MWTheme::AmberAccent.CopyWithNewOpacity(0.65f), true, 1.8f);
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 3, AllottedGeometry.ToPaintGeometry(), DuneCurve, ESlateDrawEffect::None, MWTheme::CyanAccent.CopyWithNewOpacity(0.45f), true, 1.2f);
		}
		else // Rust Tactical: Derrick Core + Container corridors
		{
			TArray<FVector2D> DerrickBox = {
				C + FVector2D(-10.0f, -10.0f), C + FVector2D(10.0f, -10.0f),
				C + FVector2D(10.0f, 10.0f), C + FVector2D(-10.0f, 10.0f), C + FVector2D(-10.0f, -10.0f)
			};
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 3, AllottedGeometry.ToPaintGeometry(), DerrickBox, ESlateDrawEffect::None, MWTheme::AmberAccent.CopyWithNewOpacity(0.65f), true, 1.5f);
		}

		// 5. Radar Sweep Line (40 deg azimuth)
		TArray<FVector2D> SweepLine = { C, C + FVector2D(MaxR * 0.76f, -MaxR * 0.64f) };
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 4, AllottedGeometry.ToPaintGeometry(), SweepLine, ESlateDrawEffect::None, MWTheme::CyanAccent.CopyWithNewOpacity(0.85f), true, 1.6f);

		// 6. Glowing Objective & Unit Blips (Amber Center Flag B, Emerald Friendly, Crimson Hostiles)
		auto DrawBlip = [&](FVector2D Offset, const FLinearColor& Col)
		{
			FSlateDrawElement::MakeBox(
				OutDrawElements,
				LayerId + 5,
				AllottedGeometry.ToPaintGeometry(FVector2D(5.0f, 5.0f), FSlateLayoutTransform(C + Offset - FVector2D(2.5f, 2.5f))),
				FAppStyle::Get().GetBrush("WhiteBrush"),
				ESlateDrawEffect::None,
				Col
			);
		};
		DrawBlip(FVector2D(0.0f, 0.0f), MWTheme::AmberAccent);
		DrawBlip(FVector2D(-18.0f, 14.0f), MWTheme::EmeraldReady);
		DrawBlip(FVector2D(-10.0f, 22.0f), MWTheme::EmeraldReady);
		DrawBlip(FVector2D(16.0f, -15.0f), MWTheme::CrimsonAlert);
		DrawBlip(FVector2D(24.0f, -6.0f), MWTheme::CrimsonAlert);

		return LayerId + 6;
	}
};

/**
 * SFPSSegmentedBarWidget
 * Renders the 10-segment tactical status bars (ENEMY PRESENCE / FRIENDLY UNITS) in the Right Panel.
 */
class SFPSSegmentedBarWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFPSSegmentedBarWidget)
		: _FilledSegments(7)
		, _TotalSegments(10)
		, _ActiveColor(MWTheme::CrimsonAlert)
	{}
		SLATE_ARGUMENT(int32, FilledSegments)
		SLATE_ARGUMENT(int32, TotalSegments)
		SLATE_ARGUMENT(FLinearColor, ActiveColor)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
		const int32 Total = FMath::Max(1, InArgs._TotalSegments);
		const int32 Filled = FMath::Clamp(InArgs._FilledSegments, 0, Total);

		for (int32 i = 0; i < Total; ++i)
		{
			const bool bOn = (i < Filled);
			Row->AddSlot()
				.AutoWidth()
				.Padding(0.0f, 0.0f, 2.5f, 0.0f)
				[
					SNew(SBox)
					.WidthOverride(7.0f)
					.HeightOverride(9.0f)
					[
						SNew(SBorder)
						.BorderBackgroundColor(bOn ? InArgs._ActiveColor : FLinearColor(0.10f, 0.16f, 0.24f, 0.65f))
						.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
					]
				];
		}

		ChildSlot
		[
			Row
		];
	}
};

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



class SFPSTheaterMapPreviewWidget : public SCompoundWidget

{

public:

	SLATE_BEGIN_ARGS(SFPSTheaterMapPreviewWidget)

		: _TheaterIndex(0)

	{}

		SLATE_ATTRIBUTE(int32, TheaterIndex)

	SLATE_END_ARGS()



	void Construct(const FArguments& InArgs)

	{

		TheaterIndex = InArgs._TheaterIndex;

	}



	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override

	{

		const FVector2D LocalSize = AllottedGeometry.GetLocalSize();

		if (LocalSize.X <= 20.0f || LocalSize.Y <= 20.0f) return LayerId;



		const int32 CurrentTheater = TheaterIndex.Get(0);

		const FSlateBrush* WhiteBrush = FAppStyle::Get().GetBrush("WhiteBrush");



		// 1. Dark blueprint background

		FSlateDrawElement::MakeBox(

			OutDrawElements,

			LayerId,

			AllottedGeometry.ToPaintGeometry(FVector2f(LocalSize), FSlateLayoutTransform()),

			WhiteBrush,

			ESlateDrawEffect::None,

			FLinearColor(0.02f, 0.04f, 0.07f, 0.88f)

		);



		// 2. Tactical grid lines

		const int32 NumGridX = 6;

		const int32 NumGridY = 3;

		for (int32 gx = 1; gx < NumGridX; ++gx)

		{

			const float XPos = (LocalSize.X / float(NumGridX)) * float(gx);

			TArray<FVector2D> GridLine;

			GridLine.Add(FVector2D(XPos, 2.0f));

			GridLine.Add(FVector2D(XPos, LocalSize.Y - 2.0f));

			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(), GridLine, ESlateDrawEffect::None, FLinearColor(0.08f, 0.14f, 0.20f, 0.5f), false, 1.0f);

		}

		for (int32 gy = 1; gy < NumGridY; ++gy)

		{

			const float YPos = (LocalSize.Y / float(NumGridY)) * float(gy);

			TArray<FVector2D> GridLine;

			GridLine.Add(FVector2D(2.0f, YPos));

			GridLine.Add(FVector2D(LocalSize.X - 2.0f, YPos));

			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(), GridLine, ESlateDrawEffect::None, FLinearColor(0.08f, 0.14f, 0.20f, 0.5f), false, 1.0f);

		}



		if (CurrentTheater == 0)

		{

			// ==========================================

			// RUST TACTICAL: 60M x 60M DESERT OIL YARD

			// ==========================================

			const FVector2D BoundsMin(24.0f, 8.0f);

			const FVector2D BoundsMax(LocalSize.X - 24.0f, LocalSize.Y - 10.0f);



			// Perimeter box

			TArray<FVector2D> Perim;

			Perim.Add(BoundsMin);

			Perim.Add(FVector2D(BoundsMax.X, BoundsMin.Y));

			Perim.Add(BoundsMax);

			Perim.Add(FVector2D(BoundsMin.X, BoundsMax.Y));

			Perim.Add(BoundsMin);

			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, AllottedGeometry.ToPaintGeometry(), Perim, ESlateDrawEffect::None, MWTheme::AmberAccent.CopyWithNewOpacity(0.75f), false, 1.5f);



			// Central Derrick (Square + X-Truss)

			const FVector2D Center = (BoundsMin + BoundsMax) * 0.5f;

			const float DerrickR = 13.0f;

			TArray<FVector2D> DerrickBox;

			DerrickBox.Add(Center + FVector2D(-DerrickR, -DerrickR));

			DerrickBox.Add(Center + FVector2D(DerrickR, -DerrickR));

			DerrickBox.Add(Center + FVector2D(DerrickR, DerrickR));

			DerrickBox.Add(Center + FVector2D(-DerrickR, DerrickR));

			DerrickBox.Add(Center + FVector2D(-DerrickR, -DerrickR));

			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 3, AllottedGeometry.ToPaintGeometry(), DerrickBox, ESlateDrawEffect::None, MWTheme::AmberAccent, false, 2.0f);



			TArray<FVector2D> Cross1;

			Cross1.Add(Center + FVector2D(-DerrickR, -DerrickR));

			Cross1.Add(Center + FVector2D(DerrickR, DerrickR));

			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 3, AllottedGeometry.ToPaintGeometry(), Cross1, ESlateDrawEffect::None, MWTheme::AmberAccent.CopyWithNewOpacity(0.6f), false, 1.0f);



			TArray<FVector2D> Cross2;

			Cross2.Add(Center + FVector2D(-DerrickR, DerrickR));

			Cross2.Add(Center + FVector2D(DerrickR, -DerrickR));

			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 3, AllottedGeometry.ToPaintGeometry(), Cross2, ESlateDrawEffect::None, MWTheme::AmberAccent.CopyWithNewOpacity(0.6f), false, 1.0f);



			// Containers (Flank Lane A)

			FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 3, AllottedGeometry.ToPaintGeometry(FVector2f(20.0f, 9.0f), FSlateLayoutTransform(FVector2f(BoundsMin.X + 12.0f, Center.Y - 14.0f))), WhiteBrush, ESlateDrawEffect::None, FLinearColor(0.12f, 0.35f, 0.65f, 0.85f));

			FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 3, AllottedGeometry.ToPaintGeometry(FVector2f(20.0f, 9.0f), FSlateLayoutTransform(FVector2f(BoundsMin.X + 12.0f, Center.Y + 4.0f))), WhiteBrush, ESlateDrawEffect::None, FLinearColor(0.65f, 0.18f, 0.12f, 0.85f));



			// Pipeline Trench (Flank Lane B)

			TArray<FVector2D> PipeLine1;

			PipeLine1.Add(FVector2D(BoundsMax.X - 22.0f, BoundsMin.Y + 4.0f));

			PipeLine1.Add(FVector2D(BoundsMax.X - 22.0f, BoundsMax.Y - 4.0f));

			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 3, AllottedGeometry.ToPaintGeometry(), PipeLine1, ESlateDrawEffect::None, MWTheme::CyanAccent.CopyWithNewOpacity(0.85f), false, 3.0f);

		}

		else

		{

			// ==============================================================

			// COASTAL OPERATIONS: 120M x 120M DUNES, SEAWALL DIKE & PIER

			// ==============================================================

			const FVector2D BoundsMin(24.0f, 8.0f);

			const FVector2D BoundsMax(LocalSize.X - 24.0f, LocalSize.Y - 10.0f);



			// Perimeter box

			TArray<FVector2D> Perim;

			Perim.Add(BoundsMin);

			Perim.Add(FVector2D(BoundsMax.X, BoundsMin.Y));

			Perim.Add(BoundsMax);

			Perim.Add(FVector2D(BoundsMin.X, BoundsMax.Y));

			Perim.Add(BoundsMin);

			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, AllottedGeometry.ToPaintGeometry(), Perim, ESlateDrawEffect::None, MWTheme::CyanAccent.CopyWithNewOpacity(0.75f), false, 1.5f);



			// 1. Sand Dune Contours (Undulating sine curves across top sector)

			const FLinearColor DuneColor = FLinearColor(0.85f, 0.65f, 0.28f, 0.70f);

			for (int32 dune = 0; dune < 3; ++dune)

			{

				const float BaseY = BoundsMin.Y + 10.0f + dune * 12.0f;

				const int32 NumSteps = 24;

				TArray<FVector2D> DuneCurve;

				DuneCurve.Reserve(NumSteps + 1);

				for (int32 s = 0; s <= NumSteps; ++s)

				{

					const float Frac = float(s) / float(NumSteps);

					const float CurX = FMath::Lerp(BoundsMin.X + 4.0f, BoundsMax.X - 4.0f, Frac);

					const float WaveOffset = FMath::Sin(Frac * 3.0f * UE_PI + float(dune) * 1.2f) * 4.0f;

					DuneCurve.Add(FVector2D(CurX, BaseY + WaveOffset));

				}

				FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 3, AllottedGeometry.ToPaintGeometry(), DuneCurve, ESlateDrawEffect::None, DuneColor, false, 1.5f);

			}



			// 2. Seawall Dike Spine (Diagonal asphalt crest road with dual edges)

			const FVector2D DikeStart(BoundsMin.X + 6.0f, BoundsMax.Y - 24.0f);

			const FVector2D DikeEnd(BoundsMax.X - 6.0f, BoundsMin.Y + 34.0f);



			TArray<FVector2D> DikeCenter;

			DikeCenter.Add(DikeStart);

			DikeCenter.Add(DikeEnd);

			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 4, AllottedGeometry.ToPaintGeometry(), DikeCenter, ESlateDrawEffect::None, FLinearColor(0.24f, 0.26f, 0.28f, 1.0f), false, 6.0f);



			TArray<FVector2D> DikeEdgeTop;

			DikeEdgeTop.Add(DikeStart + FVector2D(0.0f, -3.0f));

			DikeEdgeTop.Add(DikeEnd + FVector2D(0.0f, -3.0f));

			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 5, AllottedGeometry.ToPaintGeometry(), DikeEdgeTop, ESlateDrawEffect::None, FLinearColor(0.48f, 0.50f, 0.52f, 0.85f), false, 1.0f);



			TArray<FVector2D> DikeEdgeBot;

			DikeEdgeBot.Add(DikeStart + FVector2D(0.0f, 3.0f));

			DikeEdgeBot.Add(DikeEnd + FVector2D(0.0f, 3.0f));

			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 5, AllottedGeometry.ToPaintGeometry(), DikeEdgeBot, ESlateDrawEffect::None, FLinearColor(0.48f, 0.50f, 0.52f, 0.85f), false, 1.0f);



			// 3. Maritime Pier (Wharf jutting down into water from the dike)

			const FVector2D PierOrigin(BoundsMin.X + (BoundsMax.X - BoundsMin.X) * 0.62f, BoundsMax.Y - 18.0f);

			const FVector2D PierTip = PierOrigin + FVector2D(0.0f, 16.0f);



			TArray<FVector2D> PierDeck;

			PierDeck.Add(PierOrigin);

			PierDeck.Add(PierTip);

			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 5, AllottedGeometry.ToPaintGeometry(), PierDeck, ESlateDrawEffect::None, FLinearColor(0.55f, 0.42f, 0.28f, 0.95f), false, 5.0f);



			// Pier Pilings dots

			FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 6, AllottedGeometry.ToPaintGeometry(FVector2f(3.0f, 3.0f), FSlateLayoutTransform(FVector2f(PierTip.X - 3.5f, PierTip.Y - 1.5f))), WhiteBrush, ESlateDrawEffect::None, MWTheme::CyanAccent);

			FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 6, AllottedGeometry.ToPaintGeometry(FVector2f(3.0f, 3.0f), FSlateLayoutTransform(FVector2f(PierTip.X + 1.5f, PierTip.Y - 1.5f))), WhiteBrush, ESlateDrawEffect::None, MWTheme::CyanAccent);



			// 4. Ocean Littoral Waves (Cyan arcs in bottom right ocean corner)

			for (int32 w = 0; w < 3; ++w)

			{

				const float WaveY = BoundsMax.Y - 8.0f + float(w) * 3.5f;

				TArray<FVector2D> OceanWave;

				OceanWave.Add(FVector2D(BoundsMax.X - 34.0f, WaveY));

				OceanWave.Add(FVector2D(BoundsMax.X - 24.0f, WaveY - 2.0f));

				OceanWave.Add(FVector2D(BoundsMax.X - 14.0f, WaveY));

				OceanWave.Add(FVector2D(BoundsMax.X - 4.0f, WaveY - 2.0f));

				FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 4, AllottedGeometry.ToPaintGeometry(), OceanWave, ESlateDrawEffect::None, MWTheme::CyanAccent.CopyWithNewOpacity(0.55f), false, 1.0f);

			}

		}



		// Outer tactical frame & corner pips

		TArray<FVector2D> FramePts;

		FramePts.Add(FVector2D(0.0f, 0.0f));

		FramePts.Add(FVector2D(LocalSize.X, 0.0f));

		FramePts.Add(FVector2D(LocalSize.X, LocalSize.Y));

		FramePts.Add(FVector2D(0.0f, LocalSize.Y));

		FramePts.Add(FVector2D(0.0f, 0.0f));

		FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 7, AllottedGeometry.ToPaintGeometry(), FramePts, ESlateDrawEffect::None, MWTheme::GlassBorder, false, 1.0f);



		return LayerId + 8;

	}



	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(340.0f, 105.0f); }



private:

	TAttribute<int32> TheaterIndex;

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



	if (UGameInstance* GI = GetGameInstance())

	{

		if (UFPSAuthSubsystem* Auth = GI->GetSubsystem<UFPSAuthSubsystem>())

		{

			Auth->OnAuthStateChanged.RemoveDynamic(this, &UMainMenuWidget::HandleAuthStateChanged);

			Auth->OnAuthStateChanged.AddDynamic(this, &UMainMenuWidget::HandleAuthStateChanged);



			Auth->OnAuthError.RemoveDynamic(this, &UMainMenuWidget::HandleAuthError);

			Auth->OnAuthError.AddDynamic(this, &UMainMenuWidget::HandleAuthError);

		}



		if (UFPSEconomySubsystem* Economy = GI->GetSubsystem<UFPSEconomySubsystem>())

		{

			SelectedTheaterIndex = Economy->GetSelectedTheaterIndex();

		}

	}



	UpdateTheaterCardVisuals();

	UpdateCurrencyHeader();

	UpdateAuthUI();

	ReadMenuHardwareSettings();

	UpdateMenuGraphicsUI();

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
	// Left Navigation Row Builder matching Docs/Menu_Mockup_Reference.jpg (01..06 with Vector Icons & Dividers)
	auto MakeMockupSideNavItem = [this](const FString& NumStr, int32 IconIdx, const FString& Title, const FString& Subtitle, EFPSMenuSection Section, TSharedPtr<SBorder>& OutIndicator, bool bShowBottomDivider = true) -> TSharedRef<SWidget>
	{
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBox)
				.HeightOverride(64.0f)
				[
					SNew(SButton)
					.ButtonColorAndOpacity(FLinearColor::Transparent)
					.ContentPadding(0.0f)
					.OnClicked_Lambda([this, Section]()
					{
						NavigateToSection(Section);
						return FReply::Handled();
					})
					[
						SNew(SHorizontalBox)
						// Left 5px Glowing Cyan Active Bar
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SBox)
							.WidthOverride(5.0f)
							[
								SAssignNew(OutIndicator, SBorder)
								.BorderBackgroundColor(Section == EFPSMenuSection::Play ? MWTheme::CyanAccent : FLinearColor::Transparent)
								.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
							]
						]
						// Row Content (Large Cyan Index + Vector Line-Art Icon + Bold Title + Muted Subtitle)
						+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						.VAlign(VAlign_Center)
						.Padding(14.0f, 6.0f)
						[
							SNew(SHorizontalBox)
							// Large Cyan Number (01..06)
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							.Padding(0.0f, 0.0f, 14.0f, 0.0f)
							[
								SNew(STextBlock)
								.Text(FText::FromString(NumStr))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 15))
								.ColorAndOpacity(MWTheme::CyanAccent)
							]
							// Custom Vector Line-Art Icon (28x28)
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							.Padding(0.0f, 0.0f, 14.0f, 0.0f)
							[
								SNew(SFPSMenuVectorIconWidget)
								.IconIndex(IconIdx)
							]
							// Title & Subtitle Stack
							+ SHorizontalBox::Slot()
							.FillWidth(1.0f)
							.VAlign(VAlign_Center)
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot()
								.AutoHeight()
								[
									SNew(STextBlock)
									.Text(FText::FromString(Title))
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
									.ColorAndOpacity(MWTheme::TextPrimary)
								]
								+ SVerticalBox::Slot()
								.AutoHeight()
								.Padding(0.0f, 2.0f, 0.0f, 0.0f)
								[
									SNew(STextBlock)
									.Text(FText::FromString(Subtitle))
									.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
									.ColorAndOpacity(MWTheme::TextMuted)
								]
							]
						]
					]
				]
			]
			// Crisp 1px Horizontal Divider Line between items
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBox)
				.HeightOverride(bShowBottomDivider ? 1.0f : 0.0f)
				.Padding(12.0f, 0.0f)
				[
					SNew(SBorder)
					.BorderBackgroundColor(MWTheme::CyanAccent.CopyWithNewOpacity(0.18f))
					.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
				]
			];
	};

	// Dedicated Operator Customization View (Slot 5): Unobstructed 3D Operator on Left/Center + Categorized Submenu on Right
	auto BuildOperatorCustomizationSubmenuView = [this]() -> TSharedRef<SWidget>
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.Padding(0.0f, 0.0f, 16.0f, 0.0f)
			[
				BuildCenterViewportHUD()
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SBox)
				.WidthOverride(410.0f)
				[
					SNew(SFPSChamferedHudFrameWidget)
					.Padding(FMargin(18.0f, 16.0f))
					[
						BuildRightBlade()
					]
				]
			];
	};

	return SNew(SOverlay)
		// Layer 0: Subtle Atmospheric Vignette (leaves center 3D operator 100% sharp)
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor(0.01f, 0.02f, 0.05f, 0.10f))
			.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
			.Padding(0.0f)
		]
		// Layer 1: Top Glowing Cyan Horizon Line
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
		// Layer 2: Main 4-Zone Mockup Layout (Top Telemetry Bar + Left HUD Nav + Center/Right Viewport + Bottom Footer)
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		.Padding(20.0f, 10.0f, 20.0f, 10.0f)
		[
			SNew(SVerticalBox)
			// Zone 1: Top Telemetry Header Bar (with yellow EXIT button on far right)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 14.0f)
			[
				SNew(SBackgroundBlur)
				.BlurStrength(16.0f)
				.bApplyAlphaToBlur(true)
				.Padding(0.0f)
				[
					SNew(SBorder)
					.BorderBackgroundColor(MWTheme::CyanAccent.CopyWithNewOpacity(0.32f))
					.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
					.Padding(1.0f)
					[
						SNew(SBorder)
						.BorderBackgroundColor(FLinearColor(0.014f, 0.032f, 0.065f, 0.86f))
						.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
						.Padding(16.0f, 8.0f)
						[
							BuildHeader()
						]
					]
				]
			]
			// Zone 2: Main Body (Left Chamfered Navigation Panel + Right Content Switcher)
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SNew(SHorizontalBox)
				// LEFT CHAMFERED HUD NAVIGATION PANEL (295px wide — matches Docs/Menu_Mockup_Reference.jpg)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.0f, 0.0f, 18.0f, 0.0f)
				[
					SNew(SBox)
					.WidthOverride(295.0f)
					[
						SNew(SFPSChamferedHudFrameWidget)
						.Padding(FMargin(6.0f, 14.0f))
						[
							SNew(SVerticalBox)
							// 01: DEPLOYMENT (Mission Selection & Briefing)
							+ SVerticalBox::Slot().AutoHeight()
							[
								MakeMockupSideNavItem(TEXT("01"), 0, TEXT("DEPLOYMENT"), TEXT("MISSION SELECTION & BRIEFING"), EFPSMenuSection::Play, TabIndicatorPlay)
							]
							// 02: CHARACTER & LOADOUT (Operator Customization)
							+ SVerticalBox::Slot().AutoHeight()
							[
								MakeMockupSideNavItem(TEXT("02"), 1, TEXT("CHARACTER & LOADOUT"), TEXT("OPERATOR CUSTOMIZATION"), EFPSMenuSection::Customization, TabIndicatorOperator)
							]
							// 03: ARSENAL (Weapons & Gear)
							+ SVerticalBox::Slot().AutoHeight()
							[
								MakeMockupSideNavItem(TEXT("03"), 2, TEXT("ARSENAL"), TEXT("WEAPONS & GEAR"), EFPSMenuSection::Loadout, TabIndicatorGunsmith)
							]
							// 04: TACTICAL MAP (Intel & Strategy / Server Browser)
							+ SVerticalBox::Slot().AutoHeight()
							[
								MakeMockupSideNavItem(TEXT("04"), 3, TEXT("TACTICAL MAP"), TEXT("INTEL & STRATEGY"), EFPSMenuSection::ServerBrowser, MWTheme::TabIndicatorServerBrowser)
							]
							// 05: OPERATIONS (Campaign Progression / Bounties & Vault)
							+ SVerticalBox::Slot().AutoHeight()
							[
								MakeMockupSideNavItem(TEXT("05"), 4, TEXT("OPERATIONS"), TEXT("CAMPAIGN PROGRESSION"), EFPSMenuSection::DailyRewards, TabIndicatorDailyRewards)
							]
							// 06: SYSTEM OPTIONS (Graphics / Audio / Controls)
							+ SVerticalBox::Slot().AutoHeight()
							[
								MakeMockupSideNavItem(TEXT("06"), 5, TEXT("SYSTEM OPTIONS"), TEXT("GRAPHICS / AUDIO / CONTROLS"), EFPSMenuSection::Settings, TabIndicatorSettings, false)
							]
							// Spacer pushing bottom watermark down
							+ SVerticalBox::Slot()
							.FillHeight(1.0f)
							[
								SNew(SBox)
							]
							// Bottom Panel Watermark (/// NOBLE FERMI — TACTICAL WARFARE SIMULATION)
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(16.0f, 10.0f, 16.0f, 4.0f)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
								.AutoWidth()
								.VAlign(VAlign_Center)
								.Padding(0.0f, 0.0f, 8.0f, 0.0f)
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("///")))
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
									.ColorAndOpacity(MWTheme::CyanAccent.CopyWithNewOpacity(0.65f))
								]
								+ SHorizontalBox::Slot()
								.FillWidth(1.0f)
								.VAlign(VAlign_Center)
								[
									SNew(SVerticalBox)
									+ SVerticalBox::Slot().AutoHeight()
									[
										SNew(STextBlock)
										.Text(FText::FromString(TEXT("NOBLE FERMI")))
										.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
										.ColorAndOpacity(MWTheme::TextSecondary)
									]
									+ SVerticalBox::Slot().AutoHeight()
									[
										SNew(STextBlock)
										.Text(FText::FromString(TEXT("TACTICAL WARFARE SIMULATION")))
										.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
										.ColorAndOpacity(MWTheme::TextMuted)
									]
								]
							]
						]
					]
				]
				// RIGHT VIEWPORT SWITCHER
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SAssignNew(ContentSwitcher, SWidgetSwitcher)
					.WidgetIndex(0)
					+ SWidgetSwitcher::Slot() [ BuildTacticalLobbyView() ]               // 0: Arrival Home (Unobstructed 3D Operator + Right Mission Briefing Panel)
					+ SWidgetSwitcher::Slot() [ BuildGunsmithView() ]                    // 1: 03 Arsenal / Gunsmith
					+ SWidgetSwitcher::Slot() [ BuildServerBrowserView() ]               // 2: 04 Tactical Map & Network
					+ SWidgetSwitcher::Slot() [ BuildSettingsView() ]                    // 3: 06 System Options
					+ SWidgetSwitcher::Slot() [ BuildDailyRewardsView() ]                // 4: 05 Operations / Armory Vault
					+ SWidgetSwitcher::Slot() [ BuildOperatorCustomizationSubmenuView() ] // 5: 02 Character & Loadout
				]
			]
			// Zone 3: Bottom Chamfered Telemetry & Motto Footer Bar
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
					.BorderBackgroundColor(MWTheme::CyanAccent.CopyWithNewOpacity(0.32f))
					.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
					.Padding(1.0f)
					[
						SNew(SBorder)
						.BorderBackgroundColor(FLinearColor(0.014f, 0.032f, 0.065f, 0.86f))
						.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
						.Padding(18.0f, 7.0f)
						[
							BuildFooter()
						]
					]
				]
			]
		]
		// Layer 3: Tactical Operator Authentication & 6-Digit Website Verification Modal Overlay
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			BuildAuthModal()
		];
}

TSharedRef<SWidget> UMainMenuWidget::BuildHeader()
{
	return SNew(SHorizontalBox)
		// Far Left: Hexagonal Shield Emblem + NOBLE FERMI (TACTICAL WARFARE SIMULATION) + ONLINE • 3/8 OPERATORS
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
				SNew(SFPSNobleFermiEmblemWidget)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, 18.0f, 0.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("NOBLE ")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 15))
						.ColorAndOpacity(MWTheme::TextPrimary)
					]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("FERMI")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 15))
						.ColorAndOpacity(MWTheme::AmberAccent)
					]
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("TACTICAL WARFARE SIMULATION")))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
					.ColorAndOpacity(MWTheme::TextMuted)
				]
			]
			// Angled Status Pill: ONLINE • 3/8 OPERATORS
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0.0f, 0.0f, 6.0f, 0.0f)
				[
					SNew(SBox)
					.WidthOverride(7.0f)
					.HeightOverride(7.0f)
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
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("ONLINE  •  3/8 OPERATORS")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
					.ColorAndOpacity(MWTheme::CyanAccent)
				]
			]
		]
		// Center Telemetry Strip: VERSION 1.2.8.47 | EU-WEST-1 | 42 MS | Gold & Opals
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, 16.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("VERSION 1.2.8.47    |    EU-WEST-1    |    42 MS")))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
				.ColorAndOpacity(MWTheme::TextSecondary)
			]
			// Gold Balance
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, 10.0f, 0.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 4.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("🪙")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
					.ColorAndOpacity(MWTheme::AmberAccent)
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SAssignNew(GoldBalanceText, STextBlock)
					.Text_Lambda([this]()
					{
						if (UGameInstance* GI = GetGameInstance())
						{
							if (UFPSEconomySubsystem* Economy = GI->GetSubsystem<UFPSEconomySubsystem>())
							{
								return FText::AsNumber(Economy->GetGoldBalance());
							}
						}
						return FText::FromString(TEXT("1,500"));
					})
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
					.ColorAndOpacity(MWTheme::AmberAccent)
				]
			]
			// Opal Balance
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 4.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("💎")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
					.ColorAndOpacity(MWTheme::CyanAccent)
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SAssignNew(OpalBalanceText, STextBlock)
					.Text_Lambda([this]()
					{
						if (UGameInstance* GI = GetGameInstance())
						{
							if (UFPSEconomySubsystem* Economy = GI->GetSubsystem<UFPSEconomySubsystem>())
							{
								return FText::AsNumber(Economy->GetOpalBalance());
							}
						}
						return FText::FromString(TEXT("50"));
					})
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
					.ColorAndOpacity(MWTheme::CyanAccent)
				]
			]
		]
		// Far Right: LOADOUT | SETTINGS | ACCOUNT + Bright Yellow [ ⏻ EXIT ] Button (matching mockup!)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SHorizontalBox)
			// LOADOUT Quick Link (Highlighted in Amber like mockup)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, 10.0f, 0.0f)
			[
				SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "SimpleButton")
				.OnClicked_Lambda([this]()
				{
					NavigateToSection(EFPSMenuSection::Customization);
					return FReply::Handled();
				})
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("LOADOUT")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
					.ColorAndOpacity(MWTheme::AmberAccent)
				]
			]
			// SETTINGS Quick Link
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, 10.0f, 0.0f)
			[
				SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "SimpleButton")
				.OnClicked_Lambda([this]()
				{
					NavigateToSection(EFPSMenuSection::Settings);
					return FReply::Handled();
				})
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("⚙ SETTINGS")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
					.ColorAndOpacity(MWTheme::TextSecondary)
				]
			]
			// ACCOUNT / CLOUD SYNC Link
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, 14.0f, 0.0f)
			[
				SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "SimpleButton")
				.OnClicked_Lambda([this]() { return HandleLogoutOrSwitchClicked(); })
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 4.0f, 0.0f)
					[
						SAssignNew(HeaderOperatorStatusBadge, STextBlock)
						.Text(FText::FromString(TEXT("🎧 OPERATOR")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
						.ColorAndOpacity(MWTheme::CyanAccent)
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SAssignNew(HeaderAuthActionText, STextBlock)
						.Text(FText::FromString(TEXT("")))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
						.ColorAndOpacity(MWTheme::TextMuted)
					]
				]
			]
			// Prominent Amber/Yellow [ ⏻ EXIT ] Button at Top-Right (exact match to mockup!)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SButton)
				.ButtonColorAndOpacity(FLinearColor::Transparent)
				.ContentPadding(0.0f)
				.OnClicked_Lambda([this]() { return HandleQuitClicked(); })
				[
					SNew(SBorder)
					.BorderBackgroundColor(MWTheme::AmberAccent)
					.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
					.Padding(14.0f, 6.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("⏻  EXIT")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
						.ColorAndOpacity(FLinearColor(0.02f, 0.03f, 0.05f, 1.0f))
					]
				]
			]
		];
}

TSharedRef<SWidget> UMainMenuWidget::BuildTacticalLobbyView()
{
	return SNew(SHorizontalBox)
		// Zone A: Completely Unobstructed Full-Height 3D Operator in Center Viewport!
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		.Padding(0.0f, 0.0f, 18.0f, 0.0f)
		[
			SNew(SBox)
		]
		// Zone B: Right Chamfered Mission Briefing & Tactical Overview Panel (395px wide — matches mockup!)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SBox)
			.WidthOverride(395.0f)
			[
				SNew(SFPSChamferedHudFrameWidget)
				.Padding(FMargin(18.0f, 16.0f))
				[
					BuildLeftBlade()
				]
			]
		];
}

TSharedRef<SWidget> UMainMenuWidget::BuildLeftBlade()
{
	auto MakeMockupActionPill = [](const FString& Label, TFunction<FReply()> OnClick) -> TSharedRef<SWidget>
	{
		return SNew(SButton)
			.ButtonColorAndOpacity(FLinearColor::Transparent)
			.ContentPadding(0.0f)
			.OnClicked_Lambda(OnClick)
			[
				SNew(SBorder)
				.BorderBackgroundColor(MWTheme::CyanAccent.CopyWithNewOpacity(0.35f))
				.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
				.Padding(1.0f)
				[
					SNew(SBorder)
					.BorderBackgroundColor(FLinearColor(0.02f, 0.05f, 0.09f, 0.90f))
					.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
					.Padding(8.0f, 7.0f)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(FText::FromString(Label))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
						.ColorAndOpacity(MWTheme::TextPrimary)
					]
				]
			];
	};

	return SNew(SVerticalBox)
		// 1. Top Panel Brand Header (Shield Emblem + NOBLE FERMI — TACTICAL WARFARE SIMULATION)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 16.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, 12.0f, 0.0f)
			[
				SNew(SFPSNobleFermiEmblemWidget)
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("NOBLE ")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
						.ColorAndOpacity(MWTheme::TextPrimary)
					]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("FERMI")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
						.ColorAndOpacity(MWTheme::AmberAccent)
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 1.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("TACTICAL WARFARE SIMULATION")))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
					.ColorAndOpacity(MWTheme::TextMuted)
				]
			]
		]
		// 2. ACTIVE OPERATION Header + LOBBY 3/8
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
				.Text(FText::FromString(TEXT("ACTIVE OPERATION")))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
				.ColorAndOpacity(MWTheme::CyanAccent)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("LOBBY 3/8")))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
				.ColorAndOpacity(MWTheme::TextSecondary)
			]
		]
		// 3. Clickable ACTIVE OPERATION Selector Box (Click '>' to cycle between Rust Tactical & Coastal Operations!)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 14.0f)
		[
			SNew(SButton)
			.ButtonColorAndOpacity(FLinearColor::Transparent)
			.ContentPadding(0.0f)
			.ToolTipText(FText::FromString(TEXT("Click to cycle Combat Operation (Rust Tactical <-> Coastal Operations)")))
			.OnClicked_Lambda([this]()
			{
				const int32 NextTheater = (SelectedTheaterIndex == 0) ? 1 : 0;
				return HandleTheaterCardClicked(NextTheater);
			})
			[
				SNew(SBorder)
				.BorderBackgroundColor(MWTheme::CyanAccent)
				.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
				.Padding(1.5f)
				[
					SNew(SBorder)
					.BorderBackgroundColor(FLinearColor(0.025f, 0.065f, 0.125f, 0.92f))
					.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
					.Padding(14.0f, 12.0f)
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
								.Text_Lambda([this]()
								{
									return FText::FromString(SelectedTheaterIndex == 1
										? TEXT("OPERATION: COASTAL OPS")
										: TEXT("OPERATION: RUST TACTICAL"));
								})
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
								.ColorAndOpacity(MWTheme::TextPrimary)
							]
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0.0f, 3.0f, 0.0f, 0.0f)
							[
								SNew(STextBlock)
								.Text_Lambda([this]()
								{
									return FText::FromString(SelectedTheaterIndex == 1
										? TEXT("North Sea Littoral / Dike & Pier")
										: TEXT("Industrial Combat / High Intensity"));
								})
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
								.ColorAndOpacity(MWTheme::TextSecondary)
							]
						]
						// Right Amber '>' Cycler Chevron (exact match to mockup!)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(10.0f, 0.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("❯")))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
							.ColorAndOpacity(MWTheme::AmberAccent)
						]
					]
				]
			]
		]
		// 4. OPERATION BRIEF Section
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 4.0f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("OPERATION BRIEF")))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
			.ColorAndOpacity(MWTheme::TextPrimary)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 14.0f)
		[
			SAssignNew(TheaterTelemetryText, STextBlock)
			.Text_Lambda([this]()
			{
				const FString ModeName = (SelectedModeIndex == 1) ? TEXT("Chrono Dilation Lab") : ((SelectedModeIndex == 2) ? TEXT("AI Bot Skirmish") : TEXT("6v6 Tactical TDM"));
				if (SelectedTheaterIndex == 1)
				{
					return FText::FromString(FString::Printf(
						TEXT("Secure the North Sea seawall dike crest, eliminate hostiles across the dunes.\nMode: %s  |  Recommended: Kit 1, Level IV Plates."),
						*ModeName));
				}
				return FText::FromString(FString::Printf(
					TEXT("Secure the central oil derrick, eliminate hostiles, extract tactical intel.\nMode: %s  |  Recommended: Kit 2, Medium Armor."),
					*ModeName));
			})
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
			.ColorAndOpacity(MWTheme::TextSecondary)
			.AutoWrapText(true)
		]
		// Thin Divider
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 10.0f)
		[
			SNew(SBox)
			.HeightOverride(1.0f)
			[
				SNew(SBorder)
				.BorderBackgroundColor(MWTheme::CyanAccent.CopyWithNewOpacity(0.22f))
				.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
			]
		]
		// 5. TACTICAL OVERVIEW Header
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 10.0f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("TACTICAL OVERVIEW")))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
			.ColorAndOpacity(MWTheme::CyanAccent)
		]
		// 6. TACTICAL OVERVIEW Split Row: Circular Radar Globe (Left) + Segmented Threat/Unit/Time Meters (Right)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 16.0f)
		[
			SNew(SHorizontalBox)
			// Left: Circular Tactical Radar Globe (132x132)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, 16.0f, 0.0f)
			[
				SNew(SFPSTacticalRadarGlobeWidget)
				.TheaterIndex_Lambda([this]() { return SelectedTheaterIndex; })
			]
			// Right: 3 Stacked Telemetry Readouts (ENEMY PRESENCE, FRIENDLY UNITS, MISSION TIME)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				SNew(SVerticalBox)
				// Meter 1: ENEMY PRESENCE (HIGH)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 10.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 4.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("ENEMY PRESENCE")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 7))
						.ColorAndOpacity(MWTheme::TextMuted)
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							SNew(SFPSSegmentedBarWidget)
							.FilledSegments(8)
							.TotalSegments(10)
							.ActiveColor(MWTheme::CrimsonAlert)
						]
						+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Right).VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("HIGH")))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
							.ColorAndOpacity(MWTheme::CrimsonAlert)
						]
					]
				]
				// Meter 2: FRIENDLY UNITS (3/8)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 10.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 4.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("FRIENDLY UNITS")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 7))
						.ColorAndOpacity(MWTheme::TextMuted)
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							SNew(SFPSSegmentedBarWidget)
							.FilledSegments(4)
							.TotalSegments(10)
							.ActiveColor(MWTheme::CyanAccent)
						]
						+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Right).VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("3/8")))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
							.ColorAndOpacity(MWTheme::TextSecondary)
						]
					]
				]
				// Meter 3: MISSION TIME (00:15:00)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 3.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("MISSION TIME")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 7))
						.ColorAndOpacity(MWTheme::TextMuted)
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("⏱  00:15:00")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
						.ColorAndOpacity(MWTheme::TextPrimary)
					]
				]
			]
		]
		// Spacer so action pills & yellow DEPLOY button sit cleanly at the bottom of the right panel
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SNew(SBox)
		]
		// 7. Three Quick Action Pills ([ VIEW INTEL ]  [ LOADOUT ]  [ RULES ]) matching mockup
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 12.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.Padding(0.0f, 0.0f, 4.0f, 0.0f)
			[
				MakeMockupActionPill(TEXT("VIEW INTEL"), [this]()
				{
					NavigateToSection(EFPSMenuSection::ServerBrowser);
					return FReply::Handled();
				})
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.Padding(2.0f, 0.0f)
			[
				MakeMockupActionPill(TEXT("LOADOUT"), [this]()
				{
					NavigateToSection(EFPSMenuSection::Customization);
					return FReply::Handled();
				})
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.Padding(4.0f, 0.0f, 0.0f, 0.0f)
			[
				MakeMockupActionPill(TEXT("RULES"), [this]()
				{
					const int32 NextMode = (SelectedModeIndex + 1) % 3;
					return HandleModeCardClicked(NextMode);
				})
			]
		]
		// 8. Prominent Bright Amber/Yellow [ DEPLOY TO AREA > ] Hero Button (exact match to mockup!)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SBox)
			.HeightOverride(58.0f)
			[
				SNew(SButton)
				.ButtonColorAndOpacity(FLinearColor::Transparent)
				.ContentPadding(0.0f)
				.OnClicked_Lambda([this]() { return HandleQuickplayClicked(); })
				[
					SNew(SBorder)
					.BorderBackgroundColor(MWTheme::AmberAccent)
					.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
					.Padding(18.0f, 8.0f)
					[
						SNew(SHorizontalBox)
						// Left Target Icon
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(0.0f, 0.0f, 12.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("🎯")))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 15))
							.ColorAndOpacity(FLinearColor(0.02f, 0.03f, 0.05f, 1.0f))
						]
						// Center Title + Active Operation Subtitle
						+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot()
							.AutoHeight()
							.HAlign(HAlign_Center)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("DEPLOY TO AREA")))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
								.ColorAndOpacity(FLinearColor(0.02f, 0.03f, 0.05f, 1.0f))
							]
							+ SVerticalBox::Slot()
							.AutoHeight()
							.HAlign(HAlign_Center)
							[
								SNew(STextBlock)
								.Text_Lambda([this]()
								{
									return FText::FromString(SelectedTheaterIndex == 1
										? TEXT("OPERATION: COASTAL OPERATIONS")
										: TEXT("OPERATION: RUST TACTICAL"));
								})
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
								.ColorAndOpacity(FLinearColor(0.14f, 0.10f, 0.02f, 1.0f))
							]
						]
						// Right '>' Chevron
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(12.0f, 0.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("❯")))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 15))
							.ColorAndOpacity(FLinearColor(0.02f, 0.03f, 0.05f, 1.0f))
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



	auto MakeCategorySubTab = [](int32 TabIdx, const FString& Label, TSharedPtr<SBorder>& OutBorder) -> TSharedRef<SWidget>
	{
		return SNew(SButton)
			.ButtonColorAndOpacity(FLinearColor::Transparent)
			.ContentPadding(0.0f)
			.OnClicked_Lambda([TabIdx]()
			{
				MWTheme::ActiveOperatorCategoryTab = TabIdx;
				if (MWTheme::OperatorCategorySwitcher.IsValid())
				{
					MWTheme::OperatorCategorySwitcher->SetActiveWidgetIndex(TabIdx);
				}
				if (MWTheme::OperatorCatBorder0.IsValid()) MWTheme::OperatorCatBorder0->SetBorderBackgroundColor(TabIdx == 0 ? MWTheme::ActiveBorder : MWTheme::GlassBorder);
				if (MWTheme::OperatorCatBorder1.IsValid()) MWTheme::OperatorCatBorder1->SetBorderBackgroundColor(TabIdx == 1 ? MWTheme::ActiveBorder : MWTheme::GlassBorder);
				if (MWTheme::OperatorCatBorder2.IsValid()) MWTheme::OperatorCatBorder2->SetBorderBackgroundColor(TabIdx == 2 ? MWTheme::ActiveBorder : MWTheme::GlassBorder);
				if (MWTheme::OperatorCatBorder3.IsValid()) MWTheme::OperatorCatBorder3->SetBorderBackgroundColor(TabIdx == 3 ? MWTheme::ActiveBorder : MWTheme::GlassBorder);
				return FReply::Handled();
			})
			[
				SAssignNew(OutBorder, SBorder)
				.BorderBackgroundColor(MWTheme::ActiveOperatorCategoryTab == TabIdx ? MWTheme::ActiveBorder : MWTheme::GlassBorder)
				.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
				.Padding(1.5f)
				[
					SNew(SBorder)
					.BorderBackgroundColor(MWTheme::DarkGlassCard)
					.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
					.Padding(6.0f, 7.0f)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(FText::FromString(Label))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
						.ColorAndOpacity(MWTheme::TextPrimary)
					]
				]
			];
	};

	return SNew(SVerticalBox)
		// Submenu Header + Back to Deployment button
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 10.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("02 // OPERATOR RIG & APPEARANCE")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
					.ColorAndOpacity(MWTheme::CyanAccent)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("LIVE 3D SKELETAL ARMOR & CAMOUFLAGE BAY")))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
					.ColorAndOpacity(MWTheme::TextMuted)
				]
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SButton)
				.ButtonColorAndOpacity(MWTheme::DarkGlassCard)
				.ContentPadding(FMargin(8.0f, 4.0f))
				.OnClicked_Lambda([this]()
				{
					NavigateToSection(EFPSMenuSection::Play);
					return FReply::Handled();
				})
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("◄ BACK TO DEPLOY")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
					.ColorAndOpacity(MWTheme::AmberAccent)
				]
			]
		]
		// 4 Category Sub-Tabs ([ 01 CAMO ] [ 02 HEADGEAR ] [ 03 VEST ] [ 04 RIFLE ])
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 12.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(0.0f, 0.0f, 3.0f, 0.0f)
			[
				MakeCategorySubTab(0, TEXT("01 // CAMO"), MWTheme::OperatorCatBorder0)
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.0f, 0.0f)
			[
				MakeCategorySubTab(1, TEXT("02 // HELMET"), MWTheme::OperatorCatBorder1)
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.0f, 0.0f)
			[
				MakeCategorySubTab(2, TEXT("03 // VEST"), MWTheme::OperatorCatBorder2)
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(3.0f, 0.0f, 0.0f, 0.0f)
			[
				MakeCategorySubTab(3, TEXT("04 // RIFLE"), MWTheme::OperatorCatBorder3)
			]
		]
		// Category Content Switcher (Only shows the selected category so it's never overwhelming!)
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SAssignNew(MWTheme::OperatorCategorySwitcher, SWidgetSwitcher)
			.WidgetIndex(MWTheme::ActiveOperatorCategoryTab)
			// Tab 0: Uniform Camouflage Swatches
			+ SWidgetSwitcher::Slot()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("SELECT UNIFORM CAMOUFLAGE PATTERN:")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
					.ColorAndOpacity(MWTheme::AmberAccent)
				]
				+ SVerticalBox::Slot().AutoHeight() [ MakeCamoSwatchButton(0, TEXT("camo_digital"), TEXT("TASK FORCE ECHO SLATE"), TEXT("Spec-Ops Ripstop // Deep Charcoal & Slate"), FLinearColor(0.12f, 0.15f, 0.19f), 0, 0) ]
				+ SVerticalBox::Slot().AutoHeight() [ MakeCamoSwatchButton(1, TEXT("camo_woodland"), TEXT("WOODLAND TACTICAL"), TEXT("North Sea Littoral // Olive & Khaki"), FLinearColor(0.18f, 0.26f, 0.14f), 2000, 30) ]
				+ SVerticalBox::Slot().AutoHeight() [ MakeCamoSwatchButton(2, TEXT("camo_desert"), TEXT("DESERT SPEC-OPS"), TEXT("Rust Yard // Coyote Tan & FDE"), FLinearColor(0.50f, 0.44f, 0.32f), 2500, 40) ]
				+ SVerticalBox::Slot().AutoHeight() [ MakeCamoSwatchButton(3, TEXT("camo_blackout"), TEXT("STEALTH BLACKOUT"), TEXT("Night Ops // Obsidian Matte"), FLinearColor(0.06f, 0.06f, 0.08f), 3500, 60) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f, 0.0f, 0.0f)
				[
					SAssignNew(CamoStatusText, STextBlock)
					.Text(FText::FromString(TEXT("ACTIVE SWATCH: DIGITAL // URBAN CHARCOAL")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
					.ColorAndOpacity(MWTheme::CyanAccent)
				]
			]
			// Tab 1: Ballistic Headgear & NVGs
			+ SWidgetSwitcher::Slot()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("SELECT BALLISTIC HEADGEAR & OPTICS:")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
					.ColorAndOpacity(MWTheme::AmberAccent)
				]
				+ SVerticalBox::Slot().AutoHeight() [ MakePurchasableItemBtn(TEXT("headgear_fast"), TEXT("FAST BALLISTIC HELMET (STD)"), 0, 0, [this]() { return HandleHeadgearSelected(0); }) ]
				+ SVerticalBox::Slot().AutoHeight() [ MakePurchasableItemBtn(TEXT("headgear_quadnods"), TEXT("GPNVG-18 QUAD-NODS (PHOSPHOR)"), 3000, 50, [this]() { return HandleHeadgearSelected(1); }) ]
				+ SVerticalBox::Slot().AutoHeight() [ MakePurchasableItemBtn(TEXT("headgear_balaclava"), TEXT("SPEC OPS FR BALACLAVA"), 1800, 25, [this]() { return HandleHeadgearSelected(2); }) ]
				+ SVerticalBox::Slot().AutoHeight() [ MakePurchasableItemBtn(TEXT("headgear_cap"), TEXT("TACTICAL FLEECE CAP & PELTOR"), 1200, 20, [this]() { return HandleHeadgearSelected(3); }) ]
			]
			// Tab 2: Thoracic Plate Carriers & Rigs
			+ SWidgetSwitcher::Slot()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("SELECT THORACIC PLATE CARRIER RIG:")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
					.ColorAndOpacity(MWTheme::AmberAccent)
				]
				+ SVerticalBox::Slot().AutoHeight() [ MakePurchasableItemBtn(TEXT("vest_level4"), TEXT("LEVEL IV HEAVY CERAMIC CARRIER"), 0, 0, [this]() { return HandleVestSelected(0); }) ]
				+ SVerticalBox::Slot().AutoHeight() [ MakePurchasableItemBtn(TEXT("vest_jpc"), TEXT("LIGHTWEIGHT JPC SWIMMER CUT"), 2200, 35, [this]() { return HandleVestSelected(1); }) ]
				+ SVerticalBox::Slot().AutoHeight() [ MakePurchasableItemBtn(TEXT("vest_recon"), TEXT("RECON CHEST HARNESS & POUCHES"), 2800, 45, [this]() { return HandleVestSelected(2); }) ]
			]
			// Tab 3: NF-98 Rifle Finishes
			+ SWidgetSwitcher::Slot()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("SELECT NF-98 CARBINE RECEIVER FINISH:")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
					.ColorAndOpacity(MWTheme::AmberAccent)
				]
				+ SVerticalBox::Slot().AutoHeight() [ MakePurchasableItemBtn(TEXT("weapon_steel"), TEXT("FACTORY BLUED ORDNANCE STEEL"), 0, 0, [this]() { return HandleWeaponFinishSelected(0); }) ]
				+ SVerticalBox::Slot().AutoHeight() [ MakePurchasableItemBtn(TEXT("weapon_cerakote"), TEXT("DESERT FDE CERAKOTE"), 1800, 30, [this]() { return HandleWeaponFinishSelected(1); }) ]
				+ SVerticalBox::Slot().AutoHeight() [ MakePurchasableItemBtn(TEXT("weapon_carbon"), TEXT("AUTOCLAVE CARBON FIBER"), 3200, 50, [this]() { return HandleWeaponFinishSelected(2); }) ]
				+ SVerticalBox::Slot().AutoHeight() [ MakePurchasableItemBtn(TEXT("weapon_damascene"), TEXT("24K GOLD & OPAL DAMASCENE"), 6500, 90, [this]() { return HandleWeaponFinishSelected(3); }) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f, 0.0f, 0.0f)
				[
					SAssignNew(WeaponFinishStatusText, STextBlock)
					.Text(FText::FromString(TEXT("RIFLE FINISH: FACTORY BLUED STEEL")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
					.ColorAndOpacity(MWTheme::CyanAccent)
				]
			]
		]
		// Bottom Pinned Controls: 3D Operator Stance & Active Loadout Summary
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 10.0f, 0.0f, 0.0f)
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
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("// 3D POSE & ANIMATION PREVIEW")))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
							.ColorAndOpacity(MWTheme::AmberAccent)
						]
						+ SHorizontalBox::Slot().AutoWidth()
						[
							SAssignNew(StanceStatusText, STextBlock)
							.Text(FText::FromString(TEXT("STANCE: LOW-READY CARBINE")))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
							.ColorAndOpacity(MWTheme::CyanAccent)
						]
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(0.0f, 0.0f, 3.0f, 0.0f)
						[
							SNew(SButton)
							.ButtonColorAndOpacity(MWTheme::DarkGlass)
							.OnClicked_Lambda([this]()
							{
								if (PreviewActor) PreviewActor->SetPreviewStance(0);
								if (StanceStatusText.IsValid()) StanceStatusText->SetText(FText::FromString(TEXT("STANCE: LOW-READY CARBINE")));
								return FReply::Handled();
							})
							.HAlign(HAlign_Center)
							[
								SNew(STextBlock).Text(FText::FromString(TEXT("LOW READY"))).Font(FCoreStyle::GetDefaultFontStyle("Bold", 8)).ColorAndOpacity(MWTheme::CyanAccent)
							]
						]
						+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.0f, 0.0f)
						[
							SNew(SButton)
							.ButtonColorAndOpacity(MWTheme::DarkGlass)
							.OnClicked_Lambda([this]()
							{
								if (PreviewActor) PreviewActor->SetPreviewStance(1);
								if (StanceStatusText.IsValid()) StanceStatusText->SetText(FText::FromString(TEXT("STANCE: HIGH-READY ADS")));
								return FReply::Handled();
							})
							.HAlign(HAlign_Center)
							[
								SNew(STextBlock).Text(FText::FromString(TEXT("HIGH ADS"))).Font(FCoreStyle::GetDefaultFontStyle("Bold", 8)).ColorAndOpacity(MWTheme::TextPrimary)
							]
						]
						+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(3.0f, 0.0f, 0.0f, 0.0f)
						[
							SNew(SButton)
							.ButtonColorAndOpacity(MWTheme::DarkGlass)
							.OnClicked_Lambda([this]()
							{
								if (PreviewActor) PreviewActor->SetPreviewStance(2);
								if (StanceStatusText.IsValid()) StanceStatusText->SetText(FText::FromString(TEXT("STANCE: PATROL WALK")));
								return FReply::Handled();
							})
							.HAlign(HAlign_Center)
							[
								SNew(STextBlock).Text(FText::FromString(TEXT("WALK CYCLE"))).Font(FCoreStyle::GetDefaultFontStyle("Bold", 8)).ColorAndOpacity(MWTheme::TextPrimary)
							]
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
		.BlurStrength(16.0f)
		.bApplyAlphaToBlur(true)
		.Padding(0.0f)
		[
			SNew(SBorder)
			.BorderBackgroundColor(MWTheme::GlassBorder)
			.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
			.Padding(1.5f)
			[
				SNew(SBorder)
				.BorderBackgroundColor(MWTheme::DarkGlass)
				.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
				.Padding(18.0f)
				[
					SNew(SHorizontalBox)
					// Left Column: Regional 128-Hz Dedicated Servers + Listen Room Host + Direct IP Connect
					+ SHorizontalBox::Slot()
					.FillWidth(0.55f)
					.Padding(0.0f, 0.0f, 16.0f, 0.0f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 0.0f, 0.0f, 10.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("05 // AUTHORITATIVE 128-TICK NETWORK & DIRECT IP")))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
							.ColorAndOpacity(MWTheme::CyanAccent)
						]
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
								.Padding(14.0f)
								[
									SNew(SVerticalBox)
									+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)
									[
										SNew(STextBlock)
										.Text(FText::FromString(TEXT("REGIONAL 128-HZ DEDICATED VPS NODES")))
										.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
										.ColorAndOpacity(MWTheme::AmberAccent)
									]
									+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f)
									[
										SNew(STextBlock)
										.Text(FText::FromString(TEXT("• [EU-WEST] Amsterdam #1 — 10/12 Operators | Map: Coastal Operations | 14ms")))
										.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
										.ColorAndOpacity(MWTheme::TextPrimary)
									]
									+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f)
									[
										SNew(STextBlock)
										.Text(FText::FromString(TEXT("• [EU-CENTRAL] Frankfurt #2 — 12/12 Operators | Map: Rust Tactical | 18ms")))
										.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
										.ColorAndOpacity(MWTheme::TextPrimary)
									]
									+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f)
									[
										SNew(STextBlock)
										.Text(FText::FromString(TEXT("• [LOCALHOST] 127.0.0.1:7777 — Dedicated Server Binary Ready")))
										.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
										.ColorAndOpacity(MWTheme::EmeraldReady)
									]
								]
							]
						]
						// Direct IP Connect Box
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 0.0f, 0.0f, 10.0f)
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
									+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)
									[
										SNew(STextBlock)
										.Text(FText::FromString(TEXT("DIRECT VPS DEDICATED IP CONNECTION:")))
										.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
										.ColorAndOpacity(MWTheme::TextMuted)
									]
									+ SVerticalBox::Slot().AutoHeight()
									[
										SNew(SHorizontalBox)
										+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(0.0f, 0.0f, 8.0f, 0.0f)
										[
											SAssignNew(DirectConnectInput, SEditableTextBox)
											.Text(FText::FromString(TEXT("127.0.0.1:7777")))
											.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
											.HintText(FText::FromString(TEXT("IP:PORT")))
										]
										+ SHorizontalBox::Slot().AutoWidth()
										[
											SNew(SBox)
											.WidthOverride(90.0f)
											.HeightOverride(32.0f)
											[
												SNew(SButton)
												.ButtonColorAndOpacity(MWTheme::CyanAccent)
												.OnClicked_Lambda([this]() { return HandleDirectConnectClicked(); })
												.HAlign(HAlign_Center)
												.VAlign(VAlign_Center)
												[
													SNew(STextBlock)
													.Text(FText::FromString(TEXT("CONNECT ►")))
													.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
													.ColorAndOpacity(FLinearColor(0.02f, 0.04f, 0.06f, 1.0f))
												]
											]
										]
									]
								]
							]
						]
						// Host 128-Hz Listen Server Button
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SBox)
							.HeightOverride(40.0f)
							[
								SNew(SButton)
								.ButtonColorAndOpacity(FLinearColor::Transparent)
								.ContentPadding(0.0f)
								.OnClicked_Lambda([this]() { return HandleHostListenClicked(); })
								[
									SNew(SBorder)
									.BorderBackgroundColor(MWTheme::AmberAccent)
									.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
									.HAlign(HAlign_Center)
									.VAlign(VAlign_Center)
									[
										SNew(STextBlock)
										.Text(FText::FromString(TEXT("HOST 128-HZ LISTEN SERVER ROOM  ►")))
										.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
										.ColorAndOpacity(FLinearColor(0.02f, 0.03f, 0.05f, 1.0f))
									]
								]
							]
						]
					]
					// Right Column: Active Daily Contracts & Bounty Board
					+ SHorizontalBox::Slot()
					.FillWidth(0.45f)
					[
						BuildBountyBoardPanel()
					]
				]
			]
		];
}

TSharedRef<SWidget> UMainMenuWidget::BuildSettingsView()

{

	ReadMenuHardwareSettings();



	const FSlateFontInfo SectionHeaderFont = FCoreStyle::GetDefaultFontStyle("Bold", 11);

	const FSlateFontInfo HeaderTitleFont = FCoreStyle::GetDefaultFontStyle("Bold", 14);

	const FSlateFontInfo SubTitleFont = FCoreStyle::GetDefaultFontStyle("Regular", 9);

	const FSlateFontInfo PresetTitleFont = FCoreStyle::GetDefaultFontStyle("Bold", 10);

	const FSlateFontInfo PresetSubFont = FCoreStyle::GetDefaultFontStyle("Regular", 8);

	const FSlateFontInfo ToggleFont = FCoreStyle::GetDefaultFontStyle("Bold", 9);



	auto MakePresetCard = [this, PresetTitleFont, PresetSubFont](int32 Idx, const FString& Title, const FString& Subtitle, const FString& BadgeTag, const FLinearColor& TagColor, TSharedPtr<SBorder>& OutBorder) -> TSharedRef<SWidget>

	{

		return SNew(SBox)

			.Padding(0.0f, 0.0f, 0.0f, 8.0f)

			[

				SNew(SButton)

				.ButtonColorAndOpacity(FLinearColor::Transparent)

				.ContentPadding(0.0f)

				.OnClicked_Lambda([this, Idx]() { return HandleMenuGraphicsPresetClicked(Idx); })

				[

					SAssignNew(OutBorder, SBorder)

					.BorderBackgroundColor(MenuGraphicsPresetIndex == Idx ? MWTheme::ActiveBorder : MWTheme::GlassBorder)

					.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))

					.Padding(1.5f)

					[

						SNew(SBorder)

						.BorderBackgroundColor(MWTheme::DarkGlassCard)

						.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))

						.Padding(12.0f, 10.0f)

						[

							SNew(SVerticalBox)

							+ SVerticalBox::Slot()

							.AutoHeight()

							.Padding(0.0f, 0.0f, 0.0f, 3.0f)

							[

								SNew(SHorizontalBox)

								+ SHorizontalBox::Slot()

								.FillWidth(1.0f)

								.VAlign(VAlign_Center)

								[

									SNew(STextBlock)

									.Text(FText::FromString(Title))

									.Font(PresetTitleFont)

									.ColorAndOpacity(MWTheme::AmberAccent)

								]

								+ SHorizontalBox::Slot()

								.AutoWidth()

								.VAlign(VAlign_Center)

								[

									SNew(STextBlock)

									.Text(FText::FromString(BadgeTag))

									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))

									.ColorAndOpacity(TagColor)

								]

							]

							+ SVerticalBox::Slot()

							.AutoHeight()

							[

								SNew(STextBlock)

								.Text(FText::FromString(Subtitle))

								.Font(PresetSubFont)

								.ColorAndOpacity(MWTheme::TextSecondary)

							]

						]

					]

				]

			];

	};



	auto MakeToggleRow = [this, ToggleFont](TSharedPtr<STextBlock>& OutBlock, const FString& DefaultText, FOnClicked OnClicked) -> TSharedRef<SWidget>

	{

		return SNew(SBox)

			.HeightOverride(38.0f)

			.Padding(0.0f, 2.0f)

			[

				SNew(SButton)

				.ButtonColorAndOpacity(FLinearColor::Transparent)

				.ContentPadding(0.0f)

				.OnClicked(OnClicked)

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

						.Padding(8.0f, 4.0f)

						[

							SAssignNew(OutBlock, STextBlock)

							.Text(FText::FromString(DefaultText))

							.Font(ToggleFont)

							.ColorAndOpacity(MWTheme::AmberAccent)

						]

					]

				]

			];

	};



	TSharedRef<SWidget> SettingsWidget = SNew(SBackgroundBlur)

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

					// Header

					+ SVerticalBox::Slot()

					.AutoHeight()

					.Padding(0.0f, 0.0f, 0.0f, 4.0f)

					[

						SNew(STextBlock)

						.Text(FText::FromString(TEXT("/// OPERATOR HARDWARE & GRAPHICS ENGINE CONFIGURATION")))

						.Font(HeaderTitleFont)

						.ColorAndOpacity(MWTheme::CyanAccent)

					]

					+ SVerticalBox::Slot()

					.AutoHeight()

					.Padding(0.0f, 0.0f, 0.0f, 12.0f)

					[

						SNew(STextBlock)

						.Text(FText::FromString(TEXT("INSTANT SCALING: LAPTOP PERFORMANCE (60+ FPS) <-> ULTRA CINEMATIC (LUMEN + VSM) // ZERO RESTART")))

						.Font(SubTitleFont)

						.ColorAndOpacity(MWTheme::TextMuted)

					]

					// Active Preset Telemetry Banner

					+ SVerticalBox::Slot()

					.AutoHeight()

					.Padding(0.0f, 0.0f, 0.0f, 12.0f)

					[

						SNew(SBorder)

						.BorderBackgroundColor(MWTheme::DarkGlassCard)

						.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))

						.Padding(12.0f, 8.0f)

						.HAlign(HAlign_Center)

						[

							SAssignNew(MenuPresetBadge, STextBlock)

							.Text(FText::FromString(TEXT("ACTIVE PRESET: [ LAPTOP / ESPORTS (60+ FPS) ]")))

							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))

							.ColorAndOpacity(MWTheme::CyanAccent)

						]

					]

					// 4 Main Presets (2x2 Grid)

					+ SVerticalBox::Slot()

					.AutoHeight()

					.Padding(0.0f, 0.0f, 0.0f, 4.0f)

					[

						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()

						.FillWidth(0.5f)

						.Padding(0.0f, 0.0f, 6.0f, 0.0f)

						[

							MakePresetCard(

								0,

								TEXT("[ 1 ] LAPTOP / ESPORTS (MAX FPS)"),

								TEXT("Lumen OFF • Virtual Shadows OFF • 70% TSR Scale • Target 60-144 FPS on Laptops"),

								TEXT("[ LAPTOP RECOMMENDED ]"),

								MWTheme::CyanAccent,

								PresetCardBorder0

							)

						]

						+ SHorizontalBox::Slot()

						.FillWidth(0.5f)

						.Padding(6.0f, 0.0f, 0.0f, 0.0f)

						[

							MakePresetCard(

								1,

								TEXT("[ 2 ] BALANCED (MEDIUM QUALITY)"),

								TEXT("Lumen OFF • Standard CSM Shadows • 85% TSR • Smooth 60 FPS on Mid-Range GPUs"),

								TEXT("[ SWEET SPOT ]"),

								MWTheme::EmeraldReady,

								PresetCardBorder1

							)

						]

					]

					+ SVerticalBox::Slot()

					.AutoHeight()

					.Padding(0.0f, 0.0f, 0.0f, 10.0f)

					[

						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()

						.FillWidth(0.5f)

						.Padding(0.0f, 0.0f, 6.0f, 0.0f)

						[

							MakePresetCard(

								2,

								TEXT("[ 3 ] HIGH (LUMEN SOFT GI)"),

								TEXT("Lumen Raytraced GI ON • Standard CSM Shadows • 100% Native Scale"),

								TEXT("[ NEXT-GEN GI ]"),

								MWTheme::AmberAccent,

								PresetCardBorder2

							)

						]

						+ SHorizontalBox::Slot()

						.FillWidth(0.5f)

						.Padding(6.0f, 0.0f, 0.0f, 0.0f)

						[

							MakePresetCard(

								3,

								TEXT("[ 4 ] ULTRA / BEAUTIFUL (MAX CINEMATIC)"),

								TEXT("Lumen Raytraced GI & Reflections ON • Virtual Shadow Maps ON • 100% Native • Volumetric Fog ON"),

								TEXT("[ MAX FIDELITY ]"),

								MWTheme::CrimsonAlert,

								PresetCardBorder3

							)

						]

					]

					// Granular Toggles Sub-Header

					+ SVerticalBox::Slot()

					.AutoHeight()

					.Padding(0.0f, 4.0f, 0.0f, 4.0f)

					[

						SNew(STextBlock)

						.Text(FText::FromString(TEXT("/// GRANULAR HARDWARE TOGGLES (FINE TUNING)")))

						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))

						.ColorAndOpacity(MWTheme::AmberAccent)

					]

					+ SVerticalBox::Slot()

					.AutoHeight()

					.Padding(0.0f, 0.0f, 0.0f, 8.0f)

					[

						SNew(STextBlock)

						.Text(FText::FromString(TEXT("FINE-TUNE INDIVIDUAL RAYTRACING, VIRTUAL SHADOW MAPS, 3D RESOLUTION SCALE & REFRESH LIMITS")))

						.Font(SubTitleFont)

						.ColorAndOpacity(MWTheme::TextMuted)

					]

					// Granular Toggles (2x2 Grid)

					+ SVerticalBox::Slot()

					.AutoHeight()

					.Padding(0.0f, 0.0f, 0.0f, 4.0f)

					[

						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()

						.FillWidth(0.5f)

						.Padding(0.0f, 0.0f, 4.0f, 0.0f)

						[

							MakeToggleRow(MenuLumenLabel, TEXT("LUMEN GI: [ OFF // HIGH FPS ]"), FOnClicked::CreateUObject(this, &UMainMenuWidget::HandleMenuToggleLumenClicked))

						]

						+ SHorizontalBox::Slot()

						.FillWidth(0.5f)

						.Padding(4.0f, 0.0f, 0.0f, 0.0f)

						[

							MakeToggleRow(MenuVSMLabel, TEXT("VIRTUAL SHADOWS: [ OFF // FAST CSM ]"), FOnClicked::CreateUObject(this, &UMainMenuWidget::HandleMenuToggleVSMClicked))

						]

					]

					+ SVerticalBox::Slot()

					.AutoHeight()

					.Padding(0.0f, 0.0f, 0.0f, 14.0f)

					[

						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()

						.FillWidth(0.5f)

						.Padding(0.0f, 0.0f, 4.0f, 0.0f)

						[

							MakeToggleRow(MenuResolutionLabel, TEXT("RENDER SCALE: [ 75% ]"), FOnClicked::CreateUObject(this, &UMainMenuWidget::HandleMenuCycleResolutionClicked))

						]

						+ SHorizontalBox::Slot()

						.FillWidth(0.5f)

						.Padding(4.0f, 0.0f, 0.0f, 0.0f)

						[

							MakeToggleRow(MenuFrameRateLimitLabel, TEXT("FRAME RATE CAP: [ 144 FPS ]"), FOnClicked::CreateUObject(this, &UMainMenuWidget::HandleMenuCycleFpsLimitClicked))

						]

					]

					// Separator

					+ SVerticalBox::Slot()

					.AutoHeight()

					.Padding(0.0f, 6.0f, 0.0f, 12.0f)

					[

						SNew(SSeparator)

						.ColorAndOpacity(MWTheme::GlassBorder)

					]

					// Gameplay & Controls Card

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

								.Font(SectionHeaderFont)

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

					// Audio & Acoustic Simulation Card

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

								.Font(SectionHeaderFont)

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



	UpdateMenuGraphicsUI();

	return SettingsWidget;

}



void UMainMenuWidget::ReadMenuHardwareSettings()

{

	bool bFoundSavedConfig = false;

	if (GConfig)

	{

		int32 LumenVal = 0;

		if (GConfig->GetInt(TEXT("SystemSettings"), TEXT("r.DynamicGlobalIlluminationMethod"), LumenVal, GEngineIni))

		{

			bMenuLumenActive = (LumenVal > 0);

			bFoundSavedConfig = true;

		}

		int32 VSMVal = 0;

		if (GConfig->GetInt(TEXT("SystemSettings"), TEXT("r.Shadow.Virtual.Enable"), VSMVal, GEngineIni))

		{

			bMenuVSMActive = (VSMVal > 0);

		}

		float ScreenPerc = 70.0f;

		if (GConfig->GetFloat(TEXT("SystemSettings"), TEXT("r.ScreenPercentage"), ScreenPerc, GEngineIni))

		{

			MenuScreenPercentage = ScreenPerc;

		}

	}



	if (!bFoundSavedConfig)

	{

		// Default to Laptop / Esports Preset (Preset 0) on unconfigured debug / iGPU launches

		ApplyMenuGraphicsPreset(0);

		return;

	}



	if (bMenuLumenActive && bMenuVSMActive)

	{

		MenuGraphicsPresetIndex = 3; // Ultra

	}

	else if (bMenuLumenActive && !bMenuVSMActive)

	{

		MenuGraphicsPresetIndex = 2; // High

	}

	else if (!bMenuLumenActive && MenuScreenPercentage > 80.0f)

	{

		MenuGraphicsPresetIndex = 1; // Balanced

	}

	else

	{

		MenuGraphicsPresetIndex = 0; // Laptop / Esports

	}



	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();

	if (Settings)

	{

		MenuFpsLimit = static_cast<int32>(Settings->GetFrameRateLimit());

		if (MenuFpsLimit <= 0) MenuFpsLimit = 0;

	}

}



void UMainMenuWidget::UpdateMenuGraphicsUI()

{

	if (MenuPresetBadge.IsValid())

	{

		FString BadgeText;

		FLinearColor BadgeColor;

		switch (MenuGraphicsPresetIndex)

		{

		case 0:

			BadgeText = TEXT("ACTIVE PRESET: [ LAPTOP / ESPORTS // MAX FPS (60-144) ]");

			BadgeColor = MWTheme::CyanAccent;

			break;

		case 1:

			BadgeText = TEXT("ACTIVE PRESET: [ BALANCED // MEDIUM QUALITY (SMOOTH 60) ]");

			BadgeColor = MWTheme::EmeraldReady;

			break;

		case 2:

			BadgeText = TEXT("ACTIVE PRESET: [ HIGH // LUMEN RAYTRACING ACTIVE ]");

			BadgeColor = MWTheme::AmberAccent;

			break;

		case 3:

		default:

			BadgeText = TEXT("ACTIVE PRESET: [ ULTRA // LUMEN & VIRTUAL SHADOWS (MAX CINEMATIC) ]");

			BadgeColor = MWTheme::CrimsonAlert;

			break;

		}

		MenuPresetBadge->SetText(FText::FromString(BadgeText));

		MenuPresetBadge->SetColorAndOpacity(BadgeColor);

	}



	if (MenuLumenLabel.IsValid())

	{

		MenuLumenLabel->SetText(FText::FromString(bMenuLumenActive ? TEXT("LUMEN GI: [ ON // RAYTRACED ]") : TEXT("LUMEN GI: [ OFF // HIGH FPS ]")));

		MenuLumenLabel->SetColorAndOpacity(bMenuLumenActive ? MWTheme::AmberAccent : MWTheme::TextMuted);

	}



	if (MenuVSMLabel.IsValid())

	{

		MenuVSMLabel->SetText(FText::FromString(bMenuVSMActive ? TEXT("VIRTUAL SHADOWS: [ ON // CINEMATIC ]") : TEXT("VIRTUAL SHADOWS: [ OFF // FAST CSM ]")));

		MenuVSMLabel->SetColorAndOpacity(bMenuVSMActive ? MWTheme::AmberAccent : MWTheme::TextMuted);

	}



	if (MenuResolutionLabel.IsValid())

	{

		MenuResolutionLabel->SetText(FText::FromString(FString::Printf(TEXT("RENDER SCALE: [ %.0f%% ]"), MenuScreenPercentage)));

	}



	if (MenuFrameRateLimitLabel.IsValid())

	{

		const FString FpsStr = (MenuFpsLimit > 0) ? FString::Printf(TEXT("%d FPS"), MenuFpsLimit) : TEXT("UNLIMITED");

		MenuFrameRateLimitLabel->SetText(FText::FromString(FString::Printf(TEXT("FRAME RATE CAP: [ %s ]"), *FpsStr)));

	}



	// Update preset card border glowing highlights

	if (PresetCardBorder0.IsValid())

	{

		PresetCardBorder0->SetBorderBackgroundColor(MenuGraphicsPresetIndex == 0 ? MWTheme::ActiveBorder : MWTheme::GlassBorder);

	}

	if (PresetCardBorder1.IsValid())

	{

		PresetCardBorder1->SetBorderBackgroundColor(MenuGraphicsPresetIndex == 1 ? MWTheme::ActiveBorder : MWTheme::GlassBorder);

	}

	if (PresetCardBorder2.IsValid())

	{

		PresetCardBorder2->SetBorderBackgroundColor(MenuGraphicsPresetIndex == 2 ? MWTheme::ActiveBorder : MWTheme::GlassBorder);

	}

	if (PresetCardBorder3.IsValid())

	{

		PresetCardBorder3->SetBorderBackgroundColor(MenuGraphicsPresetIndex == 3 ? MWTheme::ActiveBorder : MWTheme::GlassBorder);
	}

	if (UWorld* World = GetWorld())
	{
		if (AFPSMenuGameMode* MenuGM = Cast<AFPSMenuGameMode>(World->GetAuthGameMode()))
		{
			if (AFPSMenuHangarStage* HangarStage = MenuGM->GetHangarStage())
			{
				HangarStage->ApplyScalabilityTier(bMenuLumenActive || MenuGraphicsPresetIndex >= 2);
			}
		}
	}
}



void UMainMenuWidget::ApplyMenuGraphicsPreset(int32 PresetIndex)

{

	UWorld* World = GetWorld();

	if (!World) return;



	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();



	switch (PresetIndex)

	{

	case 0: // LAPTOP / ESPORTS (MAX FPS)

		MenuGraphicsPresetIndex = 0;

		if (Settings)

		{

			Settings->SetOverallScalabilityLevel(1); // Medium / Performance

			Settings->SetResolutionScaleValueEx(70.0f);

			Settings->SetFrameRateLimit(60.0f);

			Settings->ApplySettings(false);

			Settings->SaveSettings();

		}

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.DynamicGlobalIlluminationMethod 0"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.Shadow.Virtual.Enable 0"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.Shadow.CSM.MaxCascades 2"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.ShadowQuality 1"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.DistanceFieldShadowing 0"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.DistanceFieldAO 0"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.Lumen.Reflections.Allow 0"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.Lumen.ScreenProbeGather 0"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.VolumetricFog 0"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.VolumetricCloud 0"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.SkyLight.RealTimeReflectionCapture 0"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.ScreenPercentage 70"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.AmbientOcclusionLevels 1"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.DepthOfFieldQuality 0"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.MotionBlurQuality 0"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.ContactShadows 0"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.Nanite.MaxPixelsPerEdge 4"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("t.MaxFPS 60"));

		bMenuLumenActive = false;

		bMenuVSMActive = false;

		MenuScreenPercentage = 70.0f;

		MenuFpsLimit = 60;

		break;



	case 1: // BALANCED (MEDIUM QUALITY)

		MenuGraphicsPresetIndex = 1;

		if (Settings)

		{

			Settings->SetOverallScalabilityLevel(2); // High

			Settings->SetResolutionScaleValueEx(85.0f);

			Settings->SetFrameRateLimit(120.0f);

			Settings->ApplySettings(false);

			Settings->SaveSettings();

		}

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.DynamicGlobalIlluminationMethod 0"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.Shadow.Virtual.Enable 0"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.Shadow.CSM.MaxCascades 3"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.ShadowQuality 2"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.DistanceFieldShadowing 1"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.DistanceFieldAO 0"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.Lumen.Reflections.Allow 0"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.Lumen.ScreenProbeGather 0"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.VolumetricFog 0"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.VolumetricCloud 0"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.SkyLight.RealTimeReflectionCapture 0"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.ScreenPercentage 85"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.AmbientOcclusionLevels 2"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.DepthOfFieldQuality 0"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.MotionBlurQuality 1"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.ContactShadows 0"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("t.MaxFPS 120"));

		bMenuLumenActive = false;

		bMenuVSMActive = false;

		MenuScreenPercentage = 85.0f;

		MenuFpsLimit = 120;

		break;



	case 2: // HIGH (LUMEN SOFT GI)

		MenuGraphicsPresetIndex = 2;

		if (Settings)

		{

			Settings->SetOverallScalabilityLevel(3); // Epic

			Settings->SetResolutionScaleValueEx(100.0f);

			Settings->SetFrameRateLimit(144.0f);

			Settings->ApplySettings(false);

			Settings->SaveSettings();

		}

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.DynamicGlobalIlluminationMethod 1"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.Shadow.Virtual.Enable 0"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.Shadow.CSM.MaxCascades 4"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.ShadowQuality 3"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.DistanceFieldShadowing 1"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.DistanceFieldAO 1"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.Lumen.Reflections.Allow 1"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.Lumen.ScreenProbeGather 1"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.VolumetricFog 1"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.VolumetricCloud 1"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.SkyLight.RealTimeReflectionCapture 0"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.ScreenPercentage 100"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.AmbientOcclusionLevels 2"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.DepthOfFieldQuality 1"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.MotionBlurQuality 1"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.ContactShadows 1"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("t.MaxFPS 144"));

		bMenuLumenActive = true;

		bMenuVSMActive = false;

		MenuScreenPercentage = 100.0f;

		MenuFpsLimit = 144;

		break;



	case 3: // ULTRA / BEAUTIFUL (MAX CINEMATIC - LUMEN + VIRTUAL SHADOWS)

	default:

		MenuGraphicsPresetIndex = 3;

		if (Settings)

		{

			Settings->SetOverallScalabilityLevel(3); // Epic

			Settings->SetResolutionScaleValueEx(100.0f);

			Settings->SetFrameRateLimit(0.0f);

			Settings->ApplySettings(false);

			Settings->SaveSettings();

		}

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.DynamicGlobalIlluminationMethod 1"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.Shadow.Virtual.Enable 1"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.Shadow.CSM.MaxCascades 4"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.ShadowQuality 3"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.DistanceFieldShadowing 1"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.DistanceFieldAO 1"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.Lumen.Reflections.Allow 1"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.Lumen.ScreenProbeGather 1"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.VolumetricFog 1"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.VolumetricCloud 1"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.SkyLight.RealTimeReflectionCapture 1"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.ScreenPercentage 100"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.AmbientOcclusionLevels 3"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.DepthOfFieldQuality 2"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.MotionBlurQuality 2"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.ContactShadows 1"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("r.Nanite.MaxPixelsPerEdge 1"));

		UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("t.MaxFPS 0"));

		bMenuLumenActive = true;

		bMenuVSMActive = true;

		MenuScreenPercentage = 100.0f;

		MenuFpsLimit = 0;

		break;

	}



	// Persist to Engine.ini so it reloads on boot

	if (GConfig)

	{

		GConfig->SetInt(TEXT("SystemSettings"), TEXT("r.DynamicGlobalIlluminationMethod"), bMenuLumenActive ? 1 : 0, GEngineIni);

		GConfig->SetInt(TEXT("SystemSettings"), TEXT("r.Shadow.Virtual.Enable"), bMenuVSMActive ? 1 : 0, GEngineIni);

		GConfig->SetInt(TEXT("SystemSettings"), TEXT("r.Lumen.Reflections.Allow"), bMenuLumenActive ? 1 : 0, GEngineIni);

		GConfig->SetInt(TEXT("SystemSettings"), TEXT("r.VolumetricFog"), (MenuGraphicsPresetIndex >= 2) ? 1 : 0, GEngineIni);

		GConfig->SetFloat(TEXT("SystemSettings"), TEXT("r.ScreenPercentage"), MenuScreenPercentage, GEngineIni);

		GConfig->Flush(false, GEngineIni);

	}



	UpdateMenuGraphicsUI();

}



void UMainMenuWidget::ToggleMenuLumen()

{

	UWorld* World = GetWorld();

	if (!World) return;



	bMenuLumenActive = !bMenuLumenActive;

	UKismetSystemLibrary::ExecuteConsoleCommand(World, bMenuLumenActive ? TEXT("r.DynamicGlobalIlluminationMethod 1") : TEXT("r.DynamicGlobalIlluminationMethod 0"));

	UKismetSystemLibrary::ExecuteConsoleCommand(World, bMenuLumenActive ? TEXT("r.Lumen.Reflections.Allow 1") : TEXT("r.Lumen.Reflections.Allow 0"));



	if (GConfig)

	{

		GConfig->SetInt(TEXT("SystemSettings"), TEXT("r.DynamicGlobalIlluminationMethod"), bMenuLumenActive ? 1 : 0, GEngineIni);

		GConfig->SetInt(TEXT("SystemSettings"), TEXT("r.Lumen.Reflections.Allow"), bMenuLumenActive ? 1 : 0, GEngineIni);

		GConfig->Flush(false, GEngineIni);

	}



	if (bMenuLumenActive && bMenuVSMActive) MenuGraphicsPresetIndex = 3;

	else if (bMenuLumenActive) MenuGraphicsPresetIndex = 2;

	else MenuGraphicsPresetIndex = 0;



	UpdateMenuGraphicsUI();

}



void UMainMenuWidget::ToggleMenuVSM()

{

	UWorld* World = GetWorld();

	if (!World) return;



	bMenuVSMActive = !bMenuVSMActive;

	UKismetSystemLibrary::ExecuteConsoleCommand(World, bMenuVSMActive ? TEXT("r.Shadow.Virtual.Enable 1") : TEXT("r.Shadow.Virtual.Enable 0"));



	if (GConfig)

	{

		GConfig->SetInt(TEXT("SystemSettings"), TEXT("r.Shadow.Virtual.Enable"), bMenuVSMActive ? 1 : 0, GEngineIni);

		GConfig->Flush(false, GEngineIni);

	}



	if (bMenuLumenActive && bMenuVSMActive) MenuGraphicsPresetIndex = 3;

	else if (bMenuLumenActive) MenuGraphicsPresetIndex = 2;

	else MenuGraphicsPresetIndex = 0;



	UpdateMenuGraphicsUI();

}



void UMainMenuWidget::CycleMenuResolutionScale()

{

	UWorld* World = GetWorld();

	if (!World) return;



	if (MenuScreenPercentage <= 72.0f) MenuScreenPercentage = 85.0f;

	else if (MenuScreenPercentage <= 88.0f) MenuScreenPercentage = 100.0f;

	else MenuScreenPercentage = 70.0f;



	UKismetSystemLibrary::ExecuteConsoleCommand(World, FString::Printf(TEXT("r.ScreenPercentage %.0f"), MenuScreenPercentage));



	if (GConfig)

	{

		GConfig->SetFloat(TEXT("SystemSettings"), TEXT("r.ScreenPercentage"), MenuScreenPercentage, GEngineIni);

		GConfig->Flush(false, GEngineIni);

	}



	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();

	if (Settings)

	{

		Settings->SetResolutionScaleValueEx(MenuScreenPercentage);

		Settings->ApplySettings(false);

		Settings->SaveSettings();

	}



	UpdateMenuGraphicsUI();

}



void UMainMenuWidget::CycleMenuFrameRateLimit()

{

	if (MenuFpsLimit == 60) MenuFpsLimit = 120;

	else if (MenuFpsLimit == 120) MenuFpsLimit = 144;

	else if (MenuFpsLimit == 144) MenuFpsLimit = 240;

	else if (MenuFpsLimit == 240) MenuFpsLimit = 0;

	else MenuFpsLimit = 60;



	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();

	if (Settings)

	{

		Settings->SetFrameRateLimit(static_cast<float>(MenuFpsLimit));

		Settings->ApplySettings(false);

		Settings->SaveSettings();

	}



	UpdateMenuGraphicsUI();

}



FReply UMainMenuWidget::HandleMenuGraphicsPresetClicked(int32 PresetIndex)

{

	ApplyMenuGraphicsPreset(PresetIndex);

	return FReply::Handled();

}



FReply UMainMenuWidget::HandleMenuToggleLumenClicked()

{

	ToggleMenuLumen();

	return FReply::Handled();

}



FReply UMainMenuWidget::HandleMenuToggleVSMClicked()

{

	ToggleMenuVSM();

	return FReply::Handled();

}



FReply UMainMenuWidget::HandleMenuCycleResolutionClicked()

{

	CycleMenuResolutionScale();

	return FReply::Handled();

}



FReply UMainMenuWidget::HandleMenuCycleFpsLimitClicked()

{

	CycleMenuFrameRateLimit();

	return FReply::Handled();

}



TSharedRef<SWidget> UMainMenuWidget::BuildFooter()
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SAssignNew(NetworkStatusText, STextBlock)
			.Text(FText::FromString(TEXT("🌐  SERVER: EU-WEST-1      |      PING: 42 MS      |      128-HZ AUTHORITATIVE")))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
			.ColorAndOpacity(MWTheme::TextSecondary)
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("REAL OPERATORS. REAL TACTICS. NO EXCUSES.")))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
			.ColorAndOpacity(MWTheme::TextSecondary)
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
			TargetIndex = 0; // Clean Website-Matched Arrival Home!
			break;
		case EFPSMenuSection::Customization:
			TargetIndex = 5; // Dedicated Character Customization Submenu!
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



void UMainMenuWidget::UpdateTheaterCardVisuals()

{

	if (TheaterCardBorder0.IsValid())

	{

		TheaterCardBorder0->SetBorderBackgroundColor(SelectedTheaterIndex == 0 ? MWTheme::ActiveBorder : MWTheme::GlassBorder);

	}

	if (TheaterCardBorder1.IsValid())

	{

		TheaterCardBorder1->SetBorderBackgroundColor(SelectedTheaterIndex == 1 ? MWTheme::ActiveBorder : MWTheme::GlassBorder);

	}

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
	const bool bIsServerBrowser = (CurrentSection == EFPSMenuSection::ServerBrowser);
	const bool bIsSettings = (CurrentSection == EFPSMenuSection::Settings);

	if (MWTheme::TabIndicatorServerBrowser.IsValid())
	{
		MWTheme::TabIndicatorServerBrowser->SetBorderBackgroundColor(bIsServerBrowser ? MWTheme::CyanAccent : FLinearColor::Transparent);
	}



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



	if (UGameInstance* GI = GetGameInstance())
	{
		if (UFPSEconomySubsystem* Economy = GI->GetSubsystem<UFPSEconomySubsystem>())
		{
			Economy->SetSelectedTheaterIndex(SelectedTheaterIndex);
		}
	}

	UWorld* World = GetWorld();
	if (World)
	{
		const FString TheaterOpt = (SelectedTheaterIndex == 1) ? TEXT("Coastal") : TEXT("Rust");
		const FString OpenOptions = FString::Printf(TEXT("listen?game=/Script/NobleFermi.FPSMultiplayerGameMode?Theater=%s"), *TheaterOpt);
		UGameplayStatics::OpenLevel(World, FName(TEXT("/Engine/Maps/Entry")), true, OpenOptions);
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



void UMainMenuWidget::SelectTheater(int32 TheaterIndex)

{

	SelectedTheaterIndex = FMath::Clamp(TheaterIndex, 0, 1);

	if (UGameInstance* GI = GetGameInstance())

	{

		if (UFPSEconomySubsystem* Economy = GI->GetSubsystem<UFPSEconomySubsystem>())

		{

			Economy->SetSelectedTheaterIndex(SelectedTheaterIndex);

		}

	}

	UpdateTheaterCardVisuals();

	if (TransitionSound)

	{

		UGameplayStatics::PlaySound2D(this, TransitionSound);

	}

}



FReply UMainMenuWidget::HandleTheaterCardClicked(int32 TheaterIndex)

{

	SelectTheater(TheaterIndex);

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

				StanceStatusText->SetText(FText::FromString(TEXT("STANCE: LOW-READY CARBINE")));

				break;

			case 1:

				StanceStatusText->SetText(FText::FromString(TEXT("STANCE: HIGH-READY ADS")));

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



// =============================================================================

// NOBLE FERMI // OPERATOR AUTHENTICATION TERMINAL & MODAL IMPLEMENTATION

// =============================================================================



TSharedRef<SWidget> UMainMenuWidget::BuildAuthModal()

{

	return SAssignNew(AuthOverlayModal, SBorder)

		.BorderBackgroundColor(FLinearColor(0.01f, 0.02f, 0.04f, 0.88f))

		.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))

		.Padding(0.0f)

		.Visibility(EVisibility::Collapsed)

		[

			SNew(SOverlay)

			// Center tactical modal dialog

			+ SOverlay::Slot()

			.HAlign(HAlign_Center)

			.VAlign(VAlign_Center)

			[

				SNew(SBox)

				.WidthOverride(460.0f)

				[

					SNew(SBorder)

					.BorderBackgroundColor(FLinearColor(0.03f, 0.07f, 0.12f, 0.98f))

					.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))

					.Padding(24.0f)

					[

						SNew(SVerticalBox)

						// Top Glowing Accent Stripe

						+ SVerticalBox::Slot()

						.AutoHeight()

						.Padding(0.0f, 0.0f, 0.0f, 14.0f)

						[

							SNew(SBox)

							.HeightOverride(3.0f)

							[

								SNew(SBorder)

								.BorderBackgroundColor(MWTheme::CyanAccent)

								.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))

							]

						]

						// Title Bar

						+ SVerticalBox::Slot()

						.AutoHeight()

						.Padding(0.0f, 0.0f, 0.0f, 4.0f)

						[

							SNew(SHorizontalBox)

							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)

							[

								SNew(STextBlock)

								.Text(FText::FromString(TEXT("// ")))

								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))

								.ColorAndOpacity(MWTheme::CyanAccent)

							]

							+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)

							[

								SNew(STextBlock)

								.Text(FText::FromString(TEXT("OPERATOR AUTHENTICATION TERMINAL")))

								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))

								.ColorAndOpacity(MWTheme::AmberAccent)

							]

						]

						// Subtitle

						+ SVerticalBox::Slot()

						.AutoHeight()

						.Padding(0.0f, 0.0f, 0.0f, 16.0f)

						[

							SNew(STextBlock)

							.Text(FText::FromString(TEXT("CLOUDFLARE D1 AUTHORITATIVE EDGE LINK // ZERO-FRICTION SYNC")))

							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))

							.ColorAndOpacity(MWTheme::TextMuted)

						]

						// Mode Tabs: [ SIGN IN ] and [ NEW ENLISTMENT ]

						+ SVerticalBox::Slot()

						.AutoHeight()

						.Padding(0.0f, 0.0f, 0.0f, 16.0f)

						[

							SNew(SBorder)

							.BorderBackgroundColor(MWTheme::DarkGlassCard)

							.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))

							.Padding(2.0f)

							[

								SNew(SHorizontalBox)

								// Tab 0: SIGN IN

								+ SHorizontalBox::Slot()

								.FillWidth(1.0f)

								[

									SNew(SButton)

									.ButtonStyle(FAppStyle::Get(), "SimpleButton")

									.ContentPadding(FMargin(0.0f, 8.0f))

									.OnClicked_Lambda([this]() { return HandleAuthTabSwitch(0); })

									[

										SNew(SVerticalBox)

										+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)

										[

											SNew(STextBlock)

											.Text(FText::FromString(TEXT("[ SIGN IN ]")))

											.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))

											.ColorAndOpacity(MWTheme::TextPrimary)

										]

										+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 0.0f)

										[

											SAssignNew(AuthTabSignInIndicator, SBorder)

											.BorderBackgroundColor(MWTheme::CyanAccent)

											.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))

											[

												SNew(SBox).HeightOverride(2.0f)

											]

										]

									]

								]

								// Tab 1: NEW ENLISTMENT

								+ SHorizontalBox::Slot()

								.FillWidth(1.0f)

								[

									SNew(SButton)

									.ButtonStyle(FAppStyle::Get(), "SimpleButton")

									.ContentPadding(FMargin(0.0f, 8.0f))

									.OnClicked_Lambda([this]() { return HandleAuthTabSwitch(1); })

									[

										SNew(SVerticalBox)

										+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)

										[

											SNew(STextBlock)

											.Text(FText::FromString(TEXT("[ NEW ENLISTMENT ]")))

											.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))

											.ColorAndOpacity(MWTheme::TextPrimary)

										]

										+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 0.0f)

										[

											SAssignNew(AuthTabRegisterIndicator, SBorder)

											.BorderBackgroundColor(FLinearColor::Transparent)

											.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))

											[

												SNew(SBox).HeightOverride(2.0f)

											]

										]

									]

								]

							]

						]

						// Alert Box

						+ SVerticalBox::Slot()

						.AutoHeight()

						.Padding(0.0f, 0.0f, 0.0f, 12.0f)

						[

							SAssignNew(AuthAlertBorder, SBorder)

							.BorderBackgroundColor(FLinearColor(0.25f, 0.04f, 0.08f, 0.85f))

							.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))

							.Padding(10.0f, 6.0f)

							.Visibility(EVisibility::Collapsed)

							[

								SAssignNew(AuthAlertText, STextBlock)

								.Text(FText::GetEmpty())

								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))

								.ColorAndOpacity(MWTheme::CrimsonAlert)

								.AutoWrapText(true)

							]

						]

						// Switcher: [ SIGN IN ] vs [ REGISTER ]

						+ SVerticalBox::Slot()

						.AutoHeight()

						[

							SAssignNew(AuthModeSwitcher, SWidgetSwitcher)

							.WidgetIndex(0)

							// Slot 0: SIGN IN

							+ SWidgetSwitcher::Slot()

							[

								SNew(SVerticalBox)

								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 4.0f)

								[

									SNew(STextBlock)

									.Text(FText::FromString(TEXT("CALLSIGN OR EMAIL")))

									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))

									.ColorAndOpacity(MWTheme::CyanAccent)

								]

								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 10.0f)

								[

									SAssignNew(LoginIdentifierInput, SEditableTextBox)

									.HintText(FText::FromString(TEXT("Enter Callsign or Email...")))

									.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))

								]

								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 4.0f)

								[

									SNew(STextBlock)

									.Text(FText::FromString(TEXT("TACTICAL PASSKEY")))

									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))

									.ColorAndOpacity(MWTheme::CyanAccent)

								]

								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 16.0f)

								[

									SAssignNew(LoginPasswordInput, SEditableTextBox)

									.IsPassword(true)

									.HintText(FText::FromString(TEXT("••••••••")))

									.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))

									.OnTextCommitted_Lambda([this](const FText& Text, ETextCommit::Type CommitMethod)

									{

										if (CommitMethod == ETextCommit::OnEnter)

										{

											HandleSubmitLogin();

										}

									})

								]

								+ SVerticalBox::Slot().AutoHeight()

								[

									SNew(SButton)

									.ButtonStyle(FAppStyle::Get(), "PrimaryButton")

									.ButtonColorAndOpacity(MWTheme::AmberAccent)

									.ContentPadding(FMargin(0.0f, 10.0f))

									.HAlign(HAlign_Center)

									.OnClicked_Lambda([this]() { return HandleSubmitLogin(); })

									[

										SNew(STextBlock)

										.Text(FText::FromString(TEXT("SIGN IN & DEPLOY")))

										.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))

										.ColorAndOpacity(FLinearColor::Black)

									]

								]

							]

							// Slot 1: NEW ENLISTMENT

							+ SWidgetSwitcher::Slot()

							[

								SNew(SVerticalBox)

								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 4.0f)

								[

									SNew(STextBlock)

									.Text(FText::FromString(TEXT("OPERATOR CALLSIGN (3-20 CHARACTERS)")))

									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))

									.ColorAndOpacity(MWTheme::AmberAccent)

								]

								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)

								[

									SAssignNew(RegisterCallsignInput, SEditableTextBox)

									.HintText(FText::FromString(TEXT("e.g. GHOST_141")))

									.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))

								]

								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 4.0f)

								[

									SNew(STextBlock)

									.Text(FText::FromString(TEXT("TACTICAL EMAIL ADDRESS")))

									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))

									.ColorAndOpacity(MWTheme::AmberAccent)

								]

								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)

								[

									SAssignNew(RegisterEmailInput, SEditableTextBox)

									.HintText(FText::FromString(TEXT("operator@taskforce.io")))

									.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))

								]

								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 4.0f)

								[

									SNew(STextBlock)

									.Text(FText::FromString(TEXT("TACTICAL PASSKEY (MIN 6 CHARACTERS)")))

									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))

									.ColorAndOpacity(MWTheme::AmberAccent)

								]

								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 16.0f)

								[

									SAssignNew(RegisterPasswordInput, SEditableTextBox)

									.IsPassword(true)

									.HintText(FText::FromString(TEXT("••••••••")))

									.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))

									.OnTextCommitted_Lambda([this](const FText& Text, ETextCommit::Type CommitMethod)

									{

										if (CommitMethod == ETextCommit::OnEnter)

										{

											HandleSubmitRegister();

										}

									})

								]

								+ SVerticalBox::Slot().AutoHeight()

								[

									SNew(SButton)

									.ButtonStyle(FAppStyle::Get(), "PrimaryButton")

									.ButtonColorAndOpacity(MWTheme::CyanAccent)

									.ContentPadding(FMargin(0.0f, 10.0f))

									.HAlign(HAlign_Center)

									.OnClicked_Lambda([this]() { return HandleSubmitRegister(); })
									[
										SNew(STextBlock)
										.Text(FText::FromString(TEXT("ENLIST & CONTINUE TO WEBSITE VERIFICATION ►")))
										.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
										.ColorAndOpacity(FLinearColor::Black)
									]
								]
							]
							// Slot 2: 6-DIGIT WEBSITE VERIFICATION CODE SCREEN
							+ SWidgetSwitcher::Slot()
							[
								SNew(SVerticalBox)
								// Step 1 Box: Open Website Link
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
											+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 4.0f)
											[
												SAssignNew(MWTheme::VerificationCallsignLabel, STextBlock)
												.Text(FText::FromString(TEXT("🔑 STEP 1 // GET 6-DIGIT CODE FROM WEBSITE")))
												.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
												.ColorAndOpacity(MWTheme::AmberAccent)
											]
											+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 10.0f)
											[
												SNew(STextBlock)
												.Text(FText::FromString(TEXT("Open the Noble Fermi website in your browser. Your 6-digit Client Verification Code is displayed inside your Operator Dossier.")))
												.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
												.ColorAndOpacity(MWTheme::TextSecondary)
												.AutoWrapText(true)
											]
											+ SVerticalBox::Slot().AutoHeight()
											[
												SNew(SButton)
												.ButtonStyle(FAppStyle::Get(), "PrimaryButton")
												.ButtonColorAndOpacity(MWTheme::CyanAccent)
												.ContentPadding(FMargin(0.0f, 9.0f))
												.HAlign(HAlign_Center)
												.OnClicked_Lambda([this]()
												{
													FString TargetUrl = TEXT("https://noble-fermi.pages.dev/");
													if (!MWTheme::PendingVerificationCallsign.IsEmpty())
													{
														TargetUrl = FString::Printf(
															TEXT("https://noble-fermi.pages.dev/?verify=%s&token=%s"),
															*MWTheme::PendingVerificationCallsign,
															*MWTheme::PendingVerificationToken
														);
													}
													FPlatformProcess::LaunchURL(*TargetUrl, nullptr, nullptr);
													return FReply::Handled();
												})
												[
													SNew(STextBlock)
													.Text(FText::FromString(TEXT("🌐 OPEN WEBSITE & VIEW 6-DIGIT CODE ►")))
													.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
													.ColorAndOpacity(FLinearColor::Black)
												]
											]
										]
									]
								]
								// Step 2: Input 6-Digit Verification Code
								+ SVerticalBox::Slot()
								.AutoHeight()
								.Padding(0.0f, 0.0f, 0.0f, 4.0f)
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("STEP 2 // ENTER 6-DIGIT WEBSITE VERIFICATION CODE")))
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
									.ColorAndOpacity(MWTheme::AmberAccent)
								]
								+ SVerticalBox::Slot()
								.AutoHeight()
								.Padding(0.0f, 0.0f, 0.0f, 12.0f)
								[
									SAssignNew(MWTheme::VerificationCodeInput, SEditableTextBox)
									.HintText(FText::FromString(TEXT("Enter 6-Digit Code from Website (e.g. 817548)")))
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
								]
								// Verify & Deploy Button
								+ SVerticalBox::Slot()
								.AutoHeight()
								.Padding(0.0f, 0.0f, 0.0f, 8.0f)
								[
									SNew(SButton)
									.ButtonStyle(FAppStyle::Get(), "PrimaryButton")
									.ButtonColorAndOpacity(MWTheme::AmberAccent)
									.ContentPadding(FMargin(0.0f, 10.0f))
									.HAlign(HAlign_Center)
									.OnClicked_Lambda([this]()
									{
										if (!MWTheme::VerificationCodeInput.IsValid()) return FReply::Handled();
										const FString Code = MWTheme::VerificationCodeInput->GetText().ToString().TrimStartAndEnd();
										if (Code.IsEmpty())
										{
											HandleAuthError(TEXT("Please enter the 6-digit verification code from the website."));
											return FReply::Handled();
										}
										if (AuthAlertBorder.IsValid() && AuthAlertText.IsValid())
										{
											AuthAlertText->SetText(FText::FromString(TEXT("Verifying 6-digit code with Cloudflare D1...")));
											AuthAlertText->SetColorAndOpacity(MWTheme::CyanAccent);
											AuthAlertBorder->SetBorderBackgroundColor(FLinearColor(0.02f, 0.12f, 0.20f, 0.85f));
											AuthAlertBorder->SetVisibility(EVisibility::Visible);
										}
										if (UGameInstance* GI = GetGameInstance())
										{
											if (UFPSAuthSubsystem* Auth = GI->GetSubsystem<UFPSAuthSubsystem>())
											{
												Auth->Login(FString::Printf(TEXT("VERIFY_CODE::%s"), *MWTheme::PendingVerificationCallsign), Code, true);
											}
										}
										return FReply::Handled();
									})
									[
										SNew(STextBlock)
										.Text(FText::FromString(TEXT("✓ VERIFY CODE & UNLOCK ACCOUNT")))
										.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
										.ColorAndOpacity(FLinearColor::Black)
									]
								]
							]
						]

						// Separator

						+ SVerticalBox::Slot()

						.AutoHeight()

						.Padding(0.0f, 16.0f, 0.0f, 12.0f)

						[

							SNew(SSeparator).ColorAndOpacity(MWTheme::GlassBorder)

						]

						// 1-Click Guest / Offline Button

						+ SVerticalBox::Slot()

						.AutoHeight()

						[

							SNew(SButton)

							.ButtonStyle(FAppStyle::Get(), "SimpleButton")

							.ContentPadding(FMargin(0.0f, 6.0f))

							.HAlign(HAlign_Center)

							.OnClicked_Lambda([this]() { return HandleQuickGuestLogin(); })

							[

								SNew(STextBlock)

								.Text(FText::FromString(TEXT("⚡ QUICK GUEST / OFFLINE OPERATOR (NO SIGN-IN REQUIRED)")))

								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))

								.ColorAndOpacity(MWTheme::TextMuted)

							]

						]

					]

				]

			]

		];

}



void UMainMenuWidget::UpdateAuthUI()

{

	if (UGameInstance* GI = GetGameInstance())

	{

		if (UFPSAuthSubsystem* Auth = GI->GetSubsystem<UFPSAuthSubsystem>())

		{

			const bool bAuth = Auth->IsAuthenticated();

			const FString Callsign = Auth->GetActiveCallsign();

			const bool bOffline = Auth->IsOfflineMode();



			if (AuthOverlayModal.IsValid())

			{

				AuthOverlayModal->SetVisibility(EVisibility::Collapsed);

			}



			if (HeaderOperatorCallsignText.IsValid())

			{

				HeaderOperatorCallsignText->SetText(FText::FromString(

					bAuth ? FString::Printf(TEXT("OPERATOR: %s"), *Callsign) : TEXT("OPERATOR: NOT ENLISTED")

				));

				HeaderOperatorCallsignText->SetColorAndOpacity(bAuth ? MWTheme::AmberAccent : MWTheme::TextMuted);

			}



			if (HeaderOperatorStatusBadge.IsValid())

			{

				if (!bAuth)

				{

					HeaderOperatorStatusBadge->SetText(FText::FromString(TEXT("[OFFLINE // CLICK TO SIGN IN]")));

					HeaderOperatorStatusBadge->SetColorAndOpacity(MWTheme::CrimsonAlert);

				}

				else if (bOffline)

				{

					HeaderOperatorStatusBadge->SetText(FText::FromString(TEXT("[LOCAL // OFFLINE TACTICAL LINK ACTIVE]")));

					HeaderOperatorStatusBadge->SetColorAndOpacity(MWTheme::AmberAccent);

				}

				else

				{

					HeaderOperatorStatusBadge->SetText(FText::FromString(TEXT("[ONLINE // CLOUD SYNCED]")));

					HeaderOperatorStatusBadge->SetColorAndOpacity(MWTheme::EmeraldReady);

				}

			}



			if (HeaderAuthActionText.IsValid())

			{

				HeaderAuthActionText->SetText(FText::FromString(

					bAuth ? TEXT("[ SWITCH / LOGOUT ]") : TEXT("[ ENLIST / SIGN IN ]")

				));

			}

		}

	}

}



FReply UMainMenuWidget::HandleAuthTabSwitch(int32 TabIndex)

{

	if (AuthModeSwitcher.IsValid())

	{

		AuthModeSwitcher->SetActiveWidgetIndex(TabIndex);

	}

	if (AuthTabSignInIndicator.IsValid() && AuthTabRegisterIndicator.IsValid())

	{

		AuthTabSignInIndicator->SetBorderBackgroundColor(TabIndex == 0 ? MWTheme::CyanAccent : FLinearColor::Transparent);

		AuthTabRegisterIndicator->SetBorderBackgroundColor(TabIndex == 1 ? MWTheme::AmberAccent : FLinearColor::Transparent);

	}

	if (AuthAlertBorder.IsValid())

	{

		AuthAlertBorder->SetVisibility(EVisibility::Collapsed);

	}

	return FReply::Handled();

}



FReply UMainMenuWidget::HandleSubmitLogin()

{

	if (!LoginIdentifierInput.IsValid() || !LoginPasswordInput.IsValid()) return FReply::Handled();



	const FString Identifier = LoginIdentifierInput->GetText().ToString().TrimStartAndEnd();

	const FString Password = LoginPasswordInput->GetText().ToString();



	if (Identifier.IsEmpty() || Password.IsEmpty())

	{

		HandleAuthError(TEXT("Please enter your Callsign/Email and passkey."));

		return FReply::Handled();

	}



	if (AuthAlertBorder.IsValid() && AuthAlertText.IsValid())

	{

		AuthAlertText->SetText(FText::FromString(TEXT("Authenticating with Cloudflare Edge D1...")));

		AuthAlertText->SetColorAndOpacity(MWTheme::CyanAccent);

		AuthAlertBorder->SetBorderBackgroundColor(FLinearColor(0.02f, 0.12f, 0.20f, 0.85f));

		AuthAlertBorder->SetVisibility(EVisibility::Visible);

	}



	if (UGameInstance* GI = GetGameInstance())

	{

		if (UFPSAuthSubsystem* Auth = GI->GetSubsystem<UFPSAuthSubsystem>())

		{

			Auth->Login(Identifier, Password, true);

		}

	}

	return FReply::Handled();

}



FReply UMainMenuWidget::HandleSubmitRegister()

{

	if (!RegisterCallsignInput.IsValid() || !RegisterEmailInput.IsValid() || !RegisterPasswordInput.IsValid()) return FReply::Handled();



	const FString Callsign = RegisterCallsignInput->GetText().ToString().TrimStartAndEnd();

	const FString Email = RegisterEmailInput->GetText().ToString().TrimStartAndEnd();

	const FString Password = RegisterPasswordInput->GetText().ToString();



	if (Callsign.Len() < 3 || Callsign.Len() > 20)

	{

		HandleAuthError(TEXT("Callsign must be 3-20 alphanumeric characters."));

		return FReply::Handled();

	}



	if (Password.Len() < 6)

	{

		HandleAuthError(TEXT("Passkey must be at least 6 characters."));

		return FReply::Handled();

	}



	if (AuthAlertBorder.IsValid() && AuthAlertText.IsValid())

	{

		AuthAlertText->SetText(FText::FromString(TEXT("Transmitting credentials to Cloudflare D1...")));

		AuthAlertText->SetColorAndOpacity(MWTheme::AmberAccent);

		AuthAlertBorder->SetBorderBackgroundColor(FLinearColor(0.15f, 0.10f, 0.02f, 0.85f));

		AuthAlertBorder->SetVisibility(EVisibility::Visible);

	}



	if (UGameInstance* GI = GetGameInstance())

	{

		if (UFPSAuthSubsystem* Auth = GI->GetSubsystem<UFPSAuthSubsystem>())

		{

			Auth->Register(Callsign, Email, Password, true);

		}

	}

	return FReply::Handled();

}



FReply UMainMenuWidget::HandleQuickGuestLogin()

{

	if (UGameInstance* GI = GetGameInstance())

	{

		if (UFPSAuthSubsystem* Auth = GI->GetSubsystem<UFPSAuthSubsystem>())

		{

			Auth->LoginAsGuest();

		}

	}

	return FReply::Handled();

}



FReply UMainMenuWidget::HandleLogoutOrSwitchClicked()

{

	if (UGameInstance* GI = GetGameInstance())

	{

		if (UFPSAuthSubsystem* Auth = GI->GetSubsystem<UFPSAuthSubsystem>())

		{

			if (Auth->IsAuthenticated())

			{

				Auth->Logout();

			}

			else if (AuthOverlayModal.IsValid())

			{

				AuthOverlayModal->SetVisibility(EVisibility::Visible);

			}

		}

	}

	return FReply::Handled();

}



void UMainMenuWidget::HandleAuthStateChanged(bool bIsAuthenticated, const FString& Callsign, bool bIsOfflineMode)

{

	UpdateAuthUI();

	UpdateCurrencyHeader();



	if (APlayerController* PC = GetOwningPlayer())

	{

		if (PC->PlayerState && !Callsign.IsEmpty())

		{

			PC->PlayerState->SetPlayerName(Callsign);

		}

	}

}



void UMainMenuWidget::HandleAuthError(const FString& ErrorMessage)
{
	if (ErrorMessage.StartsWith(TEXT("VERIFY_REQUIRED::")))
	{
		const FString Payload = ErrorMessage.Mid(17);
		FString CallsignPart;
		FString TokenPart;
		if (!Payload.Split(TEXT("::"), &CallsignPart, &TokenPart))
		{
			CallsignPart = Payload;
		}

		MWTheme::PendingVerificationCallsign = CallsignPart;
		MWTheme::PendingVerificationToken = TokenPart;

		if (AuthOverlayModal.IsValid())
		{
			AuthOverlayModal->SetVisibility(EVisibility::Visible);
		}
		if (AuthModeSwitcher.IsValid())
		{
			AuthModeSwitcher->SetActiveWidgetIndex(2); // Switch to Website Verification Screen!
		}
		if (MWTheme::VerificationCallsignLabel.IsValid())
		{
			MWTheme::VerificationCallsignLabel->SetText(FText::FromString(
				FString::Printf(TEXT("🔑 STEP 1 // VERIFY OPERATOR '%s' ON WEBSITE"), *CallsignPart.ToUpper())
			));
		}
		if (AuthAlertBorder.IsValid() && AuthAlertText.IsValid())
		{
			AuthAlertText->SetText(FText::FromString(
				FString::Printf(TEXT("Account '%s' is unverified. Click 'OPEN WEBSITE' below to get your 6-digit verification code!"), *CallsignPart)
			));
			AuthAlertText->SetColorAndOpacity(MWTheme::AmberAccent);
			AuthAlertBorder->SetBorderBackgroundColor(FLinearColor(0.18f, 0.12f, 0.02f, 0.90f));
			AuthAlertBorder->SetVisibility(EVisibility::Visible);
		}
		return;
	}

	if (AuthAlertBorder.IsValid() && AuthAlertText.IsValid())
	{
		AuthAlertText->SetText(FText::FromString(ErrorMessage));
		AuthAlertText->SetColorAndOpacity(MWTheme::CrimsonAlert);
		AuthAlertBorder->SetBorderBackgroundColor(FLinearColor(0.25f, 0.04f, 0.08f, 0.85f));
		AuthAlertBorder->SetVisibility(EVisibility::Visible);
	}
}

