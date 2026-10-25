#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProceduralMeshComponent.h"
#include "FPSOperatorGearComponent.generated.h"

class USkeletalMeshComponent;
class USceneComponent;
class UMaterialInstanceDynamic;
class UTexture2D;

/**
 * UFPSOperatorGearComponent
 *
 * Socket- & Bone-Driven 3D Tactical Armor Rig, BDU Shader Calibrator, and Carbine Attachment System
 * for the 21-bone FFPSHumanSkeleton (SK_BaseMale).
 *
 * Engineered to eliminate the "skin-tight camo morphsuit" appearance by:
 * 1. Calibrating M_Operator_BaseBody with runtime low-contrast ripstop weave & seamless zone mask textures
 *    (Deep Charcoal #12161B / Slate #1A2026 BDU, Matte Black #0D1014 Balaclava under-layer, Carbon Gloves #0F1317).
 * 2. Constructing and pinning 12 multi-section 3D UProceduralMeshComponent armor & gear assemblies across all
 *    7 Loadout Zones (Head/Helmet/Goggles/Comms/Eye-Strip, Level IV Plate Carrier + NOBLE FERMI Stencil Patch +
 *    Ballistic Collar, Shoulder Pauldrons, Forearm Vambraces + Carbon Gloves, MOLLE Battle Belt + Groin Pad,
 *    Drop-Leg Kydex Pistol Holster + Thigh Rig, and Hard-Shell Knee Pads + Bloused Combat Boots).
 * 3. Pinning all gear meshes in World Space with Scale=(1,1,1) directly to SK_BaseMale bone locations and
 *    limb directional axes so they NEVER inherit the 100x FBX armature bone scale.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class NOBLEFERMI_API UFPSOperatorGearComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFPSOperatorGearComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Initialize and build all 7 tactical gear zones for the target SkeletalMeshComponent and unscaled attach parent */
	UFUNCTION(BlueprintCallable, Category = "FPS Operator Gear")
	void InitializeGearRig(USkeletalMeshComponent* InBodyMesh, USceneComponent* InUnscaledAttachParent, bool bInIncludeCarbine = true);

	/** Update world transforms of all 12 bone-pinned tactical gear meshes to match current animated pose */
	UFUNCTION(BlueprintCallable, Category = "FPS Operator Gear")
	void UpdateGearTransforms();

	/** Apply Camo Preset (0 = Task Force Echo Slate/Urban, 1 = Woodland/Naval, 2 = Arid Desert, 3 = Stealth Blackout) */
	UFUNCTION(BlueprintCallable, Category = "FPS Operator Gear")
	void ApplyCamoPreset(int32 CamoIndex);

	/** Apply Headgear Variant (0 = FAST Ballistic Helmet + Comms + Goggles, 1 = Quad-Nod NVGs, 2 = Spec-Ops Balaclava, 3 = Tactical Cap & Comms) */
	UFUNCTION(BlueprintCallable, Category = "FPS Operator Gear")
	void ApplyHeadgearVariant(int32 HeadgearIndex);

	/** Apply Armor Vest Variant (0 = Heavy Level IV Plate Carrier + Pauldrons, 1 = Lightweight JPC, 2 = Recon Chest Rig) */
	UFUNCTION(BlueprintCallable, Category = "FPS Operator Gear")
	void ApplyVestVariant(int32 VestIndex);

	/** Apply Weapon Finish Variant (0 = Anodized Gunmetal Steel, 1 = Desert Tan Cerakote, 2 = Carbon Fiber Spec-Ops, 3 = 24K Gold & Opal) */
	UFUNCTION(BlueprintCallable, Category = "FPS Operator Gear")
	void ApplyWeaponFinishVariant(int32 FinishIndex);

	/** Toggle visibility of Headgear and Plate Carrier (e.g. when helmet pops or armor shatters in combat) */
	void SetHelmetVisible(bool bVisible);
	void SetPlateCarrierShattered(bool bShattered);

	// --- 7 Loadout Zone Procedural Mesh Components ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Operator Gear|Zones")
	TObjectPtr<UProceduralMeshComponent> HeadGearProcMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Operator Gear|Zones")
	TObjectPtr<UProceduralMeshComponent> TorsoGearProcMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Operator Gear|Zones")
	TObjectPtr<UProceduralMeshComponent> LeftPauldronProcMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Operator Gear|Zones")
	TObjectPtr<UProceduralMeshComponent> RightPauldronProcMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Operator Gear|Zones")
	TObjectPtr<UProceduralMeshComponent> LeftForearmProcMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Operator Gear|Zones")
	TObjectPtr<UProceduralMeshComponent> RightForearmProcMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Operator Gear|Zones")
	TObjectPtr<UProceduralMeshComponent> LeftGloveProcMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Operator Gear|Zones")
	TObjectPtr<UProceduralMeshComponent> RightGloveProcMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Operator Gear|Zones")
	TObjectPtr<UProceduralMeshComponent> BattleBeltProcMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Operator Gear|Zones")
	TObjectPtr<UProceduralMeshComponent> RightThighProcMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Operator Gear|Zones")
	TObjectPtr<UProceduralMeshComponent> LeftThighProcMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Operator Gear|Zones")
	TObjectPtr<UProceduralMeshComponent> LeftCalfProcMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Operator Gear|Zones")
	TObjectPtr<UProceduralMeshComponent> RightCalfProcMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS Operator Gear|Weapon")
	TObjectPtr<UProceduralMeshComponent> CarbineProcMesh;

private:
	UPROPERTY()
	TObjectPtr<USkeletalMeshComponent> TargetBodyMesh;

	UPROPERTY()
	TObjectPtr<USceneComponent> UnscaledAttachParent;

	UPROPERTY()
	TObjectPtr<UTexture2D> RuntimeCamoTexture;

	UPROPERTY()
	TObjectPtr<UTexture2D> RuntimeBodyMaskTexture;

	UPROPERTY()
	TArray<TObjectPtr<UProceduralMeshComponent>> AllGearMeshes;

	bool bIncludeCarbine = true;
	bool bIsHelmetPopped = false;
	bool bIsPlateShattered = false;

	int32 CurrentCamoIndex = 0;
	int32 CurrentHeadgearIndex = 0;
	int32 CurrentVestIndex = 0;
	int32 CurrentWeaponFinishIndex = 0;

	void GenerateRuntimeOperatorTextures();
	void CalibrateOperatorBaseBodyMaterial();

	void BuildZone1_HeadAndComms();
	void BuildZone2_PlateCarrierAndCollar();
	void BuildZone3_ShoulderPauldrons();
	void BuildZone4_ForearmsAndGloves();
	void BuildZone5_BattleBeltAndGroin();
	void BuildZone6_ThighHolsterAndRig();
	void BuildZone7_KneePadsAndBoots();
	void BuildSpecOpsCarbine();

	void ApplyGearMaterialsToMesh(UProceduralMeshComponent* ProcMesh);
};
