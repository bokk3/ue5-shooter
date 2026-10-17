#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "MainMenuWidget.generated.h"

class USettingsMenuWidget;
class USoundBase;
class SWidgetSwitcher;
class SEditableTextBox;
class STextBlock;
class SBorder;
class AFPSCharacterPreviewActor;

UENUM(BlueprintType)
enum class EFPSMenuSection : uint8
{
	Home UMETA(DisplayName = "Home"),
	Play UMETA(DisplayName = "Play & Matchmaking"),
	Customization UMETA(DisplayName = "Operator Customization"),
	Loadout UMETA(DisplayName = "Weapons & Gunsmith"),
	DailyRewards UMETA(DisplayName = "Daily Rewards & Armory Vault"),
	ServerBrowser UMETA(DisplayName = "Server Browser & Host"),
	Settings UMETA(DisplayName = "Settings"),
	Career UMETA(DisplayName = "Career & Stats")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMenuSectionChangedSignature, EFPSMenuSection, NewSection);

/**
 * AAA Modern Warfare-Inspired Tactical Command Lobby Widget
 * Slate-implemented semi-transparent acrylic interface with SBackgroundBlur (14.0f),
 * 3-zone layout (Left Deployment Blade, Center 3D Operator Showcase, Right Live Loadout & Camo Inspector),
 * and high-contrast tactical HUD accents (#00E5FF Tactical Cyan, #FFB300 Afterburner Amber, #00E676 Emerald Ready).
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
	void SetPreviewActor(AFPSCharacterPreviewActor* InPreviewActor);

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
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	EFPSMenuSection CurrentSection = EFPSMenuSection::Play;
	int32 SelectedModeIndex = 0; // 0 = Rust Tactical CQB, 1 = Fermi Time Lab, 2 = Bot Skirmish

	UPROPERTY()
	TObjectPtr<AFPSCharacterPreviewActor> PreviewActor;

	TSharedPtr<SWidgetSwitcher> ContentSwitcher;
	TSharedPtr<SEditableTextBox> DirectConnectInput;
	TSharedPtr<STextBlock> SelectedLoadoutDetails;
	TSharedPtr<STextBlock> NetworkStatusText;

	// Customization Readout Pointers
	TSharedPtr<STextBlock> CamoStatusText;
	TSharedPtr<STextBlock> HeadgearStatusText;
	TSharedPtr<STextBlock> VestStatusText;
	TSharedPtr<STextBlock> WeaponFinishStatusText;
	TSharedPtr<STextBlock> StanceStatusText;
	TSharedPtr<STextBlock> RotationSpeedText;
	TSharedPtr<STextBlock> CenterPillText;
	TSharedPtr<STextBlock> BountyStatusText;

	// Mode Card Border Pointers for interactive glowing borders
	TSharedPtr<SBorder> ModeCardBorder0;
	TSharedPtr<SBorder> ModeCardBorder1;
	TSharedPtr<SBorder> ModeCardBorder2;

	// Top Tab Indicator Borders
	TSharedPtr<SBorder> TabIndicatorPlay;
	TSharedPtr<SBorder> TabIndicatorOperator;
	TSharedPtr<SBorder> TabIndicatorGunsmith;
	TSharedPtr<SBorder> TabIndicatorDailyRewards;
	TSharedPtr<SBorder> TabIndicatorSettings;

	// Top Header Persistent Balance Badges
	TSharedPtr<STextBlock> GoldBalanceText;
	TSharedPtr<STextBlock> OpalBalanceText;

	// Minigame Switcher & Telemetry Pointers
	TSharedPtr<SWidgetSwitcher> MinigameSwitcher;
	TSharedPtr<STextBlock> WheelRewardText;
	TSharedPtr<STextBlock> DartRewardText;
	TSharedPtr<STextBlock> DartTokensText;
	TSharedPtr<STextBlock> StreakStatusText;
	TSharedPtr<STextBlock> StreakDaysDisplay;

	// Fortune Wheel Physics State
	float WheelCurrentAngle = 0.0f;
	float WheelAngularVelocity = 0.0f;
	bool bIsWheelSpinning = false;
	int32 WheelTargetSlice = 0;
	float WheelSpinDuration = 0.0f;
	float WheelSpinElapsed = 0.0f;
	float WheelBurstTimer = 0.0f;

	// Tactical Dartboard State
	float DartOscillationTime = 0.0f;
	FVector2D DartReticlePos = FVector2D(0.5f, 0.5f);
	FVector2D LastDartHitPos = FVector2D(0.5f, 0.5f);
	bool bHasLastDartHit = false;
	float DartBurstTimer = 0.0f;

	// 3-Zone Modern Warfare Layout Builders
	TSharedRef<SWidget> BuildHeader();
	TSharedRef<SWidget> BuildTacticalLobbyView();
	TSharedRef<SWidget> BuildLeftBlade();
	TSharedRef<SWidget> BuildCenterViewportHUD();
	TSharedRef<SWidget> BuildRightBlade();
	TSharedRef<SWidget> BuildBountyBoardPanel();

	// Full-screen Subviews
	TSharedRef<SWidget> BuildGunsmithView();
	TSharedRef<SWidget> BuildDailyRewardsView();
	TSharedRef<SWidget> BuildServerBrowserView();
	TSharedRef<SWidget> BuildSettingsView();
	TSharedRef<SWidget> BuildFooter();

	// Interaction handlers
	FReply HandleTabClicked(EFPSMenuSection Section);
	FReply HandleModeCardClicked(int32 ModeIndex);
	FReply HandleQuickplayClicked();
	FReply HandleHostListenClicked();
	FReply HandleDirectConnectClicked();
	FReply HandleQuitClicked();
	FReply HandleSelectArchetype(int32 ArchetypeIndex);

	// Customization handlers
	FReply HandleCamoSelected(uint8 CamoIdx);
	FReply HandleHeadgearSelected(uint8 HeadgearIdx);
	FReply HandleVestSelected(uint8 VestIdx);
	FReply HandleWeaponFinishSelected(uint8 FinishIdx);
	FReply HandleToggleAimingStance();
	FReply HandleToggleTurntableRotation();
	FReply HandleSetRotationSpeed(float Speed);
	FReply HandleResetTurntableFacing();

	// Economy, Bounties & Minigame handlers
	FReply HandleSpinWheelClicked();
	FReply HandleThrowDartClicked();
	FReply HandleClaimDailyStreakClicked();
	FReply HandleSimulateNextDayClicked();
	FReply HandleSwitchMinigame(int32 MinigameIndex);
	FReply HandleClaimOpalDrop(int32 PackIndex);
	FReply HandleUnlockCustomizationItem(const FString& ItemID, bool bUseOpals);
	FReply HandleRerollBountiesClicked();
	FReply HandleSimulateBountyProgressClicked(int32 BountyIdx);

	void UpdateModeCardVisuals();
	void UpdateTabVisuals();
	void UpdateCurrencyHeader();
	void RefreshDailyRewardsUI();
};
