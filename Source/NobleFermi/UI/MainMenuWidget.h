#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "MainMenuWidget.generated.h"

class USettingsMenuWidget;
class USoundBase;

UENUM(BlueprintType)
enum class EFPSMenuSection : uint8
{
	Home UMETA(DisplayName = "Home"),
	Play UMETA(DisplayName = "Play & Matchmaking"),
	Loadout UMETA(DisplayName = "Weapons & Loadout"),
	Settings UMETA(DisplayName = "Settings"),
	Career UMETA(DisplayName = "Career & Stats")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMenuSectionChangedSignature, EFPSMenuSection, NewSection);

/**
 * AAA FPS Primary Main Menu Widget
 * Acts as the central hub for navigation, screen stacking, and 3D background camera blending.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class NOBLEFERMI_API UMainMenuWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UMainMenuWidget();

	UPROPERTY(BlueprintAssignable, Category = "FPS Menu")
	FOnMenuSectionChangedSignature OnSectionChanged;

	UPROPERTY(EditDefaultsOnly, Category = "FPS Menu|Submenus")
	TSubclassOf<USettingsMenuWidget> SettingsWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Menu|Audio")
	TObjectPtr<USoundBase> MenuMusicTrack;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Menu|Audio")
	TObjectPtr<USoundBase> TransitionSound;

	UFUNCTION(BlueprintCallable, Category = "FPS Menu")
	void NavigateToSection(EFPSMenuSection Section);

	UFUNCTION(BlueprintCallable, Category = "FPS Menu")
	void StartMatchmaking();

	UFUNCTION(BlueprintCallable, Category = "FPS Menu")
	void OpenSettings();

	UFUNCTION(BlueprintCallable, Category = "FPS Menu")
	void RequestQuit();

	UFUNCTION(BlueprintImplementableEvent, Category = "FPS Menu")
	void OnMatchmakingTriggered();

	UFUNCTION(BlueprintImplementableEvent, Category = "FPS Menu")
	void OnShowQuitConfirmation();

protected:
	virtual void NativeConstruct() override;
	virtual TOptional<FNavigationReply> NativeOnNavigation(const FGeometry& InGeometry, const FNavigationEvent& InNavigationEvent) override;

private:
	UPROPERTY()
	EFPSMenuSection CurrentSection = EFPSMenuSection::Home;
};
