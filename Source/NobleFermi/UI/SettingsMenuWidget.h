#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "GameFramework/GameUserSettings.h"
#include "SettingsMenuWidget.generated.h"

UENUM(BlueprintType)
enum class EFPSSettingsTab : uint8
{
	Video UMETA(DisplayName = "Video & Display"),
	Audio UMETA(DisplayName = "Audio"),
	Controls UMETA(DisplayName = "Gameplay & Controls"),
	Accessibility UMETA(DisplayName = "Accessibility")
};

USTRUCT(BlueprintType)
struct FFPSSettingsData
{
	GENERATED_BODY()

	// Video Settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video")
	FIntPoint Resolution = FIntPoint(1920, 1080);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video")
	TEnumAsByte<EWindowMode::Type> WindowMode = EWindowMode::Fullscreen;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video")
	bool bVSync = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video")
	int32 FrameRateLimit = 144;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video")
	int32 OverallQualityLevel = 3; // 0=Low, 1=Med, 2=High, 3=Epic, 4=Cinematic

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video")
	bool bMotionBlur = false;

	// Audio Settings (0.0 to 1.0)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	float MasterVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	float MusicVolume = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	float SFXVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	float DialogueVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	float UIVolume = 0.8f;

	// Gameplay / FPS Controls
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Controls")
	float FieldOfView = 103.0f; // Standard competitive FPS default

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Controls")
	float MouseSensitivity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Controls")
	float ADSSensitivityMultiplier = 0.85f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Controls")
	bool bInvertYAxis = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Controls")
	FLinearColor CrosshairColor = FLinearColor(0.0f, 1.0f, 0.6f, 1.0f); // High-vis tactical cyan/green
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSettingsClosedSignature);

/**
 * AAA FPS Settings Menu Widget
 * Manages game user settings, video scalability, sound class volume modulation, and FPS controls.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class NOBLEFERMI_API USettingsMenuWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	USettingsMenuWidget();

	UPROPERTY(BlueprintAssignable, Category = "FPS Settings")
	FOnSettingsClosedSignature OnSettingsClosed;

	UPROPERTY(BlueprintReadWrite, Category = "FPS Settings")
	FFPSSettingsData CurrentSettings;

	UFUNCTION(BlueprintCallable, Category = "FPS Settings")
	void LoadCurrentSettings();

	UFUNCTION(BlueprintCallable, Category = "FPS Settings")
	void ApplySettings();

	UFUNCTION(BlueprintCallable, Category = "FPS Settings")
	void ResetToDefaults();

	UFUNCTION(BlueprintCallable, Category = "FPS Settings")
	void SwitchTab(EFPSSettingsTab NewTab);

	UFUNCTION(BlueprintImplementableEvent, Category = "FPS Settings")
	void OnTabChanged(EFPSSettingsTab ActiveTab);

protected:
	virtual void NativeConstruct() override;
	virtual TOptional<FNavigationReply> NativeOnNavigation(const FGeometry& InGeometry, const FNavigationEvent& InNavigationEvent) override;

private:
	UPROPERTY()
	EFPSSettingsTab CurrentTab = EFPSSettingsTab::Video;
};
