#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FPSCombatArenaMapActor.generated.h"

class UDirectionalLightComponent;
class USkyLightComponent;
class USkyAtmosphereComponent;
class UExponentialHeightFogComponent;
class UStaticMeshComponent;
class USceneComponent;
class UPointLightComponent;
class UPostProcessComponent;
class APlayerStart;

/**
 * Procedural Tactical Combat Arena Actor: "Rust Tactical"
 * Spawns a full 60m x 60m competitive MW2-style arena complete with:
 * - Multi-level central oil derrick sniper rig (12m high) with catwalks and access ramps.
 * - Flank Lane A: 4 shipping containers with shoot-through tunnel.
 * - Flank Lane B: Elevated pipeline trench and concrete bunker.
 * - Center Lane: Armored vehicle wreck and concrete Jersey barriers.
 * - 3 Domination Flag Bases (A, B, C) with holographic beacons.
 * - 8 Tactical PlayerStarts (4 Team Alpha, 4 Team Bravo).
 * - Golden-hour desert sun, atmosphere, volumetric dust haze, and industrial floodlights.
 */
UCLASS()
class NOBLEFERMI_API AFPSCombatArenaMapActor : public AActor
{
	GENERATED_BODY()

public:
	AFPSCombatArenaMapActor();

	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map|Scene")
	TObjectPtr<USceneComponent> SceneRoot;

	// Atmospheric Lighting
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map|Lighting")
	TObjectPtr<UDirectionalLightComponent> SunLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map|Lighting")
	TObjectPtr<USkyLightComponent> SkyLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map|Lighting")
	TObjectPtr<USkyAtmosphereComponent> SkyAtmosphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map|Lighting")
	TObjectPtr<UExponentialHeightFogComponent> HeightFog;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map|Lighting")
	TObjectPtr<UPointLightComponent> TowerBeaconLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map|PostProcess")
	TObjectPtr<UPostProcessComponent> PostProcess;

	// Map Geometry Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map|Terrain")
	TObjectPtr<UStaticMeshComponent> GroundFloorMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map|Perimeter")
	TArray<TObjectPtr<UStaticMeshComponent>> PerimeterWalls;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map|Derrick")
	TArray<TObjectPtr<UStaticMeshComponent>> DerrickComponents;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map|Containers")
	TArray<TObjectPtr<UStaticMeshComponent>> ContainerComponents;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map|Pipeline")
	TArray<TObjectPtr<UStaticMeshComponent>> PipelineComponents;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map|Cover")
	TArray<TObjectPtr<UStaticMeshComponent>> BarrierComponents;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map|Objectives")
	TArray<TObjectPtr<UStaticMeshComponent>> ObjectiveFlags;

	void SpawnTacticalPlayerStarts();

protected:
	virtual void BeginPlay() override;

private:
	void ConstructPerimeter(UStaticMesh* CubeMesh, UMaterialInterface* BaseMat);
	void ConstructCentralDerrick(UStaticMesh* CubeMesh, UStaticMesh* CylinderMesh, UMaterialInterface* BaseMat);
	void ConstructContainerYard(UStaticMesh* CubeMesh, UMaterialInterface* BaseMat);
	void ConstructPipelineAndBunker(UStaticMesh* CubeMesh, UStaticMesh* CylinderMesh, UMaterialInterface* BaseMat);
	void ConstructCenterCover(UStaticMesh* CubeMesh, UMaterialInterface* BaseMat);
	void ConstructObjectives(UStaticMesh* CylinderMesh, UMaterialInterface* BaseMat);

	bool bPlayerStartsSpawned = false;
	float BeaconTimer = 0.0f;
};
