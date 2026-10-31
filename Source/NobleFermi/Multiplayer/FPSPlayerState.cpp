#include "Multiplayer/FPSPlayerState.h"
#include "Economy/FPSEconomySubsystem.h"
#include "Lyra/AbilitySystem/FPSAbilitySystemComponent.h"
#include "Lyra/AbilitySystem/FPSCombatAttributeSet.h"
#include "Lyra/Inventory/FPSInventoryManagerComponent.h"
#include "Net/UnrealNetwork.h"

AFPSPlayerState::AFPSPlayerState()
{
	bReplicates = true;
	SetNetUpdateFrequency(60.0f);

	// Lyra ASC, Combat Attributes, and Inventory hosted on PlayerState
	AbilitySystemComponent = CreateDefaultSubobject<UFPSAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<UFPSCombatAttributeSet>(TEXT("AttributeSet"));

	InventoryManager = CreateDefaultSubobject<UFPSInventoryManagerComponent>(TEXT("InventoryManager"));
}

UAbilitySystemComponent* AFPSPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
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
	if (!HasAuthority()) return;

	SetScore(GetScore() + AdditionalScore);
	OnRep_StatsChanged();
}

void AFPSPlayerState::AwardCombatAccolade(const FString& AccoladeName, int32 GoldReward)
{
	OnCombatAccoladeEarned.Broadcast(AccoladeName, GoldReward);

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UFPSEconomySubsystem* Economy = GI->GetSubsystem<UFPSEconomySubsystem>())
		{
			Economy->AddGold(GoldReward);
		}
	}
}

void AFPSPlayerState::SetPlayerTeam(EFPSPlayerTeam NewTeam)
{
	if (!HasAuthority()) return;
	PlayerTeam = NewTeam;
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
