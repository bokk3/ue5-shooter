#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "FPSCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class USkeletalMeshComponent;
class UFPSCharacterMovementComponent;
class UFPSHealthComponent;
class UFPSInventoryComponent;
class UFPSHUDWidget;

/**
 * AAA Tactical Character Class (Primary 3rd-Person Perspective)
 * Features dynamic over-the-shoulder tactical camera, camera lag, collision probe test,
 * shoulder swapping, anatomical SK_BaseMale operator body, PBR ripstop uniform,
 * full locomotion gait cycles, weapon socketing, and authoritative ragdoll physics.
 */
UCLASS()
class NOBLEFERMI_API AFPSCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AFPSCharacter(const FObjectInitializer& ObjectInitializer);

	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** Primary 3rd-Person Tactical Camera Boom */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	/** Primary 3rd-Person Tactical Follow Camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	/** First-Person Camera (Kept for secondary inspection / legacy fallback) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> FirstPersonCamera;

	/** First-Person Mesh (Hidden in 3rd person mode) */
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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UFPSBodyMechanicsComponent> BodyMechanicsComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UFPSHUDWidget> HUDWidgetClass;

	UFUNCTION(BlueprintPure, Category = "FPS Character|UI")
	UFPSHUDWidget* GetActiveHUDWidget() const { return ActiveHUDWidget; }

	UFUNCTION(BlueprintPure, Category = "FPS Character|Components")
	UFPSCharacterMovementComponent* GetFPSMovementComponent() const { return FPSMovementComponent; }

	UFUNCTION(BlueprintPure, Category = "FPS Character|Components")
	UFPSInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

	UFUNCTION(BlueprintPure, Category = "FPS Character|Components")
	class UFPSBodyMechanicsComponent* GetBodyMechanicsComponent() const { return BodyMechanicsComponent; }

	// Tactical Locomotion Animations
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS Character|Animations")
	TObjectPtr<class UAnimSequence> AnimIdleADS;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS Character|Animations")
	TObjectPtr<class UAnimSequence> AnimWalk;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS Character|Animations")
	TObjectPtr<class UAnimSequence> AnimRun;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS Character|Animations")
	TObjectPtr<class UAnimSequence> AnimSprint;

	// Tactical 3rd-Person Camera Settings
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS Character|Camera")
	float DefaultArmLength = 175.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS Character|Camera")
	float ADSArmLength = 110.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS Character|Camera")
	FVector RightShoulderOffset = FVector(0.0f, 48.0f, 25.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS Character|Camera")
	FVector LeftShoulderOffset = FVector(0.0f, -48.0f, 25.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS Character|Camera")
	FVector ADSRightShoulderOffset = FVector(10.0f, 38.0f, 18.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS Character|Camera")
	FVector ADSLeftShoulderOffset = FVector(10.0f, -38.0f, 18.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS Character|Camera")
	float DefaultFOV = 90.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS Character|Camera")
	float ADSFOV = 68.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Character|Camera")
	bool bIsRightShoulder = true;

	UFUNCTION(BlueprintCallable, Category = "FPS Character|Camera")
	void ToggleShoulderSwap();

	UFUNCTION(BlueprintPure, Category = "FPS Character|Camera")
	bool IsRightShoulder() const { return bIsRightShoulder; }

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

	// Death Handling
	UFUNCTION()
	void HandleOnDeath(AActor* KillerActor);

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

	// Tactical Camera & Locomotion Updates
	void UpdateThirdPersonCamera(float DeltaTime);
	void UpdateProceduralCameraBob(float DeltaTime);
	void UpdateWeaponSway(float DeltaTime);
	void UpdateLocomotionAnimation(float DeltaTime);

	UPROPERTY()
	TObjectPtr<class UAnimSequence> CurrentLocomotionAnim;

	float BobTimer = 0.0f;
	FVector DefaultCameraPos;
	FRotator LastControlRotation;
	FRotator CurrentWeaponSwayRot;
	bool bIsDead = false;
};
