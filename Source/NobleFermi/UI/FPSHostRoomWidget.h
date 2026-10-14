#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "Server/FPSServerSubsystem.h"
#include "FPSHostRoomWidget.generated.h"

/**
 * AAA FPS Host Room & Listen Server Widget
 * Allows players to host their own match rooms directly from the game client.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class NOBLEFERMI_API UFPSHostRoomWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UFPSHostRoomWidget();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Host Room")
	FFPSRoomSettings RoomSettings;

	UFUNCTION(BlueprintCallable, Category = "Host Room")
	void LaunchRoom();

	UFUNCTION(BlueprintCallable, Category = "Host Room")
	void SetRoomName(const FString& InName) { RoomSettings.RoomName = InName; }

	UFUNCTION(BlueprintCallable, Category = "Host Room")
	void SetSelectedMap(const FString& MapPath) { RoomSettings.MapPath = MapPath; }

	UFUNCTION(BlueprintCallable, Category = "Host Room")
	void SetMaxPlayers(int32 InMaxPlayers) { RoomSettings.MaxPlayers = InMaxPlayers; }

	UFUNCTION(BlueprintCallable, Category = "Host Room")
	void SetPassword(const FString& InPassword) { RoomSettings.Password = InPassword; }

	UFUNCTION(BlueprintCallable, Category = "Host Room")
	void SetIsLAN(bool bInIsLAN) { RoomSettings.bIsLAN = bInIsLAN; }
};
