#include "UI/SettingsMenuWidget.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/GameplayStatics.h"

USettingsMenuWidget::USettingsMenuWidget()
{
	bIsBackHandler = true;
}

void USettingsMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	LoadCurrentSettings();
}

void USettingsMenuWidget::LoadCurrentSettings()
{
	UGameUserSettings* UserSettings = UGameUserSettings::GetGameUserSettings();
	if (!UserSettings) return;

	CurrentSettings.Resolution = UserSettings->GetScreenResolution();
	CurrentSettings.WindowMode = UserSettings->GetFullscreenMode();
	CurrentSettings.bVSync = UserSettings->IsVSyncEnabled();
	CurrentSettings.FrameRateLimit = static_cast<int32>(UserSettings->GetFrameRateLimit());
	CurrentSettings.OverallQualityLevel = UserSettings->GetOverallScalabilityLevel();
}

void USettingsMenuWidget::ApplySettings()
{
	UGameUserSettings* UserSettings = UGameUserSettings::GetGameUserSettings();
	if (!UserSettings) return;

	UserSettings->SetScreenResolution(CurrentSettings.Resolution);
	UserSettings->SetFullscreenMode(CurrentSettings.WindowMode);
	UserSettings->SetVSyncEnabled(CurrentSettings.bVSync);
	UserSettings->SetFrameRateLimit(static_cast<float>(CurrentSettings.FrameRateLimit));
	
	if (CurrentSettings.OverallQualityLevel >= 0)
	{
		UserSettings->SetOverallScalabilityLevel(CurrentSettings.OverallQualityLevel);
	}

	UserSettings->ApplySettings(false);
	UserSettings->SaveSettings();
}

void USettingsMenuWidget::ResetToDefaults()
{
	CurrentSettings = FFPSSettingsData();
	ApplySettings();
}

void USettingsMenuWidget::SwitchTab(EFPSSettingsTab NewTab)
{
	CurrentTab = NewTab;
	OnTabChanged(NewTab);
}

TOptional<FNavigationReply> USettingsMenuWidget::NativeOnNavigation(const FGeometry& InGeometry, const FNavigationEvent& InNavigationEvent)
{
	return Super::NativeOnNavigation(InGeometry, InNavigationEvent);
}
