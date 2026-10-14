#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "FPSCharacter.generated.h"

class UCameraComponent;
class USkeletalMeshComponent;
class UFPSCharacterMovementComponent;
class UFPSHealthComponent;
class UFPSInventoryComponent;
class UFPSHUDWidget;

/**
 * AAA First-Person Character Class
 * Coordinates procedural weapon sway, camera bobbing, health/shield vitals, inventory, and Enhanced Input.
 */
UCLASS()
class NOBLEFERMI_API AFPSCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AFPSCharacter(const FObjectInitializer& ObjectInitializer);

	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> FirstPersonCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	TObjectPtr<USkeletalMeshComponent> Mesh1P;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UFPSCharacterMovementComponent> FPSMovementComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UFPSHealthComponent> HealthComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UFPSInventoryComponent> InventoryComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UFPSEquipmentComponent> EquipmentComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UFPSHUDWidget> HUDWidgetClass;

	// Actions
	UFUNCTION(BlueprintCallable, Category = "FPS Character|Actions")
	void StartCookingLethal();

	UFUNCTION(BlueprintCallable, Category = "FPS Character|Actions")
	void ReleaseLethal();

	UFUNCTION(BlueprintCallable, Category = "FPS Character|Actions")
	void ThrowTactical();
	UFUNCTION(BlueprintCallable, Category = "FPS Character|Actions")
	void StartFire();

	UFUNCTION(BlueprintCallable, Category = "FPS Character|Actions")
	void StopFire();

	UFUNCTION(BlueprintCallable, Category = "FPS Character|Actions")
	void StartADS();

	UFUNCTION(BlueprintCallable, Category = "FPS Character|Actions")
	void StopADS();

	UFUNCTION(BlueprintCallable, Category = "FPS Character|Actions")
	void Reload();

	UFUNCTION(BlueprintCallable, Category = "FPS Character|Actions")
	void StartSprint();

	UFUNCTION(BlueprintCallable, Category = "FPS Character|Actions")
	void StopSprint();

	UFUNCTION(BlueprintCallable, Category = "FPS Character|Actions")
	void StartCrouchOrSlide();

	UFUNCTION(BlueprintCallable, Category = "FPS Character|Actions")
	void StopCrouchOrSlide();

	// Input Handlers
	void MoveForward(float Value);
	void MoveRight(float Value);
	void Turn(float Rate);
	void LookUp(float Rate);

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TObjectPtr<UFPSHUDWidget> ActiveHUDWidget;

	// Procedural Sway & Bobbing
	void UpdateProceduralCameraBob(float DeltaTime);
	void UpdateWeaponSway(float DeltaTime);

	float BobTimer = 0.0f;
	FVector DefaultCameraPos;
	FRotator LastControlRotation;
	FRotator CurrentWeaponSwayRot;
};
