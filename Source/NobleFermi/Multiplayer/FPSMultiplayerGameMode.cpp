#include "Multiplayer/FPSMultiplayerGameMode.h"
#include "Multiplayer/FPSMultiplayerGameState.h"
#include "Multiplayer/FPSPlayerState.h"
#include "Character/FPSCharacter.h"
#include "Weapons/FPSWeaponBase.h"
#include "Map/FPSCombatArenaMapActor.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Engine/World.h"

AFPSMultiplayerGameMode::AFPSMultiplayerGameMode()
{
	PrimaryActorTick.bCanEverTick = true;

	DefaultPawnClass = AFPSCharacter::StaticClass();
	PlayerStateClass = AFPSPlayerState::StaticClass();
	GameStateClass = AFPSMultiplayerGameState::StaticClass();
}

void AFPSMultiplayerGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	UE_LOG(LogTemp, Warning, TEXT("[FPSGameMode] InitGame: Initializing 128-tick tactical multiplayer match..."));
	EnsureCombatArenaExists();
}

void AFPSMultiplayerGameMode::EnsureCombatArenaExists()
{
	if (ActiveArena) return;

	UWorld* World = GetWorld();
	if (!World) return;

	ActiveArena = Cast<AFPSCombatArenaMapActor>(UGameplayStatics::GetActorOfClass(this, AFPSCombatArenaMapActor::StaticClass()));
	if (!ActiveArena)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ActiveArena = World->SpawnActor<AFPSCombatArenaMapActor>(FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
		UE_LOG(LogTemp, Warning, TEXT("[FPSGameMode] EnsureCombatArenaExists: Procedurally spawned AFPSCombatArenaMapActor!"));
	}

	if (ActiveArena)
	{
		ActiveArena->SpawnTacticalPlayerStarts();
	}
}

void AFPSMultiplayerGameMode::BeginPlay()
{
	Super::BeginPlay();

	FPSGameState = GetGameState<AFPSMultiplayerGameState>();
	if (FPSGameState)
	{
		FPSGameState->SetMatchPhase(EFPSMatchPhase::InProgress);
	}

	EnsureCombatArenaExists();
	SpawnInitialTacticalBots();
	UE_LOG(LogTemp, Warning, TEXT("[FPSGameMode] BeginPlay: Match in progress on Rust Tactical. Arena: %s"), *GetNameSafe(ActiveArena));
}

void AFPSMultiplayerGameMode::SpawnInitialTacticalBots()
{
	for (int32 i = 0; i < TargetTacticalBotCount; ++i)
	{
		SpawnSingleTacticalBot(i);
	}
}

void AFPSMultiplayerGameMode::SpawnSingleTacticalBot(int32 BotIndex)
{
	UWorld* World = GetWorld();
	if (!World) return;

	static const FString BotCallsigns[4] = {
		TEXT("VIPER-1"),
		TEXT("SPECTRE-2"),
		TEXT("JACKAL-3"),
		TEXT("PHANTOM-4")
	};

	static const FVector BravoSpawnPoints[4] = {
		FVector(1550.0f, -880.0f, 115.0f),
		FVector(1650.0f, -220.0f, 115.0f),
		FVector(1650.0f, 340.0f, 115.0f),
		FVector(1500.0f, 920.0f, 115.0f)
	};

	const int32 SafeIdx = FMath::Clamp(BotIndex, 0, 3);
	const FVector SpawnLoc = BravoSpawnPoints[SafeIdx] + FVector(FMath::FRandRange(-80.0f, 80.0f), FMath::FRandRange(-80.0f, 80.0f), 0.0f);
	const FRotator SpawnRot(0.0f, 180.0f, 0.0f);

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	if (AFPSCharacter* BotChar = World->SpawnActor<AFPSCharacter>(AFPSCharacter::StaticClass(), SpawnLoc, SpawnRot, SpawnParams))
	{
		BotChar->SpawnDefaultController();
		BotChar->InitializeAsTacticalBot(SafeIdx, BotCallsigns[SafeIdx], EFPSPlayerTeam::Bravo);
		ActiveTacticalBots.Add(BotChar);
	}
}

void AFPSMultiplayerGameMode::HandleBotKilled(AFPSCharacter* DeadBot, AActor* KillerActor)
{
	if (!DeadBot) return;

	ActiveTacticalBots.Remove(DeadBot);

	const FString VictimName = DeadBot->GetBotCallsign();
	FString KillerName = TEXT("OPERATOR");
	FString WeaponName = TEXT("NF-98 CARBINE");

	AFPSCharacter* KillerChar = Cast<AFPSCharacter>(KillerActor);
	if (!KillerChar && KillerActor)
	{
		KillerChar = Cast<AFPSCharacter>(KillerActor->GetOwner());
	}

	if (KillerChar)
	{
		if (KillerChar->IsTacticalBot())
		{
			KillerName = KillerChar->GetBotCallsign();
		}
		else if (AFPSPlayerState* KillerPS = KillerChar->GetPlayerState<AFPSPlayerState>())
		{
			KillerName = KillerPS->GetPlayerName().IsEmpty() ? TEXT("ALPHA-1") : KillerPS->GetPlayerName();
			KillerPS->AddKill(100);
			if (FPSGameState)
			{
				FPSGameState->AddScoreToTeam(KillerPS->GetPlayerTeam(), 1);
			}
		}
		else if (FPSGameState)
		{
			FPSGameState->AddScoreToTeam(EFPSPlayerTeam::Alpha, 1);
		}

		if (AFPSWeaponBase* Wep = KillerChar->GetCurrentWeapon())
		{
			WeaponName = Wep->GetWeaponName();
		}
	}

	OnKillfeedEvent.Broadcast(KillerName, VictimName, WeaponName, false);

	// Clean up ragdoll corpse and respawn bot on its lane after RespawnDelaySeconds
	const int32 RespawnIdx = FMath::RandRange(0, 3);
	TWeakObjectPtr<AFPSCharacter> WeakDeadBot(DeadBot);
	FTimerHandle BotRespawnTimer;
	GetWorldTimerManager().SetTimer(BotRespawnTimer, FTimerDelegate::CreateWeakLambda(this, [this, WeakDeadBot, RespawnIdx]()
	{
		if (WeakDeadBot.IsValid())
		{
			if (AFPSWeaponBase* OldWep = WeakDeadBot->GetCurrentWeapon())
			{
				OldWep->Destroy();
			}
			WeakDeadBot->Destroy();
		}
		SpawnSingleTacticalBot(RespawnIdx);
	}), RespawnDelaySeconds, false);
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

AActor* AFPSMultiplayerGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	EnsureCombatArenaExists();

	AFPSPlayerState* PS = Player ? Player->GetPlayerState<AFPSPlayerState>() : nullptr;
	const FName TargetTag = (PS && PS->GetPlayerTeam() == EFPSPlayerTeam::Bravo) ? FName("TeamBravo") : FName("TeamAlpha");

	TArray<AActor*> FoundStarts;
	UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), FoundStarts);

	TArray<AActor*> ValidStarts;
	for (AActor* StartActor : FoundStarts)
	{
		APlayerStart* Start = Cast<APlayerStart>(StartActor);
		if (Start && (Start->PlayerStartTag == TargetTag || Start->Tags.Contains(TargetTag)))
		{
			ValidStarts.Add(Start);
		}
	}

	if (ValidStarts.Num() > 0)
	{
		const int32 RandomIdx = FMath::RandRange(0, ValidStarts.Num() - 1);
		AActor* SelectedStart = ValidStarts[RandomIdx];
		UE_LOG(LogTemp, Warning, TEXT("[FPSGameMode] ChoosePlayerStart: Spawning player %s at %s (%s) at %s"),
			*GetNameSafe(Player), *GetNameSafe(SelectedStart), *TargetTag.ToString(), *SelectedStart->GetActorLocation().ToString());
		return SelectedStart;
	}

	return Super::ChoosePlayerStart_Implementation(Player);
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

