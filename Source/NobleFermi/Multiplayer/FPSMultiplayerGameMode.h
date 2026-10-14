#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Multiplayer/FPSPlayerState.h"
#include "FPSMultiplayerGameMode.generated.h"

class AFPSMultiplayerGameState;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnKillfeedEventSignature, const FString&, KillerName, const FString&, VictimName, const FString&, WeaponName, bool, bIsHeadshot);

/**
 * Classic MW2 Team Deathmatch GameMode
 * Manages player spawning, kill scoring, killstreak arbitration, respawn cycles, and match conclusion.
 */
UCLASS()
class NOBLEFERMI_API AFPSMultiplayerGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AFPSMultiplayerGameMode();

	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(BlueprintAssignable, Category = "FPS Multiplayer")
	FOnKillfeedEventSignature OnKillfeedEvent;

	UFUNCTION(BlueprintCallable, Category = "FPS Multiplayer")
	void NotifyPlayerKilled(AController* Killer, AController* Victim, const FString& WeaponName, bool bIsHeadshot);

	UFUNCTION(BlueprintCallable, Category = "FPS Multiplayer")
	void TriggerTacticalNuke(AController* InstigatingPlayer);

	UPROPERTY(EditDefaultsOnly, Category = "FPS Rules")
	float RespawnDelaySeconds = 2.5f;

protected:
	virtual void BeginPlay() override;
	void RespawnPlayer(AController* ControllerToRespawn);

private:
	UPROPERTY()
	TObjectPtr<AFPSMultiplayerGameState> FPSGameState;
};
