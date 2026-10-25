// =========================================================================================
// NOBLE FERMI // PROCEDURAL ANTI-TILING COASTAL PBR MATERIAL & TEXTURE LIBRARY
// C++20 Runtime procedural texture generation and dynamic material instances for:
// Fine Beach Sand, Weathered Pier Timber, Marine Cast Concrete, Basalt Riprap Rock, Weathered Asphalt.
// Eliminates repetitive grid tiling using dual-frequency non-harmonic macro/micro variation
// and directional wind ripple normal baking.
// =========================================================================================

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/StaticMeshComponent.h"
#include "FPSCoastalMaterialLibrary.generated.h"

UENUM(BlueprintType)
enum class ECoastalSurfaceType : uint8
{
	FineBeachSand UMETA(DisplayName = "Fine Beach Sand"),
	WeatheredPierTimber UMETA(DisplayName = "Weathered Pier Timber"),
	MarineCastConcrete UMETA(DisplayName = "Marine Cast Concrete"),
	BasaltRiprapRock UMETA(DisplayName = "Basalt Riprap Rock"),
	WeatheredAsphalt UMETA(DisplayName = "Weathered Asphalt")
};

UCLASS()
class NOBLEFERMI_API UFPSCoastalMaterialLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Primary API: Retrieves or dynamically builds a cached UMaterialInstanceDynamic
	 * configured with procedural PBR textures and calibrated physical surface parameters.
	 * Can be applied onto AFPSCoastalOperationsMapActor and AFPSCoastalSplineActor with a single call.
	 */
	UFUNCTION(BlueprintCallable, Category = "FPS Coastal Materials", meta = (WorldContext = "WorldContext"))
	static UMaterialInterface* GetOrCreateCoastalMaterial(UObject* WorldContext, ECoastalSurfaceType SurfaceType, FVector2D WindDirection = FVector2D(1.0f, 0.0f));

	/**
	 * Generates or retrieves a transient 256x256 seamless PBR BaseColor texture
	 * with non-harmonic macro/micro noise variation and clamped luminance.
	 */
	UFUNCTION(BlueprintCallable, Category = "FPS Coastal Materials")
	static UTexture2D* GetOrCreateBaseColorTexture(ECoastalSurfaceType SurfaceType, FVector2D WindDirection = FVector2D(1.0f, 0.0f));

	/**
	 * Generates or retrieves a transient 256x256 tangent-space Normal map
	 * with directional transverse wind ripples, wood grain grooves, or angular rock facets.
	 */
	UFUNCTION(BlueprintCallable, Category = "FPS Coastal Materials")
	static UTexture2D* GetOrCreateNormalTexture(ECoastalSurfaceType SurfaceType, FVector2D WindDirection = FVector2D(1.0f, 0.0f));

	/**
	 * Generates or retrieves a transient 256x256 Roughness & Cavity mask texture.
	 */
	UFUNCTION(BlueprintCallable, Category = "FPS Coastal Materials")
	static UTexture2D* GetOrCreateRoughnessTexture(ECoastalSurfaceType SurfaceType);

	/**
	 * Helper to apply the coastal material directly to a static mesh component,
	 * automatically tagging the component for dynamic acoustic surface detection.
	 */
	UFUNCTION(BlueprintCallable, Category = "FPS Coastal Materials")
	static void ApplyCoastalSurfaceToMesh(UStaticMeshComponent* MeshComp, ECoastalSurfaceType SurfaceType, int32 MaterialSlot = 0, FVector2D WindDirection = FVector2D(1.0f, 0.0f));

	/** Returns calibrated linear base color for this surface archetype */
	UFUNCTION(BlueprintPure, Category = "FPS Coastal Materials")
	static FLinearColor GetSurfaceBaseColor(ECoastalSurfaceType SurfaceType);

	/** Returns calibrated PBR roughness for this surface archetype */
	UFUNCTION(BlueprintPure, Category = "FPS Coastal Materials")
	static float GetSurfaceRoughness(ECoastalSurfaceType SurfaceType);

	/** Returns calibrated PBR metallic for this surface archetype */
	UFUNCTION(BlueprintPure, Category = "FPS Coastal Materials")
	static float GetSurfaceMetallic(ECoastalSurfaceType SurfaceType);

	/** Returns calibrated PBR specular for this surface archetype */
	UFUNCTION(BlueprintPure, Category = "FPS Coastal Materials")
	static float GetSurfaceSpecular(ECoastalSurfaceType SurfaceType);

	/** Clear all cached runtime textures and dynamic material instances */
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
