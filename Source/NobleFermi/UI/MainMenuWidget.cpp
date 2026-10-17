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
}

void UMainMenuWidget::SetPreviewActor(AFPSCharacterPreviewActor* InPreviewActor)
{
	PreviewActor = InPreviewActor;
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
		// Center: Modern Warfare Tabs (PLAY, OPERATOR, GUNSMITH, SETTINGS)
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth() [ MakeTabBtn(TEXT("PLAY"), EFPSMenuSection::Play, TabIndicatorPlay) ]
			+ SHorizontalBox::Slot().AutoWidth() [ MakeTabBtn(TEXT("OPERATOR"), EFPSMenuSection::Customization, TabIndicatorOperator) ]
			+ SHorizontalBox::Slot().AutoWidth() [ MakeTabBtn(TEXT("GUNSMITH"), EFPSMenuSection::Loadout, TabIndicatorGunsmith) ]
			+ SHorizontalBox::Slot().AutoWidth() [ MakeTabBtn(TEXT("SETTINGS"), EFPSMenuSection::Settings, TabIndicatorSettings) ]
		]
		// Right: Live Telemetry Badge (128 HZ TICK // IOSTORE VERIFIED // REGION: EU-WEST)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Right)
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
	auto MakeCamoSwatchButton = [this](uint8 CamoIdx, const FString& Name, const FString& Subtitle, const FLinearColor& SwatchColor) -> TSharedRef<SWidget>
	{
		return SNew(SBox)
			.HeightOverride(40.0f)
			.Padding(0.0f, 0.0f, 0.0f, 5.0f)
			[
				SNew(SButton)
				.ButtonColorAndOpacity(FLinearColor::Transparent)
				.ContentPadding(0.0f)
				.OnClicked_Lambda([this, CamoIdx]() { return HandleCamoSelected(CamoIdx); })
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
							// Action Chevron
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("APPLY >")))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
								.ColorAndOpacity(MWTheme::CyanAccent)
							]
						]
					]
				]
			];
	};

	auto MakeQuickItemBtn = [](TFunction<FReply()> Callback, const FString& Name) -> TSharedRef<SWidget>
	{
		return SNew(SBox)
			.HeightOverride(32.0f)
			.Padding(0.0f, 0.0f, 0.0f, 4.0f)
			[
				SNew(SButton)
				.ButtonColorAndOpacity(FLinearColor::Transparent)
				.ContentPadding(0.0f)
				.OnClicked_Lambda(Callback)
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
							SNew(STextBlock)
							.Text(FText::FromString(Name))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
							.ColorAndOpacity(MWTheme::TextSecondary)
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
				MakeCamoSwatchButton(0, TEXT("DIGITAL"), TEXT("Urban Digital Metro // Gray & Charcoal"), FLinearColor(0.28f, 0.32f, 0.38f))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				MakeCamoSwatchButton(1, TEXT("WOODLAND"), TEXT("Woodland Tactical // Olive Drab & Khaki"), FLinearColor(0.18f, 0.26f, 0.14f))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				MakeCamoSwatchButton(2, TEXT("DESERT"), TEXT("Desert Spec Ops // Coyote Tan & FDE"), FLinearColor(0.50f, 0.44f, 0.32f))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				MakeCamoSwatchButton(3, TEXT("BLACKOUT"), TEXT("Stealth Blackout // Midnight Matte"), FLinearColor(0.06f, 0.06f, 0.08f))
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
			+ SVerticalBox::Slot().AutoHeight() [ MakeQuickItemBtn([this]() { return HandleHeadgearSelected(0); }, TEXT("FAST BALLISTIC HELMET")) ]
			+ SVerticalBox::Slot().AutoHeight() [ MakeQuickItemBtn([this]() { return HandleHeadgearSelected(1); }, TEXT("GPNVG-18 QUAD-NODS (PHOSPHOR)")) ]
			+ SVerticalBox::Slot().AutoHeight() [ MakeQuickItemBtn([this]() { return HandleHeadgearSelected(2); }, TEXT("SPEC OPS BALACLAVA")) ]
			+ SVerticalBox::Slot().AutoHeight() [ MakeQuickItemBtn([this]() { return HandleVestSelected(0); }, TEXT("LEVEL IV HEAVY CARRIER")) ]
			+ SVerticalBox::Slot().AutoHeight() [ MakeQuickItemBtn([this]() { return HandleVestSelected(1); }, TEXT("LIGHTWEIGHT JPC VEST")) ]
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
