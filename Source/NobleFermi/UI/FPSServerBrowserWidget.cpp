#include "UI/FPSServerBrowserWidget.h"

UFPSServerBrowserWidget::UFPSServerBrowserWidget()
{
	bIsBackHandler = true;
}

void UFPSServerBrowserWidget::NativeConstruct()
{
	Super::NativeConstruct();

	APlayerController* PC = GetOwningPlayer();
	if (PC && PC->GetGameInstance())
	{
		UFPSServerSubsystem* Subsystem = PC->GetGameInstance()->GetSubsystem<UFPSServerSubsystem>();
		if (Subsystem)
		{
			Subsystem->OnServerListUpdated.AddDynamic(this, &UFPSServerBrowserWidget::HandleServerListUpdated);
			Subsystem->RequestServerList();
		}
	}
}

void UFPSServerBrowserWidget::RefreshServers()
{
	APlayerController* PC = GetOwningPlayer();
	if (PC && PC->GetGameInstance())
	{
		UFPSServerSubsystem* Subsystem = PC->GetGameInstance()->GetSubsystem<UFPSServerSubsystem>();
		if (Subsystem)
		{
			Subsystem->RequestServerList();
		}
	}
}

void UFPSServerBrowserWidget::JoinServer(const FFPSServerInfo& ServerToJoin, const FString& Password)
{
	APlayerController* PC = GetOwningPlayer();
	if (PC && PC->GetGameInstance())
	{
		UFPSServerSubsystem* Subsystem = PC->GetGameInstance()->GetSubsystem<UFPSServerSubsystem>();
		if (Subsystem)
		{
			Subsystem->ConnectToServerByIP(PC, ServerToJoin.IPAddress, 7777, Password);
		}
	}
}

void UFPSServerBrowserWidget::DirectConnectIP(const FString& IPAddress, int32 Port, const FString& Password)
{
	APlayerController* PC = GetOwningPlayer();
	if (PC && PC->GetGameInstance())
	{
		UFPSServerSubsystem* Subsystem = PC->GetGameInstance()->GetSubsystem<UFPSServerSubsystem>();
		if (Subsystem)
		{
			Subsystem->ConnectToServerByIP(PC, IPAddress, Port, Password);
		}
	}
}

void UFPSServerBrowserWidget::HandleServerListUpdated(const TArray<FFPSServerInfo>& Servers)
{
	OnServerListReceived(Servers);
}
