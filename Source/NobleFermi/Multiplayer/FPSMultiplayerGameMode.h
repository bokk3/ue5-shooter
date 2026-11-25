#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Multiplayer/FPSPlayerState.h"
#include "FPSMultiplayerGameMode.generated.h"

class AFPSCharacter;

class AFPSMultiplayerGameState;
class AFPSCombatArenaMapActor;
class AFPSCoastalOperationsMapActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnKillfeedEventSignature, const FString&, KillerName, const FString&, VictimName, const FString&, WeaponName, bool, bIsHeadshot);

/**
 * Classic MW2 Team Deathmatch GameMode on Rust Tactical (60m x 60m)
 * and Coastal Operations (120m x 120m Dike, Dunes & Pier).
 * Manages procedural arena spawning, team player starts, kill scoring,
 * killstreak arbitration, respawn cycles, and match conclusion.
 */
UCLASS()
class NOBLEFERMI_API AFPSMultiplayerGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AFPSMultiplayerGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	void EnsureCombatArenaExists();

	/**
	 * Operator pawn for players and bots: a Blueprint child of AFPSCharacter that uses the Game
	 * Animation Sample locomotion. Falls back to plain AFPSCharacter if the asset isn't present
	 * (e.g. the third-party sample content hasn't been imported on this machine).
	 */
	UPROPERTY(EditDefaultsOnly, Category = "FPS Multiplayer")
	TSoftClassPtr<AFPSCharacter> OperatorPawnClass;

	/** Class used for bots (resolved in InitGame). */
	UPROPERTY(Transient)
	TSubclassOf<AFPSCharacter> BotPawnClass;

	UPROPERTY(BlueprintAssignable, Category = "FPS Multiplayer")
	FOnKillfeedEventSignature OnKillfeedEvent;

	UFUNCTION(BlueprintCallable, Category = "FPS Multiplayer")
	void NotifyPlayerKilled(AController* Killer, AController* Victim, const FString& WeaponName, bool bIsHeadshot);

	UFUNCTION(BlueprintCallable, Category = "FPS Multiplayer|Bots")
	void HandleBotKilled(class AFPSCharacter* DeadBot, AActor* KillerActor);

	UFUNCTION(BlueprintCallable, Category = "FPS Multiplayer|Bots")
	void SpawnInitialTacticalBots();

	UFUNCTION(BlueprintCallable, Category = "FPS Multiplayer")
	void TriggerTacticalNuke(AController* InstigatingPlayer);

	/**
	 * Theater 2 is the mechanics test range (L_Range, or ?Theater=Range): hand-built map, no procedural
	 * arena, players at TeamAlpha starts and ?Bots=N (default 2) bots at TeamBravo starts.
	 */
	static constexpr int32 RangeTheaterIndex = 2;

	UFUNCTION(BlueprintPure, Category = "FPS Map")
	int32 GetActiveTheaterIndex() const { return ActiveTheaterIndex; }

	const TCHAR* GetTheaterDisplayName() const;

	UFUNCTION(BlueprintPure, Category = "FPS Map")
	AFPSCombatArenaMapActor* GetActiveArena() const { return ActiveArena; }

	UFUNCTION(BlueprintPure, Category = "FPS Map")
	AFPSCoastalOperationsMapActor* GetActiveCoastalArena() const { return ActiveCoastalArena; }

	UPROPERTY(EditDefaultsOnly, Category = "FPS Rules")
	float RespawnDelaySeconds = 2.5f;

	UPROPERTY(EditDefaultsOnly, Category = "FPS Rules|Bots")
	int32 TargetTacticalBotCount = 4;

protected:
	virtual void BeginPlay() override;
	void RespawnPlayer(AController* ControllerToRespawn);
	void SpawnSingleTacticalBot(int32 BotIndex);

private:
	/**
	 * -NFActionTest[=StartDelaySeconds]: scripted check of the operator's weapon animations. Fires and
	 * reloads the rifle, switches to the pistol, fires and reloads it, screenshots each step to
	 * Saved/Screenshots/ActionTest and exits. Add -NFGripLog for the grip measurements.
	 */
	void RunOperatorActionTest();

	/**
	 * -NFRangeTest[=StartDelaySeconds]: scripted check of the L_Range reactive targets. Aims at the
	 * nearest targets in front of the player and fires one round at each (chest, then head on the
	 * silhouettes), logs what each target registered, screenshots to Saved/Screenshots/RangeTest
	 * before, just after each shot, after the string and once the poppers have stood up again, then exits.
	 */
	void RunRangeTargetTest();

	UPROPERTY()
	int32 ActiveTheaterIndex = 0;

	UPROPERTY()
	TObjectPtr<AFPSMultiplayerGameState> FPSGameState;

	UPROPERTY()
	TObjectPtr<AFPSCombatArenaMapActor> ActiveArena;

	UPROPERTY()
	TObjectPtr<AFPSCoastalOperationsMapActor> ActiveCoastalArena;

	UPROPERTY()
	TArray<TObjectPtr<class AFPSCharacter>> ActiveTacticalBots;
};

