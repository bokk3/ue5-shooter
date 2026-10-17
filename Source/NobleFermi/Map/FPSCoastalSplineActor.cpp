#include "Map/FPSCoastalSplineActor.h"
#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AFPSCoastalSplineActor::AFPSCoastalSplineActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComponent"));
	SplineComponent->SetupAttachment(RootComponent);
	SplineComponent->SetMobility(EComponentMobility::Movable);

	// Setup 120-meter seaward curved arc across the map (-60m to +60m along X)
	SplineComponent->ClearSplinePoints(false);
	SplineComponent->AddSplinePoint(FVector(-6000.0f, -400.0f, 0.0f), ESplineCoordinateSpace::Local, false);
	SplineComponent->AddSplinePoint(FVector(-3000.0f, -120.0f, 0.0f), ESplineCoordinateSpace::Local, false);
	SplineComponent->AddSplinePoint(FVector(0.0f, 0.0f, 0.0f), ESplineCoordinateSpace::Local, false); // Central Culvert Portal
	SplineComponent->AddSplinePoint(FVector(3000.0f, -120.0f, 0.0f), ESplineCoordinateSpace::Local, false);
	SplineComponent->AddSplinePoint(FVector(6000.0f, -400.0f, 0.0f), ESplineCoordinateSpace::Local, false);
	SplineComponent->UpdateSpline();

	// Load basic shapes
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BaseMatFinder(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	UStaticMesh* CubeMesh = CubeFinder.Succeeded() ? CubeFinder.Object : nullptr;
	UStaticMesh* CylinderMesh = CylinderFinder.Succeeded() ? CylinderFinder.Object : nullptr;
	UMaterialInterface* BaseMat = BaseMatFinder.Succeeded() ? BaseMatFinder.Object : nullptr;

	// Build procedural geometry in constructor for zero-overhead cook and headless replication
	ConstructCrestRoadway(CubeMesh, BaseMat);
	ConstructSeawardRevetment(CubeMesh, BaseMat);
	ConstructBasaltRiprapAndTetrapods(CubeMesh, CylinderMesh, BaseMat);
	ConstructLandwardSlope(CubeMesh, BaseMat);
	ConstructFlankStairs(CubeMesh, BaseMat);
	ConstructDrainageCulvert(CubeMesh, BaseMat);

	bGeometryConstructed = true;
}

void AFPSCoastalSplineActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	// Construction hook for level-editor spline updates
}

void AFPSCoastalSplineActor::BeginPlay()
{
	Super::BeginPlay();

	// Apply tactical maritime material colors at runtime
	auto ApplyTint = [](UStaticMeshComponent* Comp, const FLinearColor& Color, float Roughness = 0.85f)
	{
		if (!Comp) return;
		UMaterialInterface* Mat = Comp->GetMaterial(0);
		if (Mat)
		{
			UMaterialInstanceDynamic* DynMat = UMaterialInstanceDynamic::Create(Mat, Comp);
			if (DynMat)
			{
				DynMat->SetVectorParameterValue(TEXT("Color"), Color);
				DynMat->SetScalarParameterValue(TEXT("Roughness"), Roughness);
				Comp->SetMaterial(0, DynMat);
			}
		}
	};

	// 1. Weathered macadam crest roadway
	for (UStaticMeshComponent* Comp : CrestRoadComponents)
	{
		ApplyTint(Comp, FLinearColor(0.18f, 0.19f, 0.21f), 0.90f);
	}

	// 2. Concrete curbs
	for (UStaticMeshComponent* Comp : CurbComponents)
	{
		ApplyTint(Comp, FLinearColor(0.45f, 0.46f, 0.48f), 0.80f);
	}

	// 3. Basalt riprap armor rocks
	for (UStaticMeshComponent* Comp : BasaltRiprapComponents)
	{
		ApplyTint(Comp, FLinearColor(0.12f, 0.13f, 0.15f), 0.75f);
	}

	// 4. Wave-breaking tetrapods
	for (UStaticMeshComponent* Comp : TetrapodComponents)
	{
		ApplyTint(Comp, FLinearColor(0.38f, 0.40f, 0.42f), 0.85f);
	}

	// 5. Landward stabilized turf back-slope
	for (UStaticMeshComponent* Comp : LandwardSlopeComponents)
	{
		ApplyTint(Comp, FLinearColor(0.24f, 0.29f, 0.18f), 0.95f);
	}

	// 6. Flank access stairs
	for (UStaticMeshComponent* Comp : StairComponents)
	{
		ApplyTint(Comp, FLinearColor(0.42f, 0.44f, 0.46f), 0.82f);
	}

	// 7. Drainage Culvert & Sluice Portal
	for (UStaticMeshComponent* Comp : CulvertComponents)
	{
		if (Comp->ComponentTags.Contains(FName("Sluice_Steel")))
		{
			ApplyTint(Comp, FLinearColor(0.28f, 0.16f, 0.12f), 0.65f); // Rusted iron
		}
		else
		{
			ApplyTint(Comp, FLinearColor(0.36f, 0.38f, 0.40f), 0.88f); // Hydro-concrete
		}
	}
}

void AFPSCoastalSplineActor::BuildDikeGeometry()
{
	// Triggered if rebuild requested dynamically
}

void AFPSCoastalSplineActor::ConstructCrestRoadway(UStaticMesh* CubeMesh, UMaterialInterface* BaseMat)
{
	// 4.5m wide flat roadway at Z = +420.0f cm spanning 120m across the map (-60m to +60m)
	// Composed of 24 modular segments (500cm each) along the curved seawall spline
	const int32 NumSegments = 24;
	const float SegmentLength = 500.0f; // 5 meters

	for (int32 i = 0; i < NumSegments; ++i)
	{
		const float CenterX = -5750.0f + i * SegmentLength;
		// Skip central culvert slot (-150 to +150) for vertical sluice housing
		const float SplineY = -350.0f * FMath::Cos(CenterX / 6000.0f * (UE_PI * 0.5f));

		// Main Roadway Slab (Width 450cm, Depth 500cm, Height 40cm)
		FString RoadName = FString::Printf(TEXT("Dike_Road_Seg_%02d"), i);
		UStaticMeshComponent* RoadSeg = CreateDefaultSubobject<UStaticMeshComponent>(*RoadName);
		RoadSeg->SetupAttachment(RootComponent);
		RoadSeg->SetMobility(EComponentMobility::Movable);
		if (CubeMesh) RoadSeg->SetStaticMesh(CubeMesh);
		if (BaseMat) RoadSeg->SetMaterial(0, BaseMat);
		RoadSeg->SetRelativeLocation(FVector(CenterX, SplineY, 400.0f));
		RoadSeg->SetRelativeScale3D(FVector(SegmentLength / 100.0f, 4.50f, 0.40f));
		RoadSeg->SetCollisionProfileName(TEXT("BlockAll"));
		RoadSeg->SetCastShadow(true);
		RoadSeg->ComponentTags.Add(FName("Surface_Dike_Asphalt"));
		RoadSeg->ComponentTags.Add(FName("Roadway"));
		CrestRoadComponents.Add(RoadSeg);

		// Seaward Concrete Curb (Width 25cm, Height 30cm)
		FString CurbSeawardName = FString::Printf(TEXT("Dike_Curb_Sea_%02d"), i);
		UStaticMeshComponent* CurbSea = CreateDefaultSubobject<UStaticMeshComponent>(*CurbSeawardName);
		CurbSea->SetupAttachment(RootComponent);
		CurbSea->SetMobility(EComponentMobility::Movable);
		if (CubeMesh) CurbSea->SetStaticMesh(CubeMesh);
		if (BaseMat) CurbSea->SetMaterial(0, BaseMat);
		CurbSea->SetRelativeLocation(FVector(CenterX, SplineY - 235.0f, 425.0f));
		CurbSea->SetRelativeScale3D(FVector(SegmentLength / 100.0f, 0.25f, 0.30f));
		CurbSea->SetCollisionProfileName(TEXT("BlockAll"));
		CurbSea->ComponentTags.Add(FName("Concrete"));
		CurbComponents.Add(CurbSea);

		// Landward Concrete Curb (Width 25cm, Height 30cm)
		FString CurbLandwardName = FString::Printf(TEXT("Dike_Curb_Land_%02d"), i);
		UStaticMeshComponent* CurbLand = CreateDefaultSubobject<UStaticMeshComponent>(*CurbLandwardName);
		CurbLand->SetupAttachment(RootComponent);
		CurbLand->SetMobility(EComponentMobility::Movable);
		if (CubeMesh) CurbLand->SetStaticMesh(CubeMesh);
		if (BaseMat) CurbLand->SetMaterial(0, BaseMat);
		CurbLand->SetRelativeLocation(FVector(CenterX, SplineY + 235.0f, 425.0f));
		CurbLand->SetRelativeScale3D(FVector(SegmentLength / 100.0f, 0.25f, 0.30f));
		CurbLand->SetCollisionProfileName(TEXT("BlockAll"));
		CurbLand->ComponentTags.Add(FName("Concrete"));
		CurbComponents.Add(CurbLand);
	}

	// Central King-of-the-Hill Mooring / Sluice Gate Control Tower (Domination Flag B Base)
	UStaticMeshComponent* SluiceControlHousing = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Dike_Sluice_Housing_B"));
	SluiceControlHousing->SetupAttachment(RootComponent);
	SluiceControlHousing->SetMobility(EComponentMobility::Movable);
	if (CubeMesh) SluiceControlHousing->SetStaticMesh(CubeMesh);
	if (BaseMat) SluiceControlHousing->SetMaterial(0, BaseMat);
	SluiceControlHousing->SetRelativeLocation(FVector(0.0f, -220.0f, 475.0f));
	SluiceControlHousing->SetRelativeScale3D(FVector(2.4f, 1.6f, 1.1f)); // Waist-high ballistic bunker cover
	SluiceControlHousing->SetCollisionProfileName(TEXT("BlockAll"));
	SluiceControlHousing->ComponentTags.Add(FName("Concrete"));
	SluiceControlHousing->ComponentTags.Add(FName("Cover_HighGround"));
	CurbComponents.Add(SluiceControlHousing);
}

void AFPSCoastalSplineActor::ConstructSeawardRevetment(UStaticMesh* CubeMesh, UMaterialInterface* BaseMat)
{
	// 1:3.5 outer slope (~15.9 degrees) armored slope from Y = -225 (Z = 420) down to Y = -1695 (Z = 0)
	// Hypotenuse length = 1528 cm. Center at Y = -960, Z = 210
	const int32 NumSegments = 16;
	const float SegmentLength = 750.0f; // 7.5 meters per slab

	for (int32 i = 0; i < NumSegments; ++i)
	{
		const float CenterX = -5625.0f + i * SegmentLength;
		// Skip central culvert opening (-200 to +200)
		if (FMath::Abs(CenterX) < 300.0f) continue;

		const float SplineY = -350.0f * FMath::Cos(CenterX / 6000.0f * (UE_PI * 0.5f));

		FString RevetName = FString::Printf(TEXT("Dike_Revetment_%02d"), i);
		UStaticMeshComponent* RevetComp = CreateDefaultSubobject<UStaticMeshComponent>(*RevetName);
		RevetComp->SetupAttachment(RootComponent);
		RevetComp->SetMobility(EComponentMobility::Movable);
		if (CubeMesh) RevetComp->SetStaticMesh(CubeMesh);
		if (BaseMat) RevetComp->SetMaterial(0, BaseMat);
		RevetComp->SetRelativeLocation(FVector(CenterX, SplineY - 960.0f, 210.0f));
		RevetComp->SetRelativeRotation(FRotator(-15.94f, 0.0f, 0.0f)); // Pitch tilted 15.94 deg toward sea
		RevetComp->SetRelativeScale3D(FVector(SegmentLength / 100.0f, 15.30f, 0.40f));
		RevetComp->SetCollisionProfileName(TEXT("BlockAll"));
		RevetComp->SetCastShadow(true);
		RevetComp->ComponentTags.Add(FName("Surface_Basalt_Rock"));
		SeawardRevetmentComponents.Add(RevetComp);
	}
}

void AFPSCoastalSplineActor::ConstructBasaltRiprapAndTetrapods(UStaticMesh* CubeMesh, UStaticMesh* CylinderMesh, UMaterialInterface* BaseMat)
{
	// Heavy interlocking basalt boulders along the outer seaward revetment
	const int32 BoulderRows = 4;
	const int32 BouldersPerRow = 14;

	for (int32 r = 0; r < BoulderRows; ++r)
	{
		const float RowY = -600.0f - r * 280.0f; // from Y = -600 down to -1440
		const float RowZ = 340.0f - r * 80.0f;   // stepped down with slope

		for (int32 b = 0; b < BouldersPerRow; ++b)
		{
			const float X = -5200.0f + b * 800.0f + ((r % 2) * 400.0f);
			if (FMath::Abs(X) < 400.0f) continue; // Clear central culvert path

			// Deterministic random pseudo-variation
			const float AngleYaw = (X * 0.17f) + (r * 37.0f);
			const float ScaleVar = 0.9f + 0.35f * FMath::Frac(FMath::Sin(X * 0.01f + r) * 43758.5453f);

			FString BoulderName = FString::Printf(TEXT("Basalt_Riprap_%d_%d"), r, b);
			UStaticMeshComponent* Boulder = CreateDefaultSubobject<UStaticMeshComponent>(*BoulderName);
			Boulder->SetupAttachment(RootComponent);
			Boulder->SetMobility(EComponentMobility::Movable);
			if (CubeMesh) Boulder->SetStaticMesh(CubeMesh);
			if (BaseMat) Boulder->SetMaterial(0, BaseMat);
			Boulder->SetRelativeLocation(FVector(X, RowY, RowZ));
			Boulder->SetRelativeRotation(FRotator(FMath::Sin(AngleYaw) * 12.0f, AngleYaw, FMath::Cos(AngleYaw) * 10.0f));
			Boulder->SetRelativeScale3D(FVector(1.6f * ScaleVar, 1.4f * ScaleVar, 1.1f * ScaleVar));
			Boulder->SetCollisionProfileName(TEXT("BlockAll"));
			Boulder->SetCastShadow(true);
			Boulder->ComponentTags.Add(FName("Surface_Basalt_Rock"));
			Boulder->ComponentTags.Add(FName("Cover_Medium"));
			BasaltRiprapComponents.Add(Boulder);
		}
	}

	// Concrete wave-breaking Tetrapods along seaward toe berm (Y = -1580, Z = 60)
	const int32 NumTetrapods = 18;
	for (int32 t = 0; t < NumTetrapods; ++t)
	{
		const float X = -5100.0f + t * 600.0f;
		if (FMath::Abs(X) < 450.0f) continue; // Keep culvert channel clear

		FString TetName = FString::Printf(TEXT("Tetrapod_Core_%02d"), t);
		UStaticMeshComponent* TetCore = CreateDefaultSubobject<UStaticMeshComponent>(*TetName);
		TetCore->SetupAttachment(RootComponent);
		TetCore->SetMobility(EComponentMobility::Movable);
		if (CubeMesh) TetCore->SetStaticMesh(CubeMesh);
		if (BaseMat) TetCore->SetMaterial(0, BaseMat);
		TetCore->SetRelativeLocation(FVector(X, -1580.0f, 60.0f));
		TetCore->SetRelativeRotation(FRotator(18.0f, t * 47.0f, -22.0f));
		TetCore->SetRelativeScale3D(FVector(1.8f, 1.8f, 1.4f));
		TetCore->SetCollisionProfileName(TEXT("BlockAll"));
		TetCore->ComponentTags.Add(FName("Surface_Basalt_Rock"));
		TetCore->ComponentTags.Add(FName("WaveBreaker"));
		TetrapodComponents.Add(TetCore);
	}
}

void AFPSCoastalSplineActor::ConstructLandwardSlope(UStaticMesh* CubeMesh, UMaterialInterface* BaseMat)
{
	// 1:2.5 stabilized turf back-slope (~21.8 degrees) from Y = +225 (Z = 420) down to Y = +1275 (Z = 0)
	// Hypotenuse length = 1131 cm. Center at Y = +750, Z = 210
	const int32 NumSegments = 16;
	const float SegmentLength = 750.0f;

	for (int32 i = 0; i < NumSegments; ++i)
	{
		const float CenterX = -5625.0f + i * SegmentLength;
		if (FMath::Abs(CenterX) < 300.0f) continue; // Clear culvert channel

		const float SplineY = -350.0f * FMath::Cos(CenterX / 6000.0f * (UE_PI * 0.5f));

		FString SlopeName = FString::Printf(TEXT("Dike_LandSlope_%02d"), i);
		UStaticMeshComponent* SlopeComp = CreateDefaultSubobject<UStaticMeshComponent>(*SlopeName);
		SlopeComp->SetupAttachment(RootComponent);
		SlopeComp->SetMobility(EComponentMobility::Movable);
		if (CubeMesh) SlopeComp->SetStaticMesh(CubeMesh);
		if (BaseMat) SlopeComp->SetMaterial(0, BaseMat);
		SlopeComp->SetRelativeLocation(FVector(CenterX, SplineY + 750.0f, 210.0f));
		SlopeComp->SetRelativeRotation(FRotator(21.80f, 0.0f, 0.0f)); // Pitch tilted 21.8 deg toward land
		SlopeComp->SetRelativeScale3D(FVector(SegmentLength / 100.0f, 11.30f, 0.40f));
		SlopeComp->SetCollisionProfileName(TEXT("BlockAll"));
		SlopeComp->SetCastShadow(true);
		SlopeComp->ComponentTags.Add(FName("Surface_Sand_Dune"));
		LandwardSlopeComponents.Add(SlopeComp);
	}
}

void AFPSCoastalSplineActor::ConstructFlankStairs(UStaticMesh* CubeMesh, UMaterialInterface* BaseMat)
{
	// Flank Maintenance Stairs: Left flank at X = -3600, Right flank at X = +3600
	const float FlankX[2] = { -3600.0f, 3600.0f };
	const int32 NumSteps = 16;
	const float StepRise = 420.0f / NumSteps; // 26.25 cm
	const float StepRun = 1000.0f / NumSteps; // 62.5 cm

	for (int32 f = 0; f < 2; ++f)
	{
		const float BaseX = FlankX[f];
		const FString FlankPrefix = (f == 0) ? TEXT("Stair_Left") : TEXT("Stair_Right");

		for (int32 s = 0; s < NumSteps; ++s)
		{
			const float StepY = 1225.0f - s * StepRun;
			const float StepZ = s * StepRise + StepRise * 0.5f;

			FString StepName = FString::Printf(TEXT("%s_Step_%02d"), *FlankPrefix, s);
			UStaticMeshComponent* StepComp = CreateDefaultSubobject<UStaticMeshComponent>(*StepName);
			StepComp->SetupAttachment(RootComponent);
			StepComp->SetMobility(EComponentMobility::Movable);
			if (CubeMesh) StepComp->SetStaticMesh(CubeMesh);
			if (BaseMat) StepComp->SetMaterial(0, BaseMat);
			StepComp->SetRelativeLocation(FVector(BaseX, StepY, StepZ));
			StepComp->SetRelativeScale3D(FVector(2.40f, StepRun / 100.0f, StepRise / 100.0f));
			StepComp->SetCollisionProfileName(TEXT("BlockAll"));
			StepComp->ComponentTags.Add(FName("Concrete"));
			StepComp->ComponentTags.Add(FName("Stairs"));
			StairComponents.Add(StepComp);
		}

		// Left & Right Concrete Handrail Stringers
		for (int32 side = 0; side < 2; ++side)
		{
			const float StringerX = BaseX + (side == 0 ? -125.0f : 125.0f);
			FString StringerName = FString::Printf(TEXT("%s_Stringer_%d"), *FlankPrefix, side);
			UStaticMeshComponent* Stringer = CreateDefaultSubobject<UStaticMeshComponent>(*StringerName);
			Stringer->SetupAttachment(RootComponent);
			Stringer->SetMobility(EComponentMobility::Movable);
			if (CubeMesh) Stringer->SetStaticMesh(CubeMesh);
			if (BaseMat) Stringer->SetMaterial(0, BaseMat);
			Stringer->SetRelativeLocation(FVector(StringerX, 725.0f, 260.0f));
			Stringer->SetRelativeRotation(FRotator(21.8f, 0.0f, 0.0f));
			Stringer->SetRelativeScale3D(FVector(0.25f, 11.0f, 0.8f));
			Stringer->SetCollisionProfileName(TEXT("BlockAll"));
			Stringer->ComponentTags.Add(FName("Concrete"));
			StairComponents.Add(Stringer);
		}
	}
}

void AFPSCoastalSplineActor::ConstructDrainageCulvert(UStaticMesh* CubeMesh, UMaterialInterface* BaseMat)
{
	// CQB Drainage Culvert & Sluice Gate: SM_Dike_Culvert_Sluice
	// Tunnel cuts directly through the central Dike at X = 0 from Seaward Y = -1800 to Landward Y = +1400 (32m long)
	// Interior Clearance: 300.0f cm wide x 280.0f cm tall (OTS camera clearance)
	const float TunnelLength = 3200.0f; // 32m
	const float TunnelCenterY = -200.0f;

	// 1. Floor Slab (Width 300cm, Height 20cm, from Z = -20 to 0)
	UStaticMeshComponent* FloorSlab = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Culvert_Floor_Slab"));
	FloorSlab->SetupAttachment(RootComponent);
	FloorSlab->SetMobility(EComponentMobility::Movable);
	if (CubeMesh) FloorSlab->SetStaticMesh(CubeMesh);
	if (BaseMat) FloorSlab->SetMaterial(0, BaseMat);
	FloorSlab->SetRelativeLocation(FVector(0.0f, TunnelCenterY, -10.0f));
	FloorSlab->SetRelativeScale3D(FVector(3.00f, TunnelLength / 100.0f, 0.20f));
	FloorSlab->SetCollisionProfileName(TEXT("BlockAll"));
	FloorSlab->ComponentTags.Add(FName("Concrete"));
	FloorSlab->ComponentTags.Add(FName("Surface_Dike_Asphalt"));
	CulvertComponents.Add(FloorSlab);

	// 2. Left Wall (X = -170.0f, Thickness 40cm, Height 280cm)
	UStaticMeshComponent* LeftWall = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Culvert_Left_Wall"));
	LeftWall->SetupAttachment(RootComponent);
	LeftWall->SetMobility(EComponentMobility::Movable);
	if (CubeMesh) LeftWall->SetStaticMesh(CubeMesh);
	if (BaseMat) LeftWall->SetMaterial(0, BaseMat);
	LeftWall->SetRelativeLocation(FVector(-170.0f, TunnelCenterY, 140.0f));
	LeftWall->SetRelativeScale3D(FVector(0.40f, TunnelLength / 100.0f, 2.80f));
	LeftWall->SetCollisionProfileName(TEXT("BlockAll"));
	LeftWall->ComponentTags.Add(FName("Concrete"));
	LeftWall->ComponentTags.Add(FName("Culvert_Wall"));
	CulvertComponents.Add(LeftWall);

	// 3. Right Wall (X = +170.0f, Thickness 40cm, Height 280cm)
	UStaticMeshComponent* RightWall = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Culvert_Right_Wall"));
	RightWall->SetupAttachment(RootComponent);
	RightWall->SetMobility(EComponentMobility::Movable);
	if (CubeMesh) RightWall->SetStaticMesh(CubeMesh);
	if (BaseMat) RightWall->SetMaterial(0, BaseMat);
	RightWall->SetRelativeLocation(FVector(170.0f, TunnelCenterY, 140.0f));
	RightWall->SetRelativeScale3D(FVector(0.40f, TunnelLength / 100.0f, 2.80f));
	RightWall->SetCollisionProfileName(TEXT("BlockAll"));
	RightWall->ComponentTags.Add(FName("Concrete"));
	RightWall->ComponentTags.Add(FName("Culvert_Wall"));
	CulvertComponents.Add(RightWall);

	// 4. Heavy Reinforced Ceiling Slab (Z = +310.0f, Thickness 60cm, supports road at Z = +420.0f)
	UStaticMeshComponent* CeilingSlab = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Culvert_Ceiling_Slab"));
	CeilingSlab->SetupAttachment(RootComponent);
	CeilingSlab->SetMobility(EComponentMobility::Movable);
	if (CubeMesh) CeilingSlab->SetStaticMesh(CubeMesh);
	if (BaseMat) CeilingSlab->SetMaterial(0, BaseMat);
	CeilingSlab->SetRelativeLocation(FVector(0.0f, TunnelCenterY, 310.0f));
	CeilingSlab->SetRelativeScale3D(FVector(3.80f, TunnelLength / 100.0f, 0.60f));
	CeilingSlab->SetCollisionProfileName(TEXT("BlockAll"));
	CeilingSlab->ComponentTags.Add(FName("Concrete"));
	CulvertComponents.Add(CeilingSlab);

	// 5. Overhead Sluice Gate Rusted Steel Rib Arches (spaced every 6 meters inside)
	const float RibYOffsets[4] = { -1200.0f, -400.0f, 400.0f, 1200.0f };
	for (int32 r = 0; r < 4; ++r)
	{
		FString RibName = FString::Printf(TEXT("Culvert_Sluice_Rib_%02d"), r);
		UStaticMeshComponent* Rib = CreateDefaultSubobject<UStaticMeshComponent>(*RibName);
		Rib->SetupAttachment(RootComponent);
		Rib->SetMobility(EComponentMobility::Movable);
		if (CubeMesh) Rib->SetStaticMesh(CubeMesh);
		if (BaseMat) Rib->SetMaterial(0, BaseMat);
		Rib->SetRelativeLocation(FVector(0.0f, RibYOffsets[r], 270.0f));
		Rib->SetRelativeScale3D(FVector(3.00f, 0.35f, 0.20f)); // Overhead steel rib
		Rib->SetCollisionProfileName(TEXT("BlockAll"));
		Rib->ComponentTags.Add(FName("Sluice_Steel"));
		CulvertComponents.Add(Rib);
	}

	// 6. Seaward Concrete Wingwall Portal (Flared 45 degrees at Y = -1800)
	UStaticMeshComponent* SeaWingL = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Culvert_Sea_Wing_L"));
	SeaWingL->SetupAttachment(RootComponent);
	SeaWingL->SetMobility(EComponentMobility::Movable);
	if (CubeMesh) SeaWingL->SetStaticMesh(CubeMesh);
	if (BaseMat) SeaWingL->SetMaterial(0, BaseMat);
	SeaWingL->SetRelativeLocation(FVector(-320.0f, -1920.0f, 150.0f));
	SeaWingL->SetRelativeRotation(FRotator(0.0f, 35.0f, 0.0f));
	SeaWingL->SetRelativeScale3D(FVector(0.40f, 3.20f, 3.00f));
	SeaWingL->SetCollisionProfileName(TEXT("BlockAll"));
	SeaWingL->ComponentTags.Add(FName("Concrete"));
	CulvertComponents.Add(SeaWingL);

	UStaticMeshComponent* SeaWingR = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Culvert_Sea_Wing_R"));
	SeaWingR->SetupAttachment(RootComponent);
	SeaWingR->SetMobility(EComponentMobility::Movable);
	if (CubeMesh) SeaWingR->SetStaticMesh(CubeMesh);
	if (BaseMat) SeaWingR->SetMaterial(0, BaseMat);
	SeaWingR->SetRelativeLocation(FVector(320.0f, -1920.0f, 150.0f));
	SeaWingR->SetRelativeRotation(FRotator(0.0f, -35.0f, 0.0f));
	SeaWingR->SetRelativeScale3D(FVector(0.40f, 3.20f, 3.00f));
	SeaWingR->SetCollisionProfileName(TEXT("BlockAll"));
	SeaWingR->ComponentTags.Add(FName("Concrete"));
	CulvertComponents.Add(SeaWingR);

	// 7. Landward Concrete Wingwall Portal (Flared 45 degrees at Y = +1400)
	UStaticMeshComponent* LandWingL = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Culvert_Land_Wing_L"));
	LandWingL->SetupAttachment(RootComponent);
	LandWingL->SetMobility(EComponentMobility::Movable);
	if (CubeMesh) LandWingL->SetStaticMesh(CubeMesh);
	if (BaseMat) LandWingL->SetMaterial(0, BaseMat);
	LandWingL->SetRelativeLocation(FVector(-320.0f, 1520.0f, 150.0f));
	LandWingL->SetRelativeRotation(FRotator(0.0f, -35.0f, 0.0f));
	LandWingL->SetRelativeScale3D(FVector(0.40f, 3.20f, 3.00f));
	LandWingL->SetCollisionProfileName(TEXT("BlockAll"));
	LandWingL->ComponentTags.Add(FName("Concrete"));
	CulvertComponents.Add(LandWingL);

	UStaticMeshComponent* LandWingR = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Culvert_Land_Wing_R"));
	LandWingR->SetupAttachment(RootComponent);
	LandWingR->SetMobility(EComponentMobility::Movable);
	if (CubeMesh) LandWingR->SetStaticMesh(CubeMesh);
	if (BaseMat) LandWingR->SetMaterial(0, BaseMat);
	LandWingR->SetRelativeLocation(FVector(320.0f, 1520.0f, 150.0f));
	LandWingR->SetRelativeRotation(FRotator(0.0f, 35.0f, 0.0f));
	LandWingR->SetRelativeScale3D(FVector(0.40f, 3.20f, 3.00f));
	LandWingR->SetCollisionProfileName(TEXT("BlockAll"));
	LandWingR->ComponentTags.Add(FName("Concrete"));
	CulvertComponents.Add(LandWingR);
}
