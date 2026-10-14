#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FPSServerSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FFPSRoomSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Host Room")
	FString RoomName = TEXT("Noble Fermi Match");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Host Room")
	FString MapPath = TEXT("/Game/Maps/TDM_Crash");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Host Room")
	int32 MaxPlayers = 12;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Host Room")
	FString Password = TEXT("");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Host Room")
	bool bIsLAN = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Host Room")
	int32 ServerTickRate = 64; // 64Hz or 128Hz competitive tickrate
};

USTRUCT(BlueprintType)
struct FFPSServerInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Server Info")
	FString ServerName = TEXT("Official Dedicated Server #1");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Server Info")
	FString MapName = TEXT("Crash");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Server Info")
	FString GameModeName = TEXT("Team Deathmatch");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Server Info")
	int32 CurrentPlayers = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Server Info")
	int32 MaxPlayers = 16;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Server Info")
	int32 Ping = 22;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Server Info")
	FString IPAddress = TEXT("127.0.0.1:7777");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Server Info")
	bool bHasPassword = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Server Info")
	bool bIsDedicated = true;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnServerListUpdatedSignature, const TArray<FFPSServerInfo>&, Servers);

/**
 * AAA FPS Server & Network Subsystem
 * Manages VPS Dedicated Server command-line configuration, RCON administration,
 * Client listen-server room creation, and Direct IP connecting.
 */
UCLASS()
class NOBLEFERMI_API UFPSServerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UPROPERTY(BlueprintAssignable, Category = "FPS Server")
	FOnServerListUpdatedSignature OnServerListUpdated;

	// Client Room Hosting (Listen Server)
	UFUNCTION(BlueprintCallable, Category = "FPS Server|Hosting")
	bool HostClientRoom(APlayerController* HostController, const FFPSRoomSettings& Settings);

	// Client Join Functions
	UFUNCTION(BlueprintCallable, Category = "FPS Server|Joining")
	void ConnectToServerByIP(APlayerController* PlayerController, const FString& IPAddress, int32 Port = 7777, const FString& Password = TEXT(""));

	UFUNCTION(BlueprintCallable, Category = "FPS Server|Browser")
	void RequestServerList();

	// Dedicated Server VPS CLI & RCON API
	UFUNCTION(BlueprintCallable, Category = "FPS Server|Dedicated")
	void InitializeDedicatedServerFromCLI();

	UFUNCTION(BlueprintCallable, Category = "FPS Server|RCON")
	FString ExecuteRconCommand(const FString& CommandString, const FString& AdminPassword);

	UFUNCTION(BlueprintPure, Category = "FPS Server|Dedicated")
	bool IsDedicatedServer() const;

	UPROPERTY(BlueprintReadOnly, Category = "FPS Server|Dedicated")
	FFPSRoomSettings DedicatedSettings;

private:
	FString RconPassword = TEXT("admin123");
	TArray<FFPSServerInfo> CachedServerList;
	void SetupDefaultServerDirectory();
};
