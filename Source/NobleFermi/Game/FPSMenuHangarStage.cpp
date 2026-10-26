// =========================================================================================
// NOBLE FERMI — MAIN MENU 3D HANGAR DIORAMA & RAIN-GLASS OPTICS STAGE ACTOR
// Target Engine: Unreal Engine 5.8 (C++20 / Native Rendering Pipeline)
// =========================================================================================

#include "Game/FPSMenuHangarStage.h"
#include "Character/FPSCharacterPreviewActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PostProcessComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/Material.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/Package.h"
#include "Misc/ConfigCacheIni.h"

namespace HangarPBRCache
{
	static TObjectPtr<UTexture2D> FloorMaskTex = nullptr;
	static TObjectPtr<UTexture2D> FloorNormalTex = nullptr;
	static TObjectPtr<UTexture2D> FloorRoughnessTex = nullptr;

	static TObjectPtr<UTexture2D> RainGlassMaskTex = nullptr;
	static TObjectPtr<UTexture2D> RainGlassNormalTex = nullptr;
	static TObjectPtr<UTexture2D> RainGlassRoughnessTex = nullptr;

	static TObjectPtr<UTexture2D> TerminalScreenMaskTex0 = nullptr;
	static TObjectPtr<UTexture2D> TerminalScreenMaskTex1 = nullptr;
	static TObjectPtr<UTexture2D> TerminalScreenNormalTex = nullptr;
	static TObjectPtr<UTexture2D> TerminalScreenRoughnessTex = nullptr;

	static TObjectPtr<UTexture2D> ZeroBodyMaskTex = nullptr;
	static TObjectPtr<UTexture2D> FullSubsurfaceMaskTex = nullptr;

	static float SeamlessHashNoise(float U, float V, int32 PeriodX, int32 PeriodY)
	{
		U = FMath::Fmod(FMath::Fmod(U, 1.0f) + 1.0f, 1.0f);
		V = FMath::Fmod(FMath::Fmod(V, 1.0f) + 1.0f, 1.0f);

		const float X = U * PeriodX;
		const float Y = V * PeriodY;

		const int32 X0 = (FMath::FloorToInt(X) % PeriodX + PeriodX) % PeriodX;
		const int32 Y0 = (FMath::FloorToInt(Y) % PeriodY + PeriodY) % PeriodY;
		const int32 X1 = (X0 + 1) % PeriodX;
		const int32 Y1 = (Y0 + 1) % PeriodY;

		const float Fx = X - FMath::FloorToFloat(X);
		const float Fy = Y - FMath::FloorToFloat(Y);
		const float Sx = Fx * Fx * (3.0f - 2.0f * Fx);
		const float Sy = Fy * Fy * (3.0f - 2.0f * Fy);

		auto Hash2D = [](int32 Ix, int32 Iy) -> float
		{
			uint32 H = static_cast<uint32>(Ix * 374761393 + Iy * 668265263);
			H = (H ^ (H >> 13)) * 1274126177;
			return static_cast<float>(H & 0x7FFFFFFF) / 2147483647.0f;
		};

		const float N00 = Hash2D(X0, Y0);
		const float N10 = Hash2D(X1, Y0);
		const float N01 = Hash2D(X0, Y1);
		const float N11 = Hash2D(X1, Y1);

		return FMath::Lerp(FMath::Lerp(N00, N10, Sx), FMath::Lerp(N01, N11, Sx), Sy);
	}
}

AFPSMenuHangarStage::AFPSMenuHangarStage()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> FloorFinder(TEXT("/Game/Environment/Hangar/SM_Hangar_Floor.SM_Hangar_Floor"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> SteelMatFinder(TEXT("/Game/Characters/BaseMale/Materials/M_AK47_Steel.M_AK47_Steel"));

	UStaticMesh* CubeMesh = CubeFinder.Succeeded() ? CubeFinder.Object : nullptr;
	UStaticMesh* CylinderMesh = CylinderFinder.Succeeded() ? CylinderFinder.Object : nullptr;
	UStaticMesh* SphereMesh = SphereFinder.Succeeded() ? SphereFinder.Object : nullptr;
	UStaticMesh* FloorMesh = FloorFinder.Succeeded() ? FloorFinder.Object.Get() : CubeMesh;
	UMaterialInterface* SteelMat = SteelMatFinder.Succeeded() ? SteelMatFinder.Object.Get() : nullptr;
	UMaterialInterface* BaseMat = SteelMat;

	// Build all 5 architectural depth layers + RainFX + Three-Tone Lighting Rig
	ConstructLayer1FloorAndPlinth(FloorMesh, CubeMesh, CylinderMesh, SteelMat);
	ConstructLayer2LeftStormWindows(CubeMesh, CylinderMesh, SteelMat);
	ConstructLayer3RightCommandConsoles(CubeMesh, CylinderMesh, SteelMat);
	ConstructLayer4OverheadGantry(CubeMesh, CylinderMesh, SteelMat);
	ConstructLayer5DeepBackgroundBulkhead(CubeMesh, CylinderMesh, SteelMat);
	ConstructRainburstOpticsAndParticles(CubeMesh, SphereMesh, BaseMat);
	ConstructThreeToneLightingRig();
}

// =========================================================================================
// LAYER 1: WET INDUSTRIAL GRATE & CONCRETE FLOOR + TURNTABLE PLINTH
// =========================================================================================
void AFPSMenuHangarStage::ConstructLayer1FloorAndPlinth(UStaticMesh* FloorMesh, UStaticMesh* CubeMesh, UStaticMesh* CylinderMesh, UMaterialInterface* SteelMat)
{
	// 1. Wide Wet Industrial Floor Plane (Z top surface = -0.2cm directly under Operator's boots)
	WetFloorPlane = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WetFloorPlane"));
	WetFloorPlane->SetupAttachment(RootComponent);
	WetFloorPlane->SetMobility(EComponentMobility::Movable);
	if (FloorMesh) WetFloorPlane->SetStaticMesh(FloorMesh);
	if (SteelMat) WetFloorPlane->SetMaterial(0, SteelMat);
	WetFloorPlane->SetRelativeLocation(FVector(0.0f, 0.0f, -0.2f));
	WetFloorPlane->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
	WetFloorPlane->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WetFloorPlane->SetCastShadow(true);

	auto AddFloorGrate = [this, CubeMesh, SteelMat](const FString& Name, const FVector& Loc, const FRotator& Rot, const FVector& Scale)
	{
		UStaticMeshComponent* Comp = CreateDefaultSubobject<UStaticMeshComponent>(*Name);
		Comp->SetupAttachment(RootComponent);
		Comp->SetMobility(EComponentMobility::Movable);
		if (CubeMesh) Comp->SetStaticMesh(CubeMesh);
		if (SteelMat) Comp->SetMaterial(0, SteelMat);
		Comp->SetRelativeLocation(Loc);
		Comp->SetRelativeRotation(Rot);
		Comp->SetRelativeScale3D(Scale);
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Comp->SetCastShadow(false);
		FloorGrateStrips.Add(Comp);
	};

	// 2. Physical Foreground Brushed-Steel Grating Ribs (catches broken specular streaks from amber/cyan lights)
	for (int32 Row = 0; Row < 14; ++Row)
	{
		const float XPos = 95.0f - Row * 62.0f;
		// Transverse steel floor seam rib
		AddFloorGrate(
			FString::Printf(TEXT("FloorGrateSeam_%d"), Row),
			FVector(XPos, 0.0f, 0.05f),
			FRotator::ZeroRotator,
			FVector(0.05f, 8.5f, 0.006f)
		);
	}

	// Longitudinal drainage channel curbs flanking the central walkway (leaves wet puddle bays on +Y left and -Y right)
	AddFloorGrate(TEXT("FloorDrainCurb_Left"), FVector(-350.0f, 74.0f, 0.25f), FRotator::ZeroRotator, FVector(12.0f, 0.08f, 0.010f));
	AddFloorGrate(TEXT("FloorDrainCurb_Right"), FVector(-350.0f, -74.0f, 0.25f), FRotator::ZeroRotator, FVector(12.0f, 0.08f, 0.010f));
	AddFloorGrate(TEXT("FloorDrainCurb_FarLeft"), FVector(-350.0f, 195.0f, 0.25f), FRotator::ZeroRotator, FVector(12.0f, 0.10f, 0.012f));
	AddFloorGrate(TEXT("FloorDrainCurb_FarRight"), FVector(-350.0f, -195.0f, 0.25f), FRotator::ZeroRotator, FVector(12.0f, 0.10f, 0.012f));

	// 3. Raised Low-Profile Tactical Turntable Plinth (Handled by AFPSCharacterPreviewActor::PedestalMesh)
	PlinthOuterRing = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlinthOuterRing"));
	PlinthOuterRing->SetupAttachment(RootComponent);
	PlinthOuterRing->SetMobility(EComponentMobility::Movable);
	if (CylinderMesh) PlinthOuterRing->SetStaticMesh(CylinderMesh);
	if (SteelMat) PlinthOuterRing->SetMaterial(0, SteelMat);
	PlinthOuterRing->SetRelativeLocation(FVector(0.0f, 0.0f, -1.2f));
	PlinthOuterRing->SetRelativeScale3D(FVector(1.12f, 1.12f, 0.032f));
	PlinthOuterRing->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PlinthOuterRing->SetCastShadow(false);
	PlinthOuterRing->SetVisibility(false);

	PlinthBevelRing = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlinthBevelRing"));
	PlinthBevelRing->SetupAttachment(RootComponent);
	PlinthBevelRing->SetMobility(EComponentMobility::Movable);
	if (CylinderMesh) PlinthBevelRing->SetStaticMesh(CylinderMesh);
	if (SteelMat) PlinthBevelRing->SetMaterial(0, SteelMat);
	PlinthBevelRing->SetRelativeLocation(FVector(0.0f, 0.0f, -0.6f));
	PlinthBevelRing->SetRelativeScale3D(FVector(0.98f, 0.98f, 0.036f));
	PlinthBevelRing->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PlinthBevelRing->SetCastShadow(false);
	PlinthBevelRing->SetVisibility(false);

	// 12 Cyan Emissive Micro-Pips embedded around the Turntable Plinth perimeter (radius 136cm)
	const int32 NumPips = 12;
	const float PipRadius = 136.0f;
	for (int32 i = 0; i < NumPips; ++i)
	{
		const float AngleRad = (static_cast<float>(i) / static_cast<float>(NumPips)) * 2.0f * UE_PI;
		const float PipX = FMath::Cos(AngleRad) * PipRadius;
		const float PipY = FMath::Sin(AngleRad) * PipRadius;
		const float YawDeg = FMath::RadiansToDegrees(AngleRad);

		UStaticMeshComponent* Pip = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("PlinthCyanPip_%d"), i));
		Pip->SetupAttachment(RootComponent);
		Pip->SetMobility(EComponentMobility::Movable);
		if (CubeMesh) Pip->SetStaticMesh(CubeMesh);
		if (SteelMat) Pip->SetMaterial(0, SteelMat);
		Pip->SetRelativeLocation(FVector(PipX, PipY, 0.8f));
		Pip->SetRelativeRotation(FRotator(0.0f, YawDeg, 0.0f));
		Pip->SetRelativeScale3D(FVector(0.025f, 0.065f, 0.012f));
		Pip->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Pip->SetCastShadow(false);
		PlinthCyanPips.Add(Pip);
	}
}

// =========================================================================================
// LAYER 2: LEFT-MIDGROUND ANGLED STORM WINDOWS & STRUCTURAL RIBS (WARM BACKLIGHT ZONE)
// Note: Camera is at (+250, 0, 115) looking toward -X (Yaw=180).
// Therefore Screen Left = +Y (Operator's right shoulder), Screen Right = -Y.
// =========================================================================================
void AFPSMenuHangarStage::ConstructLayer2LeftStormWindows(UStaticMesh* CubeMesh, UStaticMesh* CylinderMesh, UMaterialInterface* SteelMat)
{
	auto AddBeam = [this, CubeMesh, SteelMat](const FString& Name, const FVector& Loc, const FRotator& Rot, const FVector& Scale)
	{
		UStaticMeshComponent* Comp = CreateDefaultSubobject<UStaticMeshComponent>(*Name);
		Comp->SetupAttachment(RootComponent);
		Comp->SetMobility(EComponentMobility::Movable);
		if (CubeMesh) Comp->SetStaticMesh(CubeMesh);
		if (SteelMat) Comp->SetMaterial(0, SteelMat);
		Comp->SetRelativeLocation(Loc);
		Comp->SetRelativeRotation(Rot);
		Comp->SetRelativeScale3D(Scale);
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Comp->SetCastShadow(true);
		LeftStructuralBeams.Add(Comp);
	};

	auto AddWindowPane = [this, CubeMesh, SteelMat](const FString& Name, const FVector& Loc, const FRotator& Rot, const FVector& Scale)
	{
		UStaticMeshComponent* Comp = CreateDefaultSubobject<UStaticMeshComponent>(*Name);
		Comp->SetupAttachment(RootComponent);
		Comp->SetMobility(EComponentMobility::Movable);
		if (CubeMesh) Comp->SetStaticMesh(CubeMesh);
		if (SteelMat) Comp->SetMaterial(0, SteelMat);
		Comp->SetRelativeLocation(Loc);
		Comp->SetRelativeRotation(Rot);
		Comp->SetRelativeScale3D(Scale);
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		// Window glass must NOT block the exterior warm sodium-amber spotlights behind the glass!
		Comp->SetCastShadow(false);
		LeftRainWindowPanes.Add(Comp);
	};

	// 1. Slanted Heavy Steel I-Beam Columns & Ceiling Skylight Ribs (tilted ~14 deg inward, marching back along -X/+Y)
	// Angled wall plane runs from (X=-210, Y=+135) to (X=-890, Y=+345) -> Yaw ~ -17 degrees
	const float WallYaw = -16.5f;
	const FRotator SlantedRibRot(0.0f, WallYaw, -13.5f);

	for (int32 i = 0; i < 6; ++i)
	{
		const float Alpha = static_cast<float>(i) / 5.0f;
		const float BeamX = FMath::Lerp(-215.0f, -840.0f, Alpha);
		const float BeamY = FMath::Lerp(132.0f, 325.0f, Alpha);

		// Main slanted vertical I-beam pillar
		AddBeam(
			FString::Printf(TEXT("LeftWindowPillar_%d"), i),
			FVector(BeamX, BeamY, 130.0f),
			SlantedRibRot,
			FVector(0.16f, 0.22f, 2.95f)
		);

		// Upper angled skylight rafter extending inward toward top-center gantry
		AddBeam(
			FString::Printf(TEXT("LeftSkylightRafter_%d"), i),
			FVector(BeamX - 10.0f, BeamY - 48.0f, 245.0f),
			FRotator(0.0f, WallYaw, -48.0f),
			FVector(0.12f, 0.16f, 1.65f)
		);
	}

	// Horizontal window transoms/mullions across the slanted left bay wall
	AddBeam(TEXT("LeftWindowTransom_Low"), FVector(-520.0f, 232.0f, 38.0f), FRotator(0.0f, WallYaw, 0.0f), FVector(7.2f, 0.16f, 0.12f));
	AddBeam(TEXT("LeftWindowTransom_Mid1"), FVector(-505.0f, 215.0f, 108.0f), FRotator(0.0f, WallYaw, 0.0f), FVector(7.2f, 0.14f, 0.09f));
	AddBeam(TEXT("LeftWindowTransom_Mid2"), FVector(-490.0f, 198.0f, 178.0f), FRotator(0.0f, WallYaw, 0.0f), FVector(7.2f, 0.14f, 0.09f));
	AddBeam(TEXT("LeftWindowTransom_Top"), FVector(-475.0f, 180.0f, 238.0f), FRotator(0.0f, WallYaw, 0.0f), FVector(7.2f, 0.18f, 0.14f));

	// 2. Rain-Streaked Bay Window Glass Panes (M_HangarWindow_Rain / M_RainGlass_Refractive)
	for (int32 BayIdx = 0; BayIdx < 5; ++BayIdx)
	{
		const float Alpha = (static_cast<float>(BayIdx) + 0.5f) / 5.0f;
		const float PaneX = FMath::Lerp(-215.0f, -840.0f, Alpha);
		const float PaneY = FMath::Lerp(138.0f, 330.0f, Alpha);

		// Main slanted bay window pane
		AddWindowPane(
			FString::Printf(TEXT("LeftRainWindowPane_%d"), BayIdx),
			FVector(PaneX, PaneY, 136.0f),
			SlantedRibRot,
			FVector(1.26f, 0.015f, 2.15f)
		);

		// Upper slanted skylight glass pane (catches bright orange overhead/diagonal glow)
		AddWindowPane(
			FString::Printf(TEXT("LeftSkylightPane_%d"), BayIdx),
			FVector(PaneX - 8.0f, PaneY - 44.0f, 244.0f),
			FRotator(0.0f, WallYaw, -48.0f),
			FVector(1.26f, 0.015f, 1.35f)
		);
	}

	// 3. Exterior Silhouetted Industrial Structures (Behind the slanted glass panes, bathed in fiery amber glow)
	auto AddSilhouette = [this, SteelMat](const FString& Name, UStaticMesh* Mesh, const FVector& Loc, const FRotator& Rot, const FVector& Scale)
	{
		UStaticMeshComponent* Comp = CreateDefaultSubobject<UStaticMeshComponent>(*Name);
		Comp->SetupAttachment(RootComponent);
		Comp->SetMobility(EComponentMobility::Movable);
		if (Mesh) Comp->SetStaticMesh(Mesh);
		if (SteelMat) Comp->SetMaterial(0, SteelMat);
		Comp->SetRelativeLocation(Loc);
		Comp->SetRelativeRotation(Rot);
		Comp->SetRelativeScale3D(Scale);
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Comp->SetCastShadow(true);
		ExteriorSilhouetteMeshes.Add(Comp);
	};

	// Silhouetted industrial machinery / robotics rig outside Bay 1-2 (visible right behind the glass on Screen Left in menu-prop.jpeg!)
	AddSilhouette(TEXT("ExtSil_MechBase"), CubeMesh, FVector(-460.0f, 255.0f, 55.0f), FRotator(0.0f, 18.0f, 0.0f), FVector(1.20f, 0.85f, 1.10f));
	AddSilhouette(TEXT("ExtSil_MechTower"), CylinderMesh, FVector(-445.0f, 240.0f, 118.0f), FRotator(8.0f, 0.0f, -12.0f), FVector(0.45f, 0.45f, 1.35f));
	AddSilhouette(TEXT("ExtSil_MechArm"), CubeMesh, FVector(-425.0f, 222.0f, 145.0f), FRotator(22.0f, -15.0f, 0.0f), FVector(0.25f, 0.90f, 0.22f));
	AddSilhouette(TEXT("ExtSil_ContainerStack"), CubeMesh, FVector(-640.0f, 330.0f, 90.0f), FRotator(0.0f, -12.0f, 0.0f), FVector(2.80f, 1.20f, 1.80f));
	AddSilhouette(TEXT("ExtSil_DerrickLegA"), CubeMesh, FVector(-580.0f, 295.0f, 175.0f), FRotator(0.0f, 0.0f, 9.0f), FVector(0.22f, 0.22f, 3.40f));
	AddSilhouette(TEXT("ExtSil_DerrickLegB"), CubeMesh, FVector(-690.0f, 335.0f, 175.0f), FRotator(0.0f, 0.0f, -9.0f), FVector(0.22f, 0.22f, 3.40f));
	AddSilhouette(TEXT("ExtSil_RefineryPipe"), CylinderMesh, FVector(-530.0f, 285.0f, 155.0f), FRotator(90.0f, WallYaw, 0.0f), FVector(0.32f, 0.32f, 4.50f));

	// Exterior Fiery Sodium-Vapor Backdrop Plane (reflects intense #FF7A1A glow behind the silhouettes & window panes)
	ExteriorFireBackdrop = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ExteriorFireBackdrop"));
	ExteriorFireBackdrop->SetupAttachment(RootComponent);
	ExteriorFireBackdrop->SetMobility(EComponentMobility::Movable);
	if (CubeMesh) ExteriorFireBackdrop->SetStaticMesh(CubeMesh);
	if (SteelMat) ExteriorFireBackdrop->SetMaterial(0, SteelMat);
	ExteriorFireBackdrop->SetRelativeLocation(FVector(-690.0f, 395.0f, 165.0f));
	ExteriorFireBackdrop->SetRelativeRotation(FRotator(0.0f, WallYaw, -12.0f));
	ExteriorFireBackdrop->SetRelativeScale3D(FVector(8.5f, 0.08f, 3.8f));
	ExteriorFireBackdrop->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ExteriorFireBackdrop->SetCastShadow(false);

	// 4. Left-Midground Interior Floor Generator Crate Prop (visible in menu-prop.jpeg at X=-310, Y=+90 with blue/cyan LED pip)
	AddBeam(TEXT("LeftMidgroundGeneratorCrate"), FVector(-315.0f, 94.0f, 27.0f), FRotator(0.0f, 12.0f, 0.0f), FVector(0.88f, 0.68f, 0.54f));
	AddBeam(TEXT("LeftMidgroundGeneratorTopLid"), FVector(-315.0f, 94.0f, 56.0f), FRotator(0.0f, 12.0f, 0.0f), FVector(0.92f, 0.72f, 0.06f));
}

// =========================================================================================
// LAYER 3: RIGHT-MIDGROUND TACTICAL COMMAND CONSOLES (THE CYAN TECH ZONE)
// Framed cleanly in the gap between Operator's left arm (Y ~ -35) and Right UI Panel (Y ~ -200).
// =========================================================================================
void AFPSMenuHangarStage::ConstructLayer3RightCommandConsoles(UStaticMesh* CubeMesh, UStaticMesh* CylinderMesh, UMaterialInterface* SteelMat)
{
	auto AddRackPart = [this, CubeMesh, SteelMat](const FString& Name, const FVector& Loc, const FRotator& Rot, const FVector& Scale)
	{
		UStaticMeshComponent* Comp = CreateDefaultSubobject<UStaticMeshComponent>(*Name);
		Comp->SetupAttachment(RootComponent);
		Comp->SetMobility(EComponentMobility::Movable);
		if (CubeMesh) Comp->SetStaticMesh(CubeMesh);
		if (SteelMat) Comp->SetMaterial(0, SteelMat);
		Comp->SetRelativeLocation(Loc);
		Comp->SetRelativeRotation(Rot);
		Comp->SetRelativeScale3D(Scale);
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Comp->SetCastShadow(true);
		ConsoleRackFrames.Add(Comp);
	};

	auto AddMonitorScreen = [this, CubeMesh, SteelMat](const FString& Name, const FVector& Loc, const FRotator& Rot, const FVector& Scale)
	{
		UStaticMeshComponent* Comp = CreateDefaultSubobject<UStaticMeshComponent>(*Name);
		Comp->SetupAttachment(RootComponent);
		Comp->SetMobility(EComponentMobility::Movable);
		if (CubeMesh) Comp->SetStaticMesh(CubeMesh);
		if (SteelMat) Comp->SetMaterial(0, SteelMat);
		Comp->SetRelativeLocation(Loc);
		Comp->SetRelativeRotation(Rot);
		Comp->SetRelativeScale3D(Scale);
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Comp->SetCastShadow(false);
		ConsoleMonitorScreens.Add(Comp);
	};

	const FRotator ConsoleRot(0.0f, 12.0f, 0.0f);

	// 1. Primary Tactical Command Console Bank (X = -385, Y = -115, angled slightly toward camera)
	// Desk base & server cabinet housing
	AddRackPart(TEXT("ConsoleDesk_Main"), FVector(-390.0f, -118.0f, 34.0f), ConsoleRot, FVector(0.75f, 1.55f, 0.68f));
	AddRackPart(TEXT("ConsoleDesk_Ledge"), FVector(-352.0f, -110.0f, 68.0f), ConsoleRot, FVector(0.38f, 1.62f, 0.06f));

	// Upper Monitor Frame Pillars & Bezel Housing
	AddRackPart(TEXT("MonitorBezel_UpperLeft"), FVector(-392.0f, -92.0f, 122.0f), ConsoleRot, FVector(0.08f, 0.56f, 0.42f));
	AddRackPart(TEXT("MonitorBezel_UpperRight"), FVector(-404.0f, -148.0f, 122.0f), ConsoleRot, FVector(0.08f, 0.52f, 0.42f));

	// Upper Left Widescreen Monitor Screen (Radar Ring & Bar Graphs in #00DDF5 Cyan & #0A2633 Teal)
	AddMonitorScreen(TEXT("MonitorScreen_UpperLeft"), FVector(-387.0f, -91.0f, 122.0f), ConsoleRot, FVector(0.02f, 0.51f, 0.37f));

	// Upper Right Widescreen Monitor Screen (Telemetry Matrix & Waveform Grid)
	AddMonitorScreen(TEXT("MonitorScreen_UpperRight"), FVector(-399.0f, -147.0f, 122.0f), ConsoleRot, FVector(0.02f, 0.47f, 0.37f));

	// Lower Dual Desk Monitors (positioned at Z = 84cm right above the desk ledge, matching menu-prop.jpeg)
	AddRackPart(TEXT("MonitorBezel_LowerLeft"), FVector(-372.0f, -76.0f, 83.0f), FRotator(8.0f, 14.0f, 0.0f), FVector(0.06f, 0.32f, 0.22f));
	AddMonitorScreen(TEXT("MonitorScreen_LowerLeft"), FVector(-368.0f, -75.0f, 83.5f), FRotator(8.0f, 14.0f, 0.0f), FVector(0.015f, 0.28f, 0.18f));

	AddRackPart(TEXT("MonitorBezel_LowerRight"), FVector(-380.0f, -116.0f, 83.0f), FRotator(8.0f, 12.0f, 0.0f), FVector(0.06f, 0.34f, 0.22f));
	AddMonitorScreen(TEXT("MonitorScreen_LowerRight"), FVector(-376.0f, -115.0f, 83.5f), FRotator(8.0f, 12.0f, 0.0f), FVector(0.015f, 0.30f, 0.18f));

	// 2. Deep-Right Mainframe Server Rack Towers (X = -510, Y = -185.. -240)
	AddRackPart(TEXT("ServerTower_A"), FVector(-510.0f, -188.0f, 105.0f), FRotator(0.0f, 18.0f, 0.0f), FVector(0.85f, 0.72f, 2.10f));
	AddRackPart(TEXT("ServerTower_B"), FVector(-560.0f, -245.0f, 110.0f), FRotator(0.0f, 18.0f, 0.0f), FVector(0.85f, 0.72f, 2.20f));

	// 3. Glowing Cyan & Amber Server Status LED Strips (including the left-midground crate pip!)
	auto AddLED = [this, CubeMesh, SteelMat](const FString& Name, const FVector& Loc, const FRotator& Rot, const FVector& Scale)
	{
		UStaticMeshComponent* Comp = CreateDefaultSubobject<UStaticMeshComponent>(*Name);
		Comp->SetupAttachment(RootComponent);
		Comp->SetMobility(EComponentMobility::Movable);
		if (CubeMesh) Comp->SetStaticMesh(CubeMesh);
		if (SteelMat) Comp->SetMaterial(0, SteelMat);
		Comp->SetRelativeLocation(Loc);
		Comp->SetRelativeRotation(Rot);
		Comp->SetRelativeScale3D(Scale);
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Comp->SetCastShadow(false);
		ConsoleStatusLEDs.Add(Comp);
	};

	// Left-midground crate glowing cyan status indicator (matches the bright cyan dot at X=-270, Y=+82, Z=26 in menu-prop.jpeg!)
	AddLED(TEXT("LeftCrate_CyanLED"), FVector(-271.0f, 82.0f, 26.0f), FRotator(0.0f, 12.0f, 0.0f), FVector(0.02f, 0.05f, 0.04f));

	// Right console deck glowing status bar strips
	for (int32 LedIdx = 0; LedIdx < 5; ++LedIdx)
	{
		AddLED(
			FString::Printf(TEXT("ServerRack_LED_%d"), LedIdx),
			FVector(-466.0f, -172.0f - LedIdx * 6.0f, 65.0f + LedIdx * 24.0f),
			FRotator(0.0f, 18.0f, 0.0f),
			FVector(0.02f, 0.16f, 0.025f)
		);
	}
}

// =========================================================================================
// LAYER 4: OVERHEAD GANTRY, PIPES & CEILING SKYLIGHTS
// =========================================================================================
void AFPSMenuHangarStage::ConstructLayer4OverheadGantry(UStaticMesh* CubeMesh, UStaticMesh* CylinderMesh, UMaterialInterface* SteelMat)
{
	auto AddGantryPart = [this, SteelMat](const FString& Name, UStaticMesh* Mesh, const FVector& Loc, const FRotator& Rot, const FVector& Scale)
	{
		UStaticMeshComponent* Comp = CreateDefaultSubobject<UStaticMeshComponent>(*Name);
		Comp->SetupAttachment(RootComponent);
		Comp->SetMobility(EComponentMobility::Movable);
		if (Mesh) Comp->SetStaticMesh(Mesh);
		if (SteelMat) Comp->SetMaterial(0, SteelMat);
		Comp->SetRelativeLocation(Loc);
		Comp->SetRelativeRotation(Rot);
		Comp->SetRelativeScale3D(Scale);
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Comp->SetCastShadow(true);
		OverheadGantryMeshes.Add(Comp);
	};

	// Transverse overhead steel I-beam trusses spanning across the upper third of the viewport (Z = 215..310)
	for (int32 TrussIdx = 0; TrussIdx < 5; ++TrussIdx)
	{
		const float TrussX = -210.0f - TrussIdx * 165.0f;
		const float TrussZ = 228.0f + TrussIdx * 14.0f;

		AddGantryPart(
			FString::Printf(TEXT("OverheadTrussBeam_%d"), TrussIdx),
			CubeMesh,
			FVector(TrussX, -20.0f, TrussZ),
			FRotator::ZeroRotator,
			FVector(0.22f, 7.50f, 0.24f)
		);
	}

	// Longitudinal overhead gantry crane rails & suspended cable/ventilation conduits
	AddGantryPart(TEXT("OverheadGantryRail_Center"), CubeMesh, FVector(-520.0f, -15.0f, 248.0f), FRotator::ZeroRotator, FVector(9.5f, 0.18f, 0.20f));
	AddGantryPart(TEXT("OverheadGantryRail_Right"), CubeMesh, FVector(-520.0f, -145.0f, 232.0f), FRotator::ZeroRotator, FVector(9.5f, 0.22f, 0.26f));

	// Cylindrical industrial pipes & conduits running along the upper-right ceiling
	AddGantryPart(TEXT("OverheadPipe_A"), CylinderMesh, FVector(-480.0f, -65.0f, 224.0f), FRotator(90.0f, 0.0f, 0.0f), FVector(0.14f, 0.14f, 8.5f));
	AddGantryPart(TEXT("OverheadPipe_B"), CylinderMesh, FVector(-480.0f, -92.0f, 232.0f), FRotator(90.0f, 0.0f, 0.0f), FVector(0.10f, 0.10f, 8.5f));

	// Dark Ceiling Closure Roof Slab (keeps top status bar 100% high-contrast & legible)
	AddGantryPart(TEXT("HangarCeilingRoofSlab"), CubeMesh, FVector(-600.0f, 0.0f, 345.0f), FRotator::ZeroRotator, FVector(18.0f, 18.0f, 0.40f));
}

// =========================================================================================
// LAYER 5: DEEP BACKGROUND WALL & ATMOSPHERIC TERMINUS
// =========================================================================================
void AFPSMenuHangarStage::ConstructLayer5DeepBackgroundBulkhead(UStaticMesh* CubeMesh, UStaticMesh* CylinderMesh, UMaterialInterface* SteelMat)
{
	auto AddBulkhead = [this, SteelMat](const FString& Name, UStaticMesh* Mesh, const FVector& Loc, const FRotator& Rot, const FVector& Scale)
	{
		UStaticMeshComponent* Comp = CreateDefaultSubobject<UStaticMeshComponent>(*Name);
		Comp->SetupAttachment(RootComponent);
		Comp->SetMobility(EComponentMobility::Movable);
		if (Mesh) Comp->SetStaticMesh(Mesh);
		if (SteelMat) Comp->SetMaterial(0, SteelMat);
		Comp->SetRelativeLocation(Loc);
		Comp->SetRelativeRotation(Rot);
		Comp->SetRelativeScale3D(Scale);
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Comp->SetCastShadow(true);
		DeepBulkheadMeshes.Add(Comp);
	};

	// Rear Hangar Bulkhead Wall at X = -720cm (7.2 meters behind Operator) — provides authoritative visible architecture
	AddBulkhead(TEXT("RearBulkheadWall"), CubeMesh, FVector(-720.0f, 0.0f, 160.0f), FRotator::ZeroRotator, FVector(0.50f, 18.0f, 4.20f));

	// Industrial Blast Door Frame & Ribbed Steel Pylons along the rear bulkhead
	AddBulkhead(TEXT("RearBlastDoorFrame_L"), CubeMesh, FVector(-680.0f, 160.0f, 130.0f), FRotator::ZeroRotator, FVector(0.45f, 0.55f, 2.80f));
	AddBulkhead(TEXT("RearBlastDoorFrame_R"), CubeMesh, FVector(-680.0f, -160.0f, 130.0f), FRotator::ZeroRotator, FVector(0.45f, 0.55f, 2.80f));
	AddBulkhead(TEXT("RearBlastDoorHeader"), CubeMesh, FVector(-680.0f, 0.0f, 255.0f), FRotator::ZeroRotator, FVector(0.50f, 3.80f, 0.45f));

	for (int32 Rib = -3; Rib <= 3; ++Rib)
	{
		AddBulkhead(
			FString::Printf(TEXT("RearBulkheadRib_%d"), Rib + 3),
			CubeMesh,
			FVector(-690.0f, Rib * 110.0f, 150.0f),
			FRotator::ZeroRotator,
			FVector(0.25f, 0.22f, 3.40f)
		);
	}

	// Right-side enclosing industrial wall behind the command consoles & Right UI Panel (Y = -360)
	AddBulkhead(TEXT("RightEnclosureWall"), CubeMesh, FVector(-450.0f, -360.0f, 160.0f), FRotator::ZeroRotator, FVector(15.0f, 0.50f, 4.20f));
	// Far Left enclosing wall behind the exterior backdrop (Y = +480)
	AddBulkhead(TEXT("LeftExteriorEnclosureWall"), CubeMesh, FVector(-450.0f, 480.0f, 160.0f), FRotator::ZeroRotator, FVector(15.0f, 0.50f, 4.20f));
}

// =========================================================================================
// SECTION 5: "RAINBURST ON GLASS" — DIEGETIC WET OPTICS & FOREFRONT RAIN PARTICLES
// Sparse vertical rain-drip chains & streaks in the left-foreground hangar bay (+Y),
// illuminated by the warm amber backlight while keeping the Operator's face 100% clean.
// =========================================================================================
void AFPSMenuHangarStage::ConstructRainburstOpticsAndParticles(UStaticMesh* CubeMesh, UStaticMesh* SphereMesh, UMaterialInterface* BaseMat)
{
	// 1. Vertical Rain-Streak Ribbons in the Left-Foreground Hangar Bay (X = +35..-280, Y = +46..+155)
	const int32 NumStreaks = 18;
	for (int32 i = 0; i < NumStreaks; ++i)
	{
		const float Frac = static_cast<float>(i) / static_cast<float>(NumStreaks);
		// Deterministic pseudo-random distribution in the left foreground bay (+Y)
		const float StreakX = FMath::Lerp(45.0f, -265.0f, FMath::Fmod(Frac * 3.7f, 1.0f));
		const float StreakY = FMath::Lerp(44.0f, 148.0f, Frac);
		const float StreakZ = FMath::Lerp(15.0f, 195.0f, FMath::Fmod(Frac * 5.3f, 1.0f));

		UStaticMeshComponent* Streak = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("ForefrontRainStreak_%d"), i));
		Streak->SetupAttachment(RootComponent);
		Streak->SetMobility(EComponentMobility::Movable);
		if (CubeMesh) Streak->SetStaticMesh(CubeMesh);
		if (BaseMat) Streak->SetMaterial(0, BaseMat);
		Streak->SetRelativeLocation(FVector(StreakX, StreakY, StreakZ));
		Streak->SetRelativeScale3D(FVector(0.003f, 0.003f, 0.22f + 0.14f * FMath::Fmod(Frac * 7.1f, 1.0f)));
		Streak->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Streak->SetCastShadow(false);
		ForefrontRainStreaks.Add(Streak);

		FRainDropTrack Track;
		Track.BaseLocation = FVector(StreakX, StreakY, StreakZ);
		Track.FallSpeed = 95.0f + 55.0f * FMath::Fmod(Frac * 4.3f, 1.0f);
		Track.MinZ = 2.0f;
		Track.MaxZ = 215.0f;
		Track.PhaseOffset = Frac * 6.28f;
		RainStreakTracks.Add(Track);
	}

	// 2. Glistening Falling Water Droplet Beads along the Left Window Drip-Chains (matches menu-prop.jpeg!)
	const int32 NumDroplets = 24;
	for (int32 i = 0; i < NumDroplets; ++i)
	{
		const float Frac = static_cast<float>(i) / static_cast<float>(NumDroplets);
		// Clustered along 5 vertical drip chains in the left foreground (Y = 48..135)
		const int32 ChainIdx = i % 5;
		const float ChainY = 48.0f + ChainIdx * 19.5f;
		const float ChainX = 35.0f - ChainIdx * 42.0f;
		const float DropZ = FMath::Lerp(8.0f, 205.0f, Frac);

		UStaticMeshComponent* Drop = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("ForefrontRainDroplet_%d"), i));
		Drop->SetupAttachment(RootComponent);
		Drop->SetMobility(EComponentMobility::Movable);
		if (SphereMesh) Drop->SetStaticMesh(SphereMesh);
		if (BaseMat) Drop->SetMaterial(0, BaseMat);
		Drop->SetRelativeLocation(FVector(ChainX, ChainY, DropZ));
		Drop->SetRelativeScale3D(FVector(0.008f, 0.008f, 0.024f));
		Drop->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Drop->SetCastShadow(false);
		ForefrontRainDroplets.Add(Drop);

		FRainDropTrack Track;
		Track.BaseLocation = FVector(ChainX, ChainY, DropZ);
		Track.FallSpeed = 68.0f + (ChainIdx * 14.0f);
		Track.MinZ = 1.0f;
		Track.MaxZ = 212.0f;
		Track.PhaseOffset = Frac * 6.28f;
		RainDropletTracks.Add(Track);
	}
}

// =========================================================================================
// SECTION 4 & 6: CINEMATIC THREE-TONE LIGHTING RIG + EXPOSURE/DOF POST-PROCESS LOCK
// =========================================================================================
void AFPSMenuHangarStage::ConstructThreeToneLightingRig()
{
	// -------------------------------------------------------------------------
	// 1. KEY & RIM WARM INDUSTRIAL BACKLIGHT (Left Rear, +Y / -X)
	// Saturated Sodium Amber / Fiery Orange (#FF7A1A to #FFA033)
	// -------------------------------------------------------------------------
	WarmWindowKeySpotLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("WarmWindowKeySpotLight"));
	WarmWindowKeySpotLight->SetupAttachment(RootComponent);
	WarmWindowKeySpotLight->SetMobility(EComponentMobility::Movable);
	WarmWindowKeySpotLight->SetRelativeLocation(FVector(-420.0f, 245.0f, 195.0f));
	WarmWindowKeySpotLight->SetRelativeRotation(FRotator(-18.0f, -32.0f, 0.0f));
	WarmWindowKeySpotLight->SetIntensity(4500.0f);
	WarmWindowKeySpotLight->SetLightColor(FLinearColor(1.0f, 0.52f, 0.16f)); // Saturated Fiery Sodium Amber (#FF7A1A)
	WarmWindowKeySpotLight->SetAttenuationRadius(2800.0f);
	WarmWindowKeySpotLight->SetInnerConeAngle(22.0f);
	WarmWindowKeySpotLight->SetOuterConeAngle(52.0f);
	WarmWindowKeySpotLight->SetVolumetricScatteringIntensity(1.5f);
	WarmWindowKeySpotLight->SetCastShadows(true);

	WarmOperatorRimSpotLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("WarmOperatorRimSpotLight"));
	WarmOperatorRimSpotLight->SetupAttachment(RootComponent);
	WarmOperatorRimSpotLight->SetMobility(EComponentMobility::Movable);
	// Positioned at Left-Rear (+Y, -X) pointing at Operator's right helmet/shoulder/leg and wet floor
	WarmOperatorRimSpotLight->SetRelativeLocation(FVector(-145.0f, 165.0f, 168.0f));
	WarmOperatorRimSpotLight->SetRelativeRotation(FRotator(-22.0f, -46.0f, 0.0f));
	WarmOperatorRimSpotLight->SetIntensity(2200.0f);
	WarmOperatorRimSpotLight->SetLightColor(FLinearColor(1.0f, 0.56f, 0.14f)); // Warm Amber-Orange (#FFA033)
	WarmOperatorRimSpotLight->SetAttenuationRadius(850.0f);
	WarmOperatorRimSpotLight->SetInnerConeAngle(26.0f);
	WarmOperatorRimSpotLight->SetOuterConeAngle(58.0f);
	WarmOperatorRimSpotLight->SetCastShadows(false);

	WarmWindowRectLight = CreateDefaultSubobject<URectLightComponent>(TEXT("WarmWindowRectLight"));
	WarmWindowRectLight->SetupAttachment(RootComponent);
	WarmWindowRectLight->SetMobility(EComponentMobility::Movable);
	// Area light along the slanted left bay windows to cast a wide orange specular ribbon across the left wet floor
	WarmWindowRectLight->SetRelativeLocation(FVector(-320.0f, 185.0f, 125.0f));
	WarmWindowRectLight->SetRelativeRotation(FRotator(-16.0f, -45.0f, 0.0f));
	WarmWindowRectLight->SetIntensity(1800.0f);
	WarmWindowRectLight->SetLightColor(FLinearColor(1.0f, 0.48f, 0.12f));
	WarmWindowRectLight->SetSourceWidth(320.0f);
	WarmWindowRectLight->SetSourceHeight(200.0f);
	WarmWindowRectLight->SetAttenuationRadius(2200.0f);
	WarmWindowRectLight->SetCastShadows(false);

	ExteriorAmberGlowLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("ExteriorAmberGlowLight"));
	ExteriorAmberGlowLight->SetupAttachment(RootComponent);
	ExteriorAmberGlowLight->SetMobility(EComponentMobility::Movable);
	ExteriorAmberGlowLight->SetRelativeLocation(FVector(-540.0f, 310.0f, 140.0f));
	ExteriorAmberGlowLight->SetIntensity(2500.0f);
	ExteriorAmberGlowLight->SetLightColor(FLinearColor(1.0f, 0.42f, 0.06f));
	ExteriorAmberGlowLight->SetAttenuationRadius(650.0f);
	ExteriorAmberGlowLight->SetCastShadows(false);

	// -------------------------------------------------------------------------
	// 2. COOL TACTICAL AMBIENT & OVERHEAD FILL (Center / Top)
	// Deep Steel Blue / Cold Cyan (#1B3B52 to #2A5C7A)
	// -------------------------------------------------------------------------
	CoolOverheadRectLight = CreateDefaultSubobject<URectLightComponent>(TEXT("CoolOverheadRectLight"));
	CoolOverheadRectLight->SetupAttachment(RootComponent);
	CoolOverheadRectLight->SetMobility(EComponentMobility::Movable);
	CoolOverheadRectLight->SetRelativeLocation(FVector(-260.0f, -20.0f, 215.0f));
	CoolOverheadRectLight->SetRelativeRotation(FRotator(-75.0f, 0.0f, 0.0f));
	CoolOverheadRectLight->SetIntensity(1600.0f);
	CoolOverheadRectLight->SetLightColor(FLinearColor(0.20f, 0.38f, 0.54f)); // Deep Steel Blue (#2A5C7A)
	CoolOverheadRectLight->SetSourceWidth(400.0f);
	CoolOverheadRectLight->SetSourceHeight(300.0f);
	CoolOverheadRectLight->SetAttenuationRadius(2600.0f);
	CoolOverheadRectLight->SetCastShadows(false);

	HangarSkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("HangarSkyLight"));
	HangarSkyLight->SetupAttachment(RootComponent);
	HangarSkyLight->SetMobility(EComponentMobility::Movable);
	HangarSkyLight->SetIntensity(0.8f);
	HangarSkyLight->SetLightColor(FLinearColor(0.35f, 0.48f, 0.62f)); // Cool Steel Ambient
	HangarSkyLight->bLowerHemisphereIsBlack = false;
	HangarSkyLight->LowerHemisphereColor = FLinearColor(0.15f, 0.22f, 0.28f, 1.0f);
	HangarSkyLight->bRealTimeCapture = false;

	// -------------------------------------------------------------------------
	// 3. CYAN EQUIPMENT & RIM COUNTER-LIGHT (Right Rear, -Y / -X)
	// Noble Fermi Cyan (#00DDF5)
	// -------------------------------------------------------------------------
	CyanConsoleBounceLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("CyanConsoleBounceLight"));
	CyanConsoleBounceLight->SetupAttachment(RootComponent);
	CyanConsoleBounceLight->SetMobility(EComponentMobility::Movable);
	CyanConsoleBounceLight->SetRelativeLocation(FVector(-330.0f, -115.0f, 95.0f));
	CyanConsoleBounceLight->SetIntensity(2200.0f);
	CyanConsoleBounceLight->SetLightColor(FLinearColor(0.0f, 0.867f, 0.961f)); // Noble Fermi Cyan (#00DDF5)
	CyanConsoleBounceLight->SetAttenuationRadius(2000.0f);
	CyanConsoleBounceLight->SetCastShadows(false);

	CyanOperatorRimSpotLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("CyanOperatorRimSpotLight"));
	CyanOperatorRimSpotLight->SetupAttachment(RootComponent);
	CyanOperatorRimSpotLight->SetMobility(EComponentMobility::Movable);
	// Positioned behind Operator's left side (-Y) casting cool turquoise rim on left arm/vest & right floor puddles
	CyanOperatorRimSpotLight->SetRelativeLocation(FVector(-150.0f, -135.0f, 148.0f));
	CyanOperatorRimSpotLight->SetRelativeRotation(FRotator(-18.0f, 44.0f, 0.0f));
	CyanOperatorRimSpotLight->SetIntensity(2400.0f);
	CyanOperatorRimSpotLight->SetLightColor(FLinearColor(0.0f, 0.867f, 0.961f)); // #00DDF5
	CyanOperatorRimSpotLight->SetAttenuationRadius(720.0f);
	CyanOperatorRimSpotLight->SetInnerConeAngle(25.0f);
	CyanOperatorRimSpotLight->SetOuterConeAngle(56.0f);
	CyanOperatorRimSpotLight->SetCastShadows(false);

	CyanScreenIllumRectLight = CreateDefaultSubobject<URectLightComponent>(TEXT("CyanScreenIllumRectLight"));
	CyanScreenIllumRectLight->SetupAttachment(RootComponent);
	CyanScreenIllumRectLight->SetMobility(EComponentMobility::Movable);
	// Placed 22cm in front of the right-side monitors pointing directly AT the monitor screens to boost #00DDF5 pixels to 3.5x HDR
	CyanScreenIllumRectLight->SetRelativeLocation(FVector(-362.0f, -112.0f, 108.0f));
	CyanScreenIllumRectLight->SetRelativeRotation(FRotator(0.0f, -168.0f, 0.0f));
	CyanScreenIllumRectLight->SetIntensity(1200.0f);
	CyanScreenIllumRectLight->SetLightColor(FLinearColor(0.0f, 0.92f, 1.0f));
	CyanScreenIllumRectLight->SetSourceWidth(115.0f);
	CyanScreenIllumRectLight->SetSourceHeight(75.0f);
	CyanScreenIllumRectLight->SetAttenuationRadius(85.0f);
	CyanScreenIllumRectLight->SetBarnDoorAngle(45.0f);
	CyanScreenIllumRectLight->SetCastShadows(false);

	LeftCrateCyanPipLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("LeftCrateCyanPipLight"));
	LeftCrateCyanPipLight->SetupAttachment(RootComponent);
	LeftCrateCyanPipLight->SetMobility(EComponentMobility::Movable);
	LeftCrateCyanPipLight->SetRelativeLocation(FVector(-265.0f, 82.0f, 26.0f));
	LeftCrateCyanPipLight->SetIntensity(400.0f);
	LeftCrateCyanPipLight->SetLightColor(FLinearColor(0.0f, 0.867f, 0.961f));
	LeftCrateCyanPipLight->SetAttenuationRadius(85.0f);
	LeftCrateCyanPipLight->SetCastShadows(false);

	// -------------------------------------------------------------------------
	// 4. OPERATOR FRONT FILL LIGHT (Subtle Readability Light near Camera Axis)
	// -------------------------------------------------------------------------
	OperatorFrontFillSpotLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("OperatorFrontFillSpotLight"));
	OperatorFrontFillSpotLight->SetupAttachment(RootComponent);
	OperatorFrontFillSpotLight->SetMobility(EComponentMobility::Movable);
	OperatorFrontFillSpotLight->SetRelativeLocation(FVector(235.0f, 18.0f, 132.0f));
	OperatorFrontFillSpotLight->SetRelativeRotation(FRotator(-6.0f, 176.0f, 0.0f));
	OperatorFrontFillSpotLight->SetIntensity(1200.0f);
	OperatorFrontFillSpotLight->SetLightColor(FLinearColor(0.85f, 0.90f, 0.96f));
	OperatorFrontFillSpotLight->SetAttenuationRadius(1200.0f);
	OperatorFrontFillSpotLight->SetInnerConeAngle(35.0f);
	OperatorFrontFillSpotLight->SetOuterConeAngle(65.0f);
	OperatorFrontFillSpotLight->SetCastShadows(false);

	// 4B. Dedicated Rear Hangar Bulkhead Architectural Wash Light
	BulkheadWashLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("BulkheadWashLight"));
	BulkheadWashLight->SetupAttachment(RootComponent);
	BulkheadWashLight->SetMobility(EComponentMobility::Movable);
	BulkheadWashLight->SetRelativeLocation(FVector(-550.0f, 0.0f, 220.0f));
	BulkheadWashLight->SetIntensity(3000.0f);
	BulkheadWashLight->SetLightColor(FLinearColor(0.24f, 0.40f, 0.56f));
	BulkheadWashLight->SetAttenuationRadius(2200.0f);
	BulkheadWashLight->SetCastShadows(false);

	// -------------------------------------------------------------------------
	// 5. VOLUMETRIC FOG & LIGHT SHAFTS (#040A12 Deep Dark Navy)
	// -------------------------------------------------------------------------
	HangarHeightFog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("HangarHeightFog"));
	HangarHeightFog->SetupAttachment(RootComponent);
	HangarHeightFog->SetFogDensity(0.00002f);
	HangarHeightFog->SetFogHeightFalloff(0.25f);
	HangarHeightFog->SetFogInscatteringColor(FLinearColor(0.016f, 0.039f, 0.071f)); // #040A12
	HangarHeightFog->SetVolumetricFog(false); // Enabled dynamically when Ultra GI mode is active
	HangarHeightFog->SetVolumetricFogScatteringDistribution(0.65f);
	HangarHeightFog->SetVolumetricFogAlbedo(FColor(28, 42, 58));
	HangarHeightFog->SetVolumetricFogExtinctionScale(1.8f);

	// -------------------------------------------------------------------------
	// 6. CAMERA, EXPOSURE & POST-PROCESS LOCK
	// Cinematic Auto Exposure + Full Depth of Field + Deep Navy Shadows
	// -------------------------------------------------------------------------
	HangarPostProcess = CreateDefaultSubobject<UPostProcessComponent>(TEXT("HangarPostProcess"));
	HangarPostProcess->SetupAttachment(RootComponent);
	HangarPostProcess->bUnbound = true;
	HangarPostProcess->Priority = 10.0f;

	FPostProcessSettings& PP = HangarPostProcess->Settings;
	// Eye Adaptation Auto Exposure
	PP.bOverride_AutoExposureMethod = true;
	PP.AutoExposureMethod = EAutoExposureMethod::AEM_Histogram;
	PP.bOverride_AutoExposureBias = true;
	PP.AutoExposureBias = 0.0f;
	PP.bOverride_AutoExposureMinBrightness = false;
	PP.bOverride_AutoExposureMaxBrightness = false;
	PP.bOverride_DepthOfFieldFocalDistance = false;
	PP.bOverride_DepthOfFieldFstop = false;

	// Controlled Bloom so cyan terminals and amber windows glow without whiteout clipping
	PP.bOverride_BloomIntensity = true;
	PP.BloomIntensity = 0.28f;
	PP.bOverride_BloomThreshold = true;
	PP.BloomThreshold = 0.85f;

	// Physical Depth of Field disabled to guarantee razor-sharp 3D operator & environment rendering
	PP.bOverride_DepthOfFieldFocalDistance = false;
	PP.bOverride_DepthOfFieldFstop = false;
	PP.bOverride_DepthOfFieldSensorWidth = false;

	// Vignette (0.35 intensity) & Deep Navy Shadow Grading (#050B13)
	PP.bOverride_VignetteIntensity = true;
	PP.VignetteIntensity = 0.35f;
	PP.bOverride_ColorGammaShadows = true;
	PP.ColorGammaShadows = FVector4(0.95f, 0.98f, 1.04f, 1.0f);
	PP.bOverride_ColorGainHighlights = true;
	PP.ColorGainHighlights = FVector4(1.03f, 1.01f, 1.02f, 1.0f);
}

// =========================================================================================
// RUNTIME PROCEDURAL PBR TEXTURE & SHADER GENERATION
// =========================================================================================
UTexture2D* AFPSMenuHangarStage::CreateTransientPBRTexture(int32 Width, int32 Height, const TArray<FColor>& Pixels, bool bSRGB, bool bIsNormalMap)
{
	if (Width <= 0 || Height <= 0 || Pixels.Num() != Width * Height)
	{
		return nullptr;
	}

	UTexture2D* Tex = UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8);
	if (!Tex)
	{
		return nullptr;
	}

	Tex->SRGB = bSRGB;
	Tex->CompressionSettings = bIsNormalMap ? TC_Normalmap : TC_Default;
	Tex->Filter = TF_Trilinear;
	Tex->AddressX = TA_Wrap;
	Tex->AddressY = TA_Wrap;

	if (Tex->GetPlatformData() && Tex->GetPlatformData()->Mips.Num() > 0)
	{
		FTexture2DMipMap& Mip = Tex->GetPlatformData()->Mips[0];
		void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
		if (Data)
		{
			FMemory::Memcpy(Data, Pixels.GetData(), Pixels.Num() * sizeof(FColor));
			Mip.BulkData.Unlock();
		}
	}

	Tex->UpdateResource();
	Tex->AddToRoot();
	return Tex;
}

void AFPSMenuHangarStage::GenerateWetHangarFloorTextures(TArray<FColor>& OutMask, TArray<FColor>& OutNormal, TArray<FColor>& OutRoughness)
{
	const int32 Size = 256;
	const int32 Total = Size * Size;
	OutMask.SetNumUninitialized(Total);
	OutNormal.SetNumUninitialized(Total);
	OutRoughness.SetNumUninitialized(Total);

	for (int32 Y = 0; Y < Size; ++Y)
	{
		const float V = static_cast<float>(Y) / static_cast<float>(Size);
		for (int32 X = 0; X < Size; ++X)
		{
			const float U = static_cast<float>(X) / static_cast<float>(Size);
			const int32 Idx = Y * Size + X;

			// Industrial Grate Grid (16x16 cells per tile)
			const float GridU = FMath::Fmod(U * 16.0f, 1.0f);
			const float GridV = FMath::Fmod(V * 16.0f, 1.0f);
			const bool bIsGrateBarU = (GridU < 0.18f || GridU > 0.82f);
			const bool bIsGrateBarV = (GridV < 0.16f || GridV > 0.84f);
			const bool bIsGrateBar = (bIsGrateBarU || bIsGrateBarV);

			// Organic Puddle Factor (Dual-frequency seamless noise)
			const float MacroPuddle = HangarPBRCache::SeamlessHashNoise(U, V, 3, 3);
			const float MicroNoise = HangarPBRCache::SeamlessHashNoise(U, V, 18, 18);
			const float PuddleWeight = FMath::Clamp((MacroPuddle * 0.75f + MicroNoise * 0.25f - 0.42f) * 3.2f, 0.0f, 1.0f);

			// Mask R = Steel Grate Treads, Mask G = Wet Puddle Mirror Zones
			const uint8 MaskR = bIsGrateBar ? static_cast<uint8>(FMath::Lerp(220.0f, 70.0f, PuddleWeight)) : 25;
			const uint8 MaskG = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt(PuddleWeight * 255.0f), 0, 255));
			OutMask[Idx] = FColor(MaskR, MaskG, 0, 255);

			// Normal Map: Crisp grate bevels where dry, flat mirror normal with subtle water ripple in puddles
			float NormX = 0.0f;
			float NormY = 0.0f;
			if (bIsGrateBarU)
			{
				NormX = (GridU < 0.18f) ? 0.55f : -0.55f;
			}
			if (bIsGrateBarV)
			{
				NormY = (GridV < 0.16f) ? 0.55f : -0.55f;
			}
			// Attenuate grate bump inside deep puddles and add micro water ripple
			const float Ripple = FMath::Sin((U * 12.0f + V * 8.0f) * 2.0f * UE_PI) * 0.06f;
			NormX = FMath::Lerp(NormX + (MicroNoise - 0.5f) * 0.22f, Ripple, PuddleWeight * 0.85f);
			NormY = FMath::Lerp(NormY + (MicroNoise - 0.5f) * 0.22f, -Ripple, PuddleWeight * 0.85f);

			const FVector N = FVector(NormX, NormY, 1.0f).GetSafeNormal();
			OutNormal[Idx] = FColor(
				static_cast<uint8>(FMath::Clamp(FMath::RoundToInt((N.X * 0.5f + 0.5f) * 255.0f), 0, 255)),
				static_cast<uint8>(FMath::Clamp(FMath::RoundToInt((N.Y * 0.5f + 0.5f) * 255.0f), 0, 255)),
				static_cast<uint8>(FMath::Clamp(FMath::RoundToInt((N.Z * 0.5f + 0.5f) * 255.0f), 0, 255)),
				255
			);

			// Spatial Roughness: 0.08–0.14 in wet puddles (crisp specular orange/cyan reflections), 0.48–0.62 on grate treads
			const float DryRough = bIsGrateBar ? (0.46f + MicroNoise * 0.14f) : 0.32f;
			const float FinalRough = FMath::Lerp(DryRough, 0.09f + MicroNoise * 0.04f, PuddleWeight);
			const uint8 RoughByte = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt(FinalRough * 255.0f), 0, 255));
			OutRoughness[Idx] = FColor(RoughByte, RoughByte, RoughByte, 255);
		}
	}
}

void AFPSMenuHangarStage::GenerateRainGlassTextures(TArray<FColor>& OutMask, TArray<FColor>& OutNormal, TArray<FColor>& OutRoughness)
{
	const int32 Size = 256;
	const int32 Total = Size * Size;
	OutMask.SetNumUninitialized(Total);
	OutNormal.SetNumUninitialized(Total);
	OutRoughness.SetNumUninitialized(Total);

	for (int32 Y = 0; Y < Size; ++Y)
	{
		const float V = static_cast<float>(Y) / static_cast<float>(Size);
		for (int32 X = 0; X < Size; ++X)
		{
			const float U = static_cast<float>(X) / static_cast<float>(Size);
			const int32 Idx = Y * Size + X;

			// High-frequency vertical rain rivulet trails along U, stretched vertically along V
			const float RivuletA = HangarPBRCache::SeamlessHashNoise(U, V, 28, 3);
			const float RivuletB = HangarPBRCache::SeamlessHashNoise(U + 0.37f, V * 2.0f, 44, 5);
			const float DropletNoise = HangarPBRCache::SeamlessHashNoise(U, V, 36, 36);

			const float StreakIntensity = FMath::Clamp((RivuletA * 0.65f + RivuletB * 0.35f - 0.52f) * 3.8f, 0.0f, 1.0f);
			const float BeadIntensity = (DropletNoise > 0.78f) ? FMath::Clamp((DropletNoise - 0.78f) * 4.5f, 0.0f, 1.0f) : 0.0f;
			const float TotalWet = FMath::Clamp(StreakIntensity + BeadIntensity, 0.0f, 1.0f);

			// Mask R = Smoked Glass Base, Mask G = Warm Amber Rivulet Highlight, Mask B = Subsurface Warm Transmission
			const uint8 MaskR = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt(StreakIntensity * 180.0f), 0, 255));
			const uint8 MaskG = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt(TotalWet * 255.0f), 0, 255));
			const uint8 MaskB = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt((0.45f + 0.55f * TotalWet) * 255.0f), 0, 255));
			OutMask[Idx] = FColor(MaskR, MaskG, MaskB, 255);

			// Strong Horizontal Normal Gradient across vertical rain streaks so orange backlight refracts into vertical ribbons!
			const float NextRivulet = HangarPBRCache::SeamlessHashNoise(U + (1.5f / Size), V, 28, 3);
			const float NormX = (NextRivulet - RivuletA) * 6.5f;
			const float NormY = (BeadIntensity > 0.0f) ? -0.45f : 0.0f;
			const FVector N = FVector(NormX, NormY, 1.0f).GetSafeNormal();

			OutNormal[Idx] = FColor(
				static_cast<uint8>(FMath::Clamp(FMath::RoundToInt((N.X * 0.5f + 0.5f) * 255.0f), 0, 255)),
				static_cast<uint8>(FMath::Clamp(FMath::RoundToInt((N.Y * 0.5f + 0.5f) * 255.0f), 0, 255)),
				static_cast<uint8>(FMath::Clamp(FMath::RoundToInt((N.Z * 0.5f + 0.5f) * 255.0f), 0, 255)),
				255
			);

			// Roughness: 0.05 on wet rivulets, 0.28 on condensation glass
			const float GlassRough = FMath::Lerp(0.28f, 0.05f, TotalWet);
			const uint8 RoughByte = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt(GlassRough * 255.0f), 0, 255));
			OutRoughness[Idx] = FColor(RoughByte, RoughByte, RoughByte, 255);
		}
	}
}

void AFPSMenuHangarStage::GenerateTerminalScreenTextures(int32 ScreenVariant, TArray<FColor>& OutMask, TArray<FColor>& OutNormal, TArray<FColor>& OutRoughness)
{
	const int32 Size = 256;
	const int32 Total = Size * Size;
	OutMask.SetNumUninitialized(Total);
	OutNormal.SetNumUninitialized(Total);
	OutRoughness.SetNumUninitialized(Total);

	for (int32 Y = 0; Y < Size; ++Y)
	{
		const float V = static_cast<float>(Y) / static_cast<float>(Size);
		for (int32 X = 0; X < Size; ++X)
		{
			const float U = static_cast<float>(X) / static_cast<float>(Size);
			const int32 Idx = Y * Size + X;

			float TealMask = 0.0f; // Channel R -> Teal (#0A2633) UI panels/grid
			float CyanMask = 0.0f; // Channel G -> Bright Noble Fermi Cyan (#00DDF5) radar rings, bar graphs & telemetry

			// Outer HUD border frame
			const bool bBorder = (U > 0.04f && U < 0.96f && V > 0.06f && V < 0.94f) &&
				(U < 0.06f || U > 0.94f || V < 0.085f || V > 0.915f);
			if (bBorder)
			{
				CyanMask = 0.85f;
			}

			// Subtle background tactical grid
			if (FMath::Fmod(U * 16.0f, 1.0f) < 0.06f || FMath::Fmod(V * 12.0f, 1.0f) < 0.06f)
			{
				TealMask = 0.45f;
			}

			if (ScreenVariant == 0)
			{
				// Variant 0 (Upper-Left & Lower-Right Monitors): Circular Radar Globe on Right + Telemetry Bar Graphs on Left
				// 1. Circular Radar Rings centered at (U=0.66, V=0.48)
				const float Dx = (U - 0.66f) * 1.35f;
				const float Dy = (V - 0.48f);
				const float Dist = FMath::Sqrt(Dx * Dx + Dy * Dy);

				if (Dist < 0.34f)
				{
					TealMask = FMath::Max(TealMask, 0.75f);
					// 3 Concentric Radar Rings
					if (FMath::Abs(Dist - 0.33f) < 0.016f || FMath::Abs(Dist - 0.22f) < 0.013f || FMath::Abs(Dist - 0.11f) < 0.013f || Dist < 0.032f)
					{
						CyanMask = 1.0f;
					}
					// Crosshairs
					if (FMath::Abs(Dx) < 0.010f || FMath::Abs(Dy) < 0.010f)
					{
						CyanMask = FMath::Max(CyanMask, 0.65f);
					}
				}

				// 2. Left Telemetry Bar Graphs & Header Blocks (U = 0.10 .. 0.38)
				if (U >= 0.10f && U <= 0.38f)
				{
					const int32 Row = FMath::FloorToInt(V * 14.0f);
					const float RowFrac = FMath::Fmod(V * 14.0f, 1.0f);
					if (Row >= 2 && Row <= 11 && RowFrac > 0.22f && RowFrac < 0.78f)
					{
						const float BarLen = 0.14f + 0.14f * FMath::Abs(FMath::Sin(Row * 1.9f));
						if (U - 0.10f < BarLen)
						{
							CyanMask = (Row % 3 == 0) ? 1.0f : 0.75f;
						}
						else
						{
							TealMask = 0.85f;
						}
					}
				}
			}
			else
			{
				// Variant 1 (Upper-Right & Lower-Left Monitors): Multi-Column Data Matrix & Waveform Readout
				for (int32 Col = 0; Col < 3; ++Col)
				{
					const float ColMin = 0.10f + Col * 0.28f;
					const float ColMax = ColMin + 0.22f;
					if (U >= ColMin && U <= ColMax && V >= 0.14f && V <= 0.86f)
					{
						TealMask = FMath::Max(TealMask, 0.65f);
						const int32 Line = FMath::FloorToInt(V * 18.0f);
						const float LineFrac = FMath::Fmod(V * 18.0f, 1.0f);
						if (LineFrac > 0.25f && LineFrac < 0.75f)
						{
							const float FillW = 0.06f + 0.15f * FMath::Abs(FMath::Cos(Line * 1.7f + Col * 2.3f));
							if (U - ColMin < FillW)
							{
								CyanMask = ((Line + Col) % 2 == 0) ? 1.0f : 0.70f;
							}
						}
					}
				}
			}

			// CRT horizontal scanline modulation
			const float Scanline = (Y % 2 == 0) ? 1.0f : 0.82f;
			CyanMask *= Scanline;

			OutMask[Idx] = FColor(
				static_cast<uint8>(FMath::Clamp(FMath::RoundToInt(TealMask * 255.0f), 0, 255)),
				static_cast<uint8>(FMath::Clamp(FMath::RoundToInt(CyanMask * 255.0f), 0, 255)),
				0,
				255
			);
			OutNormal[Idx] = FColor(128, 128, 255, 255);
			OutRoughness[Idx] = FColor(30, 30, 30, 255); // Glossy monitor screen (Roughness ~0.12)
		}
	}
}

void AFPSMenuHangarStage::BuildAndApplyProceduralPBRMaterials()
{
	// 1. Ensure cached procedural textures exist
	if (!HangarPBRCache::FloorMaskTex)
	{
		TArray<FColor> Mask, Norm, Rough;
		GenerateWetHangarFloorTextures(Mask, Norm, Rough);
		HangarPBRCache::FloorMaskTex = CreateTransientPBRTexture(256, 256, Mask, false, false);
		HangarPBRCache::FloorNormalTex = CreateTransientPBRTexture(256, 256, Norm, false, true);
		HangarPBRCache::FloorRoughnessTex = CreateTransientPBRTexture(256, 256, Rough, false, false);
	}

	if (!HangarPBRCache::RainGlassMaskTex)
	{
		TArray<FColor> Mask, Norm, Rough;
		GenerateRainGlassTextures(Mask, Norm, Rough);
		HangarPBRCache::RainGlassMaskTex = CreateTransientPBRTexture(256, 256, Mask, false, false);
		HangarPBRCache::RainGlassNormalTex = CreateTransientPBRTexture(256, 256, Norm, false, true);
		HangarPBRCache::RainGlassRoughnessTex = CreateTransientPBRTexture(256, 256, Rough, false, false);
	}

	if (!HangarPBRCache::TerminalScreenMaskTex0)
	{
		TArray<FColor> Mask0, Mask1, Norm, Rough;
		GenerateTerminalScreenTextures(0, Mask0, Norm, Rough);
		GenerateTerminalScreenTextures(1, Mask1, Norm, Rough);
		HangarPBRCache::TerminalScreenMaskTex0 = CreateTransientPBRTexture(256, 256, Mask0, false, false);
		HangarPBRCache::TerminalScreenMaskTex1 = CreateTransientPBRTexture(256, 256, Mask1, false, false);
		HangarPBRCache::TerminalScreenNormalTex = CreateTransientPBRTexture(256, 256, Norm, false, true);
		HangarPBRCache::TerminalScreenRoughnessTex = CreateTransientPBRTexture(256, 256, Rough, false, false);
	}

	if (!HangarPBRCache::ZeroBodyMaskTex)
	{
		TArray<FColor> ZeroPixels;
		ZeroPixels.Init(FColor(0, 0, 0, 0), 16 * 16);
		HangarPBRCache::ZeroBodyMaskTex = CreateTransientPBRTexture(16, 16, ZeroPixels, false, false);

		TArray<FColor> SubsurfacePixels;
		SubsurfacePixels.Init(FColor(0, 0, 255, 0), 16 * 16);
		HangarPBRCache::FullSubsurfaceMaskTex = CreateTransientPBRTexture(16, 16, SubsurfacePixels, false, false);
	}

	UMaterialInterface* MasterMultiMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Characters/BaseMale/Materials/M_Operator_BaseBody.M_Operator_BaseBody"));
	UMaterialInterface* SteelBaseMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Characters/BaseMale/Materials/M_AK47_Steel.M_AK47_Steel"));
	if (!SteelBaseMat)
	{
		SteelBaseMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	}

	// -------------------------------------------------------------------------
	// 2. Apply M_HangarFloor_Wet onto WetFloorPlane
	// BaseColor: #070B10 to #0E151D, Roughness: 0.08-0.14 in puddles, 0.48-0.62 on grates
	// -------------------------------------------------------------------------
	if (WetFloorPlane)
	{
		if (SteelBaseMat) WetFloorPlane->SetMaterial(0, SteelBaseMat);
		WetFloorMID = WetFloorPlane->CreateDynamicMaterialInstance(0);
		if (WetFloorMID)
		{
			WetFloorMID->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.05f, 0.07f, 0.09f, 1.0f));
			WetFloorMID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.05f, 0.07f, 0.09f, 1.0f));
			WetFloorMID->SetScalarParameterValue(TEXT("Roughness"), 0.18f);
			WetFloorMID->SetScalarParameterValue(TEXT("Metallic"), 0.85f);
		}
	}

	// Apply Dark Brushed Metallic Steel (#0A1017) to Floor Grate Strips & Turntable Plinth
	auto SetMetallicSteelColor = [SteelBaseMat](UStaticMeshComponent* Comp, const FLinearColor& LinearCol)
	{
		if (!Comp) return;
		if (SteelBaseMat) Comp->SetMaterial(0, SteelBaseMat);
		if (UMaterialInstanceDynamic* MID = Comp->CreateDynamicMaterialInstance(0))
		{
			MID->SetVectorParameterValue(TEXT("BaseColor"), LinearCol);
			MID->SetVectorParameterValue(TEXT("Color"), LinearCol);
		}
	};

	for (UStaticMeshComponent* Strip : FloorGrateStrips)
	{
		SetMetallicSteelColor(Strip, FLinearColor(0.20f, 0.25f, 0.30f, 1.0f));
	}
	SetMetallicSteelColor(PlinthOuterRing, FLinearColor(0.22f, 0.28f, 0.35f, 1.0f));
	SetMetallicSteelColor(PlinthBevelRing, FLinearColor(0.30f, 0.36f, 0.44f, 1.0f));

	// Cyan Emissive Micro-Pips around Turntable Plinth
	for (UStaticMeshComponent* Pip : PlinthCyanPips)
	{
		SetMetallicSteelColor(Pip, FLinearColor(0.0f, 0.90f, 1.0f, 1.0f));
	}

	// -------------------------------------------------------------------------
	// 3. Apply M_HangarWindow_Rain / M_RainGlass_Refractive onto LeftRainWindowPanes
	// -------------------------------------------------------------------------
	RainWindowMIDs.Empty();
	for (UStaticMeshComponent* Pane : LeftRainWindowPanes)
	{
		if (!Pane) continue;
		if (MasterMultiMat)
		{
			Pane->SetMaterial(0, MasterMultiMat);
			if (UMaterialInstanceDynamic* GlassMID = Pane->CreateDynamicMaterialInstance(0))
			{
				GlassMID->SetTextureParameterValue(TEXT("Texture_Camo"), HangarPBRCache::RainGlassMaskTex);
				GlassMID->SetTextureParameterValue(TEXT("Texture_BodyMask"), HangarPBRCache::RainGlassMaskTex);
				GlassMID->SetTextureParameterValue(TEXT("Texture_RipstopNormal"), HangarPBRCache::RainGlassNormalTex);
				GlassMID->SetTextureParameterValue(TEXT("Texture_RipstopRoughness"), HangarPBRCache::RainGlassRoughnessTex);

				// Dark smoked window tint + warm sodium-amber rain streak highlights + subsurface transmission
				GlassMID->SetVectorParameterValue(TEXT("CamoColorPrimary"), FLinearColor(0.02f, 0.03f, 0.05f, 1.0f));
				GlassMID->SetVectorParameterValue(TEXT("CamoColorSecondary"), FLinearColor(0.45f, 0.18f, 0.04f, 1.0f));
				GlassMID->SetVectorParameterValue(TEXT("CamoColorTertiary"), FLinearColor(0.95f, 0.45f, 0.10f, 1.0f));
				GlassMID->SetVectorParameterValue(TEXT("SkinTone"), FLinearColor(0.55f, 0.24f, 0.06f, 1.0f));
				GlassMID->SetScalarParameterValue(TEXT("RoughnessBase"), 0.35f);
				GlassMID->SetScalarParameterValue(TEXT("SubsurfaceIntensity"), 2.8f);
				GlassMID->SetScalarParameterValue(TEXT("CamoTiling"), 2.0f);
				GlassMID->SetScalarParameterValue(TEXT("RipstopTiling"), 3.0f);
				RainWindowMIDs.Add(GlassMID);
			}
		}
	}

	// Exterior Fire Backdrop (warm fiery sodium-amber surface illuminated by ExteriorAmberGlowLight)
	if (ExteriorFireBackdrop && MasterMultiMat)
	{
		ExteriorFireBackdrop->SetMaterial(0, MasterMultiMat);
		if (UMaterialInstanceDynamic* FireMID = ExteriorFireBackdrop->CreateDynamicMaterialInstance(0))
		{
			FireMID->SetTextureParameterValue(TEXT("Texture_Camo"), HangarPBRCache::RainGlassMaskTex);
			FireMID->SetTextureParameterValue(TEXT("Texture_BodyMask"), HangarPBRCache::FullSubsurfaceMaskTex);
			FireMID->SetVectorParameterValue(TEXT("SkinTone"), FLinearColor(1.0f, 0.46f, 0.08f, 1.0f));
			FireMID->SetScalarParameterValue(TEXT("SubsurfaceIntensity"), 3.5f);
			FireMID->SetScalarParameterValue(TEXT("RoughnessBase"), 0.25f);
		}
	}

	// Dark Industrial Steel on Beams, Silhouettes, Console Racks, Gantry, and Deep Bulkhead
	for (UStaticMeshComponent* Beam : LeftStructuralBeams)
	{
		SetMetallicSteelColor(Beam, FLinearColor(0.18f, 0.24f, 0.30f, 1.0f));
	}
	for (UStaticMeshComponent* Sil : ExteriorSilhouetteMeshes)
	{
		SetMetallicSteelColor(Sil, FLinearColor(0.14f, 0.18f, 0.22f, 1.0f));
	}
	for (UStaticMeshComponent* Rack : ConsoleRackFrames)
	{
		SetMetallicSteelColor(Rack, FLinearColor(0.20f, 0.26f, 0.32f, 1.0f));
	}
	for (UStaticMeshComponent* Gantry : OverheadGantryMeshes)
	{
		SetMetallicSteelColor(Gantry, FLinearColor(0.18f, 0.24f, 0.30f, 1.0f));
	}
	for (UStaticMeshComponent* Bulkhead : DeepBulkheadMeshes)
	{
		SetMetallicSteelColor(Bulkhead, FLinearColor(0.16f, 0.22f, 0.28f, 1.0f));
	}

	// -------------------------------------------------------------------------
	// 4. Apply M_HangarTerminal_Screen onto ConsoleMonitorScreens
	// Procedural Noble Fermi Cyan (#00DDF5) & Teal (#0A2633) radar/telemetry readouts
	// -------------------------------------------------------------------------
	TerminalScreenMIDs.Empty();
	for (int32 i = 0; i < ConsoleMonitorScreens.Num(); ++i)
	{
		UStaticMeshComponent* ScreenComp = ConsoleMonitorScreens[i];
		if (!ScreenComp || !MasterMultiMat) continue;

		ScreenComp->SetMaterial(0, MasterMultiMat);
		if (UMaterialInstanceDynamic* ScreenMID = ScreenComp->CreateDynamicMaterialInstance(0))
		{
			UTexture2D* ChosenMask = (i % 2 == 0) ? HangarPBRCache::TerminalScreenMaskTex0.Get() : HangarPBRCache::TerminalScreenMaskTex1.Get();
			ScreenMID->SetTextureParameterValue(TEXT("Texture_Camo"), ChosenMask);
			ScreenMID->SetTextureParameterValue(TEXT("Texture_BodyMask"), HangarPBRCache::ZeroBodyMaskTex);
			ScreenMID->SetTextureParameterValue(TEXT("Texture_RipstopNormal"), HangarPBRCache::TerminalScreenNormalTex);
			ScreenMID->SetTextureParameterValue(TEXT("Texture_RipstopRoughness"), HangarPBRCache::TerminalScreenRoughnessTex);

			// Primary: Deep Obsidian Screen Back (#02080D), Secondary: Teal (#0A2633), Tertiary: Bright Noble Fermi Cyan (#00DDF5)
			ScreenMID->SetVectorParameterValue(TEXT("CamoColorPrimary"), FLinearColor(0.004f, 0.015f, 0.025f, 1.0f));
			ScreenMID->SetVectorParameterValue(TEXT("CamoColorSecondary"), FLinearColor(0.015f, 0.149f, 0.200f, 1.0f));
			ScreenMID->SetVectorParameterValue(TEXT("CamoColorTertiary"), FLinearColor(0.0f, 0.867f, 0.961f, 1.0f));
			ScreenMID->SetScalarParameterValue(TEXT("RoughnessBase"), 0.15f);
			ScreenMID->SetScalarParameterValue(TEXT("SubsurfaceIntensity"), 0.0f);
			ScreenMID->SetScalarParameterValue(TEXT("CamoTiling"), 1.0f);
			ScreenMID->SetScalarParameterValue(TEXT("RipstopTiling"), 1.0f);
			TerminalScreenMIDs.Add(ScreenMID);
		}
	}

	// Console Status LEDs
	for (int32 i = 0; i < ConsoleStatusLEDs.Num(); ++i)
	{
		const FLinearColor LedCol = (i == 2) ? FLinearColor(1.0f, 0.55f, 0.08f, 1.0f) : FLinearColor(0.0f, 0.90f, 1.0f, 1.0f);
		SetMetallicSteelColor(ConsoleStatusLEDs[i], LedCol);
	}

	// -------------------------------------------------------------------------
	// 5. Forefront Rain Streaks & Water Droplet Beads (catches warm amber & cool cyan light)
	// -------------------------------------------------------------------------
	for (UStaticMeshComponent* Streak : ForefrontRainStreaks)
	{
		SetMetallicSteelColor(Streak, FLinearColor(0.75f, 0.52f, 0.34f, 1.0f));
	}
	for (UStaticMeshComponent* Drop : ForefrontRainDroplets)
	{
		SetMetallicSteelColor(Drop, FLinearColor(0.88f, 0.64f, 0.38f, 1.0f));
	}
}

void AFPSMenuHangarStage::BeginPlay()
{
	Super::BeginPlay();

	BuildAndApplyProceduralPBRMaterials();

	if (HangarSkyLight)
	{
		HangarSkyLight->RecaptureSky();
	}

	// Detect initial Scalability Tier from GEngineIni (Laptop FPS vs. Ultra GI)
	int32 SavedLumenMethod = 0;
	if (GConfig)
	{
		GConfig->GetInt(TEXT("SystemSettings"), TEXT("r.DynamicGlobalIlluminationMethod"), SavedLumenMethod, GEngineIni);
	}
	ApplyScalabilityTier(SavedLumenMethod > 0);
}

void AFPSMenuHangarStage::ConfigurePreviewActorAndCamera(AFPSCharacterPreviewActor* InPreviewActor)
{
	LinkedPreviewActor = InPreviewActor;
	if (!InPreviewActor)
	{
		return;
	}

	// Align PreviewActor's built-in studio lights with the Hangar's Left-Amber / Right-Cyan split lighting
	// (Note: Does NOT touch Operator mesh, rig, camo material, or turntable rotation!)
	if (InPreviewActor->StudioKeyLight)
	{
		InPreviewActor->StudioKeyLight->SetRelativeLocation(FVector(110.0f, 155.0f, 185.0f));
		InPreviewActor->StudioKeyLight->SetRelativeRotation(FRotator(-22.0f, -125.0f, 0.0f));
		InPreviewActor->StudioKeyLight->SetIntensity(1500.0f);
		InPreviewActor->StudioKeyLight->SetLightColor(FLinearColor(1.0f, 0.70f, 0.35f)); // Warm Amber
	}

	if (InPreviewActor->StudioRimLight)
	{
		InPreviewActor->StudioRimLight->SetRelativeLocation(FVector(-95.0f, -155.0f, 160.0f));
		InPreviewActor->StudioRimLight->SetRelativeRotation(FRotator(-16.0f, 55.0f, 0.0f));
		InPreviewActor->StudioRimLight->SetIntensity(1800.0f);
		InPreviewActor->StudioRimLight->SetLightColor(FLinearColor(0.0f, 0.898f, 1.0f)); // Tactical Cyan
	}

	if (InPreviewActor->StudioFillLight)
	{
		InPreviewActor->StudioFillLight->SetRelativeLocation(FVector(150.0f, 0.0f, 115.0f));
		InPreviewActor->StudioFillLight->SetIntensity(600.0f);
		InPreviewActor->StudioFillLight->SetLightColor(FLinearColor(1.0f, 0.96f, 0.90f));
	}

	if (InPreviewActor->StudioAmbientLight)
	{
		InPreviewActor->StudioAmbientLight->SetIntensity(200.0f);
		InPreviewActor->StudioAmbientLight->SetLightColor(FLinearColor(0.35f, 0.45f, 0.55f));
	}

	if (InPreviewActor->StudioSunLight)
	{
		InPreviewActor->StudioSunLight->SetIntensity(0.0f);
	}

	// Lock PreviewCamera Exposure & Vignette
	if (InPreviewActor->PreviewCamera)
	{
		FPostProcessSettings& CamPP = InPreviewActor->PreviewCamera->PostProcessSettings;
		CamPP.bOverride_AutoExposureMethod = true;
		CamPP.AutoExposureMethod = EAutoExposureMethod::AEM_Histogram;
		CamPP.bOverride_AutoExposureBias = true;
		CamPP.AutoExposureBias = 0.0f;
		CamPP.bOverride_AutoExposureMinBrightness = false;
		CamPP.AutoExposureMinBrightness = 0.5f;
		CamPP.bOverride_AutoExposureMaxBrightness = false;
		CamPP.AutoExposureMaxBrightness = 8.0f;

		CamPP.bOverride_BloomIntensity = true;
		CamPP.BloomIntensity = 0.28f;

		CamPP.bOverride_DepthOfFieldFocalDistance = false;
		CamPP.bOverride_DepthOfFieldFstop = false;
		CamPP.bOverride_DepthOfFieldSensorWidth = false;

		CamPP.bOverride_VignetteIntensity = true;
		CamPP.VignetteIntensity = 0.25f;
	}
}

void AFPSMenuHangarStage::ApplyScalabilityTier(bool bInUltraGIMode)
{
	bUltraGIActive = bInUltraGIMode;

	if (HangarHeightFog)
	{
		// Ultra GI Mode enables volumetric fog beams through the slanted storm windows;
		// Laptop FPS Mode disables volumetric fog supersampling for locked high frame rates.
		HangarHeightFog->SetVolumetricFog(bUltraGIActive);
	}

	if (WarmOperatorRimSpotLight)
	{
		WarmOperatorRimSpotLight->SetCastShadows(bUltraGIActive);
	}

	if (CyanOperatorRimSpotLight)
	{
		CyanOperatorRimSpotLight->SetCastShadows(bUltraGIActive);
	}
}

void AFPSMenuHangarStage::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	StageAnimTime += DeltaTime;

	// 1. Animate Falling Rain Streaks in the Left-Foreground Hangar Bay
	for (int32 i = 0; i < ForefrontRainStreaks.Num() && i < RainStreakTracks.Num(); ++i)
	{
		if (UStaticMeshComponent* Streak = ForefrontRainStreaks[i])
		{
			const FRainDropTrack& Track = RainStreakTracks[i];
			const float Span = Track.MaxZ - Track.MinZ;
			const float CurrZ = Track.MaxZ - FMath::Fmod(Track.BaseLocation.Z + StageAnimTime * Track.FallSpeed, Span);
			Streak->SetRelativeLocation(FVector(Track.BaseLocation.X, Track.BaseLocation.Y, CurrZ));
		}
	}

	// 2. Animate Glistening Water Droplet Beads trickling down the Left Window Drip-Chains
	for (int32 i = 0; i < ForefrontRainDroplets.Num() && i < RainDropletTracks.Num(); ++i)
	{
		if (UStaticMeshComponent* Drop = ForefrontRainDroplets[i])
		{
			const FRainDropTrack& Track = RainDropletTracks[i];
			const float Span = Track.MaxZ - Track.MinZ;
			const float CurrZ = Track.MaxZ - FMath::Fmod(Track.BaseLocation.Z + StageAnimTime * Track.FallSpeed, Span);
			Drop->SetRelativeLocation(FVector(Track.BaseLocation.X, Track.BaseLocation.Y, CurrZ));
		}
	}

	// 3. Subtle Living Industrial Fire Flicker on Left Storm Windows & Cyan Telemetry Pulse on Right Consoles
	if (WarmWindowKeySpotLight)
	{
		const float FirePulse = 1.0f + 0.06f * FMath::Sin(StageAnimTime * 4.2f) + 0.03f * FMath::Cos(StageAnimTime * 9.7f);
		WarmWindowKeySpotLight->SetIntensity(4500.0f * FirePulse);
	}

	if (CyanConsoleBounceLight)
	{
		const float CyberPulse = 1.0f + 0.05f * FMath::Sin(StageAnimTime * 2.8f);
		CyanConsoleBounceLight->SetIntensity(2200.0f * CyberPulse);
	}
}
