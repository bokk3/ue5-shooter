#include "UI/MainMenuWidget.h"
#include "UI/SettingsMenuWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

UMainMenuWidget::UMainMenuWidget()
{
	bIsBackHandler = false;
}

void UMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (MenuMusicTrack)
	{
		UGameplayStatics::PlaySound2D(this, MenuMusicTrack);
	}
}

void UMainMenuWidget::NavigateToSection(EFPSMenuSection Section)
{
	CurrentSection = Section;

	if (TransitionSound)
	{
		UGameplayStatics::PlaySound2D(this, TransitionSound);
	}

	OnSectionChanged.Broadcast(Section);
}

void UMainMenuWidget::StartMatchmaking()
{
	if (TransitionSound)
	{
		UGameplayStatics::PlaySound2D(this, TransitionSound);
	}

	OnMatchmakingTriggered();
}

void UMainMenuWidget::OpenSettings()
{
	NavigateToSection(EFPSMenuSection::Settings);
}

void UMainMenuWidget::RequestQuit()
{
	OnShowQuitConfirmation();
}

TOptional<FNavigationReply> UMainMenuWidget::NativeOnNavigation(const FGeometry& InGeometry, const FNavigationEvent& InNavigationEvent)
{
	return Super::NativeOnNavigation(InGeometry, InNavigationEvent);
}
