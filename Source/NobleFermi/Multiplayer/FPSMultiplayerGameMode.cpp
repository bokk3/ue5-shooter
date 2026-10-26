#include "Multiplayer/FPSMultiplayerGameMode.h"
#include "Multiplayer/FPSMultiplayerGameState.h"
#include "Multiplayer/FPSPlayerState.h"
#include "Character/FPSCharacter.h"
#include "Weapons/FPSWeaponBase.h"
#include "Map/FPSCombatArenaMapActor.h"
#include "Map/FPSCoastalOperationsMapActor.h"
#include "Economy/FPSEconomySubsystem.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/ConfigCacheIni.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "HAL/PlatformFileManager.h"
#include "UnrealClient.h"

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

	// Check URL Option "?Theater=Coastal" or "?Theater=Rust" or command line "-Theater=Coastal"
	FString TheaterOption = UGameplayStatics::ParseOption(Options, TEXT("Theater"));
	if (TheaterOption.IsEmpty())
	{
		FParse::Value(FCommandLine::Get(), TEXT("Theater="), TheaterOption);
	}
	if (TheaterOption.Equals(TEXT("Coastal"), ESearchCase::IgnoreCase))
	{
		ActiveTheaterIndex = 1;
	}
	else if (TheaterOption.Equals(TEXT("Rust"), ESearchCase::IgnoreCase))
	{
		ActiveTheaterIndex = 0;
	}
	else
	{
		// Query persistent UFPSEconomySubsystem
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UFPSEconomySubsystem* Economy = GI->GetSubsystem<UFPSEconomySubsystem>())
			{
				ActiveTheaterIndex = Economy->GetSelectedTheaterIndex();
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[FPSGameMode] InitGame: Initializing 128-tick tactical multiplayer match in Theater %d (%s)..."),
		ActiveTheaterIndex, ActiveTheaterIndex == 1 ? TEXT("Coastal Operations") : TEXT("Rust Tactical"));

	EnsureCombatArenaExists();
}

void AFPSMultiplayerGameMode::EnsureCombatArenaExists()
{
	UWorld* World = GetWorld();
	if (!World) return;

	if (ActiveTheaterIndex == 1)
	{
		if (!ActiveCoastalArena)
		{
			ActiveCoastalArena = Cast<AFPSCoastalOperationsMapActor>(UGameplayStatics::GetActorOfClass(this, AFPSCoastalOperationsMapActor::StaticClass()));
			if (!ActiveCoastalArena)
			{
				FActorSpawnParameters SpawnParams;
				SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				ActiveCoastalArena = World->SpawnActor<AFPSCoastalOperationsMapActor>(FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
				UE_LOG(LogTemp, Warning, TEXT("[FPSGameMode] EnsureCombatArenaExists: Procedurally spawned AFPSCoastalOperationsMapActor!"));
			}
		}

		if (ActiveCoastalArena)
		{
			ActiveCoastalArena->SpawnTacticalPlayerStarts();
		}
	}
	else
	{
		if (!ActiveArena)
		{
			ActiveArena = Cast<AFPSCombatArenaMapActor>(UGameplayStatics::GetActorOfClass(this, AFPSCombatArenaMapActor::StaticClass()));
			if (!ActiveArena)
			{
				FActorSpawnParameters SpawnParams;
				SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				ActiveArena = World->SpawnActor<AFPSCombatArenaMapActor>(FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
				UE_LOG(LogTemp, Warning, TEXT("[FPSGameMode] EnsureCombatArenaExists: Procedurally spawned AFPSCombatArenaMapActor!"));
			}
		}

		if (ActiveArena)
		{
			ActiveArena->SpawnTacticalPlayerStarts();
		}
	}
}

void AFPSMultiplayerGameMode::BeginPlay()
{
	Super::BeginPlay();

	// Enforce baseline iGPU optimization on fresh match load if no user config exists yet
	if (GConfig && GetWorld())
	{
		int32 SavedLumenMethod = 0;
		if (!GConfig->GetInt(TEXT("SystemSettings"), TEXT("r.DynamicGlobalIlluminationMethod"), SavedLumenMethod, GEngineIni))
		{
			UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), TEXT("r.DynamicGlobalIlluminationMethod 0"));
			UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), TEXT("r.Shadow.Virtual.Enable 0"));
			UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), TEXT("r.Shadow.CSM.MaxCascades 2"));
			UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), TEXT("r.VolumetricFog 0"));
			UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), TEXT("r.VolumetricCloud 0"));
			UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), TEXT("r.SkyLight.RealTimeReflectionCapture 0"));
			UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), TEXT("r.ScreenPercentage 70"));
			UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), TEXT("t.MaxFPS 60"));
		}
	}

	FPSGameState = GetGameState<AFPSMultiplayerGameState>();
	if (FPSGameState)
	{
		FPSGameState->SetMatchPhase(EFPSMatchPhase::InProgress);
	}

	EnsureCombatArenaExists();
	SpawnInitialTacticalBots();
	UE_LOG(LogTemp, Warning, TEXT("[FPSGameMode] BeginPlay: Match in progress on %s. Arena: %s"),
		ActiveTheaterIndex == 1 ? TEXT("Coastal Operations") : TEXT("Rust Tactical"),
		*GetNameSafe(ActiveTheaterIndex == 1 ? (AActor*)ActiveCoastalArena : (AActor*)ActiveArena));

	// Auto screenshot verification if -TakeGameplayShot is passed on command line
	if (FParse::Param(FCommandLine::Get(), TEXT("TakeGameplayShot")))
	{
		UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), TEXT("DisableAllScreenMessages"));

		float ShotDelay = GIsEditor ? 9.0f : 14.0f;
		FParse::Value(FCommandLine::Get(), TEXT("GameplayShotDelay="), ShotDelay);

		FString ShotName = (ActiveTheaterIndex == 1) ? TEXT("gameplay_coastal_operations.png") : TEXT("gameplay_rust_tactical.png");
		FParse::Value(FCommandLine::Get(), TEXT("ShotName="), ShotName);

		// Action trigger (fire weapon burst for dynamic muzzle flash, brass casing ejection, and HUD recoil right before capture)
		FTimerHandle ActionTimerHandle;
		GetWorldTimerManager().SetTimer(ActionTimerHandle, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
			if (PC && PC->GetPawn())
			{
				if (AFPSCharacter* Char = Cast<AFPSCharacter>(PC->GetPawn()))
				{
					Char->StartFire();
				}
			}
		}), FMath::Max(0.5f, ShotDelay - 0.5f), false);

		FTimerHandle ScreenshotTimerHandle;
		GetWorldTimerManager().SetTimer(ScreenshotTimerHandle, FTimerDelegate::CreateWeakLambda(this, [this, ShotName]()
		{
			const FString ScreenshotDir = FPaths::Combine(FPaths::ProjectDir(), TEXT("Docs"), TEXT("Screenshots"));
			IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
			PlatformFile.CreateDirectoryTree(*ScreenshotDir);

			const FString ScreenshotFile = FPaths::Combine(ScreenshotDir, ShotName);
			FScreenshotRequest::RequestScreenshot(ScreenshotFile, true, false);
			UE_LOG(LogTemp, Warning, TEXT("[GAMEPLAY_SCREENSHOT] Real in-engine screenshot queued to: %s"), *ScreenshotFile);

			FScreenshotRequest::OnScreenshotRequestProcessed().AddLambda([]()
			{
				UE_LOG(LogTemp, Warning, TEXT("[GAMEPLAY_SCREENSHOT] Real in-engine screenshot successfully written to disk! Clean exit in 1s."));
				FGenericPlatformMisc::RequestExit(false);
			});

			FTimerHandle ExitTimerHandle;
			GetWorldTimerManager().SetTimer(ExitTimerHandle, FTimerDelegate::CreateWeakLambda(this, []()
			{
				UE_LOG(LogTemp, Warning, TEXT("[GAMEPLAY_SCREENSHOT] Safety timeout reached, requesting exit..."));
				FGenericPlatformMisc::RequestExit(false);
			}), 5.0f, false);
		}), ShotDelay, false);
	}
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

	static const FVector CoastalBravoSpawnPoints[4] = {
		FVector(-2200.0f, -2300.0f, 115.0f),
		FVector(-750.0f, -2450.0f, 115.0f),
		FVector(750.0f, -2450.0f, 115.0f),
		FVector(2200.0f, -2300.0f, 115.0f)
	};

	const int32 SafeIdx = FMath::Clamp(BotIndex, 0, 3);
	const FVector BaseSpawnLoc = (ActiveTheaterIndex == 1) ? CoastalBravoSpawnPoints[SafeIdx] : BravoSpawnPoints[SafeIdx];
	const FVector SpawnLoc = BaseSpawnLoc + FVector(FMath::FRandRange(-80.0f, 80.0f), FMath::FRandRange(-80.0f, 80.0f), 0.0f);
	const FRotator SpawnRot = (ActiveTheaterIndex == 1) ? FRotator(0.0f, 90.0f, 0.0f) : FRotator(0.0f, 180.0f, 0.0f);

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

