#include "Game/FPSMenuPlayerController.h"
#include "Character/FPSCharacterPreviewActor.h"
#include "Kismet/GameplayStatics.h"

AFPSMenuPlayerController::AFPSMenuPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
	PrimaryActorTick.bCanEverTick = true;
}

void AFPSMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();

	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);

	if (AActor* PreviewActor = UGameplayStatics::GetActorOfClass(this, AFPSCharacterPreviewActor::StaticClass()))
	{
		SetViewTarget(PreviewActor);
		UE_LOG(LogTemp, Warning, TEXT("[PC] Initial SetViewTarget to PreviewActor: %s"), *PreviewActor->GetName());
	}
}

void AFPSMenuPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	AActor* CurrentTarget = GetViewTarget();
	if (!CurrentTarget || CurrentTarget == this)
	{
		if (AActor* PreviewActor = UGameplayStatics::GetActorOfClass(this, AFPSCharacterPreviewActor::StaticClass()))
		{
			SetViewTarget(PreviewActor);
		}
	}
}

void AFPSMenuPlayerController::BlendToCamera(AActor* TargetCameraActor, float BlendTime)
{
	if (TargetCameraActor)
	{
		SetViewTargetWithBlend(TargetCameraActor, BlendTime, VTBlend_EaseInOut, 2.0f, true);
	}
}
