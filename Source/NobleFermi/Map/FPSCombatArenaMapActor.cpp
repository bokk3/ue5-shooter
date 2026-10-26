#include "Map/FPSCombatArenaMapActor.h"
#include "Map/FPSCoastalMaterialLibrary.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/PostProcessComponent.h"
#include "GameFramework/PlayerStart.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/World.h"

AFPSCombatArenaMapActor::AFPSCombatArenaMapActor()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	// 1. Cinematic Golden-Hour Rio Hillside Favela Lighting
	SunLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("SunLight"));
	SunLight->SetupAttachment(RootComponent);
	SunLight->SetMobility(EComponentMobility::Movable);
	SunLight->SetRelativeRotation(FRotator(-32.0f, 42.0f, 0.0f));
	SunLight->SetIntensity(12.0f);
	SunLight->SetLightColor(FLinearColor(1.0f, 0.92f, 0.80f)); // Warm golden-hour tropical sun
	SunLight->bAtmosphereSunLight = true;
	SunLight->bCastVolumetricShadow = false;
	SunLight->CastShadows = 1;

	SkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
	SkyLight->SetupAttachment(RootComponent);
	SkyLight->SetMobility(EComponentMobility::Movable);
	SkyLight->SetIntensity(3.6f);
	SkyLight->SetLightColor(FLinearColor(0.82f, 0.90f, 1.0f));
	SkyLight->bRealTimeCapture = true;

	SkyAtmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("SkyAtmosphere"));
	SkyAtmosphere->SetupAttachment(RootComponent);

	HeightFog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("HeightFog"));
	HeightFog->SetupAttachment(RootComponent);
	HeightFog->SetFogDensity(0.0010f);
	HeightFog->SetFogInscatteringColor(FLinearColor(0.88f, 0.76f, 0.58f)); // Warm tropical golden haze

	TowerBeaconLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("TowerBeaconLight"));
	TowerBeaconLight->SetupAttachment(RootComponent);
	TowerBeaconLight->SetMobility(EComponentMobility::Movable);
	TowerBeaconLight->SetRelativeLocation(FVector(0.0f, 0.0f, 780.0f));
	TowerBeaconLight->SetIntensity(30.0f);
	TowerBeaconLight->SetLightColor(FLinearColor(1.0f, 0.58f, 0.18f)); // Warm tungsten favela rooftop bulb
	TowerBeaconLight->SetAttenuationRadius(1600.0f);
	TowerBeaconLight->SetCastShadows(false);

	// Post Process: Balanced Exposure to bring out rich favela colors & textures
	PostProcess = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcess"));
	PostProcess->SetupAttachment(RootComponent);
	PostProcess->bUnbound = true;
	PostProcess->Settings.bOverride_AutoExposureMethod = true;
	PostProcess->Settings.AutoExposureMethod = EAutoExposureMethod::AEM_Basic;
	PostProcess->Settings.bOverride_AutoExposureBias = true;
	PostProcess->Settings.AutoExposureBias = 0.5f;
	PostProcess->Settings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
	PostProcess->Settings.AutoExposureApplyPhysicalCameraExposure = 0;
	PostProcess->Settings.bOverride_AutoExposureMinBrightness = true;
	PostProcess->Settings.AutoExposureMinBrightness = 0.35f;
	PostProcess->Settings.bOverride_AutoExposureMaxBrightness = true;
	PostProcess->Settings.AutoExposureMaxBrightness = 3.0f;
	PostProcess->Settings.bOverride_BloomIntensity = true;
	PostProcess->Settings.BloomIntensity = 0.15f;

	// 2. Load Base Meshes & Materials
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BaseMatFinder(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	UStaticMesh* CubeMesh = CubeFinder.Succeeded() ? CubeFinder.Object : nullptr;
	UStaticMesh* CylinderMesh = CylinderFinder.Succeeded() ? CylinderFinder.Object : nullptr;
	UMaterialInterface* BaseMat = BaseMatFinder.Succeeded() ? BaseMatFinder.Object : nullptr;

	// 3. Cobblestone Ground Plane (60m x 60m = 6000 x 6000 units, 100 units thick)
	GroundFloorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GroundFloorMesh"));
	GroundFloorMesh->SetupAttachment(RootComponent);
	GroundFloorMesh->SetMobility(EComponentMobility::Movable);
	if (CubeMesh) GroundFloorMesh->SetStaticMesh(CubeMesh);
	if (BaseMat) GroundFloorMesh->SetMaterial(0, BaseMat);
	GroundFloorMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -50.0f));
	GroundFloorMesh->SetRelativeScale3D(FVector(60.0f, 60.0f, 1.0f));
	GroundFloorMesh->SetCollisionProfileName(TEXT("BlockAll"));
	GroundFloorMesh->SetCastShadow(true);

	// 4. Construct Favela Sub-Zones
	ConstructPerimeter(CubeMesh, BaseMat);
	ConstructCentralDerrick(CubeMesh, CylinderMesh, BaseMat);
	ConstructContainerYard(CubeMesh, BaseMat);
	ConstructPipelineAndBunker(CubeMesh, CylinderMesh, BaseMat);
	ConstructCenterCover(CubeMesh, BaseMat);
	ConstructObjectives(CylinderMesh, BaseMat);
}

void AFPSCombatArenaMapActor::ConstructPerimeter(UStaticMesh* CubeMesh, UMaterialInterface* BaseMat)
{
	auto AddWall = [this, CubeMesh, BaseMat](const FString& Name, const FVector& Loc, const FVector& Scale)
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
		PerimeterWalls.Add(Wall);
	};

	// 4 Hillside Retaining Masonry Walls (60m perimeter, 3.2m high so hillside shanties rise above them!)
	AddWall(TEXT("Wall_North"), FVector(3000.0f, 0.0f, 160.0f), FVector(1.0f, 60.0f, 3.2f));
	AddWall(TEXT("Wall_South"), FVector(-3000.0f, 0.0f, 160.0f), FVector(1.0f, 60.0f, 3.2f));
	AddWall(TEXT("Wall_East"), FVector(0.0f, 3000.0f, 160.0f), FVector(60.0f, 1.0f, 3.2f));
	AddWall(TEXT("Wall_West"), FVector(0.0f, -3000.0f, 160.0f), FVector(60.0f, 1.0f, 3.2f));
}

void AFPSCombatArenaMapActor::ConstructCentralDerrick(UStaticMesh* CubeMesh, UStaticMesh* CylinderMesh, UMaterialInterface* BaseMat)
{
	auto AddPart = [this, BaseMat](const FString& Name, UStaticMesh* Mesh, const FVector& Loc, const FRotator& Rot, const FVector& Scale)
	{
		UStaticMeshComponent* Comp = CreateDefaultSubobject<UStaticMeshComponent>(*Name);
		Comp->SetupAttachment(RootComponent);
		Comp->SetMobility(EComponentMobility::Movable);
		if (Mesh) Comp->SetStaticMesh(Mesh);
		if (BaseMat) Comp->SetMaterial(0, BaseMat);
		Comp->SetRelativeLocation(Loc);
		Comp->SetRelativeRotation(Rot);
		Comp->SetRelativeScale3D(Scale);
		Comp->SetCollisionProfileName(TEXT("BlockAll"));
		Comp->SetCastShadow(true);
		DerrickComponents.Add(Comp);
	};

	// Central Favela Laje Watchtower Foundation Plinth (11m x 11m x 0.45m)
	AddPart(TEXT("Derrick_Base"), CubeMesh, FVector(0.0f, 0.0f, 22.0f), FRotator::ZeroRotator, FVector(11.0f, 11.0f, 0.45f));

	// Central Brick Masonry Core House (2.6m x 2.6m x 7.2m)
	AddPart(TEXT("Derrick_Core"), CubeMesh, FVector(0.0f, 0.0f, 365.0f), FRotator::ZeroRotator, FVector(2.6f, 2.6f, 7.2f));

	// 4 Reinforced Concrete Structural Corner Pillars (vertical 0 deg)
	AddPart(TEXT("Derrick_Pylon_NW"), CubeMesh, FVector(380.0f, -380.0f, 365.0f), FRotator::ZeroRotator, FVector(0.55f, 0.55f, 7.3f));
	AddPart(TEXT("Derrick_Pylon_NE"), CubeMesh, FVector(380.0f, 380.0f, 365.0f), FRotator::ZeroRotator, FVector(0.55f, 0.55f, 7.3f));
	AddPart(TEXT("Derrick_Pylon_SW"), CubeMesh, FVector(-380.0f, -380.0f, 365.0f), FRotator::ZeroRotator, FVector(0.55f, 0.55f, 7.3f));
	AddPart(TEXT("Derrick_Pylon_SE"), CubeMesh, FVector(-380.0f, 380.0f, 365.0f), FRotator::ZeroRotator, FVector(0.55f, 0.55f, 7.3f));

	// Second-Story Concrete Laje Terrace (Z = 350)
	AddPart(TEXT("Derrick_Catwalk"), CubeMesh, FVector(0.0f, 0.0f, 350.0f), FRotator::ZeroRotator, FVector(8.6f, 8.6f, 0.22f));

	// Brick Laje Parapet Walls
	AddPart(TEXT("Derrick_Rail_N"), CubeMesh, FVector(420.0f, 0.0f, 390.0f), FRotator::ZeroRotator, FVector(0.24f, 8.6f, 0.8f));
	AddPart(TEXT("Derrick_Rail_S"), CubeMesh, FVector(-420.0f, 0.0f, 390.0f), FRotator::ZeroRotator, FVector(0.24f, 8.6f, 0.8f));

	// Concrete Access Ramps (West & East)
	AddPart(TEXT("Derrick_Ramp_West"), CubeMesh, FVector(0.0f, -800.0f, 175.0f), FRotator(25.5f, 90.0f, 0.0f), FVector(8.2f, 2.2f, 0.25f));
	AddPart(TEXT("Derrick_Ramp_East"), CubeMesh, FVector(0.0f, 800.0f, 175.0f), FRotator(-25.5f, 90.0f, 0.0f), FVector(8.2f, 2.2f, 0.25f));

	// Top Laje Sniper Roof Slab (Z = 720)
	AddPart(TEXT("Derrick_Top_Platform"), CubeMesh, FVector(0.0f, 0.0f, 720.0f), FRotator::ZeroRotator, FVector(4.2f, 4.2f, 0.22f));
	AddPart(TEXT("Derrick_Top_Rail_N"), CubeMesh, FVector(200.0f, 0.0f, 755.0f), FRotator::ZeroRotator, FVector(0.22f, 4.2f, 0.7f));
	AddPart(TEXT("Derrick_Top_Rail_S"), CubeMesh, FVector(-200.0f, 0.0f, 755.0f), FRotator::ZeroRotator, FVector(0.22f, 4.2f, 0.7f));
}

void AFPSCombatArenaMapActor::ConstructContainerYard(UStaticMesh* CubeMesh, UMaterialInterface* BaseMat)
{
	auto AddContainer = [this, CubeMesh, BaseMat](const FString& Name, const FVector& Loc, const FRotator& Rot, const FVector& Scale)
	{
		UStaticMeshComponent* Comp = CreateDefaultSubobject<UStaticMeshComponent>(*Name);
		Comp->SetupAttachment(RootComponent);
		Comp->SetMobility(EComponentMobility::Movable);
		if (CubeMesh) Comp->SetStaticMesh(CubeMesh);
		if (BaseMat) Comp->SetMaterial(0, BaseMat);
		Comp->SetRelativeLocation(Loc);
		Comp->SetRelativeRotation(Rot);
		Comp->SetRelativeScale3D(Scale);
		Comp->SetCollisionProfileName(TEXT("BlockAll"));
		Comp->SetCastShadow(true);
		ContainerComponents.Add(Comp);
	};

	// West Beco Favela Shanty Blocks & Shoot-Through Alley Passage
	AddContainer(TEXT("Container_Blue"), FVector(-1400.0f, -1400.0f, 135.0f), FRotator(0.0f, 12.0f, 0.0f), FVector(5.4f, 2.8f, 2.7f));
	AddContainer(TEXT("Container_Red"), FVector(0.0f, -1800.0f, 135.0f), FRotator(0.0f, 0.0f, 0.0f), FVector(5.6f, 2.8f, 2.7f));
	AddContainer(TEXT("Container_Olive"), FVector(1400.0f, -1400.0f, 135.0f), FRotator(0.0f, -14.0f, 0.0f), FVector(5.4f, 2.8f, 2.7f));

	// Shoot-Through Beco Covered Alleyway (Left Wall, Right Wall, Concrete Laje Roof)
	AddContainer(TEXT("Container_Tunnel_WallL"), FVector(-200.0f, -1080.0f, 130.0f), FRotator::ZeroRotator, FVector(6.0f, 0.24f, 2.6f));
	AddContainer(TEXT("Container_Tunnel_WallR"), FVector(-200.0f, -1320.0f, 130.0f), FRotator::ZeroRotator, FVector(6.0f, 0.24f, 2.6f));
	AddContainer(TEXT("Container_Tunnel_Roof"), FVector(-200.0f, -1200.0f, 262.0f), FRotator::ZeroRotator, FVector(6.2f, 2.6f, 0.22f));
}

void AFPSCombatArenaMapActor::ConstructPipelineAndBunker(UStaticMesh* CubeMesh, UStaticMesh* CylinderMesh, UMaterialInterface* BaseMat)
{
	auto AddPipe = [this, BaseMat](const FString& Name, UStaticMesh* Mesh, const FVector& Loc, const FRotator& Rot, const FVector& Scale)
	{
		UStaticMeshComponent* Comp = CreateDefaultSubobject<UStaticMeshComponent>(*Name);
		Comp->SetupAttachment(RootComponent);
		Comp->SetMobility(EComponentMobility::Movable);
		if (Mesh) Comp->SetStaticMesh(Mesh);
		if (BaseMat) Comp->SetMaterial(0, BaseMat);
		Comp->SetRelativeLocation(Loc);
		Comp->SetRelativeRotation(Rot);
		Comp->SetRelativeScale3D(Scale);
		Comp->SetCollisionProfileName(TEXT("BlockAll"));
		Comp->SetCastShadow(true);
		PipelineComponents.Add(Comp);
	};

	// East Beco Stepped Brick & Cinder-Block Retaining Terrace Walls (replaces oil pipeline cylinders)
	AddPipe(TEXT("Pipe_Seg_1"), CubeMesh, FVector(-1200.0f, 1800.0f, 65.0f), FRotator::ZeroRotator, FVector(10.5f, 0.55f, 1.3f));
	AddPipe(TEXT("Pipe_Seg_2"), CubeMesh, FVector(0.0f, 1800.0f, 65.0f), FRotator::ZeroRotator, FVector(10.5f, 0.55f, 1.3f));
	AddPipe(TEXT("Pipe_Seg_3"), CubeMesh, FVector(1200.0f, 1800.0f, 65.0f), FRotator::ZeroRotator, FVector(10.5f, 0.55f, 1.3f));

	// Masonry Buttress Pillars
	AddPipe(TEXT("Pipe_Cradle_1"), CubeMesh, FVector(-600.0f, 1800.0f, 80.0f), FRotator::ZeroRotator, FVector(0.9f, 0.9f, 1.6f));
	AddPipe(TEXT("Pipe_Cradle_2"), CubeMesh, FVector(600.0f, 1800.0f, 80.0f), FRotator::ZeroRotator, FVector(0.9f, 0.9f, 1.6f));

	// East Beco Ground-Floor Brick Guardhouse (Casa da Esquina)
	AddPipe(TEXT("Bunker_Wall_Back"), CubeMesh, FVector(200.0f, 1550.0f, 105.0f), FRotator::ZeroRotator, FVector(4.0f, 0.35f, 2.1f));
	AddPipe(TEXT("Bunker_Wall_L"), CubeMesh, FVector(400.0f, 1400.0f, 105.0f), FRotator::ZeroRotator, FVector(0.35f, 3.0f, 2.1f));
	AddPipe(TEXT("Bunker_Wall_R"), CubeMesh, FVector(0.0f, 1400.0f, 105.0f), FRotator::ZeroRotator, FVector(0.35f, 3.0f, 2.1f));
	AddPipe(TEXT("Bunker_Roof"), CubeMesh, FVector(200.0f, 1400.0f, 215.0f), FRotator::ZeroRotator, FVector(4.2f, 3.2f, 0.25f));
}

void AFPSCombatArenaMapActor::ConstructCenterCover(UStaticMesh* CubeMesh, UMaterialInterface* BaseMat)
{
	auto AddBarrier = [this, CubeMesh, BaseMat](const FString& Name, const FVector& Loc, const FRotator& Rot, const FVector& Scale)
	{
		UStaticMeshComponent* Comp = CreateDefaultSubobject<UStaticMeshComponent>(*Name);
		Comp->SetupAttachment(RootComponent);
		Comp->SetMobility(EComponentMobility::Movable);
		if (CubeMesh) Comp->SetStaticMesh(CubeMesh);
		if (BaseMat) Comp->SetMaterial(0, BaseMat);
		Comp->SetRelativeLocation(Loc);
		Comp->SetRelativeRotation(Rot);
		Comp->SetRelativeScale3D(Scale);
		Comp->SetCollisionProfileName(TEXT("BlockAll"));
		Comp->SetCastShadow(true);
		BarrierComponents.Add(Comp);
	};

	// Center Plaza Brick Market Stall & Timber Counter
	AddBarrier(TEXT("Vehicle_Hull"), FVector(180.0f, -220.0f, 52.0f), FRotator(0.0f, 15.0f, 0.0f), FVector(3.2f, 1.8f, 1.05f));
	AddBarrier(TEXT("Vehicle_Turret"), FVector(180.0f, -220.0f, 125.0f), FRotator(0.0f, 15.0f, 0.0f), FVector(3.0f, 0.35f, 0.55f));

	// Plaza Masonry & Cinder-Block Cover Walls
	AddBarrier(TEXT("Barrier_1"), FVector(-800.0f, -420.0f, 52.0f), FRotator(0.0f, 35.0f, 0.0f), FVector(2.1f, 0.55f, 1.05f));
	AddBarrier(TEXT("Barrier_2"), FVector(-800.0f, 420.0f, 52.0f), FRotator(0.0f, -35.0f, 0.0f), FVector(2.1f, 0.55f, 1.05f));
	AddBarrier(TEXT("Barrier_3"), FVector(800.0f, -420.0f, 52.0f), FRotator(0.0f, -35.0f, 0.0f), FVector(2.1f, 0.55f, 1.05f));
	AddBarrier(TEXT("Barrier_4"), FVector(800.0f, 420.0f, 52.0f), FRotator(0.0f, 35.0f, 0.0f), FVector(2.1f, 0.55f, 1.05f));

	AddBarrier(TEXT("Barrier_Alpha_Exit"), FVector(-1800.0f, 0.0f, 48.0f), FRotator::ZeroRotator, FVector(2.8f, 0.55f, 0.95f));
	AddBarrier(TEXT("Barrier_Bravo_Exit"), FVector(1800.0f, 0.0f, 48.0f), FRotator::ZeroRotator, FVector(2.8f, 0.55f, 0.95f));
}

void AFPSCombatArenaMapActor::ConstructObjectives(UStaticMesh* CylinderMesh, UMaterialInterface* BaseMat)
{
	auto AddFlag = [this, CylinderMesh, BaseMat](const FString& Name, const FVector& Loc)
	{
		UStaticMeshComponent* FlagPole = CreateDefaultSubobject<UStaticMeshComponent>(*Name);
		FlagPole->SetupAttachment(RootComponent);
		FlagPole->SetMobility(EComponentMobility::Movable);
		if (CylinderMesh) FlagPole->SetStaticMesh(CylinderMesh);
		if (BaseMat) FlagPole->SetMaterial(0, BaseMat);
		FlagPole->SetRelativeLocation(Loc);
		FlagPole->SetRelativeScale3D(FVector(0.15f, 0.15f, 3.5f));
		FlagPole->SetCollisionProfileName(TEXT("NoCollision"));
		ObjectiveFlags.Add(FlagPole);
	};

	// 3 Domination Flags
	AddFlag(TEXT("Flag_Alpha_A"), FVector(-1400.0f, -1000.0f, 175.0f));
	AddFlag(TEXT("Flag_Bravo_B"), FVector(0.0f, 0.0f, 200.0f));
	AddFlag(TEXT("Flag_Charlie_C"), FVector(1400.0f, 1000.0f, 175.0f));
}

void AFPSCombatArenaMapActor::BeginPlay()
{
	Super::BeginPlay();

	if (SkyLight)
	{
		SkyLight->RecaptureSky();
	}

	// 1. Ground: Portuguese Cobblestone Plaza (Pedra Portuguesa / Paralelepipedo)
	UFPSCoastalMaterialLibrary::ApplyCoastalSurfaceToMesh(GroundFloorMesh, ECoastalSurfaceType::FavelaCobble);

	// 2. Perimeter Retaining Walls: Unpainted Cinder Blocks & Terracotta Brick
	for (int32 i = 0; i < PerimeterWalls.Num(); ++i)
	{
		UFPSCoastalMaterialLibrary::ApplyCoastalSurfaceToMesh(
			PerimeterWalls[i],
			(i % 2 == 0) ? ECoastalSurfaceType::FavelaCinder : ECoastalSurfaceType::FavelaBrick);
	}

	// 3. Central 3-Story Favela Laje Watchtower: Concrete Pillars, Slabs & Tijolo Baiano Brick Core
	for (int32 i = 0; i < DerrickComponents.Num(); ++i)
	{
		UStaticMeshComponent* Part = DerrickComponents[i];
		if (!Part) continue;
		const FString PartName = Part->GetName();
		if (PartName.Contains(TEXT("Core")) || PartName.Contains(TEXT("Rail")))
		{
			UFPSCoastalMaterialLibrary::ApplyCoastalSurfaceToMesh(Part, ECoastalSurfaceType::FavelaBrick);
		}
		else if (PartName.Contains(TEXT("Base")))
		{
			UFPSCoastalMaterialLibrary::ApplyCoastalSurfaceToMesh(Part, ECoastalSurfaceType::FavelaCobble);
		}
		else
		{
			UFPSCoastalMaterialLibrary::ApplyCoastalSurfaceToMesh(Part, ECoastalSurfaceType::MarineCastConcrete);
		}
	}

	// 4. West Beco Shanty Blocks & Covered Alleyway
	for (int32 i = 0; i < ContainerComponents.Num(); ++i)
	{
		ECoastalSurfaceType Surf = ECoastalSurfaceType::FavelaBrick;
		if (i == 0 || i == 2) Surf = ECoastalSurfaceType::FavelaStucco;
		else if (i == 3 || i == 4) Surf = ECoastalSurfaceType::FavelaCinder;
		else if (i == 5) Surf = ECoastalSurfaceType::MarineCastConcrete;
		UFPSCoastalMaterialLibrary::ApplyCoastalSurfaceToMesh(ContainerComponents[i], Surf);
	}

	// 5. East Beco Retaining Walls & Brick Guardhouse
	for (int32 i = 0; i < PipelineComponents.Num(); ++i)
	{
		const ECoastalSurfaceType Surf = (i < 3)
			? ECoastalSurfaceType::FavelaCinder
			: ((i == 8) ? ECoastalSurfaceType::MarineCastConcrete : ECoastalSurfaceType::FavelaBrick);
		UFPSCoastalMaterialLibrary::ApplyCoastalSurfaceToMesh(PipelineComponents[i], Surf);
	}

	// 6. Plaza Market Stall & Masonry Cover Barriers
	for (int32 i = 0; i < BarrierComponents.Num(); ++i)
	{
		const ECoastalSurfaceType Surf = (i == 1)
			? ECoastalSurfaceType::WeatheredPierTimber
			: ((i % 2 == 0) ? ECoastalSurfaceType::FavelaBrick : ECoastalSurfaceType::FavelaCinder);
		UFPSCoastalMaterialLibrary::ApplyCoastalSurfaceToMesh(BarrierComponents[i], Surf);
	}

	// 7. Objectives
	for (UStaticMeshComponent* Flag : ObjectiveFlags)
	{
		UFPSCoastalMaterialLibrary::ApplyCoastalSurfaceToMesh(Flag, ECoastalSurfaceType::FavelaMetal);
	}

	// 8. Populate Instanced Favela Architecture, Corrugated Roofs, Drainage Gutters, Foliage & Swaying Wires
	UFPSCoastalMaterialLibrary::PopulateRochaVelhaFavela(this, RootComponent);

	// 9. Spawn Player Starts
	SpawnTacticalPlayerStarts();
}

void AFPSCombatArenaMapActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (TowerBeaconLight)
	{
		BeaconTimer += DeltaTime * 3.5f;
		float Intensity = 2600.0f + 1200.0f * FMath::Sin(BeaconTimer);
		TowerBeaconLight->SetIntensity(Intensity);
	}
}

void AFPSCombatArenaMapActor::SpawnTacticalPlayerStarts()
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

	// 4 Team Alpha Spawns (South Cobblestone Plaza, Z=115cm)
	CreateStart(FVector(-2400.0f, -600.0f, 115.0f), FRotator(0.0f, 0.0f, 0.0f), FName("TeamAlpha"));
	CreateStart(FVector(-2400.0f, -200.0f, 115.0f), FRotator(0.0f, 0.0f, 0.0f), FName("TeamAlpha"));
	CreateStart(FVector(-2400.0f, 200.0f, 115.0f), FRotator(0.0f, 0.0f, 0.0f), FName("TeamAlpha"));
	CreateStart(FVector(-2400.0f, 600.0f, 115.0f), FRotator(0.0f, 0.0f, 0.0f), FName("TeamAlpha"));

	// 4 Team Bravo Spawns (North Hillside Terrace, Z=115cm)
	CreateStart(FVector(2400.0f, -600.0f, 115.0f), FRotator(0.0f, 180.0f, 0.0f), FName("TeamBravo"));
	CreateStart(FVector(2400.0f, -200.0f, 115.0f), FRotator(0.0f, 180.0f, 0.0f), FName("TeamBravo"));
	CreateStart(FVector(2400.0f, 200.0f, 115.0f), FRotator(0.0f, 180.0f, 0.0f), FName("TeamBravo"));
	CreateStart(FVector(2400.0f, 600.0f, 115.0f), FRotator(0.0f, 180.0f, 0.0f), FName("TeamBravo"));

	UE_LOG(LogTemp, Warning, TEXT("[FavelaArena] Favela // Rocha Velha: Successfully spawned 8 PlayerStarts (4 TeamAlpha, 4 TeamBravo)."));
}
