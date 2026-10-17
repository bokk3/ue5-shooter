#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FPSCoastalSplineActor.generated.h"

class USplineComponent;
class UStaticMeshComponent;
class UBoxComponent;
class UMaterialInterface;

/**
 * Procedural Seawall Dike Spline Actor for "Coastal Operations"
 * Constructs our 3-zone North Sea coastal engineering profile:
 * 1. Seaward Revetment (Outer Slope): 1:3.5 armored basalt riprap & concrete tetrapods.
 * 2. Crest Roadway (Top Spine): 4.5m wide elevated macadam/asphalt road at Z = +420.0f cm (Domination Flag B).
 * 3. Landward Back-Slope (Inner Slope): 1:2.5 stabilized turf slope down to backcountry dunes.
 * 4. Embedded Sub-Structures:
 *    - Concrete Access Stairs on Left & Right flanks.
 *    - CQB Drainage Culvert & Sluice Gate (300cm wide x 280cm tall interior for OTS camera clearance).
 */
UCLASS()
class NOBLEFERMI_API AFPSCoastalSplineActor : public AActor
{
	GENERATED_BODY()

public:
	AFPSCoastalSplineActor();

	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Coastal|Scene")
	TObjectPtr<USceneComponent> SceneRoot;

	/** Central guide spline for seawall embankment curvature */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Coastal|Spline")
	TObjectPtr<USplineComponent> SplineComponent;

	// Generated Embankment Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Coastal|Roadway")
	TArray<TObjectPtr<UStaticMeshComponent>> CrestRoadComponents;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Coastal|Revetment")
	TArray<TObjectPtr<UStaticMeshComponent>> SeawardRevetmentComponents;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Coastal|Armor")
	TArray<TObjectPtr<UStaticMeshComponent>> BasaltRiprapComponents;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Coastal|Armor")
	TArray<TObjectPtr<UStaticMeshComponent>> TetrapodComponents;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Coastal|BackSlope")
	TArray<TObjectPtr<UStaticMeshComponent>> LandwardSlopeComponents;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Coastal|Stairs")
	TArray<TObjectPtr<UStaticMeshComponent>> StairComponents;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Coastal|Culvert")
	TArray<TObjectPtr<UStaticMeshComponent>> CulvertComponents;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Coastal|Curbs")
	TArray<TObjectPtr<UStaticMeshComponent>> CurbComponents;

	/** Re-builds all procedural dike geometry deterministically */
	UFUNCTION(BlueprintCallable, Category = "Coastal|Procedural")
	void BuildDikeGeometry();

protected:
	virtual void BeginPlay() override;

private:
	void ConstructCrestRoadway(UStaticMesh* CubeMesh, UMaterialInterface* BaseMat);
	void ConstructSeawardRevetment(UStaticMesh* CubeMesh, UMaterialInterface* BaseMat);
	void ConstructBasaltRiprapAndTetrapods(UStaticMesh* CubeMesh, UStaticMesh* CylinderMesh, UMaterialInterface* BaseMat);
	void ConstructLandwardSlope(UStaticMesh* CubeMesh, UMaterialInterface* BaseMat);
	void ConstructFlankStairs(UStaticMesh* CubeMesh, UMaterialInterface* BaseMat);
	void ConstructDrainageCulvert(UStaticMesh* CubeMesh, UMaterialInterface* BaseMat);

	void ClearGeneratedComponents();

	bool bGeometryConstructed = false;
};
