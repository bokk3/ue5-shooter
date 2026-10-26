// =========================================================================================
// NOBLE FERMI // PROCEDURAL ANTI-TILING COASTAL & FAVELA PBR MATERIAL & ENVIRONMENT LIBRARY
// C++20 Runtime PBR material binding and instanced Favela architectural kit builder:
// Exposed Tijolo Baiano Brick, Cinder Blocks (CMU), Peeling Pastel Stucco, Cobblestone,
// Cracked Asphalt, Red Laterite Clay, Corrugated Zinc Roofing Plates, Tropical GPU-Wind
// Foliage, and Catenary Swaying Overhead Electrical Wiring ("Gatos").
// =========================================================================================

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "FPSCoastalMaterialLibrary.generated.h"

UENUM(BlueprintType)
enum class ECoastalSurfaceType : uint8
{
	FineBeachSand UMETA(DisplayName = "Fine Beach Sand"),
	WeatheredPierTimber UMETA(DisplayName = "Weathered Pier Timber"),
	MarineCastConcrete UMETA(DisplayName = "Marine Cast Concrete"),
	BasaltRiprapRock UMETA(DisplayName = "Basalt Riprap Rock"),
	WeatheredAsphalt UMETA(DisplayName = "Weathered Asphalt"),
	FavelaBrick UMETA(DisplayName = "Favela Tijolo Baiano Brick"),
	FavelaCinder UMETA(DisplayName = "Favela Cinder Block CMU"),
	FavelaStucco UMETA(DisplayName = "Favela Peeling Pastel Stucco"),
	FavelaCobble UMETA(DisplayName = "Favela Portuguese Cobblestone"),
	FavelaDirt UMETA(DisplayName = "Favela Red Laterite Clay"),
	FavelaMetal UMETA(DisplayName = "Favela Rusted Corrugated Metal"),
	FavelaPuddle UMETA(DisplayName = "Favela Rainwater Puddle")
};

/**
 * Instanced Favela Environment Kit Builder (ISM Batching + GPU WPO Foliage & Cables).
 * Populates Rocha Velha Favela (Theater 0) and Baia Brava Favela (Theater 1) with
 * multi-story shanties, corrugated roofs, drainage gutters, tropical foliage, and swaying wires.
 */
struct NOBLEFERMI_API FFPSFavelaEnvironmentKit
{
	AActor* Owner = nullptr;
	USceneComponent* Root = nullptr;

	UStaticMesh* CubeMesh = nullptr;
	UStaticMesh* CylinderMesh = nullptr;
	UStaticMesh* RoofSheetMesh = nullptr;
	UStaticMesh* CableMesh = nullptr;
	UStaticMesh* WeedsMesh = nullptr;
	UStaticMesh* IvyMesh = nullptr;
	UStaticMesh* BananaMesh = nullptr;
	UStaticMesh* TireMesh = nullptr;
	UStaticMesh* WaterTankMesh = nullptr;

	UInstancedStaticMeshComponent* BrickISM = nullptr;
	UInstancedStaticMeshComponent* CinderISM = nullptr;
	UInstancedStaticMeshComponent* StuccoISM = nullptr;
	UInstancedStaticMeshComponent* ConcreteISM = nullptr;
	UInstancedStaticMeshComponent* CobbleISM = nullptr;
	UInstancedStaticMeshComponent* AsphaltISM = nullptr;
	UInstancedStaticMeshComponent* DirtISM = nullptr;
	UInstancedStaticMeshComponent* WoodISM = nullptr;
	UInstancedStaticMeshComponent* MetalISM = nullptr;
	UInstancedStaticMeshComponent* DarkISM = nullptr;
	UInstancedStaticMeshComponent* PuddleISM = nullptr;
	UInstancedStaticMeshComponent* RoofSheetISM = nullptr;
	UInstancedStaticMeshComponent* TireISM = nullptr;
	UInstancedStaticMeshComponent* WaterTankISM = nullptr;
	UInstancedStaticMeshComponent* WeedsISM = nullptr;
	UInstancedStaticMeshComponent* IvyISM = nullptr;
	UInstancedStaticMeshComponent* BananaISM = nullptr;
	UInstancedStaticMeshComponent* CableISM = nullptr;

	void Init(AActor* InOwner, USceneComponent* InRoot);

	void AddBox(UInstancedStaticMeshComponent* ISM, const FVector& Center, const FVector& SizeCm, const FRotator& Rot = FRotator::ZeroRotator, const FLinearColor& Tint = FLinearColor::White, float Peel = 1.0f, float BaseZ = 0.0f);
	void AddMesh(UInstancedStaticMeshComponent* ISM, const FVector& Loc, const FRotator& Rot, const FVector& Scale, const FLinearColor& Tint = FLinearColor::White, float Peel = 1.0f, float BaseZ = 0.0f);

	void AddRoofCorrugated(const FVector& RoofCenterTop, float LengthX, float WidthY, float YawDeg, float PitchDeg, int32 Seed, bool bAddWaterTank = true);
	void AddShanty(const FVector& BaseCenter, float SizeX, float SizeY, float YawDeg, int32 NumStories, int32 Seed, bool bCantileverUpper = true, bool bHasBalcony = true, bool bAddWaterTank = true);
	void AddDrainageGutter(const FVector& StartXY, const FVector& EndXY, float GroundZ, int32 Seed);
	void AddPuddle(const FVector& CenterXY, float LengthX, float WidthY, float YawDeg, float GroundZ = 0.0f);
	FVector AddUtilityPole(const FVector& BaseLoc, float HeightCm, float YawDeg, bool bWithTransformer);
	void AddCableSpan(const FVector& Start, const FVector& End, float SagCm, float PhaseRad, float AmpScale = 1.0f, float ThicknessScale = 1.35f);
	void AddCableBundle(const FVector& A, const FVector& B, int32 NumStrands, float BaseSagCm, int32 Seed);
	void AddWeedLine(const FVector& Start, const FVector& End, int32 Count, int32 Seed, float SpreadCm = 28.0f);
	void AddBananaCluster(const FVector& BaseLoc, int32 Count, int32 Seed);
};

UCLASS()
class NOBLEFERMI_API UFPSCoastalMaterialLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "FPS Coastal Materials", meta = (WorldContext = "WorldContext"))
	static UMaterialInterface* GetOrCreateCoastalMaterial(UObject* WorldContext, ECoastalSurfaceType SurfaceType, FVector2D WindDirection = FVector2D(1.0f, 0.0f));

	UFUNCTION(BlueprintCallable, Category = "FPS Coastal Materials")
	static UTexture2D* GetOrCreateBaseColorTexture(ECoastalSurfaceType SurfaceType, FVector2D WindDirection = FVector2D(1.0f, 0.0f));

	UFUNCTION(BlueprintCallable, Category = "FPS Coastal Materials")
	static UTexture2D* GetOrCreateNormalTexture(ECoastalSurfaceType SurfaceType, FVector2D WindDirection = FVector2D(1.0f, 0.0f));

	UFUNCTION(BlueprintCallable, Category = "FPS Coastal Materials")
	static UTexture2D* GetOrCreateRoughnessTexture(ECoastalSurfaceType SurfaceType);

	UFUNCTION(BlueprintCallable, Category = "FPS Coastal Materials")
	static void ApplyCoastalSurfaceToMesh(UStaticMeshComponent* MeshComp, ECoastalSurfaceType SurfaceType, int32 MaterialSlot = 0, FVector2D WindDirection = FVector2D(1.0f, 0.0f));

	/** Loads a cooked Favela Material Instance by short name (e.g. "MI_Favela_Brick") */
	static UMaterialInterface* LoadFavelaMaterial(const TCHAR* MaterialName);

	/** Builds the full Rocha Velha Favela (Theater 0) architectural & environmental dressing */
	static void PopulateRochaVelhaFavela(AActor* MapActor, USceneComponent* RootComp);

	/** Builds the full Baia Brava Coastal Favela (Theater 1) architectural & environmental dressing */
	static void PopulateBaiaBravaFavela(AActor* MapActor, USceneComponent* RootComp);

	UFUNCTION(BlueprintPure, Category = "FPS Coastal Materials")
	static FLinearColor GetSurfaceBaseColor(ECoastalSurfaceType SurfaceType);

	UFUNCTION(BlueprintPure, Category = "FPS Coastal Materials")
	static float GetSurfaceRoughness(ECoastalSurfaceType SurfaceType);

	UFUNCTION(BlueprintPure, Category = "FPS Coastal Materials")
	static float GetSurfaceMetallic(ECoastalSurfaceType SurfaceType);

	UFUNCTION(BlueprintPure, Category = "FPS Coastal Materials")
	static float GetSurfaceSpecular(ECoastalSurfaceType SurfaceType);

	UFUNCTION(BlueprintCallable, Category = "FPS Coastal Materials")
	static void ClearMaterialCache();

private:
	static UTexture2D* CreateTransientTexture(int32 Width, int32 Height, const TArray<FColor>& Pixels, bool bSRGB, bool bIsNormal);

	static void GenerateSandTextures(FVector2D WindDir, TArray<FColor>& OutBaseColor, TArray<FColor>& OutNormal, TArray<FColor>& OutRoughness);
	static void GenerateTimberTextures(TArray<FColor>& OutBaseColor, TArray<FColor>& OutNormal, TArray<FColor>& OutRoughness);
	static void GenerateConcreteTextures(TArray<FColor>& OutBaseColor, TArray<FColor>& OutNormal, TArray<FColor>& OutRoughness);
	static void GenerateBasaltTextures(TArray<FColor>& OutBaseColor, TArray<FColor>& OutNormal, TArray<FColor>& OutRoughness);
	static void GenerateAsphaltTextures(TArray<FColor>& OutBaseColor, TArray<FColor>& OutNormal, TArray<FColor>& OutRoughness);

	static float SeamlessNoise(float U, float V, float ScaleX, float ScaleY);
	static float VoronoiCell(float U, float V, int32 GridCells, FVector& OutCellNormal);
};
