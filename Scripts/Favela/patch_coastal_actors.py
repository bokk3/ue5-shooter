import os

root = r"C:\Users\Boris\Documents\antigravity\noble-fermi\Source\NobleFermi\Map"

# 1. Patch FPSCoastalOperationsMapActor.cpp
coastal_cpp = os.path.join(root, "FPSCoastalOperationsMapActor.cpp")
with open(coastal_cpp, "r", encoding="utf-8") as f:
    txt = f.read()

old_begin = """\t// 1. Beach sand & ocean water
\tUFPSCoastalMaterialLibrary::ApplyCoastalSurfaceToMesh(BeachFloorMesh, ECoastalSurfaceType::FineBeachSand);
\tif (BeachFloorMesh) BeachFloorMesh->ComponentTags.AddUnique(FName("Surface_Sand_Dune"));
\tApplyTint(WaterPlaneMesh, FLinearColor(0.04f, 0.16f, 0.22f), 0.08f);

\t// 2. Weathered maritime pier planks
\tfor (UStaticMeshComponent* Plank : PierVisualPlanks)
\t{
\t\tUFPSCoastalMaterialLibrary::ApplyCoastalSurfaceToMesh(Plank, ECoastalSurfaceType::WeatheredPierTimber);
\t\tif (Plank) Plank->ComponentTags.AddUnique(FName("Surface_Pier_Wood"));
\t}

\t// 3. Pilings (weathered timber + concrete)
\tfor (UStaticMeshComponent* Piling : PierPilings)
\t{
\t\tUFPSCoastalMaterialLibrary::ApplyCoastalSurfaceToMesh(Piling, ECoastalSurfaceType::WeatheredPierTimber);
\t\tif (Piling) Piling->ComponentTags.AddUnique(FName("Surface_Pier_Wood"));
\t}

\t// 4. Transverse cross-beams
\tfor (UStaticMeshComponent* Beam : PierCrossBeams)
\t{
\t\tUFPSCoastalMaterialLibrary::ApplyCoastalSurfaceToMesh(Beam, ECoastalSurfaceType::WeatheredPierTimber);
\t\tif (Beam) Beam->ComponentTags.AddUnique(FName("Surface_Pier_Wood"));
\t}

\t// 5. Cast-iron mooring bollards
\tfor (UStaticMeshComponent* Bollard : PierBollards)
\t{
\t\tApplyTint(Bollard, FLinearColor(0.18f, 0.14f, 0.12f), 0.60f); // Rusted iron
\t}

\t// 6. Galvanized steel tube railings
\tfor (UStaticMeshComponent* Railing : PierRailings)
\t{
\t\tApplyTint(Railing, FLinearColor(0.55f, 0.57f, 0.60f), 0.40f);
\t}

\t// 7. Marine access ladders
\tfor (UStaticMeshComponent* Ladder : PierLadders)
\t{
\t\tApplyTint(Ladder, FLinearColor(0.35f, 0.36f, 0.38f), 0.50f);
\t}

\t// 8. Coastal dunes (stoss vs slip face)
\tfor (UStaticMeshComponent* Dune : DuneComponents)
\t{
\t\tUFPSCoastalMaterialLibrary::ApplyCoastalSurfaceToMesh(Dune, ECoastalSurfaceType::FineBeachSand);
\t\tif (Dune)
\t\t{
\t\t\tDune->ComponentTags.AddUnique(FName("Surface_Sand_Dune"));
\t\t}
\t}

\t// 9. Perimeter boundaries
\tfor (UStaticMeshComponent* Wall : PerimeterWalls)
\t{
\t\tUFPSCoastalMaterialLibrary::ApplyCoastalSurfaceToMesh(Wall, ECoastalSurfaceType::MarineCastConcrete);
\t}

\t// 10. Domination Objectives (A, B, C)
\tfor (UStaticMeshComponent* Flag : ObjectiveFlags)
\t{
\t\tApplyTint(Flag, FLinearColor(0.95f, 0.65f, 0.15f), 0.30f);
\t}

\tSpawnTacticalPlayerStarts();"""

new_begin = """\t// 1. Coastal Favela Cobblestone & Sand Substrate + Reflective Bay Water
\tUFPSCoastalMaterialLibrary::ApplyCoastalSurfaceToMesh(BeachFloorMesh, ECoastalSurfaceType::FineBeachSand);
\tif (BeachFloorMesh) BeachFloorMesh->ComponentTags.AddUnique(FName("Surface_Sand_Dune"));
\tUFPSCoastalMaterialLibrary::ApplyCoastalSurfaceToMesh(WaterPlaneMesh, ECoastalSurfaceType::FavelaPuddle);

\t// 2. Weathered maritime pier planks
\tfor (UStaticMeshComponent* Plank : PierVisualPlanks)
\t{
\t\tUFPSCoastalMaterialLibrary::ApplyCoastalSurfaceToMesh(Plank, ECoastalSurfaceType::WeatheredPierTimber);
\t\tif (Plank) Plank->ComponentTags.AddUnique(FName("Surface_Pier_Wood"));
\t}

\t// 3. Pilings (weathered timber + concrete)
\tfor (UStaticMeshComponent* Piling : PierPilings)
\t{
\t\tUFPSCoastalMaterialLibrary::ApplyCoastalSurfaceToMesh(Piling, ECoastalSurfaceType::WeatheredPierTimber);
\t\tif (Piling) Piling->ComponentTags.AddUnique(FName("Surface_Pier_Wood"));
\t}

\t// 4. Transverse cross-beams
\tfor (UStaticMeshComponent* Beam : PierCrossBeams)
\t{
\t\tUFPSCoastalMaterialLibrary::ApplyCoastalSurfaceToMesh(Beam, ECoastalSurfaceType::WeatheredPierTimber);
\t\tif (Beam) Beam->ComponentTags.AddUnique(FName("Surface_Pier_Wood"));
\t}

\t// 5. Cast-iron mooring bollards, railings & ladders
\tfor (UStaticMeshComponent* Bollard : PierBollards)
\t{
\t\tUFPSCoastalMaterialLibrary::ApplyCoastalSurfaceToMesh(Bollard, ECoastalSurfaceType::FavelaMetal);
\t}
\tfor (UStaticMeshComponent* Railing : PierRailings)
\t{
\t\tUFPSCoastalMaterialLibrary::ApplyCoastalSurfaceToMesh(Railing, ECoastalSurfaceType::FavelaMetal);
\t}
\tfor (UStaticMeshComponent* Ladder : PierLadders)
\t{
\t\tUFPSCoastalMaterialLibrary::ApplyCoastalSurfaceToMesh(Ladder, ECoastalSurfaceType::FavelaMetal);
\t}

\t// 8. Coastal hillside embankments & masonry berms
\tfor (int32 i = 0; i < DuneComponents.Num(); ++i)
\t{
\t\tUStaticMeshComponent* Dune = DuneComponents[i];
\t\tif (!Dune) continue;
\t\tconst FString DName = Dune->GetName();
\t\tconst ECoastalSurfaceType Surf = DName.Contains(TEXT("Cover"))
\t\t\t? ECoastalSurfaceType::FavelaBrick
\t\t\t: ECoastalSurfaceType::FavelaDirt;
\t\tUFPSCoastalMaterialLibrary::ApplyCoastalSurfaceToMesh(Dune, Surf);
\t\tDune->ComponentTags.AddUnique(FName("Surface_Sand_Dune"));
\t}

\t// 9. Perimeter boundaries
\tfor (int32 i = 0; i < PerimeterWalls.Num(); ++i)
\t{
\t\tUFPSCoastalMaterialLibrary::ApplyCoastalSurfaceToMesh(
\t\t\tPerimeterWalls[i],
\t\t\t(i % 2 == 0) ? ECoastalSurfaceType::FavelaCinder : ECoastalSurfaceType::FavelaBrick);
\t}

\t// 10. Domination Objectives (A, B, C)
\tfor (UStaticMeshComponent* Flag : ObjectiveFlags)
\t{
\t\tUFPSCoastalMaterialLibrary::ApplyCoastalSurfaceToMesh(Flag, ECoastalSurfaceType::FavelaMetal);
\t}

\t// 11. Populate Baia Brava Coastal Favela Shanties, Corrugated Roofs, Drainage Gutters, Foliage & Swaying Wires
\tUFPSCoastalMaterialLibrary::PopulateBaiaBravaFavela(this, RootComponent);

\tSpawnTacticalPlayerStarts();"""

assert old_begin in txt, "old_begin not found in FPSCoastalOperationsMapActor.cpp"
txt = txt.replace(old_begin, new_begin, 1)

old_berms = """\t// 3. Central Interdune Basin & Flag A Waist/Chest Defilade Berms
\tAddDune(TEXT("Dune_Basin_Cover_W"), FVector(-750.0f, 3650.0f, 75.0f), FRotator(0.0f, 25.0f, 0.0f), FVector(8.5f, 3.2f, 1.6f), false);
\tAddDune(TEXT("Dune_Basin_Cover_E"), FVector(750.0f, 3650.0f, 75.0f), FRotator(0.0f, -25.0f, 0.0f), FVector(8.5f, 3.2f, 1.6f), false);"""

new_berms = """\t// 3. Avenue Flank Brick Retaining Walls (leaving central cobblestone avenue clear for sightlines)
\tAddDune(TEXT("Dune_Basin_Cover_W"), FVector(-1580.0f, 3650.0f, 55.0f), FRotator(0.0f, 12.0f, 0.0f), FVector(4.2f, 1.2f, 1.1f), false);
\tAddDune(TEXT("Dune_Basin_Cover_E"), FVector(1580.0f, 3650.0f, 55.0f), FRotator(0.0f, -12.0f, 0.0f), FVector(4.2f, 1.2f, 1.1f), false);"""

assert old_berms in txt, "old_berms not found"
txt = txt.replace(old_berms, new_berms, 1)

txt = txt.replace(
    'TEXT("[CoastalOps] Coastal Operations: Successfully spawned 8 PlayerStarts (4 TeamAlpha, 4 TeamBravo).")',
    'TEXT("[BaiaBrava] Favela // Baia Brava: Successfully spawned 8 PlayerStarts (4 TeamAlpha, 4 TeamBravo).")'
)

with open(coastal_cpp, "w", encoding="utf-8", newline="\r\n") as f:
    f.write(txt.replace("\r\n", "\n"))
print("Updated FPSCoastalOperationsMapActor.cpp")

# 2. Patch FPSCoastalSplineActor.cpp
spline_cpp = os.path.join(root, "FPSCoastalSplineActor.cpp")
with open(spline_cpp, "r", encoding="utf-8") as f:
    stxt = f.read()

old_spline_begin = """void AFPSCoastalSplineActor::BeginPlay()
{
\tSuper::BeginPlay();

\tauto ApplyTint = [](UStaticMeshComponent* Comp, const FLinearColor& Color, float Roughness = 0.80f)
\t{
\t\tif (!Comp) return;
\t\tif (UMaterialInstanceDynamic* DynMat = Comp->CreateDynamicMaterialInstance(0))
\t\t{
\t\t\tDynMat->SetVectorParameterValue(TEXT("Color"), Color);
\t\t\tDynMat->SetVectorParameterValue(TEXT("BaseColor"), Color);
\t\t\tDynMat->SetScalarParameterValue(TEXT("Roughness"), Roughness);
\t\t}
\t};

\t// 1. Weathered macadam crest roadway (Dark slate asphalt)
\tfor (UStaticMeshComponent* Comp : CrestRoadComponents)
\t{
\t\tUFPSCoastalMaterialLibrary::ApplyCoastalSurfaceToMesh(Comp, ECoastalSurfaceType::WeatheredAsphalt);
\t\tApplyTint(Comp, FLinearColor(0.16f, 0.17f, 0.19f), 0.75f);
\t}

\t// 2. Concrete curbs & central bunker housing
\tfor (UStaticMeshComponent* Comp : CurbComponents)
\t{
\t\tUFPSCoastalMaterialLibrary::ApplyCoastalSurfaceToMesh(Comp, ECoastalSurfaceType::MarineCastConcrete);
\t\tApplyTint(Comp, FLinearColor(0.42f, 0.44f, 0.46f), 0.78f);
\t}

\t// 3. Seaward armored revetment slope slabs (Previously missing in BeginPlay!)
\tfor (UStaticMeshComponent* Comp : SeawardRevetmentComponents)
\t{
\t\tUFPSCoastalMaterialLibrary::ApplyCoastalSurfaceToMesh(Comp, ECoastalSurfaceType::BasaltRiprapRock);
\t\tApplyTint(Comp, FLinearColor(0.22f, 0.24f, 0.27f), 0.72f);
\t}

\t// 4. Basalt riprap armor rocks
\tfor (UStaticMeshComponent* Comp : BasaltRiprapComponents)
\t{
\t\tUFPSCoastalMaterialLibrary::ApplyCoastalSurfaceToMesh(Comp, ECoastalSurfaceType::BasaltRiprapRock);
\t\tApplyTint(Comp, FLinearColor(0.13f, 0.14f, 0.16f), 0.65f);
\t}

\t// 5. Wave-breaking concrete tetrapods
\tfor (UStaticMeshComponent* Comp : TetrapodComponents)
\t{
\t\tUFPSCoastalMaterialLibrary::ApplyCoastalSurfaceToMesh(Comp, ECoastalSurfaceType::MarineCastConcrete);
\t\tApplyTint(Comp, FLinearColor(0.48f, 0.49f, 0.50f), 0.78f);
\t}

\t// 6. Landward stabilized coastal turf / marram grass back-slope
\tfor (UStaticMeshComponent* Comp : LandwardSlopeComponents)
\t{
\t\tUFPSCoastalMaterialLibrary::ApplyCoastalSurfaceToMesh(Comp, ECoastalSurfaceType::FineBeachSand);
\t\tApplyTint(Comp, FLinearColor(0.26f, 0.31f, 0.22f), 0.88f); // North Sea coastal marram turf green-tan
\t}

\t// 7. Flank access stairs & pier abutment ramp
\tfor (UStaticMeshComponent* Comp : StairComponents)
\t{
\t\tUFPSCoastalMaterialLibrary::ApplyCoastalSurfaceToMesh(Comp, ECoastalSurfaceType::MarineCastConcrete);
\t\tApplyTint(Comp, FLinearColor(0.46f, 0.47f, 0.49f), 0.76f);
\t}

\t// 8. Drainage Culvert & Sluice Portal
\tfor (UStaticMeshComponent* Comp : CulvertComponents)
\t{
\t\tif (Comp && Comp->ComponentTags.Contains(FName("Sluice_Steel")))
\t\t{
\t\t\tApplyTint(Comp, FLinearColor(0.32f, 0.16f, 0.10f), 0.60f); // Rusted iron ribs
\t\t}
\t\telse
\t\t{
\t\t\tUFPSCoastalMaterialLibrary::ApplyCoastalSurfaceToMesh(Comp, ECoastalSurfaceType::MarineCastConcrete);
\t\t\tApplyTint(Comp, FLinearColor(0.36f, 0.38f, 0.40f), 0.80f);
\t\t}
\t}
}"""

new_spline_begin = """void AFPSCoastalSplineActor::BeginPlay()
{
\tSuper::BeginPlay();

\t// 1. Weathered macadam crest roadway
\tfor (UStaticMeshComponent* Comp : CrestRoadComponents)
\t{
\t\tUFPSCoastalMaterialLibrary::ApplyCoastalSurfaceToMesh(Comp, ECoastalSurfaceType::WeatheredAsphalt);
\t}

\t// 2. Concrete curbs & central bunker housing
\tfor (UStaticMeshComponent* Comp : CurbComponents)
\t{
\t\tUFPSCoastalMaterialLibrary::ApplyCoastalSurfaceToMesh(Comp, ECoastalSurfaceType::FavelaBrick);
\t}

\t// 3. Seaward armored cobblestone revetment slope slabs
\tfor (UStaticMeshComponent* Comp : SeawardRevetmentComponents)
\t{
\t\tUFPSCoastalMaterialLibrary::ApplyCoastalSurfaceToMesh(Comp, ECoastalSurfaceType::FavelaCobble);
\t}

\t// 4. Basalt riprap armor rocks
\tfor (UStaticMeshComponent* Comp : BasaltRiprapComponents)
\t{
\t\tUFPSCoastalMaterialLibrary::ApplyCoastalSurfaceToMesh(Comp, ECoastalSurfaceType::BasaltRiprapRock);
\t}

\t// 5. Wave-breaking concrete tetrapods
\tfor (UStaticMeshComponent* Comp : TetrapodComponents)
\t{
\t\tUFPSCoastalMaterialLibrary::ApplyCoastalSurfaceToMesh(Comp, ECoastalSurfaceType::MarineCastConcrete);
\t}

\t// 6. Landward stabilized coastal favela slope (Cobblestone & Cinder Block)
\tfor (int32 i = 0; i < LandwardSlopeComponents.Num(); ++i)
\t{
\t\tUFPSCoastalMaterialLibrary::ApplyCoastalSurfaceToMesh(
\t\t\tLandwardSlopeComponents[i],
\t\t\t(i % 2 == 0) ? ECoastalSurfaceType::FavelaCobble : ECoastalSurfaceType::FavelaCinder);
\t}

\t// 7. Flank access stairs & pier abutment ramp
\tfor (UStaticMeshComponent* Comp : StairComponents)
\t{
\t\tUFPSCoastalMaterialLibrary::ApplyCoastalSurfaceToMesh(Comp, ECoastalSurfaceType::FavelaCobble);
\t}

\t// 8. Drainage Culvert & Sluice Portal
\tfor (UStaticMeshComponent* Comp : CulvertComponents)
\t{
\t\tif (Comp && Comp->ComponentTags.Contains(FName("Sluice_Steel")))
\t\t{
\t\t\tUFPSCoastalMaterialLibrary::ApplyCoastalSurfaceToMesh(Comp, ECoastalSurfaceType::FavelaMetal);
\t\t}
\t\telse
\t\t{
\t\t\tUFPSCoastalMaterialLibrary::ApplyCoastalSurfaceToMesh(Comp, ECoastalSurfaceType::FavelaBrick);
\t\t}
\t}
}"""

assert old_spline_begin in stxt, "old_spline_begin not found in FPSCoastalSplineActor.cpp"
stxt = stxt.replace(old_spline_begin, new_spline_begin, 1)

with open(spline_cpp, "w", encoding="utf-8", newline="\r\n") as f:
    f.write(stxt.replace("\r\n", "\n"))
print("Updated FPSCoastalSplineActor.cpp")
