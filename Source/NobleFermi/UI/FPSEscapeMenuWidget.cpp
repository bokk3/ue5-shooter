#include "UI/FPSEscapeMenuWidget.h"
#include "Character/FPSCharacter.h"
#include "UI/FPSHUDWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/GameUserSettings.h"
#include "Misc/ConfigCacheIni.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Images/SImage.h"
#include "Styling/CoreStyle.h"
#include "Styling/AppStyle.h"

UFPSEscapeMenuWidget::UFPSEscapeMenuWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(true);
}

void UFPSEscapeMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	ReadCurrentHardwareSettings();
	UpdateTelemetryLabels();
}

void UFPSEscapeMenuWidget::SetOwningCharacter(AFPSCharacter* InCharacter)
{
	OwningFPSCharacter = InCharacter;
}

void UFPSEscapeMenuWidget::ReadCurrentHardwareSettings()
{
	bool bFoundSavedConfig = false;
	if (GConfig)
	{
		int32 LumenVal = 0;
		if (GConfig->GetInt(TEXT("SystemSettings"), TEXT("r.DynamicGlobalIlluminationMethod"), LumenVal, GEngineIni))
		{
			bLumenActive = (LumenVal > 0);
			bFoundSavedConfig = true;
		}
		int32 VSMVal = 0;
		if (GConfig->GetInt(TEXT("SystemSettings"), TEXT("r.Shadow.Virtual.Enable"), VSMVal, GEngineIni))
		{
			bVSMActive = (VSMVal > 0);
		}
		float ScreenPerc = 70.0f;
		if (GConfig->GetFloat(TEXT("SystemSettings"), TEXT("r.ScreenPercentage"), ScreenPerc, GEngineIni))
		{
			CurrentScreenPercentage = ScreenPerc;
		}
	}

	if (!bFoundSavedConfig)
	{
		CurrentPresetIndex = 0; // Default to Laptop / Esports for iGPU development safety
		bLumenActive = false;
		bVSMActive = false;
		CurrentScreenPercentage = 70.0f;
	}
	else if (bLumenActive && bVSMActive)
	{
		CurrentPresetIndex = 3; // Ultra
	}
	else if (bLumenActive && !bVSMActive)
	{
		CurrentPresetIndex = 2; // High
	}
	else if (!bLumenActive && CurrentScreenPercentage > 80.0f)
	{
		CurrentPresetIndex = 1; // Balanced
	}
	else
	{
		CurrentPresetIndex = 0; // Laptop / Esports
	}

	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	if (Settings)
	{
		CurrentFpsLimit = static_cast<int32>(Settings->GetFrameRateLimit());
		if (CurrentFpsLimit <= 0) CurrentFpsLimit = 0;
	}
}

TSharedRef<SWidget> UFPSEscapeMenuWidget::RebuildWidget()
{
	ReadCurrentHardwareSettings();

	return SNew(SOverlay)
		// Fullscreen dark tactical backdrop
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor(0.012f, 0.018f, 0.028f, 0.90f))
			.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
		]
		// Switcher between Main Command Menu and Graphics Settings Menu
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SAssignNew(MenuSwitcher, SWidgetSwitcher)
			.WidgetIndex(0)
			+ SWidgetSwitcher::Slot()
			[
				BuildMainCommandView()
			]
			+ SWidgetSwitcher::Slot()
			[
				BuildGraphicsSettingsView()
			]
		];
}

TSharedRef<SWidget> UFPSEscapeMenuWidget::BuildMainCommandView()
{
	const FSlateFontInfo TitleFont = FCoreStyle::GetDefaultFontStyle("Bold", 17);
	const FSlateFontInfo SubTitleFont = FCoreStyle::GetDefaultFontStyle("Regular", 10);
	const FSlateFontInfo ButtonFont = FCoreStyle::GetDefaultFontStyle("Bold", 12);

	const FLinearColor AmberColor(1.0f, 0.70f, 0.0f, 1.0f);   // #FFB300
	const FLinearColor CyanColor(0.0f, 0.90f, 1.0f, 1.0f);    // #00E5FF
	const FLinearColor MutedColor(0.55f, 0.65f, 0.75f, 0.8f);

	auto CreateMenuButton = [this, ButtonFont, AmberColor](const FText& LabelText, FOnClicked OnClickedDelegate) -> TSharedRef<SWidget>
	{
		return SNew(SBox)
			.HeightOverride(44.0f)
			.Padding(FMargin(0.0f, 4.0f))
			[
				SNew(SButton)
				.OnClicked(OnClickedDelegate)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LabelText)
					.Font(ButtonFont)
					.ColorAndOpacity(AmberColor)
				]
			];
	};

	return SNew(SBox)
		.WidthOverride(480.0f)
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor(0.04f, 0.07f, 0.11f, 0.96f))
			.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
			.Padding(FMargin(28.0f, 24.0f))
			[
				SNew(SVerticalBox)
				// Header
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.Padding(0.0f, 0.0f, 0.0f, 4.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("NOBLE FERMI // TACTICAL COMMAND")))
					.Font(TitleFont)
					.ColorAndOpacity(AmberColor)
				]
				// Subtitle telemetry
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.Padding(0.0f, 0.0f, 0.0f, 16.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("LIVE SERVER 128 HZ // NON-PAUSED SESSION")))
					.Font(SubTitleFont)
					.ColorAndOpacity(CyanColor)
				]
				// Button list
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					CreateMenuButton(
						FText::FromString(TEXT("RESUME OPERATION  [ESC / P]")),
						FOnClicked::CreateUObject(this, &UFPSEscapeMenuWidget::OnResumeClicked)
					)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					CreateMenuButton(
						FText::FromString(TEXT("GRAPHICS & PERFORMANCE PRESETS")),
						FOnClicked::CreateUObject(this, &UFPSEscapeMenuWidget::OnSettingsClicked)
					)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					CreateMenuButton(
						FText::FromString(TEXT("RETURN TO MAIN MENU")),
						FOnClicked::CreateUObject(this, &UFPSEscapeMenuWidget::OnMainMenuClicked)
					)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					CreateMenuButton(
						FText::FromString(TEXT("QUIT TO DESKTOP")),
						FOnClicked::CreateUObject(this, &UFPSEscapeMenuWidget::OnExitClicked)
					)
				]
				// Footer note
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.Padding(0.0f, 16.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("PRESS [ESC] OR [P] TO TOGGLE COMMAND OVERLAY")))
					.Font(SubTitleFont)
					.ColorAndOpacity(MutedColor)
				]
			]
		];
}

TSharedRef<SWidget> UFPSEscapeMenuWidget::BuildGraphicsSettingsView()
{
	const FSlateFontInfo HeaderFont = FCoreStyle::GetDefaultFontStyle("Bold", 16);
	const FSlateFontInfo SubTitleFont = FCoreStyle::GetDefaultFontStyle("Regular", 10);
	const FSlateFontInfo PresetBtnFont = FCoreStyle::GetDefaultFontStyle("Bold", 11);
	const FSlateFontInfo PresetSubFont = FCoreStyle::GetDefaultFontStyle("Regular", 9);
	const FSlateFontInfo ToggleFont = FCoreStyle::GetDefaultFontStyle("Bold", 10);

	const FLinearColor AmberColor(1.0f, 0.70f, 0.0f, 1.0f);   // #FFB300
	const FLinearColor CyanColor(0.0f, 0.90f, 1.0f, 1.0f);    // #00E5FF
	const FLinearColor EmeraldColor(0.0f, 0.90f, 0.46f, 1.0f); // #00E676
	const FLinearColor MutedColor(0.55f, 0.65f, 0.75f, 0.8f);

	auto MakePresetCard = [this, PresetBtnFont, PresetSubFont, CyanColor, AmberColor](int32 Idx, const FString& Title, const FString& Subtitle) -> TSharedRef<SWidget>
	{
		return SNew(SBox)
			.HeightOverride(52.0f)
			.Padding(FMargin(0.0f, 3.0f))
			[
				SNew(SButton)
				.OnClicked(FOnClicked::CreateUObject(this, &UFPSEscapeMenuWidget::OnPresetClicked, Idx))
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Left)
					.Padding(8.0f, 2.0f, 0.0f, 1.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(Title))
						.Font(PresetBtnFont)
						.ColorAndOpacity(AmberColor)
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Left)
					.Padding(8.0f, 0.0f, 0.0f, 2.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(Subtitle))
						.Font(PresetSubFont)
						.ColorAndOpacity(CyanColor)
					]
				]
			];
	};

	auto MakeToggleRow = [this, ToggleFont, AmberColor](TSharedPtr<STextBlock>& OutBlock, const FString& DefaultText, FOnClicked OnClicked) -> TSharedRef<SWidget>
	{
		return SNew(SBox)
			.HeightOverride(38.0f)
			.Padding(FMargin(0.0f, 2.0f))
			[
				SNew(SButton)
				.OnClicked(OnClicked)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SAssignNew(OutBlock, STextBlock)
					.Text(FText::FromString(DefaultText))
					.Font(ToggleFont)
					.ColorAndOpacity(AmberColor)
				]
			];
	};

	TSharedRef<SWidget> Widget = SNew(SBox)
		.WidthOverride(560.0f)
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor(0.035f, 0.06f, 0.095f, 0.96f))
			.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
			.Padding(FMargin(24.0f, 20.0f))
			[
				SNew(SVerticalBox)
				// Header
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.Padding(0.0f, 0.0f, 0.0f, 2.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("NOBLE FERMI // GRAPHICS & ENGINE TUNING")))
					.Font(HeaderFont)
					.ColorAndOpacity(AmberColor)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.Padding(0.0f, 0.0f, 0.0f, 10.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("REAL-TIME HARDWARE SCALING // ZERO RESTART REQUIRED")))
					.Font(SubTitleFont)
					.ColorAndOpacity(CyanColor)
				]
				// Active Preset Telemetry Banner
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 10.0f)
				[
					SNew(SBorder)
					.BorderBackgroundColor(FLinearColor(0.06f, 0.10f, 0.16f, 0.90f))
					.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
					.Padding(FMargin(10.0f, 8.0f))
					.HAlign(HAlign_Center)
					[
						SAssignNew(ActivePresetBadge, STextBlock)
						.Text(FText::FromString(TEXT("ACTIVE PRESET: [ LAPTOP / ESPORTS (60+ FPS) ]")))
						.Font(PresetBtnFont)
						.ColorAndOpacity(CyanColor)
					]
				]
				// 4 Main Presets
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					MakePresetCard(0, TEXT("[ 1 ] LAPTOP / ESPORTS (MAX FPS)"), TEXT("Lumen OFF • Virtual Shadows OFF • 70% TSR • 60-144 FPS on Laptops"))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					MakePresetCard(1, TEXT("[ 2 ] BALANCED (MEDIUM QUALITY)"), TEXT("Lumen OFF • Standard CSM Shadows • 85% TSR • Smooth 60 FPS"))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					MakePresetCard(2, TEXT("[ 3 ] HIGH (LUMEN SOFT GI)"), TEXT("Lumen Raytraced GI ON • Standard Shadows • 100% Native Scale"))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					MakePresetCard(3, TEXT("[ 4 ] ULTRA / BEAUTIFUL (MAX CINEMATIC)"), TEXT("Lumen Raytraced GI & Reflections ON • Virtual Shadow Maps ON • 100% Native"))
				]
				// Separator
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 8.0f)
				[
					SNew(SSeparator)
					.ColorAndOpacity(FLinearColor(0.15f, 0.25f, 0.35f, 0.8f))
				]
				// Granular Tuning Controls Header
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Left)
				.Padding(0.0f, 0.0f, 0.0f, 4.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("/// GRANULAR HARDWARE TOGGLES:")))
					.Font(SubTitleFont)
					.ColorAndOpacity(CyanColor)
				]
				// 2x2 Grid for Granular Toggles
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.FillWidth(0.5f)
					.Padding(0.0f, 0.0f, 4.0f, 0.0f)
					[
						MakeToggleRow(LumenToggleLabel, TEXT("LUMEN GI: [ OFF ]"), FOnClicked::CreateUObject(this, &UFPSEscapeMenuWidget::OnToggleLumenClicked))
					]
					+ SHorizontalBox::Slot()
					.FillWidth(0.5f)
					.Padding(4.0f, 0.0f, 0.0f, 0.0f)
					[
						MakeToggleRow(VSMToggleLabel, TEXT("VIRTUAL SHADOWS: [ OFF ]"), FOnClicked::CreateUObject(this, &UFPSEscapeMenuWidget::OnToggleVSMClicked))
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.FillWidth(0.5f)
					.Padding(0.0f, 0.0f, 4.0f, 0.0f)
					[
						MakeToggleRow(ResolutionScaleLabel, TEXT("RENDER SCALE: [ 75% ]"), FOnClicked::CreateUObject(this, &UFPSEscapeMenuWidget::OnCycleResolutionClicked))
					]
					+ SHorizontalBox::Slot()
					.FillWidth(0.5f)
					.Padding(4.0f, 0.0f, 0.0f, 0.0f)
					[
						MakeToggleRow(FrameRateLimitLabel, TEXT("FRAME RATE CAP: [ 144 ]"), FOnClicked::CreateUObject(this, &UFPSEscapeMenuWidget::OnCycleFpsLimitClicked))
					]
				]
				// Navigation Bottom Row
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 12.0f, 0.0f, 0.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.FillWidth(0.5f)
					.Padding(0.0f, 0.0f, 4.0f, 0.0f)
					[
						SNew(SBox)
						.HeightOverride(40.0f)
						[
							SNew(SButton)
							.OnClicked(FOnClicked::CreateUObject(this, &UFPSEscapeMenuWidget::OnBackToCommandClicked))
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("< COMMAND MENU")))
								.Font(PresetBtnFont)
								.ColorAndOpacity(CyanColor)
							]
						]
					]
					+ SHorizontalBox::Slot()
					.FillWidth(0.5f)
					.Padding(4.0f, 0.0f, 0.0f, 0.0f)
					[
						SNew(SBox)
						.HeightOverride(40.0f)
						[
							SNew(SButton)
							.OnClicked(FOnClicked::CreateUObject(this, &UFPSEscapeMenuWidget::OnResumeClicked))
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("RESUME [ESC]")))
								.Font(PresetBtnFont)
								.ColorAndOpacity(EmeraldColor)
							]
						]
					]
				]
			]
		];

	UpdateTelemetryLabels();
	return Widget;
}

void UFPSEscapeMenuWidget::UpdateTelemetryLabels()
{
	if (ActivePresetBadge.IsValid())
	{
		FString BadgeText;
		FLinearColor BadgeColor;
		switch (CurrentPresetIndex)
		{
		case 0:
			BadgeText = TEXT("ACTIVE PRESET: [ LAPTOP / ESPORTS // MAX FPS (60-144) ]");
			BadgeColor = FLinearColor(0.0f, 0.90f, 1.0f, 1.0f); // Cyan
			break;
		case 1:
			BadgeText = TEXT("ACTIVE PRESET: [ BALANCED // MEDIUM QUALITY (SMOOTH 60) ]");
			BadgeColor = FLinearColor(0.2f, 0.9f, 0.5f, 1.0f); // Emerald
			break;
		case 2:
			BadgeText = TEXT("ACTIVE PRESET: [ HIGH // LUMEN RAYTRACING ACTIVE ]");
			BadgeColor = FLinearColor(1.0f, 0.70f, 0.0f, 1.0f); // Amber
			break;
		case 3:
		default:
			BadgeText = TEXT("ACTIVE PRESET: [ ULTRA // LUMEN & VIRTUAL SHADOWS (MAX CINEMATIC) ]");
			BadgeColor = FLinearColor(1.0f, 0.20f, 0.35f, 1.0f); // Crimson
			break;
		}
		ActivePresetBadge->SetText(FText::FromString(BadgeText));
		ActivePresetBadge->SetColorAndOpacity(BadgeColor);
	}

	if (LumenToggleLabel.IsValid())
	{
		LumenToggleLabel->SetText(FText::FromString(bLumenActive ? TEXT("LUMEN GI: [ ON // RAYTRACED ]") : TEXT("LUMEN GI: [ OFF // HIGH FPS ]")));
		LumenToggleLabel->SetColorAndOpacity(bLumenActive ? FLinearColor(1.0f, 0.70f, 0.0f, 1.0f) : FLinearColor(0.55f, 0.65f, 0.75f, 1.0f));
	}

	if (VSMToggleLabel.IsValid())
	{
		VSMToggleLabel->SetText(FText::FromString(bVSMActive ? TEXT("VIRTUAL SHADOWS: [ ON // CINEMATIC ]") : TEXT("VIRTUAL SHADOWS: [ OFF // FAST CSM ]")));
		VSMToggleLabel->SetColorAndOpacity(bVSMActive ? FLinearColor(1.0f, 0.70f, 0.0f, 1.0f) : FLinearColor(0.55f, 0.65f, 0.75f, 1.0f));
	}

	if (ResolutionScaleLabel.IsValid())
	{
		ResolutionScaleLabel->SetText(FText::FromString(FString::Printf(TEXT("RENDER SCALE: [ %.0f%% ]"), CurrentScreenPercentage)));
	}

	if (FrameRateLimitLabel.IsValid())
	{
		const FString FpsStr = (CurrentFpsLimit > 0) ? FString::Printf(TEXT("%d FPS"), CurrentFpsLimit) : TEXT("UNLIMITED");
		FrameRateLimitLabel->SetText(FText::FromString(FString::Printf(TEXT("FRAME RATE CAP: [ %s ]"), *FpsStr)));
	}
}

void UFPSEscapeMenuWidget::ApplyGraphicsPreset(int32 PresetIndex)
{
	UWorld* World = GetWorld();
	if (!World) return;

	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();

	switch (PresetIndex)
	{
	case 0: // LAPTOP / ESPORTS (MAX FPS)
		CurrentPresetIndex = 0;
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
		bLumenActive = false;
		bVSMActive = false;
		CurrentScreenPercentage = 70.0f;
		CurrentFpsLimit = 60;
		break;

	case 1: // BALANCED (MEDIUM)
		CurrentPresetIndex = 1;
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
		bLumenActive = false;
		bVSMActive = false;
		CurrentScreenPercentage = 85.0f;
		CurrentFpsLimit = 120;
		break;

	case 2: // HIGH (LUMEN ON, VSM OFF)
		CurrentPresetIndex = 2;
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
		bLumenActive = true;
		bVSMActive = false;
		CurrentScreenPercentage = 100.0f;
		CurrentFpsLimit = 144;
		break;

	case 3: // ULTRA / BEAUTIFUL (LUMEN + VIRTUAL SHADOWS)
	default:
		CurrentPresetIndex = 3;
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
		bLumenActive = true;
		bVSMActive = true;
		CurrentScreenPercentage = 100.0f;
		CurrentFpsLimit = 0;
		break;
	}

	// Persist to Engine.ini so it reloads on boot
	if (GConfig)
	{
		GConfig->SetInt(TEXT("SystemSettings"), TEXT("r.DynamicGlobalIlluminationMethod"), bLumenActive ? 1 : 0, GEngineIni);
		GConfig->SetInt(TEXT("SystemSettings"), TEXT("r.Shadow.Virtual.Enable"), bVSMActive ? 1 : 0, GEngineIni);
		GConfig->SetInt(TEXT("SystemSettings"), TEXT("r.Lumen.Reflections.Allow"), bLumenActive ? 1 : 0, GEngineIni);
		GConfig->SetInt(TEXT("SystemSettings"), TEXT("r.VolumetricFog"), (CurrentPresetIndex >= 2) ? 1 : 0, GEngineIni);
		GConfig->SetFloat(TEXT("SystemSettings"), TEXT("r.ScreenPercentage"), CurrentScreenPercentage, GEngineIni);
		GConfig->Flush(false, GEngineIni);
	}

	// Trigger HUD accolade feedback if character is active
	if (OwningFPSCharacter.IsValid() && OwningFPSCharacter->GetActiveHUDWidget())
	{
		const FString PresetName = (CurrentPresetIndex == 0) ? TEXT("LAPTOP / ESPORTS (60+ FPS)") :
		                           (CurrentPresetIndex == 1) ? TEXT("BALANCED GRAPHICS") :
		                           (CurrentPresetIndex == 2) ? TEXT("HIGH (LUMEN ACTIVE)") :
		                           TEXT("ULTRA // LUMEN + VIRTUAL SHADOWS");
		OwningFPSCharacter->GetActiveHUDWidget()->TriggerAccolade(FString::Printf(TEXT("GRAPHICS: %s"), *PresetName), 0, FLinearColor(0.0f, 0.90f, 1.0f));
	}

	UpdateTelemetryLabels();
}

void UFPSEscapeMenuWidget::ToggleLumen()
{
	UWorld* World = GetWorld();
	if (!World) return;

	bLumenActive = !bLumenActive;
	UKismetSystemLibrary::ExecuteConsoleCommand(World, bLumenActive ? TEXT("r.DynamicGlobalIlluminationMethod 1") : TEXT("r.DynamicGlobalIlluminationMethod 0"));
	UKismetSystemLibrary::ExecuteConsoleCommand(World, bLumenActive ? TEXT("r.Lumen.Reflections.Allow 1") : TEXT("r.Lumen.Reflections.Allow 0"));

	if (GConfig)
	{
		GConfig->SetInt(TEXT("SystemSettings"), TEXT("r.DynamicGlobalIlluminationMethod"), bLumenActive ? 1 : 0, GEngineIni);
		GConfig->SetInt(TEXT("SystemSettings"), TEXT("r.Lumen.Reflections.Allow"), bLumenActive ? 1 : 0, GEngineIni);
		GConfig->Flush(false, GEngineIni);
	}

	if (bLumenActive && bVSMActive) CurrentPresetIndex = 3;
	else if (bLumenActive) CurrentPresetIndex = 2;
	else CurrentPresetIndex = 0;

	UpdateTelemetryLabels();
}

void UFPSEscapeMenuWidget::ToggleVSM()
{
	UWorld* World = GetWorld();
	if (!World) return;

	bVSMActive = !bVSMActive;
	UKismetSystemLibrary::ExecuteConsoleCommand(World, bVSMActive ? TEXT("r.Shadow.Virtual.Enable 1") : TEXT("r.Shadow.Virtual.Enable 0"));

	if (GConfig)
	{
		GConfig->SetInt(TEXT("SystemSettings"), TEXT("r.Shadow.Virtual.Enable"), bVSMActive ? 1 : 0, GEngineIni);
		GConfig->Flush(false, GEngineIni);
	}

	if (bLumenActive && bVSMActive) CurrentPresetIndex = 3;
	else if (bLumenActive) CurrentPresetIndex = 2;
	else CurrentPresetIndex = 0;

	UpdateTelemetryLabels();
}

void UFPSEscapeMenuWidget::CycleResolutionScale()
{
	UWorld* World = GetWorld();
	if (!World) return;

	if (CurrentScreenPercentage <= 72.0f) CurrentScreenPercentage = 85.0f;
	else if (CurrentScreenPercentage <= 88.0f) CurrentScreenPercentage = 100.0f;
	else CurrentScreenPercentage = 70.0f;

	UKismetSystemLibrary::ExecuteConsoleCommand(World, FString::Printf(TEXT("r.ScreenPercentage %.0f"), CurrentScreenPercentage));

	if (GConfig)
	{
		GConfig->SetFloat(TEXT("SystemSettings"), TEXT("r.ScreenPercentage"), CurrentScreenPercentage, GEngineIni);
		GConfig->Flush(false, GEngineIni);
	}

	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	if (Settings)
	{
		Settings->SetResolutionScaleValueEx(CurrentScreenPercentage);
		Settings->ApplySettings(false);
		Settings->SaveSettings();
	}

	UpdateTelemetryLabels();
}

void UFPSEscapeMenuWidget::CycleFrameRateLimit()
{
	if (CurrentFpsLimit == 60) CurrentFpsLimit = 120;
	else if (CurrentFpsLimit == 120) CurrentFpsLimit = 144;
	else if (CurrentFpsLimit == 144) CurrentFpsLimit = 0;
	else CurrentFpsLimit = 60;

	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	if (Settings)
	{
		Settings->SetFrameRateLimit(static_cast<float>(CurrentFpsLimit));
		Settings->ApplySettings(false);
		Settings->SaveSettings();
	}

	UpdateTelemetryLabels();
}

FReply UFPSEscapeMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape || InKeyEvent.GetKey() == EKeys::P)
	{
		if (MenuSwitcher.IsValid() && MenuSwitcher->GetActiveWidgetIndex() == 1)
		{
			// Go back to main command view
			MenuSwitcher->SetActiveWidgetIndex(0);
			return FReply::Handled();
		}
		ResumeOperation();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UFPSEscapeMenuWidget::ResumeOperation()
{
	if (OwningFPSCharacter.IsValid())
	{
		OwningFPSCharacter->ToggleEscapeMenu();
	}
}

void UFPSEscapeMenuWidget::OpenSettings()
{
	if (MenuSwitcher.IsValid())
	{
		MenuSwitcher->SetActiveWidgetIndex(1);
	}
}

void UFPSEscapeMenuWidget::ReturnToMainMenu()
{
	UGameplayStatics::OpenLevel(this, FName(TEXT("/Game/Maps/MainMenuMap")));
}

void UFPSEscapeMenuWidget::ExitToDesktop()
{
	UKismetSystemLibrary::QuitGame(GetWorld(), nullptr, EQuitPreference::Quit, false);
}

FReply UFPSEscapeMenuWidget::OnResumeClicked()
{
	ResumeOperation();
	return FReply::Handled();
}

FReply UFPSEscapeMenuWidget::OnSettingsClicked()
{
	OpenSettings();
	return FReply::Handled();
}

FReply UFPSEscapeMenuWidget::OnMainMenuClicked()
{
	ReturnToMainMenu();
	return FReply::Handled();
}

FReply UFPSEscapeMenuWidget::OnExitClicked()
{
	ExitToDesktop();
	return FReply::Handled();
}

FReply UFPSEscapeMenuWidget::OnPresetClicked(int32 PresetIdx)
{
	ApplyGraphicsPreset(PresetIdx);
	return FReply::Handled();
}

FReply UFPSEscapeMenuWidget::OnToggleLumenClicked()
{
	ToggleLumen();
	return FReply::Handled();
}

FReply UFPSEscapeMenuWidget::OnToggleVSMClicked()
{
	ToggleVSM();
	return FReply::Handled();
}

FReply UFPSEscapeMenuWidget::OnCycleResolutionClicked()
{
	CycleResolutionScale();
	return FReply::Handled();
}

FReply UFPSEscapeMenuWidget::OnCycleFpsLimitClicked()
{
	CycleFrameRateLimit();
	return FReply::Handled();
}

FReply UFPSEscapeMenuWidget::OnBackToCommandClicked()
{
	if (MenuSwitcher.IsValid())
	{
		MenuSwitcher->SetActiveWidgetIndex(0);
	}
	return FReply::Handled();
}
