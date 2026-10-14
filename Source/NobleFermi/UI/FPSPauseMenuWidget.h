#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "FPSPauseMenuWidget.generated.h"

class USettingsMenuWidget;

/**
 * AAA FPS In-Game Pause Menu Widget
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class NOBLEFERMI_API UFPSPauseMenuWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UFPSPauseMenuWidget();

	UPROPERTY(EditDefaultsOnly, Category = "Pause Menu|Submenus")
	TSubclassOf<USettingsMenuWidget> SettingsWidgetClass;

	UFUNCTION(BlueprintCallable, Category = "Pause Menu")
	void ResumeGame();

	UFUNCTION(BlueprintCallable, Category = "Pause Menu")
	void OpenSettings();

	UFUNCTION(BlueprintCallable, Category = "Pause Menu")
	void LeaveMatchToMainMenu();

protected:
	virtual void NativeConstruct() override;
};
