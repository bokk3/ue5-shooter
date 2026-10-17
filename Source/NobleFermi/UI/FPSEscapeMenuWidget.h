#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FPSEscapeMenuWidget.generated.h"

class AFPSCharacter;
class SWidget;

/**
 * Tactical Escape / In-Game Command Menu Widget
 * Non-pausing live UI overlay adhering to the Noble Fermi Cyan/Amber military visor palette.
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

private:
	UPROPERTY()
	TWeakObjectPtr<AFPSCharacter> OwningFPSCharacter;

	FReply OnResumeClicked();
	FReply OnSettingsClicked();
	FReply OnMainMenuClicked();
	FReply OnExitClicked();
};
