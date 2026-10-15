#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "FPSMenuGameMode.generated.h"

class UMainMenuWidget;
class AFPSCharacterPreviewActor;

/**
 * GameMode for AAA FPS Main Menu Level.
 * Spawns the 3D rotating character preview actor, sets studio camera view target,
 * and launches the Slate interactive main menu HUD.
 */
UCLASS()
class NOBLEFERMI_API AFPSMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AFPSMenuGameMode();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS Menu UI")
	TSubclassOf<UMainMenuWidget> MainMenuWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS Menu 3D")
	TSubclassOf<AFPSCharacterPreviewActor> PreviewActorClass;

	UFUNCTION(BlueprintPure, Category = "FPS Menu 3D")
	AFPSCharacterPreviewActor* GetPreviewActor() const { return ActivePreviewActor; }

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TObjectPtr<UMainMenuWidget> ActiveMenuWidget;

	UPROPERTY()
	TObjectPtr<AFPSCharacterPreviewActor> ActivePreviewActor;
};
