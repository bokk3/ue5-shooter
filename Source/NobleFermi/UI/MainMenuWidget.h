#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "MainMenuWidget.generated.h"

class USettingsMenuWidget;
class USoundBase;
class SWidgetSwitcher;
class SEditableTextBox;
class STextBlock;

UENUM(BlueprintType)
enum class EFPSMenuSection : uint8
{
	Home UMETA(DisplayName = "Home"),
	Play UMETA(DisplayName = "Play & Matchmaking"),
	Loadout UMETA(DisplayName = "Weapons & Gunsmith"),
	ServerBrowser UMETA(DisplayName = "Server Browser & Host"),
	Settings UMETA(DisplayName = "Settings"),
	Career UMETA(DisplayName = "Career & Stats")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMenuSectionChangedSignature, EFPSMenuSection, NewSection);

/**
 * AAA FPS Primary Main Menu Widget
 * Slate-implemented, full-screen interactive military HUD start menu.
 */
UCLASS(BlueprintType, Blueprintable)
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
	void HostListenRoom();

	UFUNCTION(BlueprintCallable, Category = "FPS Menu")
	void ConnectToDedicatedServer(const FString& ServerAddress);

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
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	EFPSMenuSection CurrentSection = EFPSMenuSection::Home;

	TSharedPtr<SWidgetSwitcher> ContentSwitcher;
	TSharedPtr<SEditableTextBox> DirectConnectInput;
	TSharedPtr<STextBlock> SelectedLoadoutDetails;
	TSharedPtr<STextBlock> NetworkStatusText;

	TSharedRef<SWidget> BuildHeader();
	TSharedRef<SWidget> BuildLeftNavBar();
	TSharedRef<SWidget> BuildHomeView();
	TSharedRef<SWidget> BuildGunsmithView();
	TSharedRef<SWidget> BuildServerBrowserView();
	TSharedRef<SWidget> BuildSettingsView();
	TSharedRef<SWidget> BuildFooter();

	FReply HandleTabClicked(EFPSMenuSection Section);
	FReply HandleQuickplayClicked();
	FReply HandleHostListenClicked();
	FReply HandleDirectConnectClicked();
	FReply HandleQuitClicked();
	FReply HandleSelectArchetype(int32 ArchetypeIndex);
};
