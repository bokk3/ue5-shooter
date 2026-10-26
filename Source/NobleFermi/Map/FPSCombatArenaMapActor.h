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
 * Procedural Tactical Favela Combat Arena Actor: "Favela // Rocha Velha"
 * Spawns a full 60m x 60m hillside Brazilian favela tactical arena complete with:
 * - Multi-level central 3-story Laje Watchtower with concrete pillars, rebar spires, and corrugated tin roof.
 * - West & East Becos (narrow cobblestone alleys) flanked by multi-story terracotta brick, cinder-block, and peeling pastel stucco shanties.
 * - Open concrete stormwater drainage sluices (valetas), sky-reflecting rainwater puddles, and red laterite dirt shoulders.
 * - Corrugated zinc roofing plates weighted with discarded tires, cinder blocks, and 1000L blue polyethylene water tanks.
 * - Tropical urban foliage (banana palms, climbing wall ivy, gutter weeds) and swaying catenary electrical wiring ("gatos").
 * - 3 Domination Flag Bases (A, B, C) and 8 Tactical PlayerStarts (4 Team Alpha, 4 Team Bravo).
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
