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
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Misc/CommandLine.h"

UMainMenuWidget::UMainMenuWidget()
{
	bIsBackHandler = true;
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
		// Background base dark tint - subtle alpha so 3D viewport behind is visible!
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor(0.012f, 0.016f, 0.022f, 0.20f))
			.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
			.Padding(0.0f)
		]
		// Top gold accent line
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Top)
		[
			SNew(SBox)
			.HeightOverride(3.0f)
			[
				SNew(SBorder)
				.BorderBackgroundColor(FLinearColor(0.95f, 0.65f, 0.15f, 1.0f))
				.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
			]
		]
		// Main Content Container
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		.Padding(32.0f, 24.0f, 32.0f, 20.0f)
		[
			SNew(SVerticalBox)
			// Header
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 20.0f)
			[
				SNew(SBorder)
				.BorderBackgroundColor(FLinearColor(0.02f, 0.03f, 0.05f, 0.88f))
				.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
				.Padding(16.0f, 10.0f)
				[
					BuildHeader()
				]
			]
			// Horizontal split: Left Navigation / Right Content
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SNew(SHorizontalBox)
				// Left Navigation Column
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.0f, 0.0f, 24.0f, 0.0f)
				[
					SNew(SBox)
					.WidthOverride(340.0f)
					[
						SNew(SBorder)
						.BorderBackgroundColor(FLinearColor(0.02f, 0.03f, 0.05f, 0.88f))
						.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
						.Padding(16.0f, 16.0f)
						[
							BuildLeftNavBar()
						]
					]
				]
				// Vertical subtle divider
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.0f, 0.0f, 24.0f, 0.0f)
				[
					SNew(SBox)
					.WidthOverride(1.0f)
					[
						SNew(SBorder)
						.BorderBackgroundColor(FLinearColor(0.15f, 0.20f, 0.28f, 0.8f))
						.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
					]
				]
				// Right Content Switcher
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SAssignNew(ContentSwitcher, SWidgetSwitcher)
					.WidgetIndex(FParse::Param(FCommandLine::Get(), TEXT("Customization")) ? 1 : 0)
					+ SWidgetSwitcher::Slot() [ BuildHomeView() ]           // 0: Home & Matchmaking
					+ SWidgetSwitcher::Slot() [ BuildCustomizationView() ]  // 1: Operator Customization
					+ SWidgetSwitcher::Slot() [ BuildGunsmithView() ]       // 2: Gunsmith & Loadouts
					+ SWidgetSwitcher::Slot() [ BuildServerBrowserView() ]  // 3: Server Browser & VPS
					+ SWidgetSwitcher::Slot() [ BuildSettingsView() ]       // 4: Operator Settings
				]
			]
			// Footer status bar
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 16.0f, 0.0f, 0.0f)
			[
				SNew(SBorder)
				.BorderBackgroundColor(FLinearColor(0.02f, 0.03f, 0.05f, 0.88f))
				.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
				.Padding(16.0f, 8.0f)
				[
					BuildFooter()
				]
			]
		];
}

TSharedRef<SWidget> UMainMenuWidget::BuildHeader()
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("NOBLE FERMI")))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 26))
				.ColorAndOpacity(FLinearColor(0.95f, 0.65f, 0.15f, 1.0f))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 2.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("TACTICAL MILITARY WARFARE // STANDALONE MENU")))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
				.ColorAndOpacity(FLinearColor(0.6f, 0.65f, 0.75f, 1.0f))
			]
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Center)
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor(0.06f, 0.09f, 0.14f, 0.9f))
			.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
			.Padding(14.0f, 8.0f)
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
						.BorderBackgroundColor(FLinearColor(0.2f, 0.9f, 0.4f, 1.0f))
						.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
					]
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("ONLINE — 128 TICK MULTIPLAYER READY")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
					.ColorAndOpacity(FLinearColor(0.85f, 0.90f, 0.95f, 1.0f))
				]
			]
		];
}

TSharedRef<SWidget> UMainMenuWidget::BuildLeftNavBar()
{
	auto MakeNavButton = [this](const FString& Title, const FString& Subtitle, EFPSMenuSection Section) -> TSharedRef<SWidget>
	{
		return SNew(SBox)
			.HeightOverride(56.0f)
			.Padding(0.0f, 0.0f, 0.0f, 8.0f)
			[
				SNew(SButton)
				.ButtonColorAndOpacity(FLinearColor(0.08f, 0.10f, 0.15f, 1.0f))
				.OnClicked_Lambda([this, Section]() { return HandleTabClicked(Section); })
				[
					SNew(SBorder)
					.BorderBackgroundColor(FLinearColor::Transparent)
					.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
					.Padding(12.0f, 6.0f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(STextBlock)
							.Text(FText::FromString(Title))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
							.ColorAndOpacity(FLinearColor(0.92f, 0.94f, 0.98f, 1.0f))
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 2.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(Subtitle))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
							.ColorAndOpacity(FLinearColor(0.50f, 0.55f, 0.65f, 1.0f))
						]
					]
				]
			];
	};

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 12.0f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("/// OPERATIONAL DIRECTIVES")))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
			.ColorAndOpacity(FLinearColor(0.40f, 0.48f, 0.60f, 1.0f))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			MakeNavButton(TEXT("01. DEPLOYMENT // PLAY"), TEXT("Quick Match, Listen Room, Matchmaking"), EFPSMenuSection::Play)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			MakeNavButton(TEXT("02. OPERATOR // CUSTOMISATION"), TEXT("Human Male 180cm, Camo & Turntable"), EFPSMenuSection::Customization)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			MakeNavButton(TEXT("03. GUNSMITH // LOADOUTS"), TEXT("5 MW2 Archetypes & Attachments"), EFPSMenuSection::Loadout)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			MakeNavButton(TEXT("04. SERVER BROWSER & VPS"), TEXT("Dedicated Server Direct IP Connection"), EFPSMenuSection::ServerBrowser)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			MakeNavButton(TEXT("05. OPERATOR SETTINGS"), TEXT("FOV, Sensitivity, Graphics & Audio"), EFPSMenuSection::Settings)
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SNew(SBox)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 12.0f, 0.0f, 0.0f)
		[
			SNew(SBox)
			.HeightOverride(46.0f)
			[
				SNew(SButton)
				.ButtonColorAndOpacity(FLinearColor(0.35f, 0.08f, 0.08f, 1.0f))
				.OnClicked_Lambda([this]() { return HandleQuitClicked(); })
				[
					SNew(SBorder)
					.BorderBackgroundColor(FLinearColor::Transparent)
					.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("QUIT TO DESKTOP")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
						.ColorAndOpacity(FLinearColor(1.0f, 0.7f, 0.7f, 1.0f))
					]
				]
			]
		];
}

TSharedRef<SWidget> UMainMenuWidget::BuildCustomizationView()
{
	auto MakeSectionHeader = [](const FString& Title) -> TSharedRef<SWidget>
	{
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 6.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Title))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
				.ColorAndOpacity(FLinearColor(0.95f, 0.65f, 0.15f, 1.0f))
			];
	};

	auto MakeCamoButton = [this](uint8 Idx, const FString& Name, const FLinearColor& ChipColor) -> TSharedRef<SWidget>
	{
		return SNew(SBox)
			.HeightOverride(38.0f)
			.Padding(0.0f, 0.0f, 0.0f, 5.0f)
			[
				SNew(SButton)
				.ButtonColorAndOpacity(FLinearColor(0.08f, 0.11f, 0.16f, 1.0f))
				.OnClicked_Lambda([this, Idx]() { return HandleCamoSelected(Idx); })
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(10.0f, 0.0f, 8.0f, 0.0f)
					[
						SNew(SBox)
						.WidthOverride(12.0f)
						.HeightOverride(12.0f)
						[
							SNew(SBorder)
							.BorderBackgroundColor(ChipColor)
							.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
						]
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(FText::FromString(Name))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
						.ColorAndOpacity(FLinearColor(0.90f, 0.92f, 0.96f, 1.0f))
					]
				]
			];
	};

	auto MakeItemButton = [](TFunction<FReply()> Callback, const FString& Name) -> TSharedRef<SWidget>
	{
		return SNew(SBox)
			.HeightOverride(36.0f)
			.Padding(0.0f, 0.0f, 0.0f, 5.0f)
			[
				SNew(SButton)
				.ButtonColorAndOpacity(FLinearColor(0.08f, 0.11f, 0.16f, 1.0f))
				.OnClicked_Lambda(Callback)
				[
					SNew(SBorder)
					.BorderBackgroundColor(FLinearColor::Transparent)
					.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
					.Padding(10.0f, 0.0f)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(FText::FromString(Name))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
						.ColorAndOpacity(FLinearColor(0.90f, 0.92f, 0.96f, 1.0f))
					]
				]
			];
	};

	return SNew(SHorizontalBox)
		// Left Panel: Camo, Headgear & Armor Vest
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(0.0f, 0.0f, 20.0f, 0.0f)
		[
			SNew(SBox)
			.WidthOverride(330.0f)
			[
				SNew(SBorder)
				.BorderBackgroundColor(FLinearColor(0.025f, 0.035f, 0.055f, 0.92f))
				.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
				.Padding(14.0f)
				[
					SNew(SScrollBox)
					+ SScrollBox::Slot()
					[
						SNew(SVerticalBox)
						// 1. Camouflage
						+ SVerticalBox::Slot().AutoHeight() [ MakeSectionHeader(TEXT("/// OPERATOR CAMOUFLAGE")) ]
						+ SVerticalBox::Slot().AutoHeight() [ MakeCamoButton(0, TEXT("URBAN DIGITAL (METRO GREY)"), FLinearColor(0.35f, 0.38f, 0.45f)) ]
						+ SVerticalBox::Slot().AutoHeight() [ MakeCamoButton(1, TEXT("WOODLAND TACTICAL (FOREST CAMO)"), FLinearColor(0.22f, 0.35f, 0.18f)) ]
						+ SVerticalBox::Slot().AutoHeight() [ MakeCamoButton(2, TEXT("DESERT SPEC OPS (COYOTE TAN)"), FLinearColor(0.60f, 0.52f, 0.38f)) ]
						+ SVerticalBox::Slot().AutoHeight() [ MakeCamoButton(3, TEXT("STEALTH BLACKOUT (MIDNIGHT)"), FLinearColor(0.08f, 0.08f, 0.10f)) ]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 12.0f)
						[
							SAssignNew(CamoStatusText, STextBlock)
							.Text(FText::FromString(TEXT("PATTERN: URBAN DIGITAL // CHARCOAL")))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
							.ColorAndOpacity(FLinearColor(0.40f, 0.75f, 1.0f, 1.0f))
						]

						// 2. Headgear
						+ SVerticalBox::Slot().AutoHeight() [ MakeSectionHeader(TEXT("/// TACTICAL HEADGEAR")) ]
						+ SVerticalBox::Slot().AutoHeight() [ MakeItemButton([this]() { return HandleHeadgearSelected(0); }, TEXT("FAST BALLISTIC HELMET (HIGH CUT)")) ]
						+ SVerticalBox::Slot().AutoHeight() [ MakeItemButton([this]() { return HandleHeadgearSelected(1); }, TEXT("GPNVG-18 QUAD-NODS (PHOSPHOR GLOW)")) ]
						+ SVerticalBox::Slot().AutoHeight() [ MakeItemButton([this]() { return HandleHeadgearSelected(2); }, TEXT("SPEC OPS BALACLAVA (FIRE RETARDANT)")) ]
						+ SVerticalBox::Slot().AutoHeight() [ MakeItemButton([this]() { return HandleHeadgearSelected(3); }, TEXT("TACTICAL CAP & PELTOR HEADSET")) ]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 12.0f)
						[
							SAssignNew(HeadgearStatusText, STextBlock)
							.Text(FText::FromString(TEXT("HEADGEAR: FAST BALLISTIC HELMET")))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
							.ColorAndOpacity(FLinearColor(0.40f, 0.75f, 1.0f, 1.0f))
						]

						// 3. Armor Protection Vest
						+ SVerticalBox::Slot().AutoHeight() [ MakeSectionHeader(TEXT("/// BALLISTIC ARMOR VEST")) ]
						+ SVerticalBox::Slot().AutoHeight() [ MakeItemButton([this]() { return HandleVestSelected(0); }, TEXT("LEVEL IV HEAVY CARRIER (CERAMIC PLATES)")) ]
						+ SVerticalBox::Slot().AutoHeight() [ MakeItemButton([this]() { return HandleVestSelected(1); }, TEXT("LIGHTWEIGHT JPC VEST (AGILE MOBILITY)")) ]
						+ SVerticalBox::Slot().AutoHeight() [ MakeItemButton([this]() { return HandleVestSelected(2); }, TEXT("RECON TACTICAL RIG (MINIMALIST)")) ]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 6.0f)
						[
							SAssignNew(VestStatusText, STextBlock)
							.Text(FText::FromString(TEXT("ARMOR: LEVEL IV HEAVY CARRIER")))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
							.ColorAndOpacity(FLinearColor(0.40f, 0.75f, 1.0f, 1.0f))
						]
					]
				]
			]
		]

		// Center: Transparent view opening framing the 3D rotating operator in aiming stance
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		.VAlign(VAlign_Bottom)
		.Padding(12.0f, 0.0f, 12.0f, 10.0f)
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor(0.02f, 0.03f, 0.05f, 0.80f))
			.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
			.Padding(16.0f, 8.0f)
			.HAlign(HAlign_Center)
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
						.BorderBackgroundColor(FLinearColor(0.2f, 0.9f, 0.4f, 1.0f))
						.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
					]
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("LIVE 3D VIEWPORT // 360° TURNTABLE ROTATION // ADS AIM WEAPON ALIGNMENT")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
					.ColorAndOpacity(FLinearColor(0.90f, 0.92f, 0.96f, 1.0f))
				]
			]
		]

		// Right Panel: Stance, Turntable & Human Male Biometrics
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(20.0f, 0.0f, 0.0f, 0.0f)
		[
			SNew(SBox)
			.WidthOverride(330.0f)
			[
				SNew(SBorder)
				.BorderBackgroundColor(FLinearColor(0.025f, 0.035f, 0.055f, 0.92f))
				.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
				.Padding(14.0f)
				[
					SNew(SScrollBox)
					+ SScrollBox::Slot()
					[
						SNew(SVerticalBox)
						// 1. Aiming Stance
						+ SVerticalBox::Slot().AutoHeight() [ MakeSectionHeader(TEXT("/// AIMING STANCE CONTROLS")) ]
						+ SVerticalBox::Slot().AutoHeight()
						[
							MakeItemButton([this]() { return HandleToggleAimingStance(); }, TEXT("TOGGLE AIMING STANCE (ADS / LOW READY)"))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 12.0f)
						[
							SAssignNew(StanceStatusText, STextBlock)
							.Text(FText::FromString(TEXT("STANCE: HIGH-READY ADS (AIM-DOWN-SIGHTS)")))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
							.ColorAndOpacity(FLinearColor(0.95f, 0.75f, 0.2f, 1.0f))
						]

						// 2. Turntable Rotation
						+ SVerticalBox::Slot().AutoHeight() [ MakeSectionHeader(TEXT("/// TURNTABLE ROTATION")) ]
						+ SVerticalBox::Slot().AutoHeight()
						[
							MakeItemButton([this]() { return HandleToggleTurntableRotation(); }, TEXT("PAUSE / RESUME ROTATION"))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(0.0f, 0.0f, 4.0f, 0.0f)
							[
								SNew(SButton)
								.ButtonColorAndOpacity(FLinearColor(0.08f, 0.11f, 0.16f, 1.0f))
								.OnClicked_Lambda([this]() { return HandleSetRotationSpeed(12.0f); })
								.HAlign(HAlign_Center)
								[
									SNew(STextBlock).Text(FText::FromString(TEXT("12°/s"))).Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
								]
							]
							+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.0f, 0.0f)
							[
								SNew(SButton)
								.ButtonColorAndOpacity(FLinearColor(0.08f, 0.11f, 0.16f, 1.0f))
								.OnClicked_Lambda([this]() { return HandleSetRotationSpeed(24.0f); })
								.HAlign(HAlign_Center)
								[
									SNew(STextBlock).Text(FText::FromString(TEXT("24°/s"))).Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
								]
							]
							+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(4.0f, 0.0f, 0.0f, 0.0f)
							[
								SNew(SButton)
								.ButtonColorAndOpacity(FLinearColor(0.08f, 0.11f, 0.16f, 1.0f))
								.OnClicked_Lambda([this]() { return HandleSetRotationSpeed(45.0f); })
								.HAlign(HAlign_Center)
								[
									SNew(STextBlock).Text(FText::FromString(TEXT("45°/s"))).Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
								]
							]
						]
						+ SVerticalBox::Slot().AutoHeight()
						[
							MakeItemButton([this]() { return HandleResetTurntableFacing(); }, TEXT("RESET FRONT FACING (0° YAW)"))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 12.0f)
						[
							SAssignNew(RotationSpeedText, STextBlock)
							.Text(FText::FromString(TEXT("ROTATION: ACTIVE (18.0 DEG/SEC)")))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
							.ColorAndOpacity(FLinearColor(0.40f, 0.75f, 1.0f, 1.0f))
						]

						// 3. Male Skeletal Anatomy Telemetry
						+ SVerticalBox::Slot().AutoHeight() [ MakeSectionHeader(TEXT("/// SKELETAL ANATOMY & BIOMETRICS")) ]
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SBorder)
							.BorderBackgroundColor(FLinearColor(0.015f, 0.02f, 0.03f, 0.95f))
							.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
							.Padding(10.0f)
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot().AutoHeight()
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("• MODEL: ADULT HUMAN MALE\n"
										"• HEIGHT: 180.0 CM STANDING\n"
										"• TOTAL MASS: 82.0 KG (ANTHROPOMETRIC)\n"
										"• SKELETAL BONES: 21 HIERARCHICAL NODES\n"
										"• ARMOR: LEVEL IV CERAMIC (CHEST/THORACIC)\n"
										"• FLINCH: 2ND-ORDER SPRING-MASS-DAMPER\n"
										"• SWAY: FIRST-PERSON ADS STABILIZED\n"
										"• IK: 2-BONE PROCEDURAL FOOT ADJUSTMENT")))
									.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
									.ColorAndOpacity(FLinearColor(0.70f, 0.75f, 0.85f, 1.0f))
								]
							]
						]
					]
				]
			]
		];
}

TSharedRef<SWidget> UMainMenuWidget::BuildHomeView()
{
	return SNew(SScrollBox)
		+ SScrollBox::Slot()
		.Padding(0.0f, 0.0f, 16.0f, 0.0f)
		[
			SNew(SVerticalBox)
			// Hero Tactical Banner
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 16.0f)
			[
				SNew(SBorder)
				.BorderBackgroundColor(FLinearColor(0.05f, 0.07f, 0.11f, 0.95f))
				.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
				.Padding(20.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("MODERN COMBAT OPERATIONS")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 18))
						.ColorAndOpacity(FLinearColor(0.95f, 0.65f, 0.15f, 1.0f))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 6.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Tactical 128-tick multiplayer shooter with authoritative physics, procedural weapon sway, human male ballistics, and dedicated server hosting.")))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
						.ColorAndOpacity(FLinearColor(0.70f, 0.75f, 0.85f, 1.0f))
					]
				]
			]
			// Quick Play CTA Action Card
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 16.0f)
			[
				SNew(SBorder)
				.BorderBackgroundColor(FLinearColor(0.09f, 0.14f, 0.22f, 0.95f))
				.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
				.Padding(20.0f)
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
							.Text(FText::FromString(TEXT("QUICKPLAY — INSTANT COMBAT")))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
							.ColorAndOpacity(FLinearColor(0.92f, 0.95f, 1.0f, 1.0f))
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 4.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("Launches local 128-tick listen server or connects to the lowest ping regional node.")))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
							.ColorAndOpacity(FLinearColor(0.55f, 0.65f, 0.78f, 1.0f))
						]
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(16.0f, 0.0f, 0.0f, 0.0f)
					[
						SNew(SBox)
						.WidthOverride(180.0f)
						.HeightOverride(46.0f)
						[
							SNew(SButton)
							.ButtonColorAndOpacity(FLinearColor(0.95f, 0.65f, 0.15f, 1.0f))
							.OnClicked_Lambda([this]() { return HandleQuickplayClicked(); })
							[
								SNew(SBorder)
								.BorderBackgroundColor(FLinearColor::Transparent)
								.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
								.HAlign(HAlign_Center)
								.VAlign(VAlign_Center)
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("DEPLOY NOW")))
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
									.ColorAndOpacity(FLinearColor(0.05f, 0.06f, 0.08f, 1.0f))
								]
							]
						]
					]
				]
			]
			// Game Modes List
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 8.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("PLAYLIST ROTATION:")))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
				.ColorAndOpacity(FLinearColor(0.40f, 0.48f, 0.60f, 1.0f))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 8.0f)
			[
				SNew(SBorder)
				.BorderBackgroundColor(FLinearColor(0.04f, 0.06f, 0.09f, 0.9f))
				.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
				.Padding(14.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("• TEAM DEATHMATCH (6v6) — 7,500 Score Limit, Classic MW2 Pace")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
						.ColorAndOpacity(FLinearColor(0.85f, 0.88f, 0.94f, 1.0f))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 6.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("• SEARCH & DESTROY — Single-life tactical demolition and site defense")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
						.ColorAndOpacity(FLinearColor(0.85f, 0.88f, 0.94f, 1.0f))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 6.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("• DOMINATION — 3-flag continuous area control and frontline warfare")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
						.ColorAndOpacity(FLinearColor(0.85f, 0.88f, 0.94f, 1.0f))
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
				.ButtonColorAndOpacity(FLinearColor(0.07f, 0.09f, 0.13f, 1.0f))
				.OnClicked_Lambda([this, Idx]() { return HandleSelectArchetype(Idx); })
				[
					SNew(SBorder)
					.BorderBackgroundColor(FLinearColor::Transparent)
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
								.ColorAndOpacity(FLinearColor(0.92f, 0.94f, 0.98f, 1.0f))
							]
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0.0f, 2.0f, 0.0f, 0.0f)
							[
								SNew(STextBlock)
								.Text(FText::FromString(Weapon + TEXT(" // ") + Role))
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
								.ColorAndOpacity(FLinearColor(0.50f, 0.58f, 0.70f, 1.0f))
							]
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("SELECT >")))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
							.ColorAndOpacity(FLinearColor(0.95f, 0.65f, 0.15f, 0.8f))
						]
					]
				]
			];
	};

	return SNew(SHorizontalBox)
		// Left Archetypes List
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(0.0f, 0.0f, 20.0f, 0.0f)
		[
			SNew(SBox)
			.WidthOverride(340.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 10.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("/// SELECT COMBAT LOADOUT")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
					.ColorAndOpacity(FLinearColor(0.40f, 0.48f, 0.60f, 1.0f))
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
			.BorderBackgroundColor(FLinearColor(0.035f, 0.045f, 0.065f, 0.95f))
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
					.ColorAndOpacity(FLinearColor(0.85f, 0.88f, 0.95f, 1.0f))
				]
			]
		];
}

TSharedRef<SWidget> UMainMenuWidget::BuildServerBrowserView()
{
	return SNew(SVerticalBox)
		// Section Header
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 14.0f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("/// MULTIPLAYER HOSTING & VPS DIRECT CONNECT")))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
			.ColorAndOpacity(FLinearColor(0.40f, 0.48f, 0.60f, 1.0f))
		]
		// Direct IP Connection Box
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 16.0f)
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor(0.05f, 0.07f, 0.11f, 0.95f))
			.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
			.Padding(18.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("CONNECT DIRECTLY TO DEDICATED SERVER / VPS")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
					.ColorAndOpacity(FLinearColor(0.92f, 0.95f, 1.0f, 1.0f))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 4.0f, 0.0f, 10.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Enter VPS IP and Port (e.g., 127.0.0.1:7777 or your public VPS IP). Uses 128-tick authoritative replication.")))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
					.ColorAndOpacity(FLinearColor(0.55f, 0.62f, 0.72f, 1.0f))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.Padding(0.0f, 0.0f, 12.0f, 0.0f)
					[
						SAssignNew(DirectConnectInput, SEditableTextBox)
						.Text(FText::FromString(TEXT("127.0.0.1:7777")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
						.HintText(FText::FromString(TEXT("IP_ADDRESS:PORT")))
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SBox)
						.WidthOverride(140.0f)
						.HeightOverride(36.0f)
						[
							SNew(SButton)
							.ButtonColorAndOpacity(FLinearColor(0.12f, 0.45f, 0.25f, 1.0f))
							.OnClicked_Lambda([this]() { return HandleDirectConnectClicked(); })
							[
								SNew(SBorder)
								.BorderBackgroundColor(FLinearColor::Transparent)
								.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
								.HAlign(HAlign_Center)
								.VAlign(VAlign_Center)
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("CONNECT")))
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
									.ColorAndOpacity(FLinearColor::White)
								]
							]
						]
					]
				]
			]
		]
		// Host Listen Server Box
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 16.0f)
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor(0.05f, 0.07f, 0.11f, 0.95f))
			.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
			.Padding(18.0f)
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
						.Text(FText::FromString(TEXT("HOST LISTEN ROOM FROM CLIENT")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
						.ColorAndOpacity(FLinearColor(0.92f, 0.95f, 1.0f, 1.0f))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 4.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Launch a peer-hosted match (?listen) directly from your client machine. Friends can connect via your IP.")))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
						.ColorAndOpacity(FLinearColor(0.55f, 0.62f, 0.72f, 1.0f))
					]
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(16.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SBox)
					.WidthOverride(160.0f)
					.HeightOverride(40.0f)
					[
						SNew(SButton)
						.ButtonColorAndOpacity(FLinearColor(0.20f, 0.35f, 0.60f, 1.0f))
						.OnClicked_Lambda([this]() { return HandleHostListenClicked(); })
						[
							SNew(SBorder)
							.BorderBackgroundColor(FLinearColor::Transparent)
							.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("HOST LISTEN ROOM")))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
								.ColorAndOpacity(FLinearColor::White)
							]
						]
					]
				]
			]
		]
		// Server List Preview
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor(0.035f, 0.045f, 0.065f, 0.95f))
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
						.ColorAndOpacity(FLinearColor(0.95f, 0.65f, 0.15f, 1.0f))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 4.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("• [EU CENTRAL] Frankfurt #1 — 12/12 Players | Map: Rust Tactical | Ping: 18ms")))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
						.ColorAndOpacity(FLinearColor(0.85f, 0.88f, 0.94f, 1.0f))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 4.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("• [US EAST] Virginia #2 — 8/12 Players | Map: Terminal 24/7 | Ping: 82ms")))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
						.ColorAndOpacity(FLinearColor(0.85f, 0.88f, 0.94f, 1.0f))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 4.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("• [LOCAL VPS] 127.0.0.1:7777 — Standalone Dedicated Server Binary Ready")))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
						.ColorAndOpacity(FLinearColor(0.2f, 0.9f, 0.4f, 1.0f))
					]
				]
			]
		];
}

TSharedRef<SWidget> UMainMenuWidget::BuildSettingsView()
{
	return SNew(SScrollBox)
		+ SScrollBox::Slot()
		.Padding(0.0f, 0.0f, 16.0f, 0.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 12.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("/// OPERATOR PREFERENCES & CLIENT SETTINGS")))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
				.ColorAndOpacity(FLinearColor(0.40f, 0.48f, 0.60f, 1.0f))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 12.0f)
			[
				SNew(SBorder)
				.BorderBackgroundColor(FLinearColor(0.04f, 0.06f, 0.09f, 0.95f))
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
						.ColorAndOpacity(FLinearColor(0.95f, 0.65f, 0.15f, 1.0f))
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
						.ColorAndOpacity(FLinearColor(0.85f, 0.88f, 0.95f, 1.0f))
					]
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBorder)
				.BorderBackgroundColor(FLinearColor(0.04f, 0.06f, 0.09f, 0.95f))
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
						.ColorAndOpacity(FLinearColor(0.95f, 0.65f, 0.15f, 1.0f))
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
						.ColorAndOpacity(FLinearColor(0.85f, 0.88f, 0.95f, 1.0f))
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
			.Text(FText::FromString(TEXT("SYSTEM READY // ENGINE: UE 5.8 // BUILD: STANDALONE MONOLITHIC // NET: 128-HZ")))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
			.ColorAndOpacity(FLinearColor(0.45f, 0.50f, 0.60f, 1.0f))
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("PRESS [ESC] TO CLOSE // NOBLE FERMI MULTIPLAYER")))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
			.ColorAndOpacity(FLinearColor(0.45f, 0.50f, 0.60f, 1.0f))
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
			TargetIndex = 0;
			break;
		case EFPSMenuSection::Customization:
			TargetIndex = 1;
			break;
		case EFPSMenuSection::Loadout:
			TargetIndex = 2;
			break;
		case EFPSMenuSection::ServerBrowser:
			TargetIndex = 3;
			break;
		case EFPSMenuSection::Settings:
			TargetIndex = 4;
			break;
		default:
			TargetIndex = 0;
			break;
		}
		ContentSwitcher->SetActiveWidgetIndex(TargetIndex);
	}

	if (TransitionSound)
	{
		UGameplayStatics::PlaySound2D(this, TransitionSound);
	}

	OnSectionChanged.Broadcast(Section);
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
		case 0: CamoStatusText->SetText(FText::FromString(TEXT("PATTERN: URBAN DIGITAL // CHARCOAL"))); break;
		case 1: CamoStatusText->SetText(FText::FromString(TEXT("PATTERN: WOODLAND TACTICAL // RANGER OLIVE"))); break;
		case 2: CamoStatusText->SetText(FText::FromString(TEXT("PATTERN: DESERT SPEC OPS // COYOTE TAN"))); break;
		case 3: CamoStatusText->SetText(FText::FromString(TEXT("PATTERN: STEALTH BLACKOUT // MIDNIGHT MATTE"))); break;
		}
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
		const bool bNewAiming = !PreviewActor->IsAiming();
		PreviewActor->SetAimingStance(bNewAiming);
		if (StanceStatusText.IsValid())
		{
			StanceStatusText->SetText(FText::FromString(bNewAiming ?
				TEXT("STANCE: HIGH-READY ADS (AIM-DOWN-SIGHTS)") :
				TEXT("STANCE: LOW-READY (PATROL RELAXED)")));
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
