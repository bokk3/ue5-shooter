#include "Map/FPSCoastalOperationsMapActor.h"
#include "Map/FPSCoastalSplineActor.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/BoxComponent.h"
#include "Components/ChildActorComponent.h"
#include "GameFramework/PlayerStart.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/World.h"

AFPSCoastalOperationsMapActor::AFPSCoastalOperationsMapActor()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	// 1. Cinematic North Sea Overcast / Maritime Sun Lighting (Pure Dynamic Lumen)
	SunLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("SunLight"));
	SunLight->SetupAttachment(RootComponent);
	SunLight->SetMobility(EComponentMobility::Movable);
	SunLight->SetRelativeRotation(FRotator(-34.0f, 62.0f, 0.0f));
	SunLight->SetIntensity(8.5f); // Calibrated maritime sun lux for locked manual exposure
	SunLight->SetLightColor(FLinearColor(1.0f, 0.95f, 0.88f)); // Golden maritime sun through light sea spray
	SunLight->bAtmosphereSunLight = true;
	SunLight->bCastVolumetricShadow = true;
	SunLight->CastShadows = 1;

	SkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
	SkyLight->SetupAttachment(RootComponent);
	SkyLight->SetMobility(EComponentMobility::Movable);
	SkyLight->SetIntensity(1.8f);
	SkyLight->SetLightColor(FLinearColor(0.80f, 0.90f, 1.0f)); // Cold ocean sky ambient
	SkyLight->bRealTimeCapture = true;

	SkyAtmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("SkyAtmosphere"));
	SkyAtmosphere->SetupAttachment(RootComponent);

	HeightFog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("HeightFog"));
	HeightFog->SetupAttachment(RootComponent);
	HeightFog->SetFogDensity(0.0022f);
	HeightFog->SetFogInscatteringColor(FLinearColor(0.78f, 0.86f, 0.96f)); // Volumetric maritime sea-mist

	PierHeadNavBeacon = CreateDefaultSubobject<UPointLightComponent>(TEXT("PierHeadNavBeacon"));
	PierHeadNavBeacon->SetupAttachment(RootComponent);
	PierHeadNavBeacon->SetMobility(EComponentMobility::Movable);
	PierHeadNavBeacon->SetRelativeLocation(FVector(0.0f, -5800.0f, 450.0f));
	PierHeadNavBeacon->SetIntensity(25.0f);
	PierHeadNavBeacon->SetLightColor(FLinearColor(0.0f, 0.90f, 1.0f)); // Tactical cyan navigational beacon
	PierHeadNavBeacon->SetAttenuationRadius(2400.0f);

	DikeHazardBeacon = CreateDefaultSubobject<UPointLightComponent>(TEXT("DikeHazardBeacon"));
	DikeHazardBeacon->SetupAttachment(RootComponent);
	DikeHazardBeacon->SetMobility(EComponentMobility::Movable);
	DikeHazardBeacon->SetRelativeLocation(FVector(0.0f, -720.0f, 620.0f));
	DikeHazardBeacon->SetIntensity(30.0f);
	DikeHazardBeacon->SetLightColor(FLinearColor(1.0f, 0.25f, 0.05f)); // Amber hazard beacon on Dike Flag B
	DikeHazardBeacon->SetAttenuationRadius(2000.0f);

	// Post Process: Lock Exposure to prevent high-albedo sand blowout (AEM_Manual)
	PostProcess = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcess"));
	PostProcess->SetupAttachment(RootComponent);
	PostProcess->bUnbound = true;
	PostProcess->Settings.bOverride_AutoExposureMethod = true;
	PostProcess->Settings.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
	PostProcess->Settings.bOverride_AutoExposureBias = true;
	PostProcess->Settings.AutoExposureBias = 0.0f;
	PostProcess->Settings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
	PostProcess->Settings.AutoExposureApplyPhysicalCameraExposure = 0;
	PostProcess->Settings.bOverride_AutoExposureMinBrightness = true;
	PostProcess->Settings.AutoExposureMinBrightness = 1.0f;
	PostProcess->Settings.bOverride_AutoExposureMaxBrightness = true;
	PostProcess->Settings.AutoExposureMaxBrightness = 1.0f;
	PostProcess->Settings.bOverride_BloomIntensity = true;
	PostProcess->Settings.BloomIntensity = 0.15f;

	// 2. Embedded Seawall Dike Sub-Actor
	CoastalDikeSubActor = CreateDefaultSubobject<UChildActorComponent>(TEXT("CoastalDikeSubActor"));
	CoastalDikeSubActor->SetupAttachment(RootComponent);
	CoastalDikeSubActor->SetChildActorClass(AFPSCoastalSplineActor::StaticClass());
	CoastalDikeSubActor->SetRelativeLocation(FVector(0.0f, -500.0f, 0.0f));

	// 3. Load Base Meshes & Materials
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BaseMatFinder(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	UStaticMesh* CubeMesh = CubeFinder.Succeeded() ? CubeFinder.Object : nullptr;
	UStaticMesh* CylinderMesh = CylinderFinder.Succeeded() ? CylinderFinder.Object : nullptr;
	UMaterialInterface* BaseMat = BaseMatFinder.Succeeded() ? BaseMatFinder.Object : nullptr;

	// 4. Construct Modular Zones
	ConstructWaterAndBeach(CubeMesh, BaseMat);
	ConstructMaritimePier(CubeMesh, CylinderMesh, BaseMat);
	ConstructCoastalDunes(CubeMesh, BaseMat);
	ConstructPerimeterBoundaries(CubeMesh, BaseMat);
	ConstructObjectives(CylinderMesh, BaseMat);
}

void AFPSCoastalOperationsMapActor::BeginPlay()
{
	Super::BeginPlay();

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

	// 1. Beach sand & ocean water
	ApplyTint(BeachFloorMesh, FLinearColor(0.82f, 0.76f, 0.62f), 0.95f);
	ApplyTint(WaterPlaneMesh, FLinearColor(0.04f, 0.16f, 0.22f), 0.08f);

	// 2. Weathered maritime pier planks
	for (UStaticMeshComponent* Plank : PierVisualPlanks)
	{
		ApplyTint(Plank, FLinearColor(0.28f, 0.29f, 0.30f), 0.90f);
	}

	// 3. Pilings (weathered timber + concrete)
	for (UStaticMeshComponent* Piling : PierPilings)
	{
		ApplyTint(Piling, FLinearColor(0.22f, 0.23f, 0.24f), 0.85f);
	}

	// 4. Transverse cross-beams
	for (UStaticMeshComponent* Beam : PierCrossBeams)
	{
		ApplyTint(Beam, FLinearColor(0.20f, 0.21f, 0.22f), 0.90f);
	}

	// 5. Cast-iron mooring bollards
	for (UStaticMeshComponent* Bollard : PierBollards)
	{
		ApplyTint(Bollard, FLinearColor(0.18f, 0.14f, 0.12f), 0.60f); // Rusted iron
	}

	// 6. Galvanized steel tube railings
	for (UStaticMeshComponent* Railing : PierRailings)
	{
		ApplyTint(Railing, FLinearColor(0.55f, 0.57f, 0.60f), 0.40f);
	}

	// 7. Marine access ladders
	for (UStaticMeshComponent* Ladder : PierLadders)
	{
		ApplyTint(Ladder, FLinearColor(0.35f, 0.36f, 0.38f), 0.50f);
	}

	// 8. Coastal dunes (stoss vs slip face)
	for (UStaticMeshComponent* Dune : DuneComponents)
	{
		if (Dune->ComponentTags.Contains(FName("Dune_SlipFace")))
		{
			ApplyTint(Dune, FLinearColor(0.78f, 0.70f, 0.56f), 0.95f); // Deeper shadow on slip face
		}
		else
		{
			ApplyTint(Dune, FLinearColor(0.86f, 0.79f, 0.64f), 0.92f); // Sunlit stoss sand
		}
	}

	// 9. Perimeter boundaries
	for (UStaticMeshComponent* Wall : PerimeterWalls)
	{
		ApplyTint(Wall, FLinearColor(0.32f, 0.34f, 0.36f), 0.85f);
	}

	// 10. Domination Objectives (A, B, C)
	for (UStaticMeshComponent* Flag : ObjectiveFlags)
	{
		ApplyTint(Flag, FLinearColor(0.95f, 0.65f, 0.15f), 0.30f);
	}

	SpawnTacticalPlayerStarts();
}

void AFPSCoastalOperationsMapActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Pulse Navigational Maritime Hazard Beacons
	BeaconPulseTimer += DeltaTime * 2.5f;

	if (PierHeadNavBeacon)
	{
		float CyanPulse = 2000.0f + 1500.0f * FMath::Sin(BeaconPulseTimer);
		PierHeadNavBeacon->SetIntensity(CyanPulse);
	}

	if (DikeHazardBeacon)
	{
		float AmberPulse = 2500.0f + 2000.0f * FMath::Sin(BeaconPulseTimer + 1.5f);
		DikeHazardBeacon->SetIntensity(AmberPulse);
	}
}

void AFPSCoastalOperationsMapActor::ConstructWaterAndBeach(UStaticMesh* CubeMesh, UMaterialInterface* BaseMat)
{
	// 1. Full 120m x 120m Beach Substrate Ground Plane (Z = -50cm, thickness 100cm)
	BeachFloorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BeachFloorMesh"));
	BeachFloorMesh->SetupAttachment(RootComponent);
	BeachFloorMesh->SetMobility(EComponentMobility::Movable);
	if (CubeMesh) BeachFloorMesh->SetStaticMesh(CubeMesh);
	if (BaseMat) BeachFloorMesh->SetMaterial(0, BaseMat);
	BeachFloorMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -50.0f));
	BeachFloorMesh->SetRelativeScale3D(FVector(120.0f, 120.0f, 1.0f)); // 120m x 120m
	BeachFloorMesh->SetCollisionProfileName(TEXT("BlockAll"));
	BeachFloorMesh->SetCastShadow(true);
	BeachFloorMesh->ComponentTags.Add(FName("Surface_Sand_Dune"));

	// 2. North Sea Shallow Water Surface (Z = +30.0f cm, covering seaward half Y = -6500 to Y = -1700)
	WaterPlaneMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WaterPlaneMesh"));
	WaterPlaneMesh->SetupAttachment(RootComponent);
	WaterPlaneMesh->SetMobility(EComponentMobility::Movable);
	if (CubeMesh) WaterPlaneMesh->SetStaticMesh(CubeMesh);
	if (BaseMat) WaterPlaneMesh->SetMaterial(0, BaseMat);
	WaterPlaneMesh->SetRelativeLocation(FVector(0.0f, -4100.0f, 28.0f));
	WaterPlaneMesh->SetRelativeScale3D(FVector(120.0f, 48.0f, 0.05f)); // 120m wide x 48m seaward
	WaterPlaneMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // Visual water only
	WaterPlaneMesh->SetCastShadow(false);
	WaterPlaneMesh->ComponentTags.Add(FName("Surface_Shallow_Water"));

	// 3. Water Overlap Volume Box (for wading sound / acoustic splash triangulation)
	WaterVolumeBox = CreateDefaultSubobject<UBoxComponent>(TEXT("WaterVolumeBox"));
	WaterVolumeBox->SetupAttachment(RootComponent);
	WaterVolumeBox->SetMobility(EComponentMobility::Movable);
	WaterVolumeBox->SetRelativeLocation(FVector(0.0f, -4100.0f, 15.0f));
	WaterVolumeBox->SetBoxExtent(FVector(6000.0f, 2400.0f, 25.0f));
	WaterVolumeBox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	WaterVolumeBox->ComponentTags.Add(FName("Surface_Shallow_Water"));
}

void AFPSCoastalOperationsMapActor::ConstructMaritimePier(UStaticMesh* CubeMesh, UStaticMesh* CylinderMesh, UMaterialInterface* BaseMat)
{
	// =========================================================================
	// 48-METER MODULAR TIMBER & CONCRETE MARITIME PIER (Domination Flag C)
	// Extends from Seawall Dike Abutment (Y = -1700) to Pierhead (Y = -6500)
	// Deck Surface Level = Z = +260.0f cm (230cm above water plane Z = 30cm)
	// =========================================================================

	// -------------------------------------------------------------------------
	// CRITICAL: 128-TICK SNAG-FREE COLLISION (UCX_ Flat Box Rule)
	// Single, perfectly flat, zero-seam invisible collision boxes spanning deck!
	// -------------------------------------------------------------------------
	// Box 1: Main Pier Walkway (Width 600cm, Length 3500cm, Z = +250cm, Top = +260cm)
	UCX_Pier_Deck = CreateDefaultSubobject<UBoxComponent>(TEXT("UCX_Pier_Deck"));
	UCX_Pier_Deck->SetupAttachment(RootComponent);
	UCX_Pier_Deck->SetMobility(EComponentMobility::Movable);
	UCX_Pier_Deck->SetRelativeLocation(FVector(0.0f, -3450.0f, 250.0f));
	UCX_Pier_Deck->SetBoxExtent(FVector(300.0f, 1750.0f, 10.0f)); // 6m wide x 35m long
	UCX_Pier_Deck->SetCollisionProfileName(TEXT("BlockAll"));
	UCX_Pier_Deck->ComponentTags.Add(FName("Surface_Pier_Wood"));
	UCX_Pier_Deck->ComponentTags.Add(FName("Pier_Deck"));

	// Box 2: Pierhead T-Junction Platform (Width 1800cm, Length 1300cm, Z = +250cm)
	UCX_Pier_THead = CreateDefaultSubobject<UBoxComponent>(TEXT("UCX_Pier_THead"));
	UCX_Pier_THead->SetupAttachment(RootComponent);
	UCX_Pier_THead->SetMobility(EComponentMobility::Movable);
	UCX_Pier_THead->SetRelativeLocation(FVector(0.0f, -5850.0f, 250.0f));
	UCX_Pier_THead->SetBoxExtent(FVector(900.0f, 650.0f, 10.0f)); // 18m wide x 13m deep
	UCX_Pier_THead->SetCollisionProfileName(TEXT("BlockAll"));
	UCX_Pier_THead->ComponentTags.Add(FName("Surface_Pier_Wood"));
	UCX_Pier_THead->ComponentTags.Add(FName("Pier_THead"));

	// -------------------------------------------------------------------------
	// Visual Planks: NO COLLISION (SetCollisionEnabled::NoCollision)
	// Individual weathered planks with 1.5cm spacing and subtle height variation
	// -------------------------------------------------------------------------
	const float PlankWidth = 22.0f; // 22cm wide planks
	const float PlankGap = 1.5f;   // 1.5cm visual gap
	const float PlankStride = PlankWidth + PlankGap;

	// Main walkway planks (from Y = -1700 to -5200)
	const int32 NumMainPlanks = 148;
	for (int32 p = 0; p < NumMainPlanks; ++p)
	{
		const float PlankY = -1720.0f - p * PlankStride;
		const float HeightJitter = FMath::Sin(p * 1.73f) * 0.35f; // +/- 0.35cm visual roughness

		FString PlankName = FString::Printf(TEXT("Pier_Plank_Main_%03d"), p);
		UStaticMeshComponent* Plank = CreateDefaultSubobject<UStaticMeshComponent>(*PlankName);
		Plank->SetupAttachment(RootComponent);
		Plank->SetMobility(EComponentMobility::Movable);
		if (CubeMesh) Plank->SetStaticMesh(CubeMesh);
		if (BaseMat) Plank->SetMaterial(0, BaseMat);
		Plank->SetRelativeLocation(FVector(0.0f, PlankY, 255.0f + HeightJitter));
		Plank->SetRelativeScale3D(FVector(6.00f, PlankWidth / 100.0f, 0.10f));
		Plank->SetCollisionEnabled(ECollisionEnabled::NoCollision); // Snag-free rule
		Plank->SetCastShadow(true);
		Plank->ComponentTags.Add(FName("Surface_Pier_Wood"));
		PierVisualPlanks.Add(Plank);
	}

	// T-Junction Platform Planks (from Y = -5200 to -6500)
	const int32 NumTPlanks = 54;
	for (int32 p = 0; p < NumTPlanks; ++p)
	{
		const float PlankY = -5210.0f - p * PlankStride;
		const float HeightJitter = FMath::Cos(p * 2.11f) * 0.35f;

		FString PlankName = FString::Printf(TEXT("Pier_Plank_THead_%03d"), p);
		UStaticMeshComponent* Plank = CreateDefaultSubobject<UStaticMeshComponent>(*PlankName);
		Plank->SetupAttachment(RootComponent);
		Plank->SetMobility(EComponentMobility::Movable);
		if (CubeMesh) Plank->SetStaticMesh(CubeMesh);
		if (BaseMat) Plank->SetMaterial(0, BaseMat);
		Plank->SetRelativeLocation(FVector(0.0f, PlankY, 255.0f + HeightJitter));
		Plank->SetRelativeScale3D(FVector(18.00f, PlankWidth / 100.0f, 0.10f));
		Plank->SetCollisionEnabled(ECollisionEnabled::NoCollision); // Snag-free rule
		Plank->SetCastShadow(true);
		Plank->ComponentTags.Add(FName("Surface_Pier_Wood"));
		PierVisualPlanks.Add(Plank);
	}

	// -------------------------------------------------------------------------
	// Pilings (Weathered Timber & Octagonal Concrete) & Transverse Cross-Beams
	// 4m x 3m modular grid bays
	// -------------------------------------------------------------------------
	const int32 NumBents = 12; // 12 bays along length
	for (int32 b = 0; b < NumBents; ++b)
	{
		const float BentY = -1800.0f - b * 400.0f;
		const bool bIsTHead = (BentY <= -5200.0f);

		// Number of pilings in this bent
		const int32 PilingsInBent = bIsTHead ? 5 : 3;
		const float PilingSpacing = bIsTHead ? 380.0f : 240.0f;

		for (int32 p = 0; p < PilingsInBent; ++p)
		{
			const float PilingX = (p - (PilingsInBent - 1) * 0.5f) * PilingSpacing;

			// Timber Pillar (0.4m x 0.4m x 6.0m, driven from Z = -250 to +250)
			FString PilingName = FString::Printf(TEXT("Pier_Piling_%d_%d"), b, p);
			UStaticMeshComponent* Piling = CreateDefaultSubobject<UStaticMeshComponent>(*PilingName);
			Piling->SetupAttachment(RootComponent);
			Piling->SetMobility(EComponentMobility::Movable);
			if (CylinderMesh) Piling->SetStaticMesh(CylinderMesh);
			if (BaseMat) Piling->SetMaterial(0, BaseMat);
			Piling->SetRelativeLocation(FVector(PilingX, BentY, 0.0f));
			Piling->SetRelativeScale3D(FVector(0.42f, 0.42f, 5.20f));
			Piling->SetCollisionProfileName(TEXT("BlockAll"));
			Piling->SetCastShadow(true);
			Piling->ComponentTags.Add(FName("Surface_Pier_Wood"));
			Piling->ComponentTags.Add(FName("Piling"));
			PierPilings.Add(Piling);
		}

		// Heavy Transverse Cross-Beam (0.3m x 6.0m x 0.4m under deck at Z = 230cm)
		const float BeamWidth = bIsTHead ? 18.0f : 6.0f;
		FString BeamName = FString::Printf(TEXT("Pier_CrossBeam_%02d"), b);
		UStaticMeshComponent* Beam = CreateDefaultSubobject<UStaticMeshComponent>(*BeamName);
		Beam->SetupAttachment(RootComponent);
		Beam->SetMobility(EComponentMobility::Movable);
		if (CubeMesh) Beam->SetStaticMesh(CubeMesh);
		if (BaseMat) Beam->SetMaterial(0, BaseMat);
		Beam->SetRelativeLocation(FVector(0.0f, BentY, 230.0f));
		Beam->SetRelativeScale3D(FVector(BeamWidth, 0.35f, 0.40f));
		Beam->SetCollisionProfileName(TEXT("BlockAll"));
		Beam->ComponentTags.Add(FName("Surface_Pier_Wood"));
		PierCrossBeams.Add(Beam);
	}

	// -------------------------------------------------------------------------
	// Cast-Iron Mooring Bollards (Waist-high ballistic cover: 0.6m x 0.6m x 0.7m)
	// -------------------------------------------------------------------------
	auto AddBollard = [this, CylinderMesh, BaseMat](const FString& Name, const FVector& Loc)
	{
		UStaticMeshComponent* Bollard = CreateDefaultSubobject<UStaticMeshComponent>(*Name);
		Bollard->SetupAttachment(RootComponent);
		Bollard->SetMobility(EComponentMobility::Movable);
		if (CylinderMesh) Bollard->SetStaticMesh(CylinderMesh);
		if (BaseMat) Bollard->SetMaterial(0, BaseMat);
		Bollard->SetRelativeLocation(Loc);
		Bollard->SetRelativeScale3D(FVector(0.60f, 0.60f, 0.70f)); // Waist-high cover (70cm)
		Bollard->SetCollisionProfileName(TEXT("BlockAll"));
		Bollard->SetCastShadow(true);
		Bollard->ComponentTags.Add(FName("Cover_Waist"));
		Bollard->ComponentTags.Add(FName("Surface_Pier_Wood"));
		Bollard->ComponentTags.Add(FName("Bollard"));
		PierBollards.Add(Bollard);
	};

	// Main walkway flank bollards
	AddBollard(TEXT("Bollard_Main_L1"), FVector(-270.0f, -2400.0f, 295.0f));
	AddBollard(TEXT("Bollard_Main_R1"), FVector(270.0f, -2400.0f, 295.0f));
	AddBollard(TEXT("Bollard_Main_L2"), FVector(-270.0f, -3800.0f, 295.0f));
	AddBollard(TEXT("Bollard_Main_R2"), FVector(270.0f, -3800.0f, 295.0f));

	// T-Junction Platform bollards (Flag C defense perimeter)
	AddBollard(TEXT("Bollard_THead_NW"), FVector(-860.0f, -5300.0f, 295.0f));
	AddBollard(TEXT("Bollard_THead_NE"), FVector(860.0f, -5300.0f, 295.0f));
	AddBollard(TEXT("Bollard_THead_SW"), FVector(-860.0f, -6350.0f, 295.0f));
	AddBollard(TEXT("Bollard_THead_SE"), FVector(860.0f, -6350.0f, 295.0f));
	AddBollard(TEXT("Bollard_THead_CenterL"), FVector(-200.0f, -5800.0f, 295.0f));
	AddBollard(TEXT("Bollard_THead_CenterR"), FVector(200.0f, -5800.0f, 295.0f));

	// -------------------------------------------------------------------------
	// Galvanized Steel Tube Railings (1.1m height, vaultable collision tags)
	// -------------------------------------------------------------------------
	auto AddRailing = [this, CubeMesh, BaseMat](const FString& Name, const FVector& Loc, const FVector& Scale, const FRotator& Rot = FRotator::ZeroRotator)
	{
		UStaticMeshComponent* Railing = CreateDefaultSubobject<UStaticMeshComponent>(*Name);
		Railing->SetupAttachment(RootComponent);
		Railing->SetMobility(EComponentMobility::Movable);
		if (CubeMesh) Railing->SetStaticMesh(CubeMesh);
		if (BaseMat) Railing->SetMaterial(0, BaseMat);
		Railing->SetRelativeLocation(Loc);
		Railing->SetRelativeRotation(Rot);
		Railing->SetRelativeScale3D(Scale);
		Railing->SetCollisionProfileName(TEXT("BlockAll"));
		Railing->ComponentTags.Add(FName("Vaultable"));
		Railing->ComponentTags.Add(FName("Metal"));
		PierRailings.Add(Railing);
	};

	// Main Walkway Left & Right Railings (leaving gap for ladders)
	AddRailing(TEXT("Railing_Main_L_A"), FVector(-295.0f, -2350.0f, 315.0f), FVector(0.12f, 13.0f, 1.10f));
	AddRailing(TEXT("Railing_Main_L_B"), FVector(-295.0f, -4350.0f, 315.0f), FVector(0.12f, 15.0f, 1.10f));

	AddRailing(TEXT("Railing_Main_R_A"), FVector(295.0f, -2350.0f, 315.0f), FVector(0.12f, 13.0f, 1.10f));
	AddRailing(TEXT("Railing_Main_R_B"), FVector(295.0f, -4350.0f, 315.0f), FVector(0.12f, 15.0f, 1.10f));

	// T-Head Seaward Railings (North, West, East)
	AddRailing(TEXT("Railing_THead_South"), FVector(0.0f, -6480.0f, 315.0f), FVector(17.80f, 0.12f, 1.10f));
	AddRailing(TEXT("Railing_THead_West"), FVector(-895.0f, -5850.0f, 315.0f), FVector(0.12f, 12.80f, 1.10f));
	AddRailing(TEXT("Railing_THead_East"), FVector(895.0f, -5850.0f, 315.0f), FVector(0.12f, 12.80f, 1.10f));

	// -------------------------------------------------------------------------
	// Marine Steel Access Ladders (Rising from water Z = 0 to deck Z = 260)
	// -------------------------------------------------------------------------
	auto AddLadder = [this, CubeMesh, BaseMat](const FString& Name, const FVector& Loc)
	{
		UStaticMeshComponent* Ladder = CreateDefaultSubobject<UStaticMeshComponent>(*Name);
		Ladder->SetupAttachment(RootComponent);
		Ladder->SetMobility(EComponentMobility::Movable);
		if (CubeMesh) Ladder->SetStaticMesh(CubeMesh);
		if (BaseMat) Ladder->SetMaterial(0, BaseMat);
		Ladder->SetRelativeLocation(Loc);
		Ladder->SetRelativeScale3D(FVector(0.15f, 0.65f, 2.70f)); // 2.7m ladder
		Ladder->SetCollisionProfileName(TEXT("BlockAll"));
		Ladder->ComponentTags.Add(FName("Ladder"));
		Ladder->ComponentTags.Add(FName("Climbable"));
		PierLadders.Add(Ladder);
	};

	// Ladder 1: Left Flank at Y = -3200
	AddLadder(TEXT("Pier_Ladder_Left"), FVector(-308.0f, -3200.0f, 135.0f));
	// Ladder 2: Right Flank at Y = -3400
	AddLadder(TEXT("Pier_Ladder_Right"), FVector(308.0f, -3400.0f, 135.0f));
}

void AFPSCoastalOperationsMapActor::ConstructCoastalDunes(UStaticMesh* CubeMesh, UMaterialInterface* BaseMat)
{
	// =========================================================================
	// BACKCOUNTRY DUNE FIELD (Y = +800 to +5800)
	// Geomorphological 12-deg Stoss (Windward) & 32-deg Slip-Face (Leeward)
	// =========================================================================
	auto AddDune = [this, CubeMesh, BaseMat](const FString& Name, const FVector& Loc, const FRotator& Rot, const FVector& Scale, bool bIsSlipFace)
	{
		UStaticMeshComponent* Dune = CreateDefaultSubobject<UStaticMeshComponent>(*Name);
		Dune->SetupAttachment(RootComponent);
		Dune->SetMobility(EComponentMobility::Movable);
		if (CubeMesh) Dune->SetStaticMesh(CubeMesh);
		if (BaseMat) Dune->SetMaterial(0, BaseMat);
		Dune->SetRelativeLocation(Loc);
		Dune->SetRelativeRotation(Rot);
		Dune->SetRelativeScale3D(Scale);
		Dune->SetCollisionProfileName(TEXT("BlockAll"));
		Dune->SetCastShadow(true);
		Dune->ComponentTags.Add(FName("Surface_Sand_Dune"));
		if (bIsSlipFace)
		{
			Dune->ComponentTags.Add(FName("Dune_SlipFace"));
		}
		DuneComponents.Add(Dune);
	};

	// 1. Primary Fore-Dune Ridge (Y = +1800, Height 450cm)
	// Stoss slope (12 deg facing seaward):
	AddDune(TEXT("Dune_Fore_Stoss_W"), FVector(-2800.0f, 1550.0f, 210.0f), FRotator(12.0f, 0.0f, 0.0f), FVector(52.0f, 14.0f, 1.2f), false);
	AddDune(TEXT("Dune_Fore_Stoss_E"), FVector(2800.0f, 1550.0f, 210.0f), FRotator(12.0f, 0.0f, 0.0f), FVector(52.0f, 14.0f, 1.2f), false);

	// Crest Ridge:
	AddDune(TEXT("Dune_Fore_Crest_W"), FVector(-2800.0f, 1850.0f, 430.0f), FRotator::ZeroRotator, FVector(52.0f, 3.0f, 0.8f), false);
	AddDune(TEXT("Dune_Fore_Crest_E"), FVector(2800.0f, 1850.0f, 430.0f), FRotator::ZeroRotator, FVector(52.0f, 3.0f, 0.8f), false);

	// Slip face (32 deg angle of repose facing inland):
	AddDune(TEXT("Dune_Fore_Slip_W"), FVector(-2800.0f, 2100.0f, 220.0f), FRotator(-32.0f, 0.0f, 0.0f), FVector(52.0f, 7.5f, 1.1f), true);
	AddDune(TEXT("Dune_Fore_Slip_E"), FVector(2800.0f, 2100.0f, 220.0f), FRotator(-32.0f, 0.0f, 0.0f), FVector(52.0f, 7.5f, 1.1f), true);

	// 2. Secondary Transverse Dune Ridge (Y = +3400, Height 520cm)
	AddDune(TEXT("Dune_Trans_Stoss_W"), FVector(-2500.0f, 3100.0f, 240.0f), FRotator(13.5f, 0.0f, 0.0f), FVector(48.0f, 15.0f, 1.4f), false);
	AddDune(TEXT("Dune_Trans_Stoss_E"), FVector(2500.0f, 3100.0f, 240.0f), FRotator(13.5f, 0.0f, 0.0f), FVector(48.0f, 15.0f, 1.4f), false);

	AddDune(TEXT("Dune_Trans_Crest_W"), FVector(-2500.0f, 3450.0f, 500.0f), FRotator::ZeroRotator, FVector(48.0f, 3.5f, 0.9f), false);
	AddDune(TEXT("Dune_Trans_Crest_E"), FVector(2500.0f, 3450.0f, 500.0f), FRotator::ZeroRotator, FVector(48.0f, 3.5f, 0.9f), false);

	AddDune(TEXT("Dune_Trans_Slip_W"), FVector(-2500.0f, 3720.0f, 260.0f), FRotator(-32.0f, 0.0f, 0.0f), FVector(48.0f, 8.5f, 1.2f), true);
	AddDune(TEXT("Dune_Trans_Slip_E"), FVector(2500.0f, 3720.0f, 260.0f), FRotator(-32.0f, 0.0f, 0.0f), FVector(48.0f, 8.5f, 1.2f), true);

	// 3. Central Interdune Basin & Flag A Defilade Dunes
	AddDune(TEXT("Dune_Basin_Cover_W"), FVector(-800.0f, 3950.0f, 180.0f), FRotator(0.0f, 35.0f, 0.0f), FVector(12.0f, 10.0f, 3.5f), false);
	AddDune(TEXT("Dune_Basin_Cover_E"), FVector(800.0f, 3950.0f, 180.0f), FRotator(0.0f, -35.0f, 0.0f), FVector(12.0f, 10.0f, 3.5f), false);

	// 4. Barchanoid Parabolic Dunes (West & East High Flank Sniping Bluffs)
	AddDune(TEXT("Dune_Peak_West"), FVector(-4800.0f, 4400.0f, 280.0f), FRotator(0.0f, 25.0f, 0.0f), FVector(18.0f, 18.0f, 5.5f), false);
	AddDune(TEXT("Dune_Peak_East"), FVector(4800.0f, 4400.0f, 280.0f), FRotator(0.0f, -25.0f, 0.0f), FVector(18.0f, 18.0f, 5.5f), false);
}

void AFPSCoastalOperationsMapActor::ConstructPerimeterBoundaries(UStaticMesh* CubeMesh, UMaterialInterface* BaseMat)
{
	auto AddPerimeter = [this, CubeMesh, BaseMat](const FString& Name, const FVector& Loc, const FVector& Scale)
	{
		UStaticMeshComponent* Wall = CreateDefaultSubobject<UStaticMeshComponent>(*Name);
		Wall->SetupAttachment(RootComponent);
		Wall->SetMobility(EComponentMobility::Movable);
		if (CubeMesh) Wall->SetStaticMesh(CubeMesh);
		if (BaseMat) Wall->SetMaterial(0, BaseMat);
		Wall->SetRelativeLocation(Loc);
		Wall->SetRelativeScale3D(Scale);
		Wall->SetCollisionProfileName(TEXT("BlockAll"));
		Wall->SetCastShadow(true);
		Wall->ComponentTags.Add(FName("Concrete"));
		PerimeterWalls.Add(Wall);
	};

	// 120m x 120m tactical arena outer boundary (Height 7m, Thickness 1.5m)
	AddPerimeter(TEXT("Boundary_North_Dunes"), FVector(0.0f, 5950.0f, 300.0f), FVector(120.0f, 1.5f, 7.0f));
	AddPerimeter(TEXT("Boundary_South_Ocean"), FVector(0.0f, -6550.0f, 200.0f), FVector(120.0f, 1.5f, 6.0f));
	AddPerimeter(TEXT("Boundary_East_Flank"), FVector(5950.0f, -300.0f, 250.0f), FVector(1.5f, 126.0f, 6.5f));
	AddPerimeter(TEXT("Boundary_West_Flank"), FVector(-5950.0f, -300.0f, 250.0f), FVector(1.5f, 126.0f, 6.5f));
}

void AFPSCoastalOperationsMapActor::ConstructObjectives(UStaticMesh* CylinderMesh, UMaterialInterface* BaseMat)
{
	auto AddObjective = [this, CylinderMesh, BaseMat](const FString& Name, const FVector& Loc, const FName& FlagTag)
	{
		UStaticMeshComponent* Flag = CreateDefaultSubobject<UStaticMeshComponent>(*Name);
		Flag->SetupAttachment(RootComponent);
		Flag->SetMobility(EComponentMobility::Movable);
		if (CylinderMesh) Flag->SetStaticMesh(CylinderMesh);
		if (BaseMat) Flag->SetMaterial(0, BaseMat);
		Flag->SetRelativeLocation(Loc);
		Flag->SetRelativeScale3D(FVector(3.2f, 3.2f, 0.25f)); // 3.2m domination capture circle
		Flag->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
		Flag->ComponentTags.Add(FName("Objective"));
		Flag->ComponentTags.Add(FlagTag);
		ObjectiveFlags.Add(Flag);
	};

	// Flag A: Backcountry Dune Interdune Basin
	AddObjective(TEXT("Flag_Base_A"), FVector(0.0f, 4000.0f, 15.0f), FName("FlagA"));

	// Flag B: Seawall Dike Crest Roadway (Central King-of-the-Hill at Z = 420cm)
	AddObjective(TEXT("Flag_Base_B"), FVector(0.0f, -720.0f, 425.0f), FName("FlagB"));

	// Flag C: Maritime Pierhead T-Junction over the North Sea (Z = 260cm)
	AddObjective(TEXT("Flag_Base_C"), FVector(0.0f, -5800.0f, 265.0f), FName("FlagC"));
}

void AFPSCoastalOperationsMapActor::SpawnTacticalPlayerStarts()
{
	if (bPlayerStartsSpawned) return;
	bPlayerStartsSpawned = true;

	UWorld* World = GetWorld();
	if (!World) return;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	auto CreateStart = [World, &SpawnParams](const FVector& Loc, const FRotator& Rot, const FName& Tag)
	{
		APlayerStart* Start = World->SpawnActor<APlayerStart>(Loc, Rot, SpawnParams);
		if (Start)
		{
			Start->PlayerStartTag = Tag;
			Start->Tags.Add(Tag);
		}
	};

	// -------------------------------------------------------------------------
	// 4 Task Force Alpha Spawns (Backcountry Dunes near Flag A)
	// Sheltered behind 5-meter sand crests in natural defilade troughs
	// -------------------------------------------------------------------------
	CreateStart(FVector(-1200.0f, 4800.0f, 30.0f), FRotator(0.0f, -90.0f, 0.0f), FName("TeamAlpha"));
	CreateStart(FVector(-400.0f, 5200.0f, 30.0f), FRotator(0.0f, -90.0f, 0.0f), FName("TeamAlpha"));
	CreateStart(FVector(400.0f, 5200.0f, 30.0f), FRotator(0.0f, -90.0f, 0.0f), FName("TeamAlpha"));
	CreateStart(FVector(1200.0f, 4800.0f, 30.0f), FRotator(0.0f, -90.0f, 0.0f), FName("TeamAlpha"));

	// -------------------------------------------------------------------------
	// 4 OpFor Bravo Spawns (Maritime Pier / Seawall Abutment near Flag C)
	// Shielded behind concrete revetment abutments and timber pier pilings
	// -------------------------------------------------------------------------
	CreateStart(FVector(-1800.0f, -2200.0f, 20.0f), FRotator(0.0f, 90.0f, 0.0f), FName("TeamBravo"));
	CreateStart(FVector(-800.0f, -2400.0f, 20.0f), FRotator(0.0f, 90.0f, 0.0f), FName("TeamBravo"));
	CreateStart(FVector(800.0f, -2400.0f, 20.0f), FRotator(0.0f, 90.0f, 0.0f), FName("TeamBravo"));
	CreateStart(FVector(1800.0f, -2200.0f, 20.0f), FRotator(0.0f, 90.0f, 0.0f), FName("TeamBravo"));

	UE_LOG(LogTemp, Warning, TEXT("[CoastalOps] Coastal Operations: Successfully spawned 8 PlayerStarts (4 TeamAlpha, 4 TeamBravo)."));
}
