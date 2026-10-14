#include "Multiplayer/FPSMultiplayerGameMode.h"
#include "Multiplayer/FPSMultiplayerGameState.h"
#include "Multiplayer/FPSPlayerState.h"
#include "Character/FPSCharacter.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

AFPSMultiplayerGameMode::AFPSMultiplayerGameMode()
{
	PrimaryActorTick.bCanEverTick = true;

	DefaultPawnClass = AFPSCharacter::StaticClass();
	PlayerStateClass = AFPSPlayerState::StaticClass();
	GameStateClass = AFPSMultiplayerGameState::StaticClass();
}

void AFPSMultiplayerGameMode::BeginPlay()
{
	Super::BeginPlay();

	FPSGameState = GetGameState<AFPSMultiplayerGameState>();
	if (FPSGameState)
	{
		FPSGameState->SetMatchPhase(EFPSMatchPhase::InProgress);
	}
}

void AFPSMultiplayerGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (FPSGameState)
	{
		FPSGameState->UpdateTimeRemaining(DeltaSeconds);
	}
}

void AFPSMultiplayerGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	AFPSPlayerState* PS = NewPlayer->GetPlayerState<AFPSPlayerState>();
	if (PS)
	{
		// Auto-balance: alternating team assignment
		static int32 PlayerCounter = 0;
		EFPSPlayerTeam AssignedTeam = (PlayerCounter++ % 2 == 0) ? EFPSPlayerTeam::Alpha : EFPSPlayerTeam::Bravo;
		PS->SetPlayerTeam(AssignedTeam);
	}
}

void AFPSMultiplayerGameMode::NotifyPlayerKilled(AController* Killer, AController* Victim, const FString& WeaponName, bool bIsHeadshot)
{
	AFPSPlayerState* KillerPS = Killer ? Killer->GetPlayerState<AFPSPlayerState>() : nullptr;
	AFPSPlayerState* VictimPS = Victim ? Victim->GetPlayerState<AFPSPlayerState>() : nullptr;

	FString KillerName = KillerPS ? KillerPS->GetPlayerName() : TEXT("World");
	FString VictimName = VictimPS ? VictimPS->GetPlayerName() : TEXT("Player");

	if (KillerPS)
	{
		KillerPS->AddKill(100);

		if (FPSGameState)
		{
			FPSGameState->AddScoreToTeam(KillerPS->GetPlayerTeam(), 1);
		}
	}

	if (VictimPS)
	{
		VictimPS->AddDeath();
	}

	// Broadcast Killfeed event
	OnKillfeedEvent.Broadcast(KillerName, VictimName, WeaponName, bIsHeadshot);

	// Schedule Victim Respawn
	if (Victim)
	{
		FTimerHandle RespawnHandle;
		FTimerDelegate RespawnDel = FTimerDelegate::CreateUObject(this, &AFPSMultiplayerGameMode::RespawnPlayer, Victim);
		GetWorldTimerManager().SetTimer(RespawnHandle, RespawnDel, RespawnDelaySeconds, false);
	}
}

void AFPSMultiplayerGameMode::RespawnPlayer(AController* ControllerToRespawn)
{
	if (!ControllerToRespawn) return;

	// Destroy existing pawn if present
	APawn* OldPawn = ControllerToRespawn->GetPawn();
	if (OldPawn)
	{
		OldPawn->Destroy();
	}

	// Respawn and possess
	RestartPlayer(ControllerToRespawn);
}

void AFPSMultiplayerGameMode::TriggerTacticalNuke(AController* InstigatingPlayer)
{
	if (!FPSGameState) return;

	AFPSPlayerState* PS = InstigatingPlayer ? InstigatingPlayer->GetPlayerState<AFPSPlayerState>() : nullptr;
	EFPSPlayerTeam WinningTeam = PS ? PS->GetPlayerTeam() : EFPSPlayerTeam::Alpha;

	FPSGameState->SetMatchPhase(EFPSMatchPhase::MatchEndedNuke);
	FPSGameState->OnMatchEnded.Broadcast(WinningTeam, EFPSMatchPhase::MatchEndedNuke);
}
