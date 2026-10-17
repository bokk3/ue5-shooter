#include "Multiplayer/FPSPlayerState.h"
#include "Economy/FPSEconomySubsystem.h"
#include "Net/UnrealNetwork.h"

AFPSPlayerState::AFPSPlayerState()
{
	bReplicates = true;
	SetNetUpdateFrequency(10.0f);
}

void AFPSPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AFPSPlayerState, Kills);
	DOREPLIFETIME(AFPSPlayerState, Deaths);
	DOREPLIFETIME(AFPSPlayerState, Assists);
	DOREPLIFETIME(AFPSPlayerState, CurrentStreak);
	DOREPLIFETIME(AFPSPlayerState, PlayerTeam);
}

void AFPSPlayerState::AddKill(int32 ScoreReward)
{
	if (!HasAuthority()) return;

	Kills++;
	CurrentStreak++;
	SetScore(GetScore() + ScoreReward);

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
	SetScore(GetScore() + ScoreReward);
	OnRep_StatsChanged();
}

void AFPSPlayerState::AddCombatScore(int32 AdditionalScore)
{
	if (!HasAuthority() || AdditionalScore <= 0) return;

	SetScore(GetScore() + AdditionalScore);
	OnRep_Score();
}

void AFPSPlayerState::AwardCombatAccolade(const FString& AccoladeName, int32 GoldReward)
{
	if (HasAuthority() && GoldReward > 0)
	{
		SetScore(GetScore() + GoldReward);
		OnRep_Score();
	}

	// If pawn is not present (or if called outside character flow), credit local player's economy
	if (!GetPawn() && GoldReward > 0)
	{
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UFPSEconomySubsystem* Economy = GI->GetSubsystem<UFPSEconomySubsystem>())
			{
				Economy->AddGold(GoldReward, AccoladeName);
			}
		}
	}

	OnCombatAccoladeEarned.Broadcast(AccoladeName, GoldReward);
}

void AFPSPlayerState::SetPlayerTeam(EFPSPlayerTeam NewTeam)
{
	if (HasAuthority())
	{
		PlayerTeam = NewTeam;
	}
}

void AFPSPlayerState::OnRep_Score()
{
	Super::OnRep_Score();
	OnRep_StatsChanged();
}

void AFPSPlayerState::OnRep_StatsChanged()
{
	OnStatsUpdated.Broadcast(Kills, Deaths, FMath::RoundToInt(GetScore()));
}

void AFPSPlayerState::OnRep_StreakChanged()
{
	OnStreakEarned.Broadcast(CurrentStreak);
}
