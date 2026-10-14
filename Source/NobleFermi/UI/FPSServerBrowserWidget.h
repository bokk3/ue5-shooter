#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "Server/FPSServerSubsystem.h"
#include "FPSServerBrowserWidget.generated.h"

/**
 * AAA FPS Server Browser & Direct IP Connect Widget
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class NOBLEFERMI_API UFPSServerBrowserWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UFPSServerBrowserWidget();

	UFUNCTION(BlueprintCallable, Category = "Server Browser")
	void RefreshServers();

	UFUNCTION(BlueprintCallable, Category = "Server Browser")
	void JoinServer(const FFPSServerInfo& ServerToJoin, const FString& Password = TEXT(""));

	UFUNCTION(BlueprintCallable, Category = "Server Browser")
	void DirectConnectIP(const FString& IPAddress, int32 Port = 7777, const FString& Password = TEXT(""));

	UFUNCTION(BlueprintImplementableEvent, Category = "Server Browser")
	void OnServerListReceived(const TArray<FFPSServerInfo>& Servers);

protected:
	virtual void NativeConstruct() override;

private:
	UFUNCTION()
	void HandleServerListUpdated(const TArray<FFPSServerInfo>& Servers);
};
