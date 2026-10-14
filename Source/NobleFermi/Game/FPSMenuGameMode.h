#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "FPSMenuGameMode.generated.h"

class UMainMenuWidget;

/**
 * GameMode for AAA FPS Main Menu Level.
 * Spawns the main menu UI and configures viewport input.
 */
UCLASS()
class NOBLEFERMI_API AFPSMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AFPSMenuGameMode();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS Menu UI")
	TSubclassOf<UMainMenuWidget> MainMenuWidgetClass;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TObjectPtr<UMainMenuWidget> ActiveMenuWidget;
};
