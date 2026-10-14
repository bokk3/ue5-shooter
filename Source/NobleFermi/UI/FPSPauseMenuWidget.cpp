#include "UI/FPSPauseMenuWidget.h"
#include "UI/SettingsMenuWidget.h"
#include "Kismet/GameplayStatics.h"

UFPSPauseMenuWidget::UFPSPauseMenuWidget()
{
	bIsBackHandler = true;
}

void UFPSPauseMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UFPSPauseMenuWidget::ResumeGame()
{
	DeactivateWidget();

	APlayerController* PC = GetOwningPlayer();
	if (PC)
	{
		UGameplayStatics::SetGamePaused(this, false);
		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = false;
	}
}

void UFPSPauseMenuWidget::OpenSettings()
{
	if (SettingsWidgetClass)
	{
		// CommonUI activatable widget activation
	}
}

void UFPSPauseMenuWidget::LeaveMatchToMainMenu()
{
	UGameplayStatics::SetGamePaused(this, false);
	UGameplayStatics::OpenLevel(this, FName(TEXT("/Game/Maps/MainMenuMap")));
}
