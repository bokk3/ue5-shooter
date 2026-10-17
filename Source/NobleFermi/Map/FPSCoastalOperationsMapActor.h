#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FPSCoastalOperationsMapActor.generated.h"

class UDirectionalLightComponent;
class USkyLightComponent;
class USkyAtmosphereComponent;
class UExponentialHeightFogComponent;
class UStaticMeshComponent;
class UPointLightComponent;
class UPostProcessComponent;
class UBoxComponent;
class APlayerStart;
class AFPSCoastalSplineActor;

/**
 * Procedural Tactical Arena Actor: "Coastal Operations"
 * 120m x 120m North Sea Coastal Dunes, Seawall Dike & Maritime Pier.
 * Features:
 * - 48m Modular Timber & Reinforced Concrete Maritime Pier with zero-seam UCX flat collision box (Flag C).
 * - Full Seawall Dike with 4.5m Crest Roadway (Flag B), Basalt Riprap, and 300x280cm Drainage Culvert.
 * - Backcountry Dune Field with geomorphological 12-deg stoss and 32-deg slip-face dunes (Flag A).
 * - Calibrated North Sea Maritime Sunlight (8.5 lux), Volumetric Mist, and AEM_Manual Anti-Whiteout Exposure.
 * - 8 Anti-Spawn-Trap PlayerStarts (4 Task Force Alpha, 4 OpFor Bravo).
 */
UCLASS()
class NOBLEFERMI_API AFPSCoastalOperationsMapActor : public AActor
{
	GENERATED_BODY()

public:
	AFPSCoastalOperationsMapActor();

	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map|Scene")
	TObjectPtr<USceneComponent> SceneRoot;

	// Atmospheric Maritime Lighting (Pure Dynamic Lumen)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map|Lighting")
	TObjectPtr<UDirectionalLightComponent> SunLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map|Lighting")
	TObjectPtr<USkyLightComponent> SkyLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map|Lighting")
	TObjectPtr<USkyAtmosphereComponent> SkyAtmosphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map|Lighting")
	TObjectPtr<UExponentialHeightFogComponent> HeightFog;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map|Lighting")
	TObjectPtr<UPointLightComponent> PierHeadNavBeacon;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map|Lighting")
	TObjectPtr<UPointLightComponent> DikeHazardBeacon;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map|PostProcess")
	TObjectPtr<UPostProcessComponent> PostProcess;

	// Terrain & Water Planes
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map|Terrain")
	TObjectPtr<UStaticMeshComponent> BeachFloorMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map|Water")
	TObjectPtr<UStaticMeshComponent> WaterPlaneMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map|Water")
	TObjectPtr<UBoxComponent> WaterVolumeBox;

	// Maritime Pier Kit Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map|Pier")
	TObjectPtr<UBoxComponent> UCX_Pier_Deck;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map|Pier")
	TObjectPtr<UBoxComponent> UCX_Pier_THead;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map|Pier")
	TArray<TObjectPtr<UStaticMeshComponent>> PierVisualPlanks;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map|Pier")
	TArray<TObjectPtr<UStaticMeshComponent>> PierPilings;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map|Pier")
	TArray<TObjectPtr<UStaticMeshComponent>> PierCrossBeams;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map|Pier")
	TArray<TObjectPtr<UStaticMeshComponent>> PierBollards;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map|Pier")
	TArray<TObjectPtr<UStaticMeshComponent>> PierRailings;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map|Pier")
	TArray<TObjectPtr<UStaticMeshComponent>> PierLadders;

	// Coastal Dunes & Backcountry
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map|Dunes")
	TArray<TObjectPtr<UStaticMeshComponent>> DuneComponents;

	// Perimeter Boundaries
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map|Perimeter")
	TArray<TObjectPtr<UStaticMeshComponent>> PerimeterWalls;

	// Domination Objective Flags (A, B, C)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map|Objectives")
	TArray<TObjectPtr<UStaticMeshComponent>> ObjectiveFlags;

	// Sub-Actor: Coastal Spline Seawall Dike
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map|Dike")
	TObjectPtr<UChildActorComponent> CoastalDikeSubActor;

	void SpawnTacticalPlayerStarts();

protected:
	virtual void BeginPlay() override;

private:
	void ConstructWaterAndBeach(UStaticMesh* CubeMesh, UMaterialInterface* BaseMat);
	void ConstructMaritimePier(UStaticMesh* CubeMesh, UStaticMesh* CylinderMesh, UMaterialInterface* BaseMat);
	void ConstructCoastalDunes(UStaticMesh* CubeMesh, UMaterialInterface* BaseMat);
	void ConstructPerimeterBoundaries(UStaticMesh* CubeMesh, UMaterialInterface* BaseMat);
	void ConstructObjectives(UStaticMesh* CylinderMesh, UMaterialInterface* BaseMat);

	bool bPlayerStartsSpawned = false;
	float BeaconPulseTimer = 0.0f;
};
