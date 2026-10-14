#include "UI/FPSHostRoomWidget.h"
#include "Kismet/GameplayStatics.h"

UFPSHostRoomWidget::UFPSHostRoomWidget()
{
	bIsBackHandler = true;
	RoomSettings.RoomName = TEXT("Boris's Tactical Lobby");
	RoomSettings.MapPath = TEXT("/Game/Maps/TDM_Crash");
	RoomSettings.MaxPlayers = 12;
}

void UFPSHostRoomWidget::LaunchRoom()
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC) return;

	UGameInstance* GI = PC->GetGameInstance();
	if (!GI) return;

	UFPSServerSubsystem* ServerSubsystem = GI->GetSubsystem<UFPSServerSubsystem>();
	if (ServerSubsystem)
	{
		ServerSubsystem->HostClientRoom(PC, RoomSettings);
	}
}
