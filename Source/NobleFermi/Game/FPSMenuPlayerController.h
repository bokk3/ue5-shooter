#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "FPSMenuPlayerController.generated.h"

class ACameraActor;

/**
 * PlayerController for Main Menu.
 * Handles mouse visibility, UI input mode, and 3D camera transitions between menu sections.
 */
UCLASS()
class NOBLEFERMI_API AFPSMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AFPSMenuPlayerController();

	UFUNCTION(BlueprintCallable, Category = "FPS Menu|Camera")
	void BlendToCamera(AActor* TargetCameraActor, float BlendTime = 1.0f);

protected:
	virtual void BeginPlay() override;
};
