#include "Game/FPSMenuPlayerController.h"

AFPSMenuPlayerController::AFPSMenuPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
}

void AFPSMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();

	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
}

void AFPSMenuPlayerController::BlendToCamera(AActor* TargetCameraActor, float BlendTime)
{
	if (TargetCameraActor)
	{
		SetViewTargetWithBlend(TargetCameraActor, BlendTime, VTBlend_EaseInOut, 2.0f, true);
	}
}
