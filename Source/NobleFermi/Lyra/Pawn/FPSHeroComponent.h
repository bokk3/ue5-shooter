#pragma once

#include "CoreMinimal.h"
#include "Components/PawnComponent.h"
#include "Lyra/Camera/FPSCameraMode.h"
#include "FPSHeroComponent.generated.h"

class UFPSPawnExtensionComponent;
class UFPSCameraComponent;
class UInputComponent;

/**
 * Lyra Hero Component for player input, camera mode coordination, and action binding
 */
UCLASS(BlueprintType, meta=(BlueprintSpawnableComponent))
class NOBLEFERMI_API UFPSHeroComponent : public UPawnComponent
{
	GENERATED_BODY()

public:
	UFPSHeroComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void InitializeComponent() override;

	void InitializePlayerInput(UInputComponent* PlayerInputComponent);

	UFUNCTION(BlueprintCallable, Category = "Hero")
	void SetAimingState(bool bIsAiming, bool bUseOptic = false);

	UFUNCTION(BlueprintPure, Category = "Hero")
	bool IsAiming() const { return bIsAimingState; }

protected:
	virtual void OnRegister() override;

	UFUNCTION()
	void OnPawnReadyToInitialize();

	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	TSubclassOf<UFPSCameraMode> DefaultCameraMode;

	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	TSubclassOf<UFPSCameraMode> ADSCameraMode;

	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	TSubclassOf<UFPSCameraMode> OpticCameraMode;

private:
	bool bIsAimingState = false;
	bool bIsOpticMode = false;

	void UpdateCameraMode();
};
