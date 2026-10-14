#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "FPSPlayerState.generated.h"

UENUM(BlueprintType)
enum class EFPSPlayerTeam : uint8
{
	None,
	Alpha, // Task Force 141 (Friendly Blue)
	Bravo  // Shadow Company / OpFor (Enemy Red)
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnPlayerStatsUpdatedSignature, int32, Kills, int32, Deaths, int32, Score);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStreakEarnedSignature, int32, CurrentStreak);

/**
 * AAA FPS Multiplayer PlayerState
 * Replicates player combat performance, team assignment, and killstreak progression.
 */
UCLASS()
class NOBLEFERMI_API AFPSPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	AFPSPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(BlueprintAssignable, Category = "FPS Multiplayer|Events")
	FOnPlayerStatsUpdatedSignature OnStatsUpdated;

	UPROPERTY(BlueprintAssignable, Category = "FPS Multiplayer|Events")
	FOnStreakEarnedSignature OnStreakEarned;

	// Stats
	UFUNCTION(BlueprintPure, Category = "FPS Stats")
	int32 GetKills() const { return Kills; }

	UFUNCTION(BlueprintPure, Category = "FPS Stats")
	int32 GetDeaths() const { return Deaths; }

	UFUNCTION(BlueprintPure, Category = "FPS Stats")
	int32 GetAssists() const { return Assists; }

	UFUNCTION(BlueprintPure, Category = "FPS Stats")
	int32 GetScore() const { return Score; }

	UFUNCTION(BlueprintPure, Category = "FPS Stats")
	int32 GetCurrentStreak() const { return CurrentStreak; }

	UFUNCTION(BlueprintPure, Category = "FPS Stats")
	EFPSPlayerTeam GetPlayerTeam() const { return PlayerTeam; }

	// Modifiers (Server Authoritative)
	UFUNCTION(BlueprintCallable, Category = "FPS Stats")
	void AddKill(int32 ScoreReward = 100);

	UFUNCTION(BlueprintCallable, Category = "FPS Stats")
	void AddDeath();

	UFUNCTION(BlueprintCallable, Category = "FPS Stats")
	void AddAssist(int32 ScoreReward = 50);

	UFUNCTION(BlueprintCallable, Category = "FPS Stats")
	void SetPlayerTeam(EFPSPlayerTeam NewTeam);

protected:
	UPROPERTY(ReplicatedUsing = OnRep_StatsChanged, VisibleInstanceOnly, Category = "FPS Stats")
	int32 Kills = 0;

	UPROPERTY(ReplicatedUsing = OnRep_StatsChanged, VisibleInstanceOnly, Category = "FPS Stats")
	int32 Deaths = 0;

	UPROPERTY(ReplicatedUsing = OnRep_StatsChanged, VisibleInstanceOnly, Category = "FPS Stats")
	int32 Assists = 0;

	UPROPERTY(ReplicatedUsing = OnRep_StatsChanged, VisibleInstanceOnly, Category = "FPS Stats")
	int32 Score = 0;

	UPROPERTY(ReplicatedUsing = OnRep_StreakChanged, VisibleInstanceOnly, Category = "FPS Stats")
	int32 CurrentStreak = 0;

	UPROPERTY(Replicated, VisibleInstanceOnly, Category = "FPS Stats")
	EFPSPlayerTeam PlayerTeam = EFPSPlayerTeam::Alpha;

	UFUNCTION()
	void OnRep_StatsChanged();

	UFUNCTION()
	void OnRep_StreakChanged();
};
