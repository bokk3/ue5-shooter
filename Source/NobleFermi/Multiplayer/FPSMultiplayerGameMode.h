#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Multiplayer/FPSPlayerState.h"
#include "FPSMultiplayerGameMode.generated.h"

class AFPSMultiplayerGameState;
class AFPSCombatArenaMapActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnKillfeedEventSignature, const FString&, KillerName, const FString&, VictimName, const FString&, WeaponName, bool, bIsHeadshot);

/**
 * Classic MW2 Team Deathmatch GameMode on Rust Tactical (60m x 60m)
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

	UPROPERTY(BlueprintAssignable, Category = "FPS Multiplayer")
	FOnKillfeedEventSignature OnKillfeedEvent;

	UFUNCTION(BlueprintCallable, Category = "FPS Multiplayer")
	void NotifyPlayerKilled(AController* Killer, AController* Victim, const FString& WeaponName, bool bIsHeadshot);

	UFUNCTION(BlueprintCallable, Category = "FPS Multiplayer")
	void TriggerTacticalNuke(AController* InstigatingPlayer);

	UFUNCTION(BlueprintPure, Category = "FPS Map")
	AFPSCombatArenaMapActor* GetActiveArena() const { return ActiveArena; }

	UPROPERTY(EditDefaultsOnly, Category = "FPS Rules")
	float RespawnDelaySeconds = 2.5f;

protected:
	virtual void BeginPlay() override;
	void RespawnPlayer(AController* ControllerToRespawn);

private:
	UPROPERTY()
	TObjectPtr<AFPSMultiplayerGameState> FPSGameState;

	UPROPERTY()
	TObjectPtr<AFPSCombatArenaMapActor> ActiveArena;
};
