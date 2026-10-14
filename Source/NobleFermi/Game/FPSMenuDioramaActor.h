#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Camera/CameraComponent.h"
#include "FPSMenuDioramaActor.generated.h"

class USkeletalMeshComponent;
class USpotLightComponent;

/**
 * AAA 3D Menu Scene Director Actor.
 * Placed in MainMenuMap to coordinate 3D cameras, operator models, weapon turntables, and lighting.
 */
UCLASS()
class NOBLEFERMI_API AFPSMenuDioramaActor : public AActor
{
	GENERATED_BODY()

public:
	AFPSMenuDioramaActor();

	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Scene|Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Scene|Cameras")
	TObjectPtr<UCameraComponent> MainCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Scene|Cameras")
	TObjectPtr<UCameraComponent> LoadoutCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Scene|Cameras")
	TObjectPtr<UCameraComponent> SettingsCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Scene|Pedestal")
	TObjectPtr<USceneComponent> WeaponTurntableRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Scene|Lighting")
	TObjectPtr<USpotLightComponent> WeaponKeyLight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scene|Inspection")
	bool bAutoRotateWeapon = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scene|Inspection")
	float TurntableRotationSpeed = 15.0f;

	UFUNCTION(BlueprintCallable, Category = "Scene|Inspection")
	void AddWeaponManualYaw(float DeltaYaw);

	UFUNCTION(BlueprintCallable, Category = "Scene|Transition")
	void SetLightingMood(bool bFocusOnWeapon);

protected:
	virtual void BeginPlay() override;
};
