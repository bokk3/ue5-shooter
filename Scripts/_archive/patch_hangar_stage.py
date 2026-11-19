import os

hangar_cpp = r"Source/NobleFermi/Game/FPSMenuHangarStage.cpp"
with open(hangar_cpp, 'rb') as f:
    text = f.read().decode('utf-8').replace('\r\n', '\n')

# 1. Constructor finders and layer 1 call
t1_old = """\tstatic ConstructorHelpers::FObjectFinder<UStaticMesh> ColumnFinder(TEXT("/Game/Environment/Hangar/SM_Hangar_Wall.SM_Hangar_Wall"));
\tstatic ConstructorHelpers::FObjectFinder<UStaticMesh> PlinthFinder(TEXT("/Game/Environment/Hangar/SM_Hangar_Plinth.SM_Hangar_Plinth"));
\tstatic ConstructorHelpers::FObjectFinder<UStaticMesh> FloorFinder(TEXT("/Game/Environment/Hangar/SM_Hangar_Floor.SM_Hangar_Floor"));
\tstatic ConstructorHelpers::FObjectFinder<UMaterialInterface> SteelMatFinder(TEXT("/Game/Characters/BaseMale/Materials/M_AK47_Steel.M_AK47_Steel"));

\tUStaticMesh* CubeMesh = ColumnFinder.Succeeded() ? ColumnFinder.Object.Get() : nullptr;
\tUStaticMesh* CylinderMesh = PlinthFinder.Succeeded() ? PlinthFinder.Object.Get() : nullptr;
\tUStaticMesh* SphereMesh = PlinthFinder.Succeeded() ? PlinthFinder.Object.Get() : nullptr;
\tUStaticMesh* FloorMesh = FloorFinder.Succeeded() ? FloorFinder.Object.Get() : CubeMesh;
\tUMaterialInterface* SteelMat = SteelMatFinder.Succeeded() ? SteelMatFinder.Object.Get() : nullptr;
\tUMaterialInterface* BaseMat = SteelMat;

\t// Build all 5 architectural depth layers + RainFX + Three-Tone Lighting Rig
\tConstructLayer1FloorAndPlinth(CubeMesh, CylinderMesh, SteelMat);"""

t1_new = """\tstatic ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
\tstatic ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
\tstatic ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
\tstatic ConstructorHelpers::FObjectFinder<UStaticMesh> FloorFinder(TEXT("/Game/Environment/Hangar/SM_Hangar_Floor.SM_Hangar_Floor"));
\tstatic ConstructorHelpers::FObjectFinder<UMaterialInterface> SteelMatFinder(TEXT("/Game/Characters/BaseMale/Materials/M_AK47_Steel.M_AK47_Steel"));

\tUStaticMesh* CubeMesh = CubeFinder.Succeeded() ? CubeFinder.Object : nullptr;
\tUStaticMesh* CylinderMesh = CylinderFinder.Succeeded() ? CylinderFinder.Object : nullptr;
\tUStaticMesh* SphereMesh = SphereFinder.Succeeded() ? SphereFinder.Object : nullptr;
\tUStaticMesh* FloorMesh = FloorFinder.Succeeded() ? FloorFinder.Object.Get() : CubeMesh;
\tUMaterialInterface* SteelMat = SteelMatFinder.Succeeded() ? SteelMatFinder.Object.Get() : nullptr;
\tUMaterialInterface* BaseMat = SteelMat;

\t// Build all 5 architectural depth layers + RainFX + Three-Tone Lighting Rig
\tConstructLayer1FloorAndPlinth(FloorMesh, CubeMesh, CylinderMesh, SteelMat);"""

assert t1_old in text, "t1_old not found!"
text = text.replace(t1_old, t1_new)

# 2. ConstructLayer1FloorAndPlinth definition
t2_old = """void AFPSMenuHangarStage::ConstructLayer1FloorAndPlinth(UStaticMesh* CubeMesh, UStaticMesh* CylinderMesh, UMaterialInterface* SteelMat)
{
\t// 1. Wide Wet Industrial Floor Plane (Z top surface = -0.2cm directly under Operator's boots)
\tWetFloorPlane = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WetFloorPlane"));
\tWetFloorPlane->SetupAttachment(RootComponent);
\tWetFloorPlane->SetMobility(EComponentMobility::Movable);
\tif (CubeMesh) WetFloorPlane->SetStaticMesh(CubeMesh);
\tif (SteelMat) WetFloorPlane->SetMaterial(0, SteelMat);
\tWetFloorPlane->SetRelativeLocation(FVector(-550.0f, 0.0f, -0.2f));
\tWetFloorPlane->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
\tWetFloorPlane->SetCollisionEnabled(ECollisionEnabled::NoCollision);
\tWetFloorPlane->SetCastShadow(true);"""

t2_new = """void AFPSMenuHangarStage::ConstructLayer1FloorAndPlinth(UStaticMesh* FloorMesh, UStaticMesh* CubeMesh, UStaticMesh* CylinderMesh, UMaterialInterface* SteelMat)
{
\t// 1. Wide Wet Industrial Floor Plane (Z top surface = -0.2cm directly under Operator's boots)
\tWetFloorPlane = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WetFloorPlane"));
\tWetFloorPlane->SetupAttachment(RootComponent);
\tWetFloorPlane->SetMobility(EComponentMobility::Movable);
\tif (FloorMesh) WetFloorPlane->SetStaticMesh(FloorMesh);
\tif (SteelMat) WetFloorPlane->SetMaterial(0, SteelMat);
\tWetFloorPlane->SetRelativeLocation(FVector(0.0f, 0.0f, -0.2f));
\tWetFloorPlane->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
\tWetFloorPlane->SetCollisionEnabled(ECollisionEnabled::NoCollision);
\tWetFloorPlane->SetCastShadow(true);"""

assert t2_old in text, "t2_old not found!"
text = text.replace(t2_old, t2_new)

# 3. Plinth rings in HangarStage hidden so PreviewActor::PedestalMesh is the clean turntable
t3_old = """\t// 3. Raised Low-Profile Tactical Turntable Plinth (2.5cm height, beveled dark metal rim around character feet)
\tPlinthOuterRing = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlinthOuterRing"));
\tPlinthOuterRing->SetupAttachment(RootComponent);
\tPlinthOuterRing->SetMobility(EComponentMobility::Movable);
\tif (CylinderMesh) PlinthOuterRing->SetStaticMesh(CylinderMesh);
\tif (SteelMat) PlinthOuterRing->SetMaterial(0, SteelMat);
\tPlinthOuterRing->SetRelativeLocation(FVector(0.0f, 0.0f, -1.2f));
\tPlinthOuterRing->SetRelativeScale3D(FVector(1.12f, 1.12f, 0.032f));
\tPlinthOuterRing->SetCollisionEnabled(ECollisionEnabled::NoCollision);
\tPlinthOuterRing->SetCastShadow(true);

\tPlinthBevelRing = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlinthBevelRing"));
\tPlinthBevelRing->SetupAttachment(RootComponent);
\tPlinthBevelRing->SetMobility(EComponentMobility::Movable);
\tif (CylinderMesh) PlinthBevelRing->SetStaticMesh(CylinderMesh);
\tif (SteelMat) PlinthBevelRing->SetMaterial(0, SteelMat);
\tPlinthBevelRing->SetRelativeLocation(FVector(0.0f, 0.0f, -0.6f));
\tPlinthBevelRing->SetRelativeScale3D(FVector(0.98f, 0.98f, 0.036f));
\tPlinthBevelRing->SetCollisionEnabled(ECollisionEnabled::NoCollision);
\tPlinthBevelRing->SetCastShadow(false);

\t// 12 Cyan Emissive Micro-Pips embedded around the Turntable Plinth perimeter
\tconst int32 NumPips = 12;
\tconst float PipRadius = 53.0f;"""

t3_new = """\t// 3. Raised Low-Profile Tactical Turntable Plinth (Handled by AFPSCharacterPreviewActor::PedestalMesh)
\tPlinthOuterRing = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlinthOuterRing"));
\tPlinthOuterRing->SetupAttachment(RootComponent);
\tPlinthOuterRing->SetMobility(EComponentMobility::Movable);
\tif (CylinderMesh) PlinthOuterRing->SetStaticMesh(CylinderMesh);
\tif (SteelMat) PlinthOuterRing->SetMaterial(0, SteelMat);
\tPlinthOuterRing->SetRelativeLocation(FVector(0.0f, 0.0f, -1.2f));
\tPlinthOuterRing->SetRelativeScale3D(FVector(1.12f, 1.12f, 0.032f));
\tPlinthOuterRing->SetCollisionEnabled(ECollisionEnabled::NoCollision);
\tPlinthOuterRing->SetCastShadow(false);
\tPlinthOuterRing->SetVisibility(false);

\tPlinthBevelRing = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlinthBevelRing"));
\tPlinthBevelRing->SetupAttachment(RootComponent);
\tPlinthBevelRing->SetMobility(EComponentMobility::Movable);
\tif (CylinderMesh) PlinthBevelRing->SetStaticMesh(CylinderMesh);
\tif (SteelMat) PlinthBevelRing->SetMaterial(0, SteelMat);
\tPlinthBevelRing->SetRelativeLocation(FVector(0.0f, 0.0f, -0.6f));
\tPlinthBevelRing->SetRelativeScale3D(FVector(0.98f, 0.98f, 0.036f));
\tPlinthBevelRing->SetCollisionEnabled(ECollisionEnabled::NoCollision);
\tPlinthBevelRing->SetCastShadow(false);
\tPlinthBevelRing->SetVisibility(false);

\t// 12 Cyan Emissive Micro-Pips embedded around the Turntable Plinth perimeter (radius 136cm)
\tconst int32 NumPips = 12;
\tconst float PipRadius = 136.0f;"""

assert t3_old in text, "t3_old not found!"
text = text.replace(t3_old, t3_new)

# 4. Turn off Depth of Field override in HangarPostProcess
t4_old = """\t// Physical Depth of Field focused precisely on Operator Turntable (Distance = 335cm)
\tPP.bOverride_DepthOfFieldFocalDistance = true;
\tPP.DepthOfFieldFocalDistance = 335.0f;
\tPP.bOverride_DepthOfFieldFstop = true;
\tPP.DepthOfFieldFstop = 3.2f;
\tPP.bOverride_DepthOfFieldSensorWidth = true;
\tPP.DepthOfFieldSensorWidth = 24.576f;"""

t4_new = """\t// Physical Depth of Field disabled to guarantee razor-sharp 3D operator & environment rendering
\tPP.bOverride_DepthOfFieldFocalDistance = false;
\tPP.bOverride_DepthOfFieldFstop = false;
\tPP.bOverride_DepthOfFieldSensorWidth = false;"""

assert t4_old in text, "t4_old not found!"
text = text.replace(t4_old, t4_new)

# 5. Ensure camera config also disables DoF
t5_old = """\t\tCamPP.bOverride_DepthOfFieldFocalDistance = false;
\t\tCamPP.bOverride_DepthOfFieldFstop = false;"""

t5_new = """\t\tCamPP.bOverride_DepthOfFieldFocalDistance = false;
\t\tCamPP.bOverride_DepthOfFieldFstop = false;
\t\tCamPP.bOverride_DepthOfFieldSensorWidth = false;"""

assert t5_old in text, "t5_old not found!"
text = text.replace(t5_old, t5_new)

# Write back strictly as CRLF
text_crlf = text.replace('\n', '\r\n')
with open(hangar_cpp, 'wb') as f:
    f.write(text_crlf.encode('utf-8'))

print("FPSMenuHangarStage.cpp successfully patched with pure CRLF!")
