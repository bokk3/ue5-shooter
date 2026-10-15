#include "Map/FPSCombatArenaMapActor.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
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

	// 1. Cinematic Golden-Hour Atmospheric Lighting
	SunLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("SunLight"));
	SunLight->SetupAttachment(RootComponent);
	SunLight->SetRelativeRotation(FRotator(-38.0f, 45.0f, 0.0f));
	SunLight->SetIntensity(75000.0f);
	SunLight->SetLightColor(FLinearColor(1.0f, 0.94f, 0.84f)); // Warm tactical desert sun
	SunLight->bAtmosphereSunLight = true;
	SunLight->bCastVolumetricShadow = true;
	SunLight->CastShadows = 1;

	SkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
	SkyLight->SetupAttachment(RootComponent);
	SkyLight->SetIntensity(1.5f);
	SkyLight->SetLightColor(FLinearColor(0.85f, 0.92f, 1.0f));
	SkyLight->bRealTimeCapture = true;

	SkyAtmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("SkyAtmosphere"));
	SkyAtmosphere->SetupAttachment(RootComponent);

	HeightFog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("HeightFog"));
	HeightFog->SetupAttachment(RootComponent);
	HeightFog->SetFogDensity(0.002f);
	HeightFog->SetFogInscatteringColor(FLinearColor(0.85f, 0.75f, 0.55f)); // Warm dust haze

	TowerBeaconLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("TowerBeaconLight"));
	TowerBeaconLight->SetupAttachment(RootComponent);
	TowerBeaconLight->SetRelativeLocation(FVector(0.0f, 0.0f, 780.0f));
	TowerBeaconLight->SetIntensity(6000.0f);
	TowerBeaconLight->SetLightColor(FLinearColor(1.0f, 0.1f, 0.05f)); // Red hazard beacon
	TowerBeaconLight->SetAttenuationRadius(1500.0f);

	// 2. Load Base Meshes & Materials
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BaseMatFinder(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	UStaticMesh* CubeMesh = CubeFinder.Succeeded() ? CubeFinder.Object : nullptr;
	UStaticMesh* CylinderMesh = CylinderFinder.Succeeded() ? CylinderFinder.Object : nullptr;
	UMaterialInterface* BaseMat = BaseMatFinder.Succeeded() ? BaseMatFinder.Object : nullptr;

	// 3. Ground Plane (60m x 60m = 6000 x 6000 units, 100 units thick)
	GroundFloorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GroundFloorMesh"));
	GroundFloorMesh->SetupAttachment(RootComponent);
	if (CubeMesh) GroundFloorMesh->SetStaticMesh(CubeMesh);
	if (BaseMat) GroundFloorMesh->SetMaterial(0, BaseMat);
	GroundFloorMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -50.0f));
	GroundFloorMesh->SetRelativeScale3D(FVector(60.0f, 60.0f, 1.0f));
	GroundFloorMesh->SetCollisionProfileName(TEXT("BlockAll"));
	GroundFloorMesh->SetCastShadow(true);

	// 4. Construct Sub-Zones
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
		if (CubeMesh) Wall->SetStaticMesh(CubeMesh);
		if (BaseMat) Wall->SetMaterial(0, BaseMat);
		Wall->SetRelativeLocation(Loc);
		Wall->SetRelativeScale3D(Scale);
		Wall->SetCollisionProfileName(TEXT("BlockAll"));
		Wall->SetCastShadow(true);
		PerimeterWalls.Add(Wall);
	};

	// 4 Blast Walls (60m perimeter, 6m high, 1m thick)
	AddWall(TEXT("Wall_North"), FVector(3000.0f, 0.0f, 250.0f), FVector(1.0f, 60.0f, 6.0f));
	AddWall(TEXT("Wall_South"), FVector(-3000.0f, 0.0f, 250.0f), FVector(1.0f, 60.0f, 6.0f));
	AddWall(TEXT("Wall_East"), FVector(0.0f, 3000.0f, 250.0f), FVector(60.0f, 1.0f, 6.0f));
	AddWall(TEXT("Wall_West"), FVector(0.0f, -3000.0f, 250.0f), FVector(60.0f, 1.0f, 6.0f));
}

void AFPSCombatArenaMapActor::ConstructCentralDerrick(UStaticMesh* CubeMesh, UStaticMesh* CylinderMesh, UMaterialInterface* BaseMat)
{
	auto AddPart = [this, BaseMat](const FString& Name, UStaticMesh* Mesh, const FVector& Loc, const FRotator& Rot, const FVector& Scale)
	{
		UStaticMeshComponent* Comp = CreateDefaultSubobject<UStaticMeshComponent>(*Name);
		Comp->SetupAttachment(RootComponent);
		if (Mesh) Comp->SetStaticMesh(Mesh);
		if (BaseMat) Comp->SetMaterial(0, BaseMat);
		Comp->SetRelativeLocation(Loc);
		Comp->SetRelativeRotation(Rot);
		Comp->SetRelativeScale3D(Scale);
		Comp->SetCollisionProfileName(TEXT("BlockAll"));
		Comp->SetCastShadow(true);
		DerrickComponents.Add(Comp);
	};

	// Derrick Base Platform (12m x 12m x 0.5m)
	AddPart(TEXT("Derrick_Base"), CubeMesh, FVector(0.0f, 0.0f, 25.0f), FRotator::ZeroRotator, FVector(12.0f, 12.0f, 0.5f));

	// Center Drill Shaft Cylinder
	AddPart(TEXT("Derrick_Core"), CylinderMesh, FVector(0.0f, 0.0f, 380.0f), FRotator::ZeroRotator, FVector(1.5f, 1.5f, 7.5f));

	// 4 Corner Steel Pylons
	AddPart(TEXT("Derrick_Pylon_NW"), CubeMesh, FVector(400.0f, -400.0f, 360.0f), FRotator(0.0f, 0.0f, 8.0f), FVector(0.8f, 0.8f, 7.2f));
	AddPart(TEXT("Derrick_Pylon_NE"), CubeMesh, FVector(400.0f, 400.0f, 360.0f), FRotator(0.0f, 0.0f, -8.0f), FVector(0.8f, 0.8f, 7.2f));
	AddPart(TEXT("Derrick_Pylon_SW"), CubeMesh, FVector(-400.0f, -400.0f, 360.0f), FRotator(0.0f, 0.0f, 8.0f), FVector(0.8f, 0.8f, 7.2f));
	AddPart(TEXT("Derrick_Pylon_SE"), CubeMesh, FVector(-400.0f, 400.0f, 360.0f), FRotator(0.0f, 0.0f, -8.0f), FVector(0.8f, 0.8f, 7.2f));

	// Mid-Level Catwalk (Z = 350)
	AddPart(TEXT("Derrick_Catwalk"), CubeMesh, FVector(0.0f, 0.0f, 350.0f), FRotator::ZeroRotator, FVector(9.0f, 9.0f, 0.2f));

	// Catwalk Railings
	AddPart(TEXT("Derrick_Rail_N"), CubeMesh, FVector(450.0f, 0.0f, 385.0f), FRotator::ZeroRotator, FVector(0.2f, 9.0f, 0.7f));
	AddPart(TEXT("Derrick_Rail_S"), CubeMesh, FVector(-450.0f, 0.0f, 385.0f), FRotator::ZeroRotator, FVector(0.2f, 9.0f, 0.7f));

	// Access Ramps (West & East)
	AddPart(TEXT("Derrick_Ramp_West"), CubeMesh, FVector(0.0f, -750.0f, 175.0f), FRotator(21.0f, 90.0f, 0.0f), FVector(2.0f, 8.0f, 0.2f));
	AddPart(TEXT("Derrick_Ramp_East"), CubeMesh, FVector(0.0f, 750.0f, 175.0f), FRotator(-21.0f, 90.0f, 0.0f), FVector(2.0f, 8.0f, 0.2f));

	// Top Sniper Platform (Z = 720)
	AddPart(TEXT("Derrick_Top_Platform"), CubeMesh, FVector(0.0f, 0.0f, 720.0f), FRotator::ZeroRotator, FVector(4.0f, 4.0f, 0.2f));
	AddPart(TEXT("Derrick_Top_Rail_N"), CubeMesh, FVector(200.0f, 0.0f, 755.0f), FRotator::ZeroRotator, FVector(0.2f, 4.0f, 0.7f));
	AddPart(TEXT("Derrick_Top_Rail_S"), CubeMesh, FVector(-200.0f, 0.0f, 755.0f), FRotator::ZeroRotator, FVector(0.2f, 4.0f, 0.7f));
}

void AFPSCombatArenaMapActor::ConstructContainerYard(UStaticMesh* CubeMesh, UMaterialInterface* BaseMat)
{
	auto AddContainer = [this, CubeMesh, BaseMat](const FString& Name, const FVector& Loc, const FRotator& Rot, const FVector& Scale)
	{
		UStaticMeshComponent* Comp = CreateDefaultSubobject<UStaticMeshComponent>(*Name);
		Comp->SetupAttachment(RootComponent);
		if (CubeMesh) Comp->SetStaticMesh(CubeMesh);
		if (BaseMat) Comp->SetMaterial(0, BaseMat);
		Comp->SetRelativeLocation(Loc);
		Comp->SetRelativeRotation(Rot);
		Comp->SetRelativeScale3D(Scale);
		Comp->SetCollisionProfileName(TEXT("BlockAll"));
		Comp->SetCastShadow(true);
		ContainerComponents.Add(Comp);
	};

	// Standard Shipping Container Dimensions: 6.0m x 2.4m x 2.6m
	AddContainer(TEXT("Container_Blue"), FVector(-1400.0f, -1400.0f, 130.0f), FRotator(0.0f, 15.0f, 0.0f), FVector(6.0f, 2.4f, 2.6f));
	AddContainer(TEXT("Container_Red"), FVector(0.0f, -1800.0f, 130.0f), FRotator(0.0f, 0.0f, 0.0f), FVector(6.0f, 2.4f, 2.6f));
	AddContainer(TEXT("Container_Olive"), FVector(1400.0f, -1400.0f, 130.0f), FRotator(0.0f, -20.0f, 0.0f), FVector(6.0f, 2.4f, 2.6f));

	// Shoot-Through Tunnel Container (Left Wall, Right Wall, Roof)
	AddContainer(TEXT("Container_Tunnel_WallL"), FVector(-200.0f, -1080.0f, 130.0f), FRotator::ZeroRotator, FVector(6.0f, 0.2f, 2.6f));
	AddContainer(TEXT("Container_Tunnel_WallR"), FVector(-200.0f, -1320.0f, 130.0f), FRotator::ZeroRotator, FVector(6.0f, 0.2f, 2.6f));
	AddContainer(TEXT("Container_Tunnel_Roof"), FVector(-200.0f, -1200.0f, 260.0f), FRotator::ZeroRotator, FVector(6.0f, 2.4f, 0.2f));
}

void AFPSCombatArenaMapActor::ConstructPipelineAndBunker(UStaticMesh* CubeMesh, UStaticMesh* CylinderMesh, UMaterialInterface* BaseMat)
{
	auto AddPipe = [this, BaseMat](const FString& Name, UStaticMesh* Mesh, const FVector& Loc, const FRotator& Rot, const FVector& Scale)
	{
		UStaticMeshComponent* Comp = CreateDefaultSubobject<UStaticMeshComponent>(*Name);
		Comp->SetupAttachment(RootComponent);
		if (Mesh) Comp->SetStaticMesh(Mesh);
		if (BaseMat) Comp->SetMaterial(0, BaseMat);
		Comp->SetRelativeLocation(Loc);
		Comp->SetRelativeRotation(Rot);
		Comp->SetRelativeScale3D(Scale);
		Comp->SetCollisionProfileName(TEXT("BlockAll"));
		Comp->SetCastShadow(true);
		PipelineComponents.Add(Comp);
	};

	// Heavy Oil Pipeline along East Flank (Y = 1800)
	AddPipe(TEXT("Pipe_Seg_1"), CylinderMesh, FVector(-1200.0f, 1800.0f, 90.0f), FRotator(0.0f, 0.0f, 90.0f), FVector(1.2f, 1.2f, 12.0f));
	AddPipe(TEXT("Pipe_Seg_2"), CylinderMesh, FVector(0.0f, 1800.0f, 90.0f), FRotator(0.0f, 0.0f, 90.0f), FVector(1.2f, 1.2f, 12.0f));
	AddPipe(TEXT("Pipe_Seg_3"), CylinderMesh, FVector(1200.0f, 1800.0f, 90.0f), FRotator(0.0f, 0.0f, 90.0f), FVector(1.2f, 1.2f, 12.0f));

	// Concrete Cradles
	AddPipe(TEXT("Pipe_Cradle_1"), CubeMesh, FVector(-600.0f, 1800.0f, 40.0f), FRotator::ZeroRotator, FVector(1.0f, 1.8f, 0.8f));
	AddPipe(TEXT("Pipe_Cradle_2"), CubeMesh, FVector(600.0f, 1800.0f, 40.0f), FRotator::ZeroRotator, FVector(1.0f, 1.8f, 0.8f));

	// Reinforced Concrete Bunker
	AddPipe(TEXT("Bunker_Wall_Back"), CubeMesh, FVector(200.0f, 1550.0f, 100.0f), FRotator::ZeroRotator, FVector(4.0f, 0.4f, 2.0f));
	AddPipe(TEXT("Bunker_Wall_L"), CubeMesh, FVector(400.0f, 1400.0f, 100.0f), FRotator::ZeroRotator, FVector(0.4f, 3.0f, 2.0f));
	AddPipe(TEXT("Bunker_Wall_R"), CubeMesh, FVector(0.0f, 1400.0f, 100.0f), FRotator::ZeroRotator, FVector(0.4f, 3.0f, 2.0f));
	AddPipe(TEXT("Bunker_Roof"), CubeMesh, FVector(200.0f, 1400.0f, 210.0f), FRotator::ZeroRotator, FVector(4.2f, 3.2f, 0.3f));
}

void AFPSCombatArenaMapActor::ConstructCenterCover(UStaticMesh* CubeMesh, UMaterialInterface* BaseMat)
{
	auto AddBarrier = [this, CubeMesh, BaseMat](const FString& Name, const FVector& Loc, const FRotator& Rot, const FVector& Scale)
	{
		UStaticMeshComponent* Comp = CreateDefaultSubobject<UStaticMeshComponent>(*Name);
		Comp->SetupAttachment(RootComponent);
		if (CubeMesh) Comp->SetStaticMesh(CubeMesh);
		if (BaseMat) Comp->SetMaterial(0, BaseMat);
		Comp->SetRelativeLocation(Loc);
		Comp->SetRelativeRotation(Rot);
		Comp->SetRelativeScale3D(Scale);
		Comp->SetCollisionProfileName(TEXT("BlockAll"));
		Comp->SetCastShadow(true);
		BarrierComponents.Add(Comp);
	};

	// Center Armored Vehicle Wreck (Hull & Turret)
	AddBarrier(TEXT("Vehicle_Hull"), FVector(200.0f, -200.0f, 80.0f), FRotator(0.0f, 25.0f, 0.0f), FVector(5.0f, 2.5f, 1.6f));
	AddBarrier(TEXT("Vehicle_Turret"), FVector(180.0f, -180.0f, 180.0f), FRotator(0.0f, 15.0f, 0.0f), FVector(2.0f, 2.0f, 1.0f));

	// Concrete Jersey Barriers (2.0m x 0.6m x 1.0m)
	AddBarrier(TEXT("Barrier_1"), FVector(-800.0f, -400.0f, 50.0f), FRotator(0.0f, 45.0f, 0.0f), FVector(2.0f, 0.6f, 1.0f));
	AddBarrier(TEXT("Barrier_2"), FVector(-800.0f, 400.0f, 50.0f), FRotator(0.0f, -45.0f, 0.0f), FVector(2.0f, 0.6f, 1.0f));
	AddBarrier(TEXT("Barrier_3"), FVector(800.0f, -400.0f, 50.0f), FRotator(0.0f, -45.0f, 0.0f), FVector(2.0f, 0.6f, 1.0f));
	AddBarrier(TEXT("Barrier_4"), FVector(800.0f, 400.0f, 50.0f), FRotator(0.0f, 45.0f, 0.0f), FVector(2.0f, 0.6f, 1.0f));

	AddBarrier(TEXT("Barrier_Alpha_Exit"), FVector(-1800.0f, 0.0f, 50.0f), FRotator::ZeroRotator, FVector(4.0f, 0.6f, 1.0f));
	AddBarrier(TEXT("Barrier_Bravo_Exit"), FVector(1800.0f, 0.0f, 50.0f), FRotator::ZeroRotator, FVector(4.0f, 0.6f, 1.0f));
}

void AFPSCombatArenaMapActor::ConstructObjectives(UStaticMesh* CylinderMesh, UMaterialInterface* BaseMat)
{
	auto AddFlag = [this, CylinderMesh, BaseMat](const FString& Name, const FVector& Loc)
	{
		UStaticMeshComponent* FlagPole = CreateDefaultSubobject<UStaticMeshComponent>(*Name);
		FlagPole->SetupAttachment(RootComponent);
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

	// Color Dynamic Materials with Tactical Military Palette
	auto SetColor = [](UStaticMeshComponent* Comp, const FLinearColor& Color)
	{
		if (Comp)
		{
			UMaterialInstanceDynamic* MID = Comp->CreateDynamicMaterialInstance(0);
			if (MID)
			{
				MID->SetVectorParameterValue(TEXT("Color"), Color);
				MID->SetVectorParameterValue(TEXT("BaseColor"), Color);
			}
		}
	};

	// Ground: Desert Industrial Asphalt
	SetColor(GroundFloorMesh, FLinearColor(0.38f, 0.35f, 0.28f));

	// Blast Walls: Reinforced Concrete
	for (UStaticMeshComponent* Wall : PerimeterWalls)
	{
		SetColor(Wall, FLinearColor(0.48f, 0.49f, 0.50f));
	}

	// Derrick: Industrial Rust Steel
	for (UStaticMeshComponent* DerrickPart : DerrickComponents)
	{
		SetColor(DerrickPart, FLinearColor(0.25f, 0.22f, 0.18f));
	}

	// Containers: Distinct Tactical Colors
	if (ContainerComponents.IsValidIndex(0)) SetColor(ContainerComponents[0], FLinearColor(0.12f, 0.28f, 0.55f)); // Blue
	if (ContainerComponents.IsValidIndex(1)) SetColor(ContainerComponents[1], FLinearColor(0.55f, 0.14f, 0.10f)); // Red
	if (ContainerComponents.IsValidIndex(2)) SetColor(ContainerComponents[2], FLinearColor(0.22f, 0.30f, 0.18f)); // Olive
	for (int32 i = 3; i < ContainerComponents.Num(); ++i)
	{
		SetColor(ContainerComponents[i], FLinearColor(0.40f, 0.38f, 0.34f)); // Tunnel
	}

	// Pipeline: Industrial Dark Metal
	for (UStaticMeshComponent* Pipe : PipelineComponents)
	{
		SetColor(Pipe, FLinearColor(0.18f, 0.19f, 0.21f));
	}

	// Barriers & Vehicle
	for (UStaticMeshComponent* Barrier : BarrierComponents)
	{
		SetColor(Barrier, FLinearColor(0.42f, 0.43f, 0.45f));
	}

	// Objectives
	for (UStaticMeshComponent* Flag : ObjectiveFlags)
	{
		SetColor(Flag, FLinearColor(0.95f, 0.65f, 0.15f));
	}

	// Spawn Player Starts
	SpawnTacticalPlayerStarts();
}

void AFPSCombatArenaMapActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Pulse Aviation Hazard Beacon Light
	if (TowerBeaconLight)
	{
		BeaconTimer += DeltaTime * 3.5f;
		float Intensity = 3000.0f + 3000.0f * FMath::Sin(BeaconTimer);
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

	// 4 Team Alpha Spawns (Task Force 141 - South Safe Revetments)
	CreateStart(FVector(-2400.0f, -600.0f, 20.0f), FRotator(0.0f, 0.0f, 0.0f), FName("TeamAlpha"));
	CreateStart(FVector(-2400.0f, -200.0f, 20.0f), FRotator(0.0f, 0.0f, 0.0f), FName("TeamAlpha"));
	CreateStart(FVector(-2400.0f, 200.0f, 20.0f), FRotator(0.0f, 0.0f, 0.0f), FName("TeamAlpha"));
	CreateStart(FVector(-2400.0f, 600.0f, 20.0f), FRotator(0.0f, 0.0f, 0.0f), FName("TeamAlpha"));

	// 4 Team Bravo Spawns (OpFor - North Loading Docks)
	CreateStart(FVector(2400.0f, -600.0f, 20.0f), FRotator(0.0f, 180.0f, 0.0f), FName("TeamBravo"));
	CreateStart(FVector(2400.0f, -200.0f, 20.0f), FRotator(0.0f, 180.0f, 0.0f), FName("TeamBravo"));
	CreateStart(FVector(2400.0f, 200.0f, 20.0f), FRotator(0.0f, 180.0f, 0.0f), FName("TeamBravo"));
	CreateStart(FVector(2400.0f, 600.0f, 20.0f), FRotator(0.0f, 180.0f, 0.0f), FName("TeamBravo"));

	UE_LOG(LogTemp, Warning, TEXT("[CombatArena] Rust Tactical: Successfully spawned 8 PlayerStarts (4 TeamAlpha, 4 TeamBravo)."));
}
