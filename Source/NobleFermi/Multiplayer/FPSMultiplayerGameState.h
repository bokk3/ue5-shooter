#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Multiplayer/FPSPlayerState.h"
#include "FPSMultiplayerGameState.generated.h"

UENUM(BlueprintType)
enum class EFPSMatchPhase : uint8
{
	PreMatchCountdown,
	InProgress,
	MatchEndedNuke,
	MatchEndedScoreLimit,
	MatchEndedTimeLimit
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTeamScoresUpdatedSignature, int32, AlphaScore, int32, BravoScore);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMatchEndedSignature, EFPSPlayerTeam, WinningTeam, EFPSMatchPhase, EndReason);

/**
 * Classic MW2 Style Multiplayer GameState
 * Synchronizes team scores, match timer, score limit, and round phase across all connected clients.
 */
UCLASS()
class NOBLEFERMI_API AFPSMultiplayerGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AFPSMultiplayerGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(BlueprintAssignable, Category = "FPS Multiplayer")
	FOnTeamScoresUpdatedSignature OnTeamScoresUpdated;

	UPROPERTY(BlueprintAssignable, Category = "FPS Multiplayer")
	FOnMatchEndedSignature OnMatchEnded;

	// Query Match State
	UFUNCTION(BlueprintPure, Category = "FPS Multiplayer")
	int32 GetAlphaScore() const { return TeamAlphaScore; }

	UFUNCTION(BlueprintPure, Category = "FPS Multiplayer")
	int32 GetBravoScore() const { return TeamBravoScore; }

	UFUNCTION(BlueprintPure, Category = "FPS Multiplayer")
	int32 GetScoreLimit() const { return ScoreLimit; }

	UFUNCTION(BlueprintPure, Category = "FPS Multiplayer")
	float GetMatchTimeRemaining() const { return MatchTimeRemaining; }

	UFUNCTION(BlueprintPure, Category = "FPS Multiplayer")
	EFPSMatchPhase GetMatchPhase() const { return MatchPhase; }

	// Authority functions
	void AddScoreToTeam(EFPSPlayerTeam Team, int32 Points);
	void SetMatchPhase(EFPSMatchPhase NewPhase);
	void UpdateTimeRemaining(float DeltaTime);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS Multiplayer|Rules")
	int32 ScoreLimit = 75;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS Multiplayer|Rules")
	float TotalMatchTimeSeconds = 600.0f; // 10 minutes

protected:
	UPROPERTY(ReplicatedUsing = OnRep_TeamScores, VisibleInstanceOnly, Category = "FPS Multiplayer|State")
	int32 TeamAlphaScore = 0;

	UPROPERTY(ReplicatedUsing = OnRep_TeamScores, VisibleInstanceOnly, Category = "FPS Multiplayer|State")
	int32 TeamBravoScore = 0;

	UPROPERTY(Replicated, VisibleInstanceOnly, Category = "FPS Multiplayer|State")
	float MatchTimeRemaining = 600.0f;

	UPROPERTY(Replicated, VisibleInstanceOnly, Category = "FPS Multiplayer|State")
	EFPSMatchPhase MatchPhase = EFPSMatchPhase::PreMatchCountdown;

	UFUNCTION()
	void OnRep_TeamScores();
};
