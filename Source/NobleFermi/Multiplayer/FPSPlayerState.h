#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "FPSPlayerState.generated.h"

class UFPSAbilitySystemComponent;
class UFPSCombatAttributeSet;
class UFPSInventoryManagerComponent;

UENUM(BlueprintType)
enum class EFPSPlayerTeam : uint8
{
	None,
	Alpha, // Task Force 141 (Friendly Blue)
	Bravo  // Shadow Company / OpFor (Enemy Red)
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnPlayerStatsUpdatedSignature, int32, Kills, int32, Deaths, int32, Score);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStreakEarnedSignature, int32, CurrentStreak);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCombatAccoladeEarnedSignature, const FString&, AccoladeName, int32, GoldReward);

/**
 * AAA FPS Multiplayer PlayerState (Lyra Replicated Architecture)
 * Hosts replicated AbilitySystemComponent, CombatAttributeSet, and Fast-Array InventoryManager.
 */
UCLASS()
class NOBLEFERMI_API AFPSPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AFPSPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// IAbilitySystemInterface
	virtual class UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintPure, Category = "Lyra|Abilities")
	UFPSAbilitySystemComponent* GetFPSAbilitySystemComponent() const { return AbilitySystemComponent; }

	UFUNCTION(BlueprintPure, Category = "Lyra|Attributes")
	UFPSCombatAttributeSet* GetCombatAttributeSet() const { return AttributeSet; }

	UFUNCTION(BlueprintPure, Category = "Lyra|Inventory")
	UFPSInventoryManagerComponent* GetInventoryManager() const { return InventoryManager; }

	UPROPERTY(BlueprintAssignable, Category = "FPS Multiplayer|Events")
	FOnPlayerStatsUpdatedSignature OnStatsUpdated;

	UPROPERTY(BlueprintAssignable, Category = "FPS Multiplayer|Events")
	FOnStreakEarnedSignature OnStreakEarned;

	UPROPERTY(BlueprintAssignable, Category = "FPS Multiplayer|Events")
	FOnCombatAccoladeEarnedSignature OnCombatAccoladeEarned;

	// Stats
	UFUNCTION(BlueprintPure, Category = "FPS Stats")
	int32 GetKills() const { return Kills; }

	UFUNCTION(BlueprintPure, Category = "FPS Stats")
	int32 GetDeaths() const { return Deaths; }

	UFUNCTION(BlueprintPure, Category = "FPS Stats")
	int32 GetAssists() const { return Assists; }

	UFUNCTION(BlueprintPure, Category = "FPS Stats")
	int32 GetCombatScore() const { return FMath::RoundToInt(GetScore()); }

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
	void AddCombatScore(int32 AdditionalScore);

	UFUNCTION(BlueprintCallable, Category = "FPS Stats")
	void AwardCombatAccolade(const FString& AccoladeName, int32 GoldReward);

	UFUNCTION(BlueprintCallable, Category = "FPS Stats")
	void SetPlayerTeam(EFPSPlayerTeam NewTeam);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lyra|Abilities")
	TObjectPtr<UFPSAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lyra|Attributes")
	TObjectPtr<UFPSCombatAttributeSet> AttributeSet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lyra|Inventory")
	TObjectPtr<UFPSInventoryManagerComponent> InventoryManager;

	UPROPERTY(ReplicatedUsing = OnRep_StatsChanged, VisibleInstanceOnly, Category = "FPS Stats")
	int32 Kills = 0;

	UPROPERTY(ReplicatedUsing = OnRep_StatsChanged, VisibleInstanceOnly, Category = "FPS Stats")
	int32 Deaths = 0;

	UPROPERTY(ReplicatedUsing = OnRep_StatsChanged, VisibleInstanceOnly, Category = "FPS Stats")
	int32 Assists = 0;

	UPROPERTY(ReplicatedUsing = OnRep_StreakChanged, VisibleInstanceOnly, Category = "FPS Stats")
	int32 CurrentStreak = 0;

	UPROPERTY(Replicated, VisibleInstanceOnly, Category = "FPS Stats")
	EFPSPlayerTeam PlayerTeam = EFPSPlayerTeam::Alpha;

	virtual void OnRep_Score() override;

	UFUNCTION()
	void OnRep_StatsChanged();

	UFUNCTION()
	void OnRep_StreakChanged();
};
