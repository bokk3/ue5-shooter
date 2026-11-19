import os
import re

print("=== APPLYING FINAL RENDER ARCHITECTURE PATCHES ===")

# -------------------------------------------------------------------------
# 1. PATCH FPSCharacterPreviewActor.h
# -------------------------------------------------------------------------
h_path = r"Source\NobleFermi\Character\FPSCharacterPreviewActor.h"
with open(h_path, "rb") as f:
    h_bytes = f.read()

h_text = h_bytes.decode("utf-8")

# Add OperatorMesh if not already present
if "OperatorMesh" not in h_text:
    target = "UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = \"FPS Preview|Skeletal\")\r\n\tTObjectPtr<USkeletalMeshComponent> BodyMesh;"
    if target not in h_text:
        target = "UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = \"FPS Preview|Skeletal\")\n\tTObjectPtr<USkeletalMeshComponent> BodyMesh;"
    
    replacement = (
        "// Master High-Fidelity Posed Operator (Vanguard Soldier with Armor Rig, Helmet, Visor, Boots & AK-47)\r\n"
        "\tUPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = \"FPS Preview|Operator\")\r\n"
        "\tTObjectPtr<UStaticMeshComponent> OperatorMesh;\r\n\r\n\t" + target.replace("\n", "\r\n")
    )
    
    if target in h_text:
        h_text = h_text.replace(target, replacement, 1)
        print("Patched FPSCharacterPreviewActor.h with OperatorMesh")
    else:
        print("WARNING: Target for OperatorMesh not found in FPSCharacterPreviewActor.h")

# Ensure CRLF line endings
h_text = re.sub(r'(?<!\r)\n', '\r\n', h_text)
with open(h_path, "wb") as f:
    f.write(h_text.encode("utf-8"))

print("Saved FPSCharacterPreviewActor.h")

# -------------------------------------------------------------------------
# 2. PATCH FPSMenuHangarStage.cpp
# -------------------------------------------------------------------------
stage_path = r"Source\NobleFermi\Game\FPSMenuHangarStage.cpp"
with open(stage_path, "rb") as f:
    stage_bytes = f.read()

stage_text = stage_bytes.decode("utf-8")

# Replace BasicShapes with project Hangar assets
old_finders = """\tstatic ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
\tstatic ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
\tstatic ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
\tstatic ConstructorHelpers::FObjectFinder<UMaterialInterface> SteelMatFinder(TEXT("/Game/Characters/BaseMale/Materials/M_AK47_Steel.M_AK47_Steel"));
\tstatic ConstructorHelpers::FObjectFinder<UMaterialInterface> BaseMatFinder(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

\tUStaticMesh* CubeMesh = CubeFinder.Succeeded() ? CubeFinder.Object : nullptr;
\tUStaticMesh* CylinderMesh = CylinderFinder.Succeeded() ? CylinderFinder.Object : nullptr;
\tUStaticMesh* SphereMesh = SphereFinder.Succeeded() ? SphereFinder.Object : nullptr;
\tUMaterialInterface* SteelMat = SteelMatFinder.Succeeded() ? SteelMatFinder.Object : (BaseMatFinder.Succeeded() ? BaseMatFinder.Object : nullptr);
\tUMaterialInterface* BaseMat = BaseMatFinder.Succeeded() ? BaseMatFinder.Object.Get() : SteelMat;"""

new_finders = """\tstatic ConstructorHelpers::FObjectFinder<UStaticMesh> ColumnFinder(TEXT("/Game/Environment/Hangar/SM_Hangar_Wall.SM_Hangar_Wall"));
\tstatic ConstructorHelpers::FObjectFinder<UStaticMesh> PlinthFinder(TEXT("/Game/Environment/Hangar/SM_Hangar_Plinth.SM_Hangar_Plinth"));
\tstatic ConstructorHelpers::FObjectFinder<UStaticMesh> FloorFinder(TEXT("/Game/Environment/Hangar/SM_Hangar_Floor.SM_Hangar_Floor"));
\tstatic ConstructorHelpers::FObjectFinder<UMaterialInterface> SteelMatFinder(TEXT("/Game/Characters/BaseMale/Materials/M_AK47_Steel.M_AK47_Steel"));

\tUStaticMesh* CubeMesh = ColumnFinder.Succeeded() ? ColumnFinder.Object : nullptr;
\tUStaticMesh* CylinderMesh = PlinthFinder.Succeeded() ? PlinthFinder.Object : nullptr;
\tUStaticMesh* SphereMesh = PlinthFinder.Succeeded() ? PlinthFinder.Object : nullptr;
\tUStaticMesh* FloorMesh = FloorFinder.Succeeded() ? FloorFinder.Object : CubeMesh;
\tUMaterialInterface* SteelMat = SteelMatFinder.Succeeded() ? SteelMatFinder.Object : nullptr;
\tUMaterialInterface* BaseMat = SteelMat;"""

# Normalize for comparison
def norm_newlines(s):
    return s.replace("\r\n", "\n")

if norm_newlines(old_finders) in norm_newlines(stage_text):
    stage_text = norm_newlines(stage_text).replace(norm_newlines(old_finders), norm_newlines(new_finders), 1)
    print("Replaced mesh finders in FPSMenuHangarStage.cpp")
else:
    print("WARNING: Old finders not found in FPSMenuHangarStage.cpp")

# Update WetFloorPlane initialization in ConstructLayer1FloorAndPlinth
old_wet_floor = """\tWetFloorPlane->SetRelativeLocation(FVector(-550.0f, 0.0f, -10.2f));
\tWetFloorPlane->SetRelativeScale3D(FVector(24.0f, 24.0f, 0.20f));"""

new_wet_floor = """\tWetFloorPlane->SetRelativeLocation(FVector(-550.0f, 0.0f, -0.2f));
\tWetFloorPlane->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));"""

if norm_newlines(old_wet_floor) in norm_newlines(stage_text):
    stage_text = norm_newlines(stage_text).replace(norm_newlines(old_wet_floor), norm_newlines(new_wet_floor), 1)
    print("Updated WetFloorPlane transform in FPSMenuHangarStage.cpp")

# Ensure CRLF line endings
stage_text = re.sub(r'(?<!\r)\n', '\r\n', stage_text)
with open(stage_path, "wb") as f:
    f.write(stage_text.encode("utf-8"))

print("Saved FPSMenuHangarStage.cpp")
