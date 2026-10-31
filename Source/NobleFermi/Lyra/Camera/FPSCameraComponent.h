#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraComponent.h"
#include "Lyra/Camera/FPSCameraMode.h"
#include "FPSCameraComponent.generated.h"

/**
 * Lyra Multi-Mode Camera Component with Smooth Transition Blending
 */
UCLASS(BlueprintType, meta=(BlueprintSpawnableComponent))
class NOBLEFERMI_API UFPSCameraComponent : public UCameraComponent
{
	GENERATED_BODY()

public:
	UFPSCameraComponent();

	virtual void GetCameraView(float DeltaTime, FMinimalViewInfo& DesiredView) override;

	UFUNCTION(BlueprintCallable, Category = "Camera")
	void SetDesiredCameraMode(TSubclassOf<UFPSCameraMode> CameraModeClass);

	UFUNCTION(BlueprintPure, Category = "Camera")
	TSubclassOf<UFPSCameraMode> GetCurrentCameraModeClass() const { return CurrentCameraModeClass; }

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	TSubclassOf<UFPSCameraMode> DefaultCameraModeClass;

	UPROPERTY(Transient)
	TSubclassOf<UFPSCameraMode> CurrentCameraModeClass;

	UPROPERTY(Transient)
	TObjectPtr<UFPSCameraMode> CurrentCameraMode;

	UPROPERTY(Transient)
	TObjectPtr<UFPSCameraMode> PreviousCameraMode;

	float BlendProgress = 1.0f;
	float CurrentBlendDuration = 0.25f;

	FFPSCameraModeView CurrentView;
};
