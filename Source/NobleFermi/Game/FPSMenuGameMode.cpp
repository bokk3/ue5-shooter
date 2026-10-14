#include "Game/FPSMenuGameMode.h"
#include "Game/FPSMenuPlayerController.h"
#include "UI/MainMenuWidget.h"
#include "Kismet/GameplayStatics.h"

AFPSMenuGameMode::AFPSMenuGameMode()
{
	PlayerControllerClass = AFPSMenuPlayerController::StaticClass();
	DefaultPawnClass = nullptr;
}

void AFPSMenuGameMode::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (PC && MainMenuWidgetClass)
	{
		ActiveMenuWidget = CreateWidget<UMainMenuWidget>(PC, MainMenuWidgetClass);
		if (ActiveMenuWidget)
		{
			ActiveMenuWidget->AddToViewport(0);
		}
	}
}
