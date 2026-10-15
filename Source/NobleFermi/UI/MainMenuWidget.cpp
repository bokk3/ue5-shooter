#include "UI/MainMenuWidget.h"
#include "UI/SettingsMenuWidget.h"
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

TSharedRef<SWidget> UMainMenuWidget::RebuildWidget()
{
	return SNew(SOverlay)
		// Background base dark tint
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor(0.025f, 0.030f, 0.040f, 0.98f))
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
				BuildHeader()
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
						BuildLeftNavBar()
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
					.WidgetIndex(0)
					+ SWidgetSwitcher::Slot() [ BuildHomeView() ]
					+ SWidgetSwitcher::Slot() [ BuildGunsmithView() ]
					+ SWidgetSwitcher::Slot() [ BuildServerBrowserView() ]
					+ SWidgetSwitcher::Slot() [ BuildSettingsView() ]
				]
			]
			// Footer status bar
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 16.0f, 0.0f, 0.0f)
			[
				BuildFooter()
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
			MakeNavButton(TEXT("02. GUNSMITH // LOADOUTS"), TEXT("5 MW2 Archetypes & Attachments"), EFPSMenuSection::Loadout)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			MakeNavButton(TEXT("03. SERVER BROWSER & VPS"), TEXT("Dedicated Server Direct IP Connection"), EFPSMenuSection::ServerBrowser)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			MakeNavButton(TEXT("04. OPERATOR SETTINGS"), TEXT("FOV, Sensitivity, Graphics & Audio"), EFPSMenuSection::Settings)
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
						.Text(FText::FromString(TEXT("Fast-paced 6v6 tactical military shooter. Master recoil compensation, tactical equipment cooking, and low TTK lethality.")))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 11))
						.ColorAndOpacity(FLinearColor(0.70f, 0.75f, 0.85f, 1.0f))
					]
				]
			]
			// Action Buttons
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 16.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(0.5f)
				.Padding(0.0f, 0.0f, 8.0f, 0.0f)
				[
					SNew(SBox)
					.HeightOverride(56.0f)
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
								.Text(FText::FromString(TEXT("[ START QUICK MATCH ]")))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
								.ColorAndOpacity(FLinearColor(0.05f, 0.05f, 0.05f, 1.0f))
							]
						]
					]
				]
				+ SHorizontalBox::Slot()
				.FillWidth(0.5f)
				.Padding(8.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SBox)
					.HeightOverride(56.0f)
					[
						SNew(SButton)
						.ButtonColorAndOpacity(FLinearColor(0.12f, 0.16f, 0.22f, 1.0f))
						.OnClicked_Lambda([this]() { return HandleHostListenClicked(); })
						[
							SNew(SBorder)
							.BorderBackgroundColor(FLinearColor::Transparent)
							.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("[ HOST LISTEN SERVER ]")))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 13))
								.ColorAndOpacity(FLinearColor(0.90f, 0.93f, 0.98f, 1.0f))
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
					.Padding(10.0f, 6.0f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(FText::FromString(Name))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
							.ColorAndOpacity(FLinearColor(0.95f, 0.65f, 0.15f, 1.0f))
						]
						+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						.HAlign(HAlign_Right)
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(FText::FromString(FString::Printf(TEXT("%s  [%s]"), *Weapon, *Role)))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
							.ColorAndOpacity(FLinearColor(0.65f, 0.70f, 0.80f, 1.0f))
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
			.Text(FText::FromString(TEXT("CREATE-A-CLASS // WEAPON GUNSMITH ARCHETYPES")))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
			.ColorAndOpacity(FLinearColor(0.95f, 0.65f, 0.15f, 1.0f))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			MakeArchetypeBtn(0, TEXT("CLASS 1: ASSAULT"), TEXT("M4A1 RIFLE"), TEXT("Balanced Versatility"))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			MakeArchetypeBtn(1, TEXT("CLASS 2: SPEC OPS"), TEXT("MP5SD SUBMACHINE"), TEXT("Stealth Infiltration"))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			MakeArchetypeBtn(2, TEXT("CLASS 3: HEAVY GUNNER"), TEXT("L86 LSW LMG"), TEXT("Area Denial & Suppression"))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			MakeArchetypeBtn(3, TEXT("CLASS 4: RECON SNIPER"), TEXT("INTERVENTION .50"), TEXT("Long Range One-Shot"))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			MakeArchetypeBtn(4, TEXT("CLASS 5: RUN & GUN"), TEXT("VECTOR .45 ACP"), TEXT("Extreme Close Quarters"))
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		.Padding(0.0f, 12.0f, 0.0f, 0.0f)
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor(0.04f, 0.06f, 0.09f, 0.9f))
			.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
			.Padding(16.0f)
			[
				SAssignNew(SelectedLoadoutDetails, STextBlock)
				.Text(FText::FromString(TEXT("SELECTED: CLASS 1 (ASSAULT)\n\n"
					"PRIMARY: M4A1 (Attachments: Red Dot Sight, Compensator, 40-Round Mag, Stippled Grip)\n"
					"SECONDARY: USP .45 Tactical (Tactical Knife)\n"
					"LETHAL: Frag Grenade (4.0s cookable fuse, cooking tick)\n"
					"TACTICAL: Flashbang (Disorientation + LOS blinding)\n"
					"PERKS: Sleight of Hand Pro | Stopping Power | Ninja Pro\n\n"
					"RECOIL STATS: Kick 1.2° | ADS Speed 210ms | Effective Range 45m | Muzzle Velocity 880 m/s")))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
				.ColorAndOpacity(FLinearColor(0.85f, 0.88f, 0.95f, 1.0f))
			]
		];
}

TSharedRef<SWidget> UMainMenuWidget::BuildServerBrowserView()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 12.0f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("SERVER BROWSER // DEDICATED VPS & DIRECT IP CONNECTION")))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
			.ColorAndOpacity(FLinearColor(0.95f, 0.65f, 0.15f, 1.0f))
		]
		// Direct IP Connection Card
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 16.0f)
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor(0.05f, 0.07f, 0.11f, 0.9f))
			.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
			.Padding(16.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("ENTER DEDICATED SERVER IP / DOMAIN:")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
					.ColorAndOpacity(FLinearColor(0.85f, 0.88f, 0.95f, 1.0f))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 8.0f, 0.0f, 12.0f)
				[
					SAssignNew(DirectConnectInput, SEditableTextBox)
					.Text(FText::FromString(TEXT("127.0.0.1:7777")))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 12))
					.Padding(8.0f)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0.0f, 0.0f, 8.0f, 0.0f)
					[
						SNew(SBox)
						.WidthOverride(180.0f)
						.HeightOverride(42.0f)
						[
							SNew(SButton)
							.ButtonColorAndOpacity(FLinearColor(0.95f, 0.65f, 0.15f, 1.0f))
							.OnClicked_Lambda([this]() { return HandleDirectConnectClicked(); })
							[
								SNew(SBorder)
								.BorderBackgroundColor(FLinearColor::Transparent)
								.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
								.HAlign(HAlign_Center)
								.VAlign(VAlign_Center)
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("CONNECT DIRECT")))
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
									.ColorAndOpacity(FLinearColor(0.05f, 0.05f, 0.05f, 1.0f))
								]
							]
						]
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SBox)
						.WidthOverride(200.0f)
						.HeightOverride(42.0f)
						[
							SNew(SButton)
							.ButtonColorAndOpacity(FLinearColor(0.12f, 0.16f, 0.22f, 1.0f))
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
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
									.ColorAndOpacity(FLinearColor(0.90f, 0.93f, 0.98f, 1.0f))
								]
							]
						]
					]
				]
			]
		]
		// VPS Hosting Information
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor(0.04f, 0.05f, 0.08f, 0.9f))
			.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
			.Padding(16.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("VPS DEDICATED SERVER HOSTING GUIDELINES:")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
					.ColorAndOpacity(FLinearColor(0.95f, 0.65f, 0.15f, 1.0f))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 6.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("To run your own high-performance 128-tick community server on Windows or Linux VPS:\n\n"
						"• Launch Command:  NobleFermi.exe -server -nullrhi -log -PORT=7777 -TickRate=128 -MaxPlayers=18\n"
						"• Docker Command:  cd Scripts/DedicatedServer && docker-compose up -d\n"
						"• Default Ports:   7777/UDP (Game Traffic), 27015/UDP (Steam Query / Master Listing)\n"
						"• RCON Terminal:   Configurable in Scripts/DedicatedServer/ServerConfig.ini\n\n"
						"Players can connect instantly using the IP entry above.")))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
					.ColorAndOpacity(FLinearColor(0.70f, 0.75f, 0.85f, 1.0f))
				]
			]
		];
}

TSharedRef<SWidget> UMainMenuWidget::BuildSettingsView()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 12.0f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("OPERATOR SETTINGS // GRAPHICS & CONTROLS")))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
			.ColorAndOpacity(FLinearColor(0.95f, 0.65f, 0.15f, 1.0f))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 12.0f)
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor(0.04f, 0.06f, 0.09f, 0.9f))
			.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
			.Padding(16.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("FIELD OF VIEW (FOV): 105° [COMPETITIVE STANDARD]")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
					.ColorAndOpacity(FLinearColor(0.85f, 0.88f, 0.95f, 1.0f))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 8.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("MOUSE SENSITIVITY: 1.85 (400 DPI equivalent: 34.6 cm / 360°)")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
					.ColorAndOpacity(FLinearColor(0.85f, 0.88f, 0.95f, 1.0f))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 8.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("FRAME RATE CAP: 240 FPS | REFLEX LOW LATENCY: ENABLED + BOOST")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
					.ColorAndOpacity(FLinearColor(0.85f, 0.88f, 0.95f, 1.0f))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 8.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("SPATIAL AUDIO: HRTF HEADPHONES WITH ACOUSTIC OCCLUSION")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
					.ColorAndOpacity(FLinearColor(0.85f, 0.88f, 0.95f, 1.0f))
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
			.Text(FText::FromString(TEXT("NETCODE: FAST-REPLICATED 128Hz | SUB-SYSTEM: ONLINE | STANDALONE READY")))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
			.ColorAndOpacity(FLinearColor(0.40f, 0.45f, 0.55f, 1.0f))
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("[ESC] CANCEL / BACK    [ENTER] CONFIRM / DEPLOY    [CLICK] SELECT")))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
			.ColorAndOpacity(FLinearColor(0.45f, 0.50f, 0.60f, 1.0f))
		];
}

void UMainMenuWidget::NavigateToSection(EFPSMenuSection Section)
{
	CurrentSection = Section;
	if (ContentSwitcher.IsValid())
	{
		ContentSwitcher->SetActiveWidgetIndex(static_cast<int32>(Section));
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
		PC->ClientTravel(TEXT("?listen"), TRAVEL_Absolute);
	}
}

void UMainMenuWidget::ConnectToDedicatedServer(const FString& ServerAddress)
{
	APlayerController* PC = GetOwningPlayer();
	if (PC)
	{
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
