#include "Multiplayer/FPSPlayerState.h"
#include "Net/UnrealNetwork.h"

AFPSPlayerState::AFPSPlayerState()
{
	bReplicates = true;
	NetUpdateFrequency = 10.0f;
}

void AFPSPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AFPSPlayerState, Kills);
	DOREPLIFETIME(AFPSPlayerState, Deaths);
	DOREPLIFETIME(AFPSPlayerState, Assists);
	DOREPLIFETIME(AFPSPlayerState, Score);
	DOREPLIFETIME(AFPSPlayerState, CurrentStreak);
	DOREPLIFETIME(AFPSPlayerState, PlayerTeam);
}

void AFPSPlayerState::AddKill(int32 ScoreReward)
{
	if (!HasAuthority()) return;

	Kills++;
	CurrentStreak++;
	Score += ScoreReward;

	OnRep_StatsChanged();
	OnRep_StreakChanged();
}

void AFPSPlayerState::AddDeath()
{
	if (!HasAuthority()) return;

	Deaths++;
	CurrentStreak = 0; // Streak reset on death! Classic MW2 rule

	OnRep_StatsChanged();
	OnRep_StreakChanged();
}

void AFPSPlayerState::AddAssist(int32 ScoreReward)
{
	if (!HasAuthority()) return;

	Assists++;
	Score += ScoreReward;
	OnRep_StatsChanged();
}

void AFPSPlayerState::SetPlayerTeam(EFPSPlayerTeam NewTeam)
{
	if (HasAuthority())
	{
		PlayerTeam = NewTeam;
	}
}

void AFPSPlayerState::OnRep_StatsChanged()
{
	OnStatsUpdated.Broadcast(Kills, Deaths, Score);
}

void AFPSPlayerState::OnRep_StreakChanged()
{
	OnStreakEarned.Broadcast(CurrentStreak);
}
