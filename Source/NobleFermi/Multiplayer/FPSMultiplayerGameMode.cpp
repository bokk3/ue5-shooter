#include "Multiplayer/FPSMultiplayerGameMode.h"
#include "Multiplayer/FPSMultiplayerGameState.h"
#include "Multiplayer/FPSPlayerState.h"
#include "Character/FPSCharacter.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Weapons/FPSWeaponBase.h"
#include "Combat/FPSRangeTarget.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "Inventory/FPSInventoryComponent.h"
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
#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <windows.h>
#include "Windows/HideWindowsPlatformTypes.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/Paths.h"
#endif


#if PLATFORM_WINDOWS
static void ApplyTacticalMultiplayerWindowIcon()
{
	if (!FSlateApplication::IsInitialized())
	{
		return;
	}

	TSharedPtr<SWindow> MainWindow = FSlateApplication::Get().GetActiveTopLevelWindow();
	if (!MainWindow.IsValid())
	{
		const TArray<TSharedRef<SWindow>>& Windows = FSlateApplication::Get().GetTopLevelWindows();
		if (Windows.Num() > 0)
		{
			MainWindow = Windows[0];
		}
	}

	if (MainWindow.IsValid() && MainWindow->GetNativeWindow().IsValid())
	{
		HWND Hwnd = (HWND)MainWindow->GetNativeWindow()->GetOSWindowHandle();
		if (Hwnd)
		{
			FString FullIcoPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("Build/Windows/Application.ico"));
			if (FPaths::FileExists(FullIcoPath))
			{
				HICON hIconBig = (HICON)LoadImageW(NULL, *FullIcoPath, IMAGE_ICON, 256, 256, LR_LOADFROMFILE);
				HICON hIconSmall = (HICON)LoadImageW(NULL, *FullIcoPath, IMAGE_ICON, 32, 32, LR_LOADFROMFILE);
				if (hIconBig)
				{
					SendMessageW(Hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIconBig);
				}
				if (hIconSmall)
				{
					SendMessageW(Hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIconSmall);
				}
			}
		}
	}
}
#endif

AFPSMultiplayerGameMode::AFPSMultiplayerGameMode()
{
	PrimaryActorTick.bCanEverTick = true;

	DefaultPawnClass = AFPSCharacter::StaticClass();
	BotPawnClass = AFPSCharacter::StaticClass();
	OperatorPawnClass = TSoftClassPtr<AFPSCharacter>(FSoftObjectPath(TEXT("/Game/NobleFermi/Characters/BP_FPSOperator.BP_FPSOperator_C")));
	PlayerStateClass = AFPSPlayerState::StaticClass();
	GameStateClass = AFPSMultiplayerGameState::StaticClass();
}

void AFPSMultiplayerGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	// Prefer the motion-matched operator Blueprint; keep the C++ character if it's unavailable
	if (!OperatorPawnClass.IsNull() && !FParse::Param(FCommandLine::Get(), TEXT("LegacyOperator")))
	{
		if (UClass* OperatorClass = OperatorPawnClass.LoadSynchronous())
		{
			DefaultPawnClass = OperatorClass;
			BotPawnClass = OperatorClass;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[GameMode] %s not found; using legacy AFPSCharacter rig"), *OperatorPawnClass.ToString());
		}
	}

	// Check URL Option "?Theater=Coastal" or "?Theater=Rust" or command line "-Theater=Coastal"
	FString TheaterOption = UGameplayStatics::ParseOption(Options, TEXT("Theater"));
	if (TheaterOption.IsEmpty())
	{
		FParse::Value(FCommandLine::Get(), TEXT("Theater="), TheaterOption);
	}
	if (TheaterOption.Equals(TEXT("Range"), ESearchCase::IgnoreCase) || MapName.Contains(TEXT("L_Range")))
	{
		// Mechanics test range: the map is hand-built, so no procedural arena; a couple of bots as targets.
		ActiveTheaterIndex = RangeTheaterIndex;
		TargetTacticalBotCount = FMath::Clamp(UGameplayStatics::GetIntOption(Options, TEXT("Bots"), 2), 0, 4);
	}
	else if (TheaterOption.Equals(TEXT("Coastal"), ESearchCase::IgnoreCase) ||
		TheaterOption.Equals(TEXT("BaiaBrava"), ESearchCase::IgnoreCase) ||
		TheaterOption.Equals(TEXT("Baia"), ESearchCase::IgnoreCase))
	{
		ActiveTheaterIndex = 1;
	}
	else if (TheaterOption.Equals(TEXT("Rust"), ESearchCase::IgnoreCase) ||
		TheaterOption.Equals(TEXT("RochaVelha"), ESearchCase::IgnoreCase) ||
		TheaterOption.Equals(TEXT("Favela"), ESearchCase::IgnoreCase))
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
		ActiveTheaterIndex, GetTheaterDisplayName());

	EnsureCombatArenaExists();
}

const TCHAR* AFPSMultiplayerGameMode::GetTheaterDisplayName() const
{
	switch (ActiveTheaterIndex)
	{
	case 1:                 return TEXT("Favela // Baía Brava");
	case RangeTheaterIndex: return TEXT("Mechanics Range");
	default:                return TEXT("Favela // Rocha Velha");
	}
}

void AFPSMultiplayerGameMode::EnsureCombatArenaExists()
{
	UWorld* World = GetWorld();
	if (!World || ActiveTheaterIndex == RangeTheaterIndex) return; // the range map brings its own geometry and starts

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
#if PLATFORM_WINDOWS
	ApplyTacticalMultiplayerWindowIcon();
#endif

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
		GetTheaterDisplayName(),
		*GetNameSafe(ActiveTheaterIndex == 1 ? (AActor*)ActiveCoastalArena : (AActor*)ActiveArena));

	// Auto screenshot verification if -TakeGameplayShot is passed on command line
	if (FParse::Param(FCommandLine::Get(), TEXT("TakeGameplayShot")))
	{
		UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), TEXT("DisableAllScreenMessages"));

		float ShotDelay = GIsEditor ? 9.0f : 14.0f;
		FParse::Value(FCommandLine::Get(), TEXT("GameplayShotDelay="), ShotDelay);

		FString ShotName = (ActiveTheaterIndex == 1) ? TEXT("gameplay_coastal_operations.png") : TEXT("gameplay_rust_tactical.png");
		FParse::Value(FCommandLine::Get(), TEXT("ShotName="), ShotName);

		// Action trigger (aim down sights, fire burst, and log weapon diagnostics right before capture)
		FTimerHandle ActionTimerHandle;
		GetWorldTimerManager().SetTimer(ActionTimerHandle, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
			if (PC && PC->GetPawn())
			{
				if (AFPSCharacter* Char = Cast<AFPSCharacter>(PC->GetPawn()))
				{
					Char->StartADS();
					Char->StartFire();

					if (Char->InventoryComponent)
					{
						AFPSWeaponBase* Weap = Char->InventoryComponent->GetActiveWeapon();
						UE_LOG(LogTemp, Warning, TEXT("[DIAG_WEAPON] Weap=%s, Loc=%s, Hidden=%d"),
							Weap ? *Weap->GetName() : TEXT("NONE"),
							Weap ? *Weap->GetActorLocation().ToString() : TEXT("NONE"),
							Weap ? Weap->IsHidden() : -1);
						if (Weap && Weap->ReceiverMesh)
						{
							UE_LOG(LogTemp, Warning, TEXT("[DIAG_WEAPON] ReceiverMesh Valid=1, Mesh=%s, Vis=%d, Hidden=%d, Radius=%.1f, Loc=%s"),
								Weap->ReceiverMesh->GetStaticMesh() ? *Weap->ReceiverMesh->GetStaticMesh()->GetName() : TEXT("NONE"),
								Weap->ReceiverMesh->IsVisible(),
								Weap->ReceiverMesh->bHiddenInGame,
								Weap->ReceiverMesh->CalcBounds(Weap->ReceiverMesh->GetComponentTransform()).SphereRadius,
								*Weap->ReceiverMesh->GetComponentLocation().ToString());
						}
					}
				}
			}
		}), FMath::Max(0.5f, ShotDelay - 1.2f), false);

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

	float ActionTestDelay = 0.0f;
	if (FParse::Param(FCommandLine::Get(), TEXT("NFActionTest")) || FParse::Value(FCommandLine::Get(), TEXT("NFActionTest="), ActionTestDelay))
	{
		RunOperatorActionTest();
	}

	if (FParse::Param(FCommandLine::Get(), TEXT("NFRangeTest")) || FParse::Value(FCommandLine::Get(), TEXT("NFRangeTest="), ActionTestDelay))
	{
		RunRangeTargetTest();
	}
}

void AFPSMultiplayerGameMode::RunRangeTargetTest()
{
	float StartDelay = GIsEditor ? 8.0f : 14.0f;
	FParse::Value(FCommandLine::Get(), TEXT("NFRangeTest="), StartDelay);
	UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), TEXT("DisableAllScreenMessages"));

	const FString ShotDir = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Screenshots"), TEXT("RangeTest"));
	FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*ShotDir);

	auto Screenshot = [ShotDir](const TCHAR* Name)
	{
		FScreenshotRequest::RequestScreenshot(FPaths::Combine(ShotDir, FString::Printf(TEXT("%s.png"), Name)), false, false);
	};
	auto LocalCharacter = [this]() -> AFPSCharacter*
	{
		APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
		return PC ? Cast<AFPSCharacter>(PC->GetPawn()) : nullptr;
	};
	auto After = [this](float Seconds, TFunction<void()> Action)
	{
		FTimerHandle Handle;
		GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, MoveTemp(Action)), Seconds, false);
	};

	After(StartDelay, [this, Screenshot, LocalCharacter, After]()
	{
		AFPSCharacter* Char = LocalCharacter();
		if (!Char)
		{
			UE_LOG(LogTemp, Warning, TEXT("[RangeTest] no local character, exiting"));
			FGenericPlatformMisc::RequestExit(false);
			return;
		}

		// The nearest targets inside a 30 degree cone in front of the player: the lane's 10 m silhouette,
		// the 15 m poppers and the 25 m paper targets.
		const FVector From = Char->GetActorLocation();
		const FVector Forward = Char->GetActorForwardVector();
		TArray<TWeakObjectPtr<AFPSRangeTarget>> Targets;
		for (TActorIterator<AFPSRangeTarget> It(GetWorld()); It; ++It)
		{
			const FVector To = (It->GetActorLocation() - From).GetSafeNormal2D();
			if (FVector::DotProduct(To, Forward.GetSafeNormal2D()) > FMath::Cos(FMath::DegreesToRadians(30.0f)))
			{
				Targets.Add(*It);
			}
		}
		Targets.Sort([From](const TWeakObjectPtr<AFPSRangeTarget>& A, const TWeakObjectPtr<AFPSRangeTarget>& B)
		{
			return FVector::DistSquared(A->GetActorLocation(), From) < FVector::DistSquared(B->GetActorLocation(), From);
		});
		Targets.SetNum(FMath::Min(Targets.Num(), 8));
		UE_LOG(LogTemp, Log, TEXT("[RangeTest] %d targets in front of the player"), Targets.Num());
		Char->StartADS();  // hip-fire spread misses the smaller targets
		Screenshot(TEXT("before"));

		// One shot per target body, plus a head shot on each silhouette.
		struct FShot { TWeakObjectPtr<AFPSRangeTarget> Target; bool bHead; };
		TArray<FShot> Shots;
		for (const TWeakObjectPtr<AFPSRangeTarget>& Target : Targets)
		{
			Shots.Add({ Target, false });
			if (Target->Kind == EFPSRangeTargetKind::Silhouette)
			{
				Shots.Add({ Target, true });
			}
		}

		const float Spacing = 0.7f;
		for (int32 Index = 0; Index < Shots.Num(); ++Index)
		{
			const FShot Shot = Shots[Index];
			auto Aim = [LocalCharacter, Shot]()
			{
				AFPSCharacter* C = LocalCharacter();
				APlayerController* PC = C ? Cast<APlayerController>(C->GetController()) : nullptr;
				if (!PC || !PC->PlayerCameraManager || !Shot.Target.IsValid())
				{
					return;
				}
				const FBox Box = Shot.Target->TargetMesh->Bounds.GetBox();
				const FVector Point(Box.GetCenter().X, Box.GetCenter().Y, Shot.bHead ? Box.Max.Z - Box.GetSize().Z * 0.07f : Box.GetCenter().Z);
				PC->SetControlRotation((Point - PC->PlayerCameraManager->GetCameraLocation()).Rotation());
			};
			const float T = 0.5f + Index * Spacing;
			After(T, Aim);
			After(T + 0.3f, Aim);  // again once the camera boom has followed the new aim
			After(T + 0.45f, [LocalCharacter]() { if (AFPSCharacter* C = LocalCharacter()) { C->StartFire(); } });
			After(T + 0.65f, [Screenshot, Index]() { Screenshot(*FString::Printf(TEXT("shot_%02d"), Index)); });
			After(T + 0.5f, [LocalCharacter, Shot]()
			{
				if (AFPSCharacter* C = LocalCharacter()) { C->StopFire(); }
				if (Shot.Target.IsValid())
				{
					UE_LOG(LogTemp, Log, TEXT("[RangeTest] shot at %s %s (%s): hits %d, down %d"), *Shot.Target->GetName(),
						Shot.bHead ? TEXT("head") : TEXT("body"), *UEnum::GetValueAsString(Shot.Target->Kind),
						Shot.Target->GetHitCount(), Shot.Target->IsKnockedDown() ? 1 : 0);
				}
			});
		}

		const float End = 0.5f + Shots.Num() * Spacing;
		After(End, [Screenshot, LocalCharacter, Targets]()
		{
			// Look back at the lane's middle so the after shot shows the fallen poppers and the readouts.
			AFPSCharacter* C = LocalCharacter();
			APlayerController* PC = C ? Cast<APlayerController>(C->GetController()) : nullptr;
			if (PC && PC->PlayerCameraManager && Targets.Num() > 0 && Targets[0].IsValid())
			{
				FVector Mid = FVector::ZeroVector;
				int32 Count = 0;
				for (const TWeakObjectPtr<AFPSRangeTarget>& Target : Targets)
				{
					if (Target.IsValid()) { Mid += Target->GetActorLocation(); ++Count; }
				}
				Mid /= FMath::Max(1, Count);
				Mid.Z += 80.0f;
				PC->SetControlRotation((Mid - PC->PlayerCameraManager->GetCameraLocation()).Rotation());
			}
		});
		After(End + 0.6f, [Screenshot]() { Screenshot(TEXT("after")); });
		After(End + 4.0f, [Screenshot, Targets]()
		{
			int32 StillDown = 0;
			for (const TWeakObjectPtr<AFPSRangeTarget>& Target : Targets)
			{
				if (Target.IsValid() && Target->IsKnockedDown()) { ++StillDown; }
			}
			UE_LOG(LogTemp, Log, TEXT("[RangeTest] after the reset delay: %d targets still down"), StillDown);
			Screenshot(TEXT("reset"));
		});
		After(End + 5.5f, []()
		{
			UE_LOG(LogTemp, Log, TEXT("[RangeTest] done, exiting"));
			FGenericPlatformMisc::RequestExit(false);
		});
	});
	UE_LOG(LogTemp, Log, TEXT("[RangeTest] scheduled, starting in %.1fs, screenshots in %s"), StartDelay, *ShotDir);
}

void AFPSMultiplayerGameMode::RunOperatorActionTest()
{
	float StartDelay = GIsEditor ? 8.0f : 14.0f;
	FParse::Value(FCommandLine::Get(), TEXT("NFActionTest="), StartDelay);
	UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), TEXT("DisableAllScreenMessages"));

	const FString ShotDir = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Screenshots"), TEXT("ActionTest"));
	FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*ShotDir);

	struct FStep
	{
		float Time;
		const TCHAR* Name;
		TFunction<void(AFPSCharacter&)> Action; // may be empty: screenshot only
		bool bScreenshot;
	};
	const TArray<FStep> Steps = {
		{ 0.0f,  TEXT("rifle_idle"),    nullptr, true },
		{ 1.0f,  TEXT("rifle_fire"),    [](AFPSCharacter& C) { C.StartFire(); }, false },
		{ 1.25f, TEXT("rifle_fire"),    nullptr, true },
		{ 1.5f,  TEXT("rifle_stop"),    [](AFPSCharacter& C) { C.StopFire(); }, false },
		{ 2.5f,  TEXT("rifle_reload"),  [](AFPSCharacter& C) { C.Reload(); }, false },
		{ 3.3f,  TEXT("rifle_reload"),  nullptr, true },
		{ 4.3f,  TEXT("rifle_reload_late"), nullptr, true },
		{ 7.0f,  TEXT("pistol_equip"),  [](AFPSCharacter& C) { C.EquipSidearm(); }, false },
		{ 7.35f, TEXT("pistol_equip"),  nullptr, true },
		{ 10.0f, TEXT("pistol_idle"),   nullptr, true },
		{ 11.0f, TEXT("pistol_fire"),   [](AFPSCharacter& C) { C.StartFire(); }, false },
		{ 11.15f, TEXT("pistol_fire"),  nullptr, true },
		{ 11.4f, TEXT("pistol_stop"),   [](AFPSCharacter& C) { C.StopFire(); }, false },
		{ 12.5f, TEXT("pistol_reload"), [](AFPSCharacter& C) { C.Reload(); }, false },
		{ 13.3f, TEXT("pistol_reload"), nullptr, true },
		{ 17.0f, TEXT("done"),          nullptr, false },
	};

	for (int32 Index = 0; Index < Steps.Num(); ++Index)
	{
		const FStep& Step = Steps[Index];
		const bool bLast = Index == Steps.Num() - 1;
		FTimerHandle Handle;
		GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this, Step, bLast, ShotDir]()
		{
			if (bLast)
			{
				UE_LOG(LogTemp, Log, TEXT("[ActionTest] done, exiting"));
				FGenericPlatformMisc::RequestExit(false);
				return;
			}
			APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
			AFPSCharacter* Char = PC ? Cast<AFPSCharacter>(PC->GetPawn()) : nullptr;
			if (!Char)
			{
				UE_LOG(LogTemp, Warning, TEXT("[ActionTest] %s: no local character"), Step.Name);
				return;
			}
			if (Step.Action)
			{
				Step.Action(*Char);
			}
			const AFPSWeaponBase* Weapon = Char->GetCurrentWeapon();
			const UAnimInstance* Anim = Char->GetMesh() ? Char->GetMesh()->GetAnimInstance() : nullptr;
			const UAnimMontage* Montage = Anim ? Anim->GetCurrentActiveMontage() : nullptr;
			UE_LOG(LogTemp, Log, TEXT("[ActionTest] %s%s: weapon %s ammo %d | montage %s"),
				Step.Name, Step.Action ? TEXT(" (action)") : TEXT(""),
				Weapon ? *Weapon->GetName() : TEXT("none"), Weapon ? Weapon->GetCurrentAmmo() : -1,
				Montage ? *Montage->GetName() : TEXT("none"));
			if (Step.bScreenshot)
			{
				const FString File = FPaths::Combine(ShotDir, FString::Printf(TEXT("%s.png"), Step.Name));
				FScreenshotRequest::RequestScreenshot(File, false, false);
			}
		}), StartDelay + Step.Time, false);
	}
	UE_LOG(LogTemp, Log, TEXT("[ActionTest] scheduled %d steps starting in %.1fs, screenshots in %s"), Steps.Num(), StartDelay, *ShotDir);
}

void AFPSMultiplayerGameMode::SpawnInitialTacticalBots()
{
	if (FParse::Param(FCommandLine::Get(), TEXT("TakeGameplayShot")))
	{
		UE_LOG(LogTemp, Warning, TEXT("[FPSGameMode] Suppressing bot spawns during -TakeGameplayShot automated capture."));
		return;
	}
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
	FVector BaseSpawnLoc = (ActiveTheaterIndex == 1) ? CoastalBravoSpawnPoints[SafeIdx] : BravoSpawnPoints[SafeIdx];
	FRotator SpawnRot = (ActiveTheaterIndex == 1) ? FRotator(0.0f, 90.0f, 0.0f) : FRotator(0.0f, 180.0f, 0.0f);
	if (ActiveTheaterIndex == RangeTheaterIndex)
	{
		TArray<AActor*> BotStarts;
		UGameplayStatics::GetAllActorsWithTag(this, FName("TeamBravo"), BotStarts);
		if (BotStarts.Num() > 0)
		{
			const AActor* Start = BotStarts[BotIndex % BotStarts.Num()];
			BaseSpawnLoc = Start->GetActorLocation();
			SpawnRot = Start->GetActorRotation();
		}
	}
	const FVector SpawnLoc = BaseSpawnLoc + FVector(FMath::FRandRange(-80.0f, 80.0f), FMath::FRandRange(-80.0f, 80.0f), 0.0f);

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	if (AFPSCharacter* BotChar = World->SpawnActor<AFPSCharacter>(BotPawnClass ? *BotPawnClass : AFPSCharacter::StaticClass(), SpawnLoc, SpawnRot, SpawnParams))
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

