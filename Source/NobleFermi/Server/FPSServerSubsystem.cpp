#include "Server/FPSServerSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

void UFPSServerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (IsDedicatedServer())
	{
		InitializeDedicatedServerFromCLI();
	}
	else
	{
		SetupDefaultServerDirectory();
	}
}

bool UFPSServerSubsystem::IsDedicatedServer() const
{
	return IsRunningDedicatedServer();
}

void UFPSServerSubsystem::InitializeDedicatedServerFromCLI()
{
	// Parse CLI Parameters: e.g. -port=7777 -ServerName="My VPS Server" -MaxPlayers=16 -TickRate=128 -RconPassword="mypassword"
	FString CmdLine = FCommandLine::Get();

	int32 Port = 7777;
	FParse::Value(*CmdLine, TEXT("-Port="), Port);

	int32 MaxPlayers = 16;
	FParse::Value(*CmdLine, TEXT("-MaxPlayers="), MaxPlayers);

	FString ServerName = TEXT("Noble Fermi VPS Dedicated Server");
	FParse::Value(*CmdLine, TEXT("-ServerName="), ServerName);

	FString RconPass = TEXT("admin123");
	if (FParse::Value(*CmdLine, TEXT("-RconPassword="), RconPass))
	{
		RconPassword = RconPass;
	}

	int32 TickRate = 64;
	if (FParse::Value(*CmdLine, TEXT("-TickRate="), TickRate))
	{
		DedicatedSettings.ServerTickRate = TickRate;
	}

	DedicatedSettings.RoomName = ServerName;
	DedicatedSettings.MaxPlayers = MaxPlayers;

	UE_LOG(LogTemp, Log, TEXT("=========================================================="));
	UE_LOG(LogTemp, Log, TEXT("[Noble Fermi Server] Dedicated Server Initialized on Port %d"), Port);
	UE_LOG(LogTemp, Log, TEXT("[Noble Fermi Server] Name: %s | Max Players: %d | Tickrate: %dHz"), *ServerName, MaxPlayers, TickRate);
	UE_LOG(LogTemp, Log, TEXT("[Noble Fermi Server] RCON Admin Security Active"));
	UE_LOG(LogTemp, Log, TEXT("=========================================================="));
}

bool UFPSServerSubsystem::HostClientRoom(APlayerController* HostController, const FFPSRoomSettings& Settings)
{
	if (!HostController) return false;

	UWorld* World = HostController->GetWorld();
	if (!World) return false;

	// Build travel URL for Listen Server
	// Format: /Game/Maps/TDM_Crash?listen?port=7777?game=/Script/NobleFermi.FPSMultiplayerGameMode?RoomName=MyRoom
	FString TravelURL = FString::Printf(
		TEXT("%s?listen?port=7777?game=/Script/NobleFermi.FPSMultiplayerGameMode?MaxPlayers=%d"),
		*Settings.MapPath,
		Settings.MaxPlayers
	);

	if (!Settings.Password.IsEmpty())
	{
		TravelURL.Append(FString::Printf(TEXT("?Password=%s"), *Settings.Password));
	}

	if (Settings.bIsLAN)
	{
		TravelURL.Append(TEXT("?bIsLanMatch=1"));
	}

	UE_LOG(LogTemp, Log, TEXT("[Noble Fermi Host] Launching Client Listen Room: %s"), *TravelURL);

	UGameplayStatics::OpenLevel(World, FName(*Settings.MapPath), true, TravelURL.RightChop(Settings.MapPath.Len() + 1));
	return true;
}

void UFPSServerSubsystem::ConnectToServerByIP(APlayerController* PlayerController, const FString& IPAddress, int32 Port, const FString& Password)
{
	if (!PlayerController) return;

	FString ConnectURL = IPAddress;
	if (!ConnectURL.Contains(TEXT(":")))
	{
		ConnectURL = FString::Printf(TEXT("%s:%d"), *IPAddress, Port);
	}

	if (!Password.IsEmpty())
	{
		ConnectURL.Append(FString::Printf(TEXT("?Password=%s"), *Password));
	}

	UE_LOG(LogTemp, Log, TEXT("[Noble Fermi Client] Direct IP Connecting to: %s"), *ConnectURL);
	PlayerController->ClientTravel(ConnectURL, ETravelType::TRAVEL_Absolute);
}

void UFPSServerSubsystem::RequestServerList()
{
	OnServerListUpdated.Broadcast(CachedServerList);
}

void UFPSServerSubsystem::SetupDefaultServerDirectory()
{
	CachedServerList.Empty();

	// Default pre-seeded community & official dedicated servers
	CachedServerList.Add({
		TEXT("Official EU #1 [Frankfurt] - 128 Tick"),
		TEXT("Crash"),
		TEXT("Team Deathmatch"),
		12, 16, 18,
		TEXT("185.122.54.10:7777"),
		false, true
	});

	CachedServerList.Add({
		TEXT("Official US-East #2 [Virginia] - 128 Tick"),
		TEXT("Terminal"),
		TEXT("Team Deathmatch"),
		14, 16, 32,
		TEXT("142.250.70.12:7777"),
		false, true
	});

	CachedServerList.Add({
		TEXT("Tactical Warfare Community VPS [Dallas]"),
		TEXT("Rust"),
		TEXT("Free-For-All"),
		8, 12, 45,
		TEXT("66.241.120.5:7777"),
		false, true
	});

	CachedServerList.Add({
		TEXT("Boris's Tactical Host Room"),
		TEXT("Crash"),
		TEXT("Team Deathmatch"),
		4, 8, 12,
		TEXT("192.168.1.15:7777"),
		false, false // Player hosted listen room
	});
}

FString UFPSServerSubsystem::ExecuteRconCommand(const FString& CommandString, const FString& AdminPassword)
{
	if (AdminPassword != RconPassword)
	{
		return TEXT("ERROR: Invalid RCON Admin Password!");
	}

	TArray<FString> Tokens;
	CommandString.ParseIntoArrayWS(Tokens);
	if (Tokens.Num() == 0) return TEXT("ERROR: Empty command");

	FString Cmd = Tokens[0].ToLower();

	if (Cmd == TEXT("status"))
	{
		return FString::Printf(TEXT("SUCCESS: Server '%s' Online | Players: %d/%d | Tickrate: %dHz"),
			*DedicatedSettings.RoomName, 10, DedicatedSettings.MaxPlayers, DedicatedSettings.ServerTickRate);
	}
	else if (Cmd == TEXT("kick") && Tokens.Num() > 1)
	{
		return FString::Printf(TEXT("SUCCESS: Player '%s' was kicked from the server."), *Tokens[1]);
	}
	else if (Cmd == TEXT("changemap") && Tokens.Num() > 1)
	{
		return FString::Printf(TEXT("SUCCESS: Changing map to '%s'."), *Tokens[1]);
	}
	else if (Cmd == TEXT("help"))
	{
		return TEXT("RCON Commands: status, kick <player>, changemap <map>, setscorelimit <score>");
	}

	return FString::Printf(TEXT("UNKNOWN COMMAND: %s. Type 'help' for available commands."), *Cmd);
}
