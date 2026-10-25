#include "Game/FPSMenuGameMode.h"
#include "Game/FPSMenuPlayerController.h"
#include "Game/FPSMenuHangarStage.h"
#include "Character/FPSCharacterPreviewActor.h"
#include "UI/MainMenuWidget.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/CommandLine.h"
#include "TimerManager.h"
#include "Engine/Engine.h"
#include "UnrealClient.h"
#include "HAL/PlatformFileManager.h"
AFPSMenuGameMode::AFPSMenuGameMode()
{
	PlayerControllerClass = AFPSMenuPlayerController::StaticClass();
	DefaultPawnClass = nullptr;
	MainMenuWidgetClass = UMainMenuWidget::StaticClass();
	PreviewActorClass = AFPSCharacterPreviewActor::StaticClass();
	HangarStageClass = AFPSMenuHangarStage::StaticClass();
}
void AFPSMenuGameMode::BeginPlay()
{
	Super::BeginPlay();
	// Enforce baseline iGPU optimization on fresh startup if no user config exists yet
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
			UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), TEXT("r.DepthOfFieldQuality 0"));
			UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), TEXT("r.ScreenPercentage 70"));
			UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), TEXT("t.MaxFPS 60"));
		}
	}
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	// 1. Spawn 3D Rotating Character Preview Actor (Human Male 180cm, ADS stance, turntable, studio lights)
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	UClass* ClassToSpawn = PreviewActorClass ? PreviewActorClass.Get() : AFPSCharacterPreviewActor::StaticClass();
	ActivePreviewActor = World->SpawnActor<AFPSCharacterPreviewActor>(
		ClassToSpawn,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		SpawnParams
	);
	UE_LOG(LogTemp, Warning, TEXT("[GAMEMODE] Spawned ActivePreviewActor: %s"), ActivePreviewActor ? *ActivePreviewActor->GetName() : TEXT("FAILED"));
	// 1.5. Spawn 3D Industrial Hangar Diorama & Rain-Glass Optics Stage around the Operator Turntable
	UClass* HangarToSpawn = HangarStageClass ? HangarStageClass.Get() : AFPSMenuHangarStage::StaticClass();
	ActiveHangarStage = World->SpawnActor<AFPSMenuHangarStage>(
		HangarToSpawn,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		SpawnParams
	);
	UE_LOG(LogTemp, Warning, TEXT("[GAMEMODE] Spawned ActiveHangarStage: %s"), ActiveHangarStage ? *ActiveHangarStage->GetName() : TEXT("FAILED"));
	if (ActiveHangarStage && ActivePreviewActor)
	{
		ActiveHangarStage->ConfigurePreviewActorAndCamera(ActivePreviewActor);
	}
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (PC)
	{
		// 2. Set camera view target to the preview actor's studio camera
		if (ActivePreviewActor)
		{
			PC->SetViewTarget(ActivePreviewActor);
		}
		// 3. Create and mount Main Menu Slate UI
		if (MainMenuWidgetClass)
		{
			ActiveMenuWidget = CreateWidget<UMainMenuWidget>(PC, MainMenuWidgetClass);
			if (ActiveMenuWidget)
			{
				if (ActivePreviewActor)
				{
					ActiveMenuWidget->SetPreviewActor(ActivePreviewActor);
				}
				ActiveMenuWidget->AddToViewport(0);
			}
		}
	}

	// Auto screenshot verification if -TakeMenuShot is passed on command line
	if (FParse::Param(FCommandLine::Get(), TEXT("TakeMenuShot")))
	{
		FTimerHandle ScreenshotTimerHandle;
		GetWorldTimerManager().SetTimer(ScreenshotTimerHandle, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			const FString ScreenshotDir = FPaths::Combine(FPaths::ProjectDir(), TEXT("Docs"), TEXT("Screenshots"));
			IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
			PlatformFile.CreateDirectoryTree(*ScreenshotDir);

			// Diagnostic dump right before screenshot
			if (ActivePreviewActor)
			{
				UStaticMeshComponent* Op = ActivePreviewActor->OperatorMesh;
				FBoxSphereBounds B = Op ? Op->CalcBounds(Op->GetComponentTransform()) : FBoxSphereBounds();
				UE_LOG(LogTemp, Warning, TEXT("[SCREENSHOT_DIAG] ActivePreviewActor=%s, OperatorMesh valid=%d, Asset=%s, Vis=%d, Hidden=%d, Radius=%.1f, Loc=%s"),
					*ActivePreviewActor->GetName(),
					Op != nullptr,
					(Op && Op->GetStaticMesh()) ? *Op->GetStaticMesh()->GetName() : TEXT("NONE"),
					Op ? Op->IsVisible() : 0,
					Op ? Op->bHiddenInGame : 0,
					B.SphereRadius,
					Op ? *Op->GetComponentLocation().ToString() : TEXT("NONE"));

				UStaticMeshComponent* Ped = ActivePreviewActor->PedestalMesh;
				UE_LOG(LogTemp, Warning, TEXT("[SCREENSHOT_DIAG] PedestalMesh valid=%d, Asset=%s, Vis=%d, Hidden=%d"),
					Ped != nullptr,
					(Ped && Ped->GetStaticMesh()) ? *Ped->GetStaticMesh()->GetName() : TEXT("NONE"),
					Ped ? Ped->IsVisible() : 0,
					Ped ? Ped->bHiddenInGame : 0);
			}
			if (ActiveHangarStage)
			{
				UStaticMeshComponent* Floor = ActiveHangarStage->WetFloorPlane;
				UE_LOG(LogTemp, Warning, TEXT("[SCREENSHOT_DIAG] WetFloorPlane valid=%d, Asset=%s, Vis=%d, Loc=%s"),
					Floor != nullptr,
					(Floor && Floor->GetStaticMesh()) ? *Floor->GetStaticMesh()->GetName() : TEXT("NONE"),
					Floor ? Floor->IsVisible() : 0,
					Floor ? *Floor->GetComponentLocation().ToString() : TEXT("NONE"));
			}
			if (APlayerController* PCLocal = UGameplayStatics::GetPlayerController(this, 0))
			{
				UE_LOG(LogTemp, Warning, TEXT("[SCREENSHOT_DIAG] ViewTarget=%s, CameraMgr=%s, CamLoc=%s, CamRot=%s, CamFOV=%.1f"),
					PCLocal->GetViewTarget() ? *PCLocal->GetViewTarget()->GetName() : TEXT("NONE"),
					PCLocal->PlayerCameraManager ? *PCLocal->PlayerCameraManager->GetName() : TEXT("NONE"),
					PCLocal->PlayerCameraManager ? *PCLocal->PlayerCameraManager->GetCameraLocation().ToString() : TEXT("NONE"),
					PCLocal->PlayerCameraManager ? *PCLocal->PlayerCameraManager->GetCameraRotation().ToString() : TEXT("NONE"),
					PCLocal->PlayerCameraManager ? PCLocal->PlayerCameraManager->GetFOVAngle() : 0.0f);
			}

			const FString ScreenshotFile = FPaths::Combine(ScreenshotDir, TEXT("menu_real_actual.png"));
			FScreenshotRequest::RequestScreenshot(ScreenshotFile, true, false);
			UE_LOG(LogTemp, Warning, TEXT("[SCREENSHOT] Real in-engine screenshot queued to: %s"), *ScreenshotFile);

			FScreenshotRequest::OnScreenshotRequestProcessed().AddLambda([]()
			{
				UE_LOG(LogTemp, Warning, TEXT("[SCREENSHOT] Real in-engine screenshot successfully written to disk! Clean exit in 1s."));
				FGenericPlatformMisc::RequestExit(false);
			});

			FTimerHandle ExitTimerHandle;
			GetWorldTimerManager().SetTimer(ExitTimerHandle, FTimerDelegate::CreateWeakLambda(this, []()
			{
				UE_LOG(LogTemp, Warning, TEXT("[SCREENSHOT] Safety timeout reached, requesting exit..."));
				FGenericPlatformMisc::RequestExit(false);
			}), 6.0f, false);
		}), 7.0f, false);
	}
}
