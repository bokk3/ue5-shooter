#include "Multiplayer/FPSMultiplayerGameState.h"
#include "Net/UnrealNetwork.h"

AFPSMultiplayerGameState::AFPSMultiplayerGameState()
{
	bReplicates = true;
	SetNetUpdateFrequency(5.0f);
	MatchTimeRemaining = TotalMatchTimeSeconds;
}

void AFPSMultiplayerGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AFPSMultiplayerGameState, TeamAlphaScore);
	DOREPLIFETIME(AFPSMultiplayerGameState, TeamBravoScore);
	DOREPLIFETIME(AFPSMultiplayerGameState, MatchTimeRemaining);
	DOREPLIFETIME(AFPSMultiplayerGameState, MatchPhase);
}

void AFPSMultiplayerGameState::AddScoreToTeam(EFPSPlayerTeam Team, int32 Points)
{
	if (!HasAuthority()) return;

	if (Team == EFPSPlayerTeam::Alpha)
	{
		TeamAlphaScore += Points;
		if (TeamAlphaScore >= ScoreLimit)
		{
			SetMatchPhase(EFPSMatchPhase::MatchEndedScoreLimit);
			OnMatchEnded.Broadcast(EFPSPlayerTeam::Alpha, EFPSMatchPhase::MatchEndedScoreLimit);
		}
	}
	else if (Team == EFPSPlayerTeam::Bravo)
	{
		TeamBravoScore += Points;
		if (TeamBravoScore >= ScoreLimit)
		{
			SetMatchPhase(EFPSMatchPhase::MatchEndedScoreLimit);
			OnMatchEnded.Broadcast(EFPSPlayerTeam::Bravo, EFPSMatchPhase::MatchEndedScoreLimit);
		}
	}

	OnRep_TeamScores();
}

void AFPSMultiplayerGameState::SetMatchPhase(EFPSMatchPhase NewPhase)
{
	if (HasAuthority())
	{
		MatchPhase = NewPhase;
	}
}

void AFPSMultiplayerGameState::UpdateTimeRemaining(float DeltaTime)
{
	if (!HasAuthority() || MatchPhase != EFPSMatchPhase::InProgress) return;

	MatchTimeRemaining = FMath::Max(0.0f, MatchTimeRemaining - DeltaTime);

	if (MatchTimeRemaining <= 0.0f)
	{
		SetMatchPhase(EFPSMatchPhase::MatchEndedTimeLimit);
		EFPSPlayerTeam Winner = (TeamAlphaScore > TeamBravoScore) ? EFPSPlayerTeam::Alpha :
			((TeamBravoScore > TeamAlphaScore) ? EFPSPlayerTeam::Bravo : EFPSPlayerTeam::None);
		OnMatchEnded.Broadcast(Winner, EFPSMatchPhase::MatchEndedTimeLimit);
	}
}

void AFPSMultiplayerGameState::OnRep_TeamScores()
{
	OnTeamScoresUpdated.Broadcast(TeamAlphaScore, TeamBravoScore);
}
