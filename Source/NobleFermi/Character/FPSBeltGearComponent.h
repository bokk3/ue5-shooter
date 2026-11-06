#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProceduralMeshComponent.h"
#include "FPSBeltGearComponent.generated.h"

class USkeletalMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UTexture2D;

UENUM(BlueprintType)
enum class EFPSBeltItemType : uint8
{
	Mag1        UMETA(DisplayName = "Primary STANAG Fast-Mag (Front-Left 1)"),
	Mag2        UMETA(DisplayName = "Secondary STANAG Fast-Mag (Front-Left 2)"),
	FragGrenade UMETA(DisplayName = "M67 Fragmentation Grenade (Front-Right)"),
	Flashbang   UMETA(DisplayName = "M84 Stun / Flashbang Grenade (Right Hip)")
};

/**
 * Modular 3D Tactical Belt Kit Component
 * Attaches individually addressable, high-detail procedural 3D belt equipment to the operator's pelvis:
 *  - BeltMagComp_1 & BeltMagComp_2: Curved 30-round STANAG magazines in Kydex open-top fast-mag pouches
 *  - BeltFragComp: M67 spherical olive-drab fragmentation grenade with yellow HE band, spoon & pull-ring
 *  - BeltFlashComp: M84 cylindrical perforated steel stun grenade with hexagonal blast caps & blue band
 * Also manages live hand-held proxy meshes (HeldMagComp, HeldGrenadeComp) during belt-grab reload & throw animations.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class NOBLEFERMI_API UFPSBeltGearComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFPSBeltGearComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Initializes and builds all 4 belt items and attaches them to the operator's skeletal mesh */
	UFUNCTION(BlueprintCallable, Category = "FPS Belt Gear")
	void InitializeBeltKit(USkeletalMeshComponent* InBodyMesh);

	/** Updates world transforms of all belt items and any active hand-held proxy mesh */
	UFUNCTION(BlueprintCallable, Category = "FPS Belt Gear")
	void UpdateBeltTransforms();

	/** Returns the world-space location of the specified belt item's grab point */
	UFUNCTION(BlueprintPure, Category = "FPS Belt Gear")
	FVector GetBeltItemWorldLocation(EFPSBeltItemType Item) const;

	/** Returns the component-space location (relative to TargetBodyMesh) of the specified belt item's grab point */
	UFUNCTION(BlueprintPure, Category = "FPS Belt Gear")
	FVector GetBeltItemComponentSpaceLocation(EFPSBeltItemType Item) const;

	/** Drives reload belt-pull visibility: hides magazine in BeltMagComp_1 while attaching HeldMagComp to hand_l */
	UFUNCTION(BlueprintCallable, Category = "FPS Belt Gear")
	void UpdateReloadBeltGrabState(bool bIsReloading, float ReloadAlpha, bool bHasReserveAmmo = true);

	/** Drives grenade throw belt-pull visibility: hides belt grenade while attaching HeldGrenadeComp to throwing hand */
	UFUNCTION(BlueprintCallable, Category = "FPS Belt Gear")
	void UpdateThrowBeltGrabState(bool bIsThrowing, float ThrowAlpha, EFPSBeltItemType ThrowItem, bool bHasFragRemaining = true, bool bHasFlashRemaining = true);

	/** Sets visibility of all belt components (e.g., for first-person or death state) */
	UFUNCTION(BlueprintCallable, Category = "FPS Belt Gear")
	void SetBeltVisibility(bool bVisible);

	// Individually Addressable 3D Belt Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Belt Gear|Components")
	TObjectPtr<UProceduralMeshComponent> BeltMagComp_1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Belt Gear|Components")
	TObjectPtr<UProceduralMeshComponent> BeltMagComp_2;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Belt Gear|Components")
	TObjectPtr<UProceduralMeshComponent> BeltFragComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Belt Gear|Components")
	TObjectPtr<UProceduralMeshComponent> BeltFlashComp;

	// Live Hand-Held Proxy Components during Reload / Throw
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Belt Gear|Hand Proxies")
	TObjectPtr<UProceduralMeshComponent> HeldMagComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Belt Gear|Hand Proxies")
	TObjectPtr<UProceduralMeshComponent> HeldFragComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Belt Gear|Hand Proxies")
	TObjectPtr<UProceduralMeshComponent> HeldFlashComp;

private:
	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> TargetBodyMesh;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> WhiteDetailTex;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> PerforatedSteelTex;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> FlatNormalTex;

	bool bMag1Grabbed = false;
	bool bFragGrabbed = false;
	bool bFlashGrabbed = false;
	bool bHeldMagVisible = false;
	bool bHeldFragVisible = false;
	bool bHeldFlashVisible = false;
	bool bMasterVisible = true;

	void GenerateRuntimeBeltTextures();
	void BuildStanagFastMagMesh(UProceduralMeshComponent* Comp, bool bIncludeKydexPouch, bool bIncludeMagazine);
	void BuildM67FragGrenadeMesh(UProceduralMeshComponent* Comp);
	void BuildM84FlashbangMesh(UProceduralMeshComponent* Comp);

	void ApplyPBRSectionMaterial(
		UProceduralMeshComponent* Comp,
		int32 SectionIdx,
		const FLinearColor& BaseColor,
		float Roughness,
		float Metallic,
		UTexture2D* CustomDiffuse = nullptr);

	bool ComputePelvisOrthonormalFrame(FVector& OutPelvisOrigin, FQuat& OutPelvisQuat, FVector& OutRight, FVector& OutFwd, FVector& OutUp) const;
	FVector GetItemLocalOffset(EFPSBeltItemType Item) const;
	FRotator GetItemLocalRotation(EFPSBeltItemType Item) const;
};
