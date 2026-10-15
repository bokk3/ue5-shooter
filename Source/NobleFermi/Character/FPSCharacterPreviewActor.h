#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FPSCharacterPreviewActor.generated.h"

class USkeletalMeshComponent;
class UStaticMeshComponent;
class USceneComponent;
class UCameraComponent;
class USpotLightComponent;
class UPointLightComponent;
class UMaterialInstanceDynamic;

UENUM(BlueprintType)
enum class EOperatorCamo : uint8
{
	UrbanDigital UMETA(DisplayName = "Urban Digital"),
	WoodlandTactical UMETA(DisplayName = "Woodland Tactical"),
	DesertSpecOps UMETA(DisplayName = "Desert Spec Ops"),
	StealthBlackout UMETA(DisplayName = "Stealth Blackout")
};

UENUM(BlueprintType)
enum class EOperatorHeadgear : uint8
{
	BallisticHelmet UMETA(DisplayName = "Fast Ballistic Helmet"),
	NightVisionGoggles UMETA(DisplayName = "NVG Quad-Nods"),
	SpecOpsBalaclava UMETA(DisplayName = "Spec Ops Balaclava"),
	CommCap UMETA(DisplayName = "Tactical Headset & Cap")
};

UENUM(BlueprintType)
enum class EOperatorArmorVest : uint8
{
	HeavyPlateCarrier UMETA(DisplayName = "Level IV Heavy Carrier"),
	LightweightJPC UMETA(DisplayName = "Lightweight JPC Vest"),
	ReconChestRig UMETA(DisplayName = "Recon Tactical Rig")
};

/**
 * 3D Rotating Operator Character Model for Menu Customisation.
 * Displays adult human male soldier (180cm, 82kg anthropometry) aiming down sights
 * with 360-degree turntable rotation, tactical studio lighting, and dynamic camo customization.
 */
UCLASS()
class NOBLEFERMI_API AFPSCharacterPreviewActor : public AActor
{
	GENERATED_BODY()

public:
	AFPSCharacterPreviewActor();

	virtual void Tick(float DeltaTime) override;

	// Core Scene Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Preview|Scene")
	TObjectPtr<USceneComponent> RootScene;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Preview|Camera")
	TObjectPtr<UCameraComponent> PreviewCamera;

	// Studio Lighting Setup (ensures illumination even in empty levels)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Preview|Lighting")
	TObjectPtr<USpotLightComponent> StudioKeyLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Preview|Lighting")
	TObjectPtr<USpotLightComponent> StudioRimLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Preview|Lighting")
	TObjectPtr<UPointLightComponent> StudioFillLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Preview|Lighting")
	TObjectPtr<UPointLightComponent> StudioAmbientLight;

	// Turntable Pivot Root (rotates the character and platform)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Preview|Turntable")
	TObjectPtr<USceneComponent> TurntableRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Preview|Turntable")
	TObjectPtr<UStaticMeshComponent> PedestalMesh;

	// Optional Skeletal Mesh (if external artist asset is assigned)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Preview|Skeletal")
	TObjectPtr<USkeletalMeshComponent> BodyMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Preview|Skeletal")
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;

	// Anatomical Human Male Bone Structure Components (180 cm anthropometry)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Preview|Anatomy")
	TObjectPtr<UStaticMeshComponent> PelvisMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Preview|Anatomy")
	TObjectPtr<UStaticMeshComponent> SpineLowerMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Preview|Anatomy")
	TObjectPtr<UStaticMeshComponent> SpineUpperMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Preview|Anatomy")
	TObjectPtr<UStaticMeshComponent> ChestMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Preview|Anatomy")
	TObjectPtr<UStaticMeshComponent> NeckMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Preview|Anatomy")
	TObjectPtr<UStaticMeshComponent> HeadMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Preview|Gear")
	TObjectPtr<UStaticMeshComponent> HelmetMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Preview|Gear")
	TObjectPtr<UStaticMeshComponent> NVGMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Preview|Gear")
	TObjectPtr<UStaticMeshComponent> BalaclavaMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Preview|Gear")
	TObjectPtr<UStaticMeshComponent> CapMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Preview|Gear")
	TObjectPtr<UStaticMeshComponent> VestMesh;

	// Arms & Hands (Tactical ADS aiming posture)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Preview|Anatomy")
	TObjectPtr<UStaticMeshComponent> RightClavicleMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Preview|Anatomy")
	TObjectPtr<UStaticMeshComponent> RightUpperArmMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Preview|Anatomy")
	TObjectPtr<UStaticMeshComponent> RightForearmMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Preview|Anatomy")
	TObjectPtr<UStaticMeshComponent> RightHandMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Preview|Anatomy")
	TObjectPtr<UStaticMeshComponent> LeftClavicleMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Preview|Anatomy")
	TObjectPtr<UStaticMeshComponent> LeftUpperArmMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Preview|Anatomy")
	TObjectPtr<UStaticMeshComponent> LeftForearmMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Preview|Anatomy")
	TObjectPtr<UStaticMeshComponent> LeftHandMesh;

	// Legs & Footwear
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Preview|Anatomy")
	TObjectPtr<UStaticMeshComponent> LeftThighMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Preview|Anatomy")
	TObjectPtr<UStaticMeshComponent> LeftCalfMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Preview|Anatomy")
	TObjectPtr<UStaticMeshComponent> LeftBootMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Preview|Anatomy")
	TObjectPtr<UStaticMeshComponent> RightThighMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Preview|Anatomy")
	TObjectPtr<UStaticMeshComponent> RightCalfMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Preview|Anatomy")
	TObjectPtr<UStaticMeshComponent> RightBootMesh;

	// Weapon Subsystem
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Preview|Weapon")
	TObjectPtr<USceneComponent> WeaponRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Preview|Weapon")
	TObjectPtr<UStaticMeshComponent> WeaponReceiverMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Preview|Weapon")
	TObjectPtr<UStaticMeshComponent> WeaponBarrelMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Preview|Weapon")
	TObjectPtr<UStaticMeshComponent> WeaponOpticMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Preview|Weapon")
	TObjectPtr<UStaticMeshComponent> WeaponMagMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Preview|Weapon")
	TObjectPtr<UStaticMeshComponent> WeaponStockMesh;

	// Customization Controls
	UFUNCTION(BlueprintCallable, Category = "FPS Operator")
	void SetCamo(EOperatorCamo NewCamo);

	UFUNCTION(BlueprintCallable, Category = "FPS Operator")
	void SetHeadgear(EOperatorHeadgear NewHeadgear);

	UFUNCTION(BlueprintCallable, Category = "FPS Operator")
	void SetArmorVest(EOperatorArmorVest NewVest);

	UFUNCTION(BlueprintCallable, Category = "FPS Operator")
	void SetAimingStance(bool bInAiming);

	UFUNCTION(BlueprintCallable, Category = "FPS Operator")
	void SetRotationSpeed(float NewSpeed) { RotationSpeed = NewSpeed; }

	UFUNCTION(BlueprintCallable, Category = "FPS Operator")
	void ToggleRotation() { bIsRotating = !bIsRotating; }

	UFUNCTION(BlueprintCallable, Category = "FPS Operator")
	void ResetFacing();

	UFUNCTION(BlueprintPure, Category = "FPS Operator")
	bool IsAiming() const { return bIsAiming; }

	UFUNCTION(BlueprintPure, Category = "FPS Operator")
	bool IsRotating() const { return bIsRotating; }

	UFUNCTION(BlueprintPure, Category = "FPS Operator")
	EOperatorCamo GetCurrentCamo() const { return CurrentCamo; }

	UFUNCTION(BlueprintPure, Category = "FPS Operator")
	EOperatorHeadgear GetCurrentHeadgear() const { return CurrentHeadgear; }

	UFUNCTION(BlueprintPure, Category = "FPS Operator")
	EOperatorArmorVest GetCurrentVest() const { return CurrentVest; }

	// Configuration
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Operator|Turntable")
	float RotationSpeed = 18.0f; // degrees per second

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Operator|Turntable")
	bool bIsRotating = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Operator|Stance")
	bool bIsAiming = true;

protected:
	virtual void BeginPlay() override;

private:
	EOperatorCamo CurrentCamo = EOperatorCamo::UrbanDigital;
	EOperatorHeadgear CurrentHeadgear = EOperatorHeadgear::BallisticHelmet;
	EOperatorArmorVest CurrentVest = EOperatorArmorVest::HeavyPlateCarrier;

	void UpdateAimingPose(float DeltaTime);
	void ApplyCamoColors();
	void ApplyHeadgearVisibility();
	void ApplyVestConfiguration();

	float AimBreathTimer = 0.0f;
	FVector BaseWeaponLoc = FVector(52.0f, 10.0f, 140.0f);
	FRotator BaseWeaponRot = FRotator(0.0f, 0.0f, 0.0f);

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> UniformMaterials;

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> ArmorMaterials;

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> WeaponMaterials;
};
