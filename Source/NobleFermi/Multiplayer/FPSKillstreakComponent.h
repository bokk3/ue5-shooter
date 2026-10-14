#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FPSKillstreakComponent.generated.h"

UENUM(BlueprintType)
enum class EFPSKillstreakType : uint8
{
	None,
	UAV,              // 3 Kills - Radar sweep
	CarePackage,      // 4 Kills - Airdrop crate
	PredatorMissile,  // 5 Kills - Steerable laser-guided missile
	HarrierStrike,    // 7 Kills - Air strike & hovering VTOL
	ChopperGunner,    // 11 Kills - Aerial thermal minigun
	TacticalNuke      // 25 Kills - Ends the match in immediate victory
};

USTRUCT(BlueprintType)
struct FFPSKillstreakDef
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Killstreak")
	EFPSKillstreakType Type = EFPSKillstreakType::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Killstreak")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Killstreak")
	int32 RequiredKills = 3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Killstreak")
	FString AnnouncerCallout;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnKillstreakEarnedSignature, EFPSKillstreakType, StreakType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnKillstreakActivatedSignature, EFPSKillstreakType, StreakType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNukeCountdownStartedSignature, float, SecondsRemaining);

/**
 * Classic MW2 Style Killstreak System Component
 * Tracks kill counts without dying, unlocks assigned streaks, and executes streak deployment.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class NOBLEFERMI_API UFPSKillstreakComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFPSKillstreakComponent();

	UPROPERTY(BlueprintAssignable, Category = "FPS Killstreaks|Events")
	FOnKillstreakEarnedSignature OnKillstreakEarned;

	UPROPERTY(BlueprintAssignable, Category = "FPS Killstreaks|Events")
	FOnKillstreakActivatedSignature OnKillstreakActivated;

	UPROPERTY(BlueprintAssignable, Category = "FPS Killstreaks|Events")
	FOnNukeCountdownStartedSignature OnNukeCountdownStarted;

	// Equipped Streak Slots (Typical MW2: 3 selected streaks)
	UPROPERTY(EditDefaultsOnly, Category = "FPS Killstreaks|Loadout")
	TArray<FFPSKillstreakDef> EquippedStreaks;

	UFUNCTION(BlueprintCallable, Category = "FPS Killstreaks")
	void NotifyKillRecorded(int32 CurrentStreak);

	UFUNCTION(BlueprintCallable, Category = "FPS Killstreaks")
	void NotifyPlayerDied();

	UFUNCTION(BlueprintCallable, Category = "FPS Killstreaks")
	bool ActivateStreak(EFPSKillstreakType StreakType);

	UFUNCTION(BlueprintPure, Category = "FPS Killstreaks")
	TArray<EFPSKillstreakType> GetAvailableStreaks() const { return AvailableStreaks; }

	UFUNCTION(BlueprintPure, Category = "FPS Killstreaks")
	bool HasStreakReady(EFPSKillstreakType StreakType) const;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TArray<EFPSKillstreakType> AvailableStreaks;

	void ExecuteUAV();
	void ExecutePredatorMissile();
	void ExecuteTacticalNuke();
};
