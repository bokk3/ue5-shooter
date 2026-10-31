# ==============================================================================
# NOBLE FERMI // Coastal Assets C++ Integration Script
# Updates FPSCoastalSplineActor and FPSCoastalOperationsMapActor to bind
# the newly generated modular meshes and PBR materials.
# ==============================================================================

import os
import sys

def update_spline_actor_h():
    path = "Source/NobleFermi/Map/FPSCoastalSplineActor.h"
    with open(path, "r", encoding="utf-8") as f:
        content = f.read()

    # Add ModularSeawallComponents property
    if "ModularSeawallComponents" not in content:
        target = "TArray<TObjectPtr<UStaticMeshComponent>> TetrapodComponents;\n"
        replacement = target + "\n\tUPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = \"Coastal|Seawall\")\n\tTArray<TObjectPtr<UStaticMeshComponent>> ModularSeawallComponents;\n"
        content = content.replace(target, replacement)

    # Add ConstructModularSeawall declaration
    if "ConstructModularSeawall" not in content:
        target = "void ConstructSeawardRevetment(UStaticMesh* CubeMesh, UMaterialInterface* BaseMat);\n"
        replacement = target + "\tvoid ConstructModularSeawall(UStaticMesh* SeawallMesh, UMaterialInterface* ConcreteMat, UMaterialInterface* MetalMat);\n"
        content = content.replace(target, replacement)

    # Update ConstructBasaltRiprapAndTetrapods declaration to accept TetrapodMesh
    if "UStaticMesh* TetrapodMesh" not in content:
        target = "void ConstructBasaltRiprapAndTetrapods(UStaticMesh* CubeMesh, UStaticMesh* CylinderMesh, UMaterialInterface* BaseMat);"
        replacement = "void ConstructBasaltRiprapAndTetrapods(UStaticMesh* CubeMesh, UStaticMesh* CylinderMesh, UStaticMesh* TetrapodMesh, UMaterialInterface* BaseMat, UMaterialInterface* ConcreteMat);"
        content = content.replace(target, replacement)

    with open(path, "w", encoding="utf-8") as f:
        f.write(content)
    print("Updated FPSCoastalSplineActor.h")


def update_spline_actor_cpp():
    path = "Source/NobleFermi/Map/FPSCoastalSplineActor.cpp"
    with open(path, "r", encoding="utf-8") as f:
        content = f.read()

    # 1. Update Constructor ObjectFinders
    old_finders = """\tstatic ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
\tstatic ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
\tstatic ConstructorHelpers::FObjectFinder<UMaterialInterface> BaseMatFinder(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

\tUStaticMesh* CubeMesh = CubeFinder.Succeeded() ? CubeFinder.Object : nullptr;
\tUStaticMesh* CylinderMesh = CylinderFinder.Succeeded() ? CylinderFinder.Object : nullptr;
\tUMaterialInterface* BaseMat = BaseMatFinder.Succeeded() ? BaseMatFinder.Object : nullptr;

\tConstructCrestRoadway(CubeMesh, BaseMat);
\tConstructSeawardRevetment(CubeMesh, BaseMat);
\tConstructBasaltRiprapAndTetrapods(CubeMesh, CylinderMesh, BaseMat);"""

    new_finders = """\tstatic ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
\tstatic ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
\tstatic ConstructorHelpers::FObjectFinder<UMaterialInterface> BaseMatFinder(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
\tstatic ConstructorHelpers::FObjectFinder<UStaticMesh> SeawallFinder(TEXT("/Game/Environment/Coastal/Meshes/SM_Seawall_Curved.SM_Seawall_Curved"));
\tstatic ConstructorHelpers::FObjectFinder<UStaticMesh> TetrapodFinder(TEXT("/Game/Environment/Coastal/Meshes/SM_Tetrapod_Concrete.SM_Tetrapod_Concrete"));
\tstatic ConstructorHelpers::FObjectFinder<UMaterialInterface> ConcreteMatFinder(TEXT("/Game/Environment/Coastal/Materials/M_Coastal_Concrete.M_Coastal_Concrete"));
\tstatic ConstructorHelpers::FObjectFinder<UMaterialInterface> MetalMatFinder(TEXT("/Game/Environment/Coastal/Materials/M_Coastal_Metal.M_Coastal_Metal"));

\tUStaticMesh* CubeMesh = CubeFinder.Succeeded() ? CubeFinder.Object : nullptr;
\tUStaticMesh* CylinderMesh = CylinderFinder.Succeeded() ? CylinderFinder.Object : nullptr;
\tUMaterialInterface* BaseMat = BaseMatFinder.Succeeded() ? BaseMatFinder.Object : nullptr;
\tUStaticMesh* SeawallMesh = SeawallFinder.Succeeded() ? SeawallFinder.Object : nullptr;
\tUStaticMesh* TetrapodMesh = TetrapodFinder.Succeeded() ? TetrapodFinder.Object : nullptr;
\tUMaterialInterface* ConcreteMat = ConcreteMatFinder.Succeeded() ? ConcreteMatFinder.Object : BaseMat;
\tUMaterialInterface* MetalMat = MetalMatFinder.Succeeded() ? MetalMatFinder.Object : BaseMat;

\tConstructCrestRoadway(CubeMesh, BaseMat);
\tConstructModularSeawall(SeawallMesh, ConcreteMat, MetalMat);
\tConstructSeawardRevetment(CubeMesh, BaseMat);
\tConstructBasaltRiprapAndTetrapods(CubeMesh, CylinderMesh, TetrapodMesh, BaseMat, ConcreteMat);"""

    if old_finders in content:
        content = content.replace(old_finders, new_finders)
    else:
        print("Warning: old_finders not found in FPSCoastalSplineActor.cpp")

    # 2. Add ConstructModularSeawall implementation
    if "ConstructModularSeawall" not in content:
        seawall_fn = """
void AFPSCoastalSplineActor::ConstructModularSeawall(UStaticMesh* SeawallMesh, UMaterialInterface* ConcreteMat, UMaterialInterface* MetalMat)
{
	if (!SeawallMesh) return;

	// 10 modular 12-meter curved seawall segments spanning 120m (-60m to +60m along X)
	const int32 NumSegments = 10;
	const float SegmentWidth = 1200.0f; // 12 meters per modular unit

	for (int32 i = 0; i < NumSegments; ++i)
	{
		const float CenterX = -5400.0f + i * SegmentWidth;

		FString SeawallName = FString::Printf(TEXT("Seawall_Modular_%02d"), i);
		UStaticMeshComponent* WallSeg = CreateDefaultSubobject<UStaticMeshComponent>(*SeawallName);
		WallSeg->SetupAttachment(RootComponent);
		WallSeg->SetMobility(EComponentMobility::Movable);
		WallSeg->SetStaticMesh(SeawallMesh);
		if (ConcreteMat) WallSeg->SetMaterial(0, ConcreteMat);
		if (MetalMat) WallSeg->SetMaterial(1, MetalMat);
		WallSeg->SetRelativeLocation(FVector(CenterX, -450.0f, 0.0f));
		WallSeg->SetRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));
		WallSeg->SetCollisionProfileName(TEXT("BlockAll"));
		WallSeg->SetCastShadow(true);
		WallSeg->ComponentTags.Add(FName("Seawall_Concrete"));
		ModularSeawallComponents.Add(WallSeg);
	}
}
"""
        # Insert before ConstructSeawardRevetment
        content = content.replace("void AFPSCoastalSplineActor::ConstructSeawardRevetment(", seawall_fn + "\nvoid AFPSCoastalSplineActor::ConstructSeawardRevetment(")

    # 3. Update ConstructBasaltRiprapAndTetrapods definition and tetrapod spawning
    old_tetrapod_def = "void AFPSCoastalSplineActor::ConstructBasaltRiprapAndTetrapods(UStaticMesh* CubeMesh, UStaticMesh* CylinderMesh, UMaterialInterface* BaseMat)"
    new_tetrapod_def = "void AFPSCoastalSplineActor::ConstructBasaltRiprapAndTetrapods(UStaticMesh* CubeMesh, UStaticMesh* CylinderMesh, UStaticMesh* TetrapodMesh, UMaterialInterface* BaseMat, UMaterialInterface* ConcreteMat)"
    if old_tetrapod_def in content:
        content = content.replace(old_tetrapod_def, new_tetrapod_def)

    old_tetrapod_spawn = """		FString TetName = FString::Printf(TEXT("Tetrapod_Core_%02d"), t);
		UStaticMeshComponent* TetCore = CreateDefaultSubobject<UStaticMeshComponent>(*TetName);
		TetCore->SetupAttachment(RootComponent);
		TetCore->SetMobility(EComponentMobility::Movable);
		if (CubeMesh) TetCore->SetStaticMesh(CubeMesh);
		if (BaseMat) TetCore->SetMaterial(0, BaseMat);
		TetCore->SetRelativeLocation(FVector(X, -1620.0f, 55.0f));
		TetCore->SetRelativeRotation(FRotator(18.0f, t * 47.0f, -22.0f));
		TetCore->SetRelativeScale3D(FVector(1.6f, 1.6f, 1.2f));
		TetCore->SetCollisionProfileName(TEXT("BlockAll"));
		TetCore->ComponentTags.Add(FName("Surface_Basalt_Rock"));
		TetCore->ComponentTags.Add(FName("WaveBreaker"));
		TetrapodComponents.Add(TetCore);"""

    new_tetrapod_spawn = """		FString TetName = FString::Printf(TEXT("Tetrapod_Core_%02d"), t);
		UStaticMeshComponent* TetCore = CreateDefaultSubobject<UStaticMeshComponent>(*TetName);
		TetCore->SetupAttachment(RootComponent);
		TetCore->SetMobility(EComponentMobility::Movable);
		if (TetrapodMesh)
		{
			TetCore->SetStaticMesh(TetrapodMesh);
			TetCore->SetRelativeLocation(FVector(X, -1620.0f, 20.0f));
			TetCore->SetRelativeRotation(FRotator(0.0f, t * 53.0f, 0.0f));
			TetCore->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
			if (ConcreteMat) TetCore->SetMaterial(0, ConcreteMat);
		}
		else
		{
			if (CubeMesh) TetCore->SetStaticMesh(CubeMesh);
			TetCore->SetRelativeLocation(FVector(X, -1620.0f, 55.0f));
			TetCore->SetRelativeRotation(FRotator(18.0f, t * 47.0f, -22.0f));
			TetCore->SetRelativeScale3D(FVector(1.6f, 1.6f, 1.2f));
			if (BaseMat) TetCore->SetMaterial(0, BaseMat);
		}
		TetCore->SetCollisionProfileName(TEXT("BlockAll"));
		TetCore->ComponentTags.Add(FName("Surface_Basalt_Rock"));
		TetCore->ComponentTags.Add(FName("WaveBreaker"));
		TetrapodComponents.Add(TetCore);"""

    if old_tetrapod_spawn in content:
        content = content.replace(old_tetrapod_spawn, new_tetrapod_spawn)
    else:
        print("Warning: old_tetrapod_spawn not found in FPSCoastalSplineActor.cpp")

    with open(path, "w", encoding="utf-8") as f:
        f.write(content)
    print("Updated FPSCoastalSplineActor.cpp")


def update_map_actor_h():
    path = "Source/NobleFermi/Map/FPSCoastalOperationsMapActor.h"
    with open(path, "r", encoding="utf-8") as f:
        content = f.read()

    # Add PierWatchtower and DuneGrassClumps
    if "PierWatchtower" not in content:
        target = "TArray<TObjectPtr<UStaticMeshComponent>> PierLadders;\n"
        replacement = target + "\n\tUPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = \"Map|Pier\")\n\tTObjectPtr<UStaticMeshComponent> PierWatchtower;\n"
        content = content.replace(target, replacement)

    if "DuneGrassClumps" not in content:
        target = "TArray<TObjectPtr<UStaticMeshComponent>> DuneComponents;\n"
        replacement = target + "\n\tUPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = \"Map|Dunes\")\n\tTArray<TObjectPtr<UStaticMeshComponent>> DuneGrassClumps;\n"
        content = content.replace(target, replacement)

    # Update ConstructMaritimePier and ConstructCoastalDunes declarations to accept meshes
    if "UStaticMesh* WatchtowerMesh" not in content:
        target = "void ConstructMaritimePier(UStaticMesh* CubeMesh, UStaticMesh* CylinderMesh, UMaterialInterface* BaseMat);"
        replacement = "void ConstructMaritimePier(UStaticMesh* CubeMesh, UStaticMesh* CylinderMesh, UStaticMesh* WatchtowerMesh, UMaterialInterface* BaseMat, UMaterialInterface* TimberMat, UMaterialInterface* MetalMat, UMaterialInterface* RoofMat);"
        content = content.replace(target, replacement)

    if "UStaticMesh* GrassClumpMesh" not in content:
        target = "void ConstructCoastalDunes(UStaticMesh* CubeMesh, UMaterialInterface* BaseMat);"
        replacement = "void ConstructCoastalDunes(UStaticMesh* CubeMesh, UStaticMesh* GrassClumpMesh, UMaterialInterface* BaseMat, UMaterialInterface* GrassMat);"
        content = content.replace(target, replacement)

    with open(path, "w", encoding="utf-8") as f:
        f.write(content)
    print("Updated FPSCoastalOperationsMapActor.h")


def update_map_actor_cpp():
    path = "Source/NobleFermi/Map/FPSCoastalOperationsMapActor.cpp"
    with open(path, "r", encoding="utf-8") as f:
        content = f.read()

    # 1. Update Constructor ObjectFinders
    old_finders = """\t// 3. Load Base Meshes & Materials
\tstatic ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
\tstatic ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
\tstatic ConstructorHelpers::FObjectFinder<UMaterialInterface> BaseMatFinder(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

\tUStaticMesh* CubeMesh = CubeFinder.Succeeded() ? CubeFinder.Object : nullptr;
\tUStaticMesh* CylinderMesh = CylinderFinder.Succeeded() ? CylinderFinder.Object : nullptr;
\tUMaterialInterface* BaseMat = BaseMatFinder.Succeeded() ? BaseMatFinder.Object : nullptr;

\t// 4. Construct Modular Zones
\tConstructWaterAndBeach(CubeMesh, BaseMat);
\tConstructMaritimePier(CubeMesh, CylinderMesh, BaseMat);
\tConstructCoastalDunes(CubeMesh, BaseMat);"""

    new_finders = """\t// 3. Load Base Meshes & Materials
\tstatic ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
\tstatic ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
\tstatic ConstructorHelpers::FObjectFinder<UMaterialInterface> BaseMatFinder(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
\tstatic ConstructorHelpers::FObjectFinder<UStaticMesh> WatchtowerFinder(TEXT("/Game/Environment/Coastal/Meshes/SM_Pier_Watchtower.SM_Pier_Watchtower"));
\tstatic ConstructorHelpers::FObjectFinder<UStaticMesh> GrassClumpFinder(TEXT("/Game/Environment/Coastal/Meshes/SM_Marram_Grass_Clump.SM_Marram_Grass_Clump"));
\tstatic ConstructorHelpers::FObjectFinder<UMaterialInterface> WetSandMatFinder(TEXT("/Game/Environment/Coastal/Materials/M_Coastal_WetSand.M_Coastal_WetSand"));
\tstatic ConstructorHelpers::FObjectFinder<UMaterialInterface> OceanWaterMatFinder(TEXT("/Game/Environment/Coastal/Materials/M_Coastal_OceanWater.M_Coastal_OceanWater"));
\tstatic ConstructorHelpers::FObjectFinder<UMaterialInterface> TimberMatFinder(TEXT("/Game/Environment/Coastal/Materials/M_Coastal_Timber.M_Coastal_Timber"));
\tstatic ConstructorHelpers::FObjectFinder<UMaterialInterface> MetalMatFinder(TEXT("/Game/Environment/Coastal/Materials/M_Coastal_Metal.M_Coastal_Metal"));
\tstatic ConstructorHelpers::FObjectFinder<UMaterialInterface> RoofMetalMatFinder(TEXT("/Game/Environment/Coastal/Materials/M_Coastal_RoofMetal.M_Coastal_RoofMetal"));
\tstatic ConstructorHelpers::FObjectFinder<UMaterialInterface> DuneGrassMatFinder(TEXT("/Game/Environment/Coastal/Materials/M_Coastal_DuneGrass.M_Coastal_DuneGrass"));

\tUStaticMesh* CubeMesh = CubeFinder.Succeeded() ? CubeFinder.Object : nullptr;
\tUStaticMesh* CylinderMesh = CylinderFinder.Succeeded() ? CylinderFinder.Object : nullptr;
\tUMaterialInterface* BaseMat = BaseMatFinder.Succeeded() ? BaseMatFinder.Object : nullptr;
\tUStaticMesh* WatchtowerMesh = WatchtowerFinder.Succeeded() ? WatchtowerFinder.Object : nullptr;
\tUStaticMesh* GrassClumpMesh = GrassClumpFinder.Succeeded() ? GrassClumpFinder.Object : nullptr;
\tUMaterialInterface* WetSandMat = WetSandMatFinder.Succeeded() ? WetSandMatFinder.Object : BaseMat;
\tUMaterialInterface* OceanWaterMat = OceanWaterMatFinder.Succeeded() ? OceanWaterMatFinder.Object : BaseMat;
\tUMaterialInterface* TimberMat = TimberMatFinder.Succeeded() ? TimberMatFinder.Object : BaseMat;
\tUMaterialInterface* MetalMat = MetalMatFinder.Succeeded() ? MetalMatFinder.Object : BaseMat;
\tUMaterialInterface* RoofMetalMat = RoofMetalMatFinder.Succeeded() ? RoofMetalMatFinder.Object : BaseMat;
\tUMaterialInterface* DuneGrassMat = DuneGrassMatFinder.Succeeded() ? DuneGrassMatFinder.Object : BaseMat;

\t// 4. Construct Modular Zones
\tConstructWaterAndBeach(CubeMesh, WetSandMat ? WetSandMat : BaseMat);
\tif (WaterPlaneMesh && OceanWaterMat) WaterPlaneMesh->SetMaterial(0, OceanWaterMat);
\tConstructMaritimePier(CubeMesh, CylinderMesh, WatchtowerMesh, BaseMat, TimberMat, MetalMat, RoofMetalMat);
\tConstructCoastalDunes(CubeMesh, GrassClumpMesh, BaseMat, DuneGrassMat);"""

    if old_finders in content:
        content = content.replace(old_finders, new_finders)
    else:
        print("Warning: old_finders not found in FPSCoastalOperationsMapActor.cpp")

    # 2. Update ConstructMaritimePier definition & add watchtower
    old_pier_def = "void AFPSCoastalOperationsMapActor::ConstructMaritimePier(UStaticMesh* CubeMesh, UStaticMesh* CylinderMesh, UMaterialInterface* BaseMat)"
    new_pier_def = "void AFPSCoastalOperationsMapActor::ConstructMaritimePier(UStaticMesh* CubeMesh, UStaticMesh* CylinderMesh, UStaticMesh* WatchtowerMesh, UMaterialInterface* BaseMat, UMaterialInterface* TimberMat, UMaterialInterface* MetalMat, UMaterialInterface* RoofMat)"
    if old_pier_def in content:
        content = content.replace(old_pier_def, new_pier_def)

    # Add watchtower placement inside ConstructMaritimePier right before bollards
    watchtower_code = """
	// Elevated Maritime Observation Watchtower & Cabin (Domination Flag C Anchor)
	if (WatchtowerMesh)
	{
		PierWatchtower = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Pier_Watchtower_Elevated"));
		PierWatchtower->SetupAttachment(RootComponent);
		PierWatchtower->SetMobility(EComponentMobility::Movable);
		PierWatchtower->SetStaticMesh(WatchtowerMesh);
		if (TimberMat) PierWatchtower->SetMaterial(0, TimberMat);
		if (RoofMat) PierWatchtower->SetMaterial(1, RoofMat);
		PierWatchtower->SetRelativeLocation(FVector(520.0f, -5850.0f, 250.0f));
		PierWatchtower->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
		PierWatchtower->SetCollisionProfileName(TEXT("BlockAll"));
		PierWatchtower->SetCastShadow(true);
		PierWatchtower->ComponentTags.Add(FName("Watchtower"));
	}
"""
    if "PierWatchtower = CreateDefaultSubobject" not in content:
        content = content.replace("// Cast-Iron Mooring Bollards", watchtower_code + "\n\t// Cast-Iron Mooring Bollards")

    # 3. Update ConstructCoastalDunes definition & add grass clumps
    old_dune_def = "void AFPSCoastalOperationsMapActor::ConstructCoastalDunes(UStaticMesh* CubeMesh, UMaterialInterface* BaseMat)"
    new_dune_def = "void AFPSCoastalOperationsMapActor::ConstructCoastalDunes(UStaticMesh* CubeMesh, UStaticMesh* GrassClumpMesh, UMaterialInterface* BaseMat, UMaterialInterface* GrassMat)"
    if old_dune_def in content:
        content = content.replace(old_dune_def, new_dune_def)

    grass_clump_code = """
	// 5. Scatter Windblown Coastal Marram Dune Grass Clumps (Docs/Screenshots/aim/coastal_operations_gameplay.jpg)
	if (GrassClumpMesh)
	{
		const FVector ClumpPositions[] = {
			FVector(-3800.0f, 1600.0f, 75.0f),
			FVector(-3500.0f, 1850.0f, 85.0f),
			FVector(-4100.0f, 2050.0f, 90.0f),
			FVector(-3700.0f, 2350.0f, 100.0f),
			FVector(-3900.0f, 2550.0f, 95.0f),
			FVector(-3400.0f, 2200.0f, 90.0f),
			FVector(3800.0f, 1600.0f, 75.0f),
			FVector(3500.0f, 1850.0f, 85.0f),
			FVector(4100.0f, 2050.0f, 90.0f),
			FVector(3700.0f, 2350.0f, 100.0f),
			FVector(3900.0f, 2550.0f, 95.0f),
			FVector(3400.0f, 2200.0f, 90.0f),
			FVector(-4200.0f, 4800.0f, 85.0f),
			FVector(-4000.0f, 5100.0f, 95.0f),
			FVector(-4400.0f, 5400.0f, 105.0f),
			FVector(4200.0f, 4800.0f, 85.0f),
			FVector(4000.0f, 5100.0f, 95.0f),
			FVector(4400.0f, 5400.0f, 105.0f),
			FVector(-4700.0f, 3700.0f, 95.0f),
			FVector(-4900.0f, 3900.0f, 105.0f),
			FVector(4700.0f, 3700.0f, 95.0f),
			FVector(4900.0f, 3900.0f, 105.0f)
		};

		for (int32 g = 0; g < UE_ARRAY_COUNT(ClumpPositions); ++g)
		{
			FString GrassName = FString::Printf(TEXT("MarramGrass_%02d"), g);
			UStaticMeshComponent* Grass = CreateDefaultSubobject<UStaticMeshComponent>(*GrassName);
			Grass->SetupAttachment(RootComponent);
			Grass->SetMobility(EComponentMobility::Movable);
			Grass->SetStaticMesh(GrassClumpMesh);
			if (GrassMat) Grass->SetMaterial(0, GrassMat);
			Grass->SetRelativeLocation(ClumpPositions[g]);
			Grass->SetRelativeRotation(FRotator(0.0f, g * 47.3f, 0.0f));
			const float ClumpScale = 0.9f + 0.35f * FMath::Frac(g * 0.37f);
			Grass->SetRelativeScale3D(FVector(ClumpScale, ClumpScale, ClumpScale));
			Grass->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Grass->SetCastShadow(true);
			DuneGrassClumps.Add(Grass);
		}
	}
"""
    if "MarramGrass_" not in content:
        content = content.replace("Dune_Peak_East", "Dune_Peak_East")
        # Insert at the end of ConstructCoastalDunes function body
        dune_end_marker = "AddDune(TEXT(\"Dune_Peak_East\"), FVector(4800.0f, 3800.0f, 100.0f), FRotator(10.0f, 115.0f, 0.0f), FVector(12.0f, 10.0f, 2.0f), false);\n}"
        content = content.replace(dune_end_marker, "AddDune(TEXT(\"Dune_Peak_East\"), FVector(4800.0f, 3800.0f, 100.0f), FRotator(10.0f, 115.0f, 0.0f), FVector(12.0f, 10.0f, 2.0f), false);" + grass_clump_code + "\n}")

    with open(path, "w", encoding="utf-8") as f:
        f.write(content)
    print("Updated FPSCoastalOperationsMapActor.cpp")

if __name__ == "__main__":
    update_spline_actor_h()
    update_spline_actor_cpp()
    update_map_actor_h()
    update_map_actor_cpp()
