#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FPSEscapeMenuWidget.generated.h"

class AFPSCharacter;
class SWidget;
class SWidgetSwitcher;
class STextBlock;

/**
 * Tactical Escape / In-Game Command & Graphics Settings Menu Widget
 * Non-pausing live UI overlay adhering to the Noble Fermi Cyan/Amber military visor palette.
 * Supports real-time graphics scaling from Laptop (60+ FPS) to Ultra (Cinematic Lumen + VSM).
 */
UCLASS(BlueprintType, Blueprintable)
class NOBLEFERMI_API UFPSEscapeMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFPSEscapeMenuWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativeConstruct() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	void SetOwningCharacter(AFPSCharacter* InCharacter);

	UFUNCTION(BlueprintCallable, Category = "FPS Escape Menu")
	void ResumeOperation();

	UFUNCTION(BlueprintCallable, Category = "FPS Escape Menu")
	void OpenSettings();

	UFUNCTION(BlueprintCallable, Category = "FPS Escape Menu")
	void ReturnToMainMenu();

	UFUNCTION(BlueprintCallable, Category = "FPS Escape Menu")
	void ExitToDesktop();

	// Real-Time Graphics Presets & Tuning
	UFUNCTION(BlueprintCallable, Category = "FPS Graphics")
	void ApplyGraphicsPreset(int32 PresetIndex);

	UFUNCTION(BlueprintCallable, Category = "FPS Graphics")
	void ToggleLumen();

	UFUNCTION(BlueprintCallable, Category = "FPS Graphics")
	void ToggleVSM();

	UFUNCTION(BlueprintCallable, Category = "FPS Graphics")
	void CycleResolutionScale();

	UFUNCTION(BlueprintCallable, Category = "FPS Graphics")
	void CycleFrameRateLimit();

private:
	UPROPERTY()
	TWeakObjectPtr<AFPSCharacter> OwningFPSCharacter;

	TSharedPtr<SWidgetSwitcher> MenuSwitcher;
	TSharedPtr<STextBlock> ActivePresetBadge;
	TSharedPtr<STextBlock> LumenToggleLabel;
	TSharedPtr<STextBlock> VSMToggleLabel;
	TSharedPtr<STextBlock> ResolutionScaleLabel;
	TSharedPtr<STextBlock> FrameRateLimitLabel;
	TSharedPtr<STextBlock> FeedbackBannerText;

	int32 CurrentPresetIndex = 0; // 0=Laptop/Esports, 1=Balanced, 2=High, 3=Ultra
	bool bLumenActive = false;
	bool bVSMActive = false;
	float CurrentScreenPercentage = 75.0f;
	int32 CurrentFpsLimit = 144;

	TSharedRef<SWidget> BuildMainCommandView();
	TSharedRef<SWidget> BuildGraphicsSettingsView();
	void UpdateTelemetryLabels();
	void ReadCurrentHardwareSettings();

	FReply OnResumeClicked();
	FReply OnSettingsClicked();
	FReply OnMainMenuClicked();
	FReply OnExitClicked();
	FReply OnPresetClicked(int32 PresetIdx);
	FReply OnToggleLumenClicked();
	FReply OnToggleVSMClicked();
	FReply OnCycleResolutionClicked();
	FReply OnCycleFpsLimitClicked();
	FReply OnBackToCommandClicked();
};
