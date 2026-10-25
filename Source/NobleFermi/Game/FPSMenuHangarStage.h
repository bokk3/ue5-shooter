// =========================================================================================
// NOBLE FERMI — MAIN MENU 3D HANGAR DIORAMA & RAIN-GLASS OPTICS STAGE ACTOR
// Target Engine: Unreal Engine 5.8 (C++20 / Native Rendering Pipeline)
// Self-contained procedural 3D Industrial Hangar Diorama, PBR Material Generator,
// Three-Tone Split Lighting Rig (Sodium Amber vs. Cold Navy vs. Noble Fermi Cyan),
// and "Rainburst on Glass" Diegetic Wet Optics.
// =========================================================================================

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FPSMenuHangarStage.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UTexture2D;
class USpotLightComponent;
class UPointLightComponent;
class URectLightComponent;
class USkyLightComponent;
class UExponentialHeightFogComponent;
class UPostProcessComponent;
class AFPSCharacterPreviewActor;

UCLASS()
class NOBLEFERMI_API AFPSMenuHangarStage : public AActor
{
	GENERATED_BODY()

public:
	AFPSMenuHangarStage();

	virtual void Tick(float DeltaTime) override;

	/** Links this hangar stage to the active Operator preview turntable actor and locks camera/lighting */
	UFUNCTION(BlueprintCallable, Category = "FPS Menu Hangar")
	void ConfigurePreviewActorAndCamera(AFPSCharacterPreviewActor* InPreviewActor);

	/** Switches between Laptop FPS Mode (analytical reflections, no volumetric fog) and Ultra GI Mode (Lumen + Volumetric Fog + Shadows) */
	UFUNCTION(BlueprintCallable, Category = "FPS Menu Hangar")
	void ApplyScalabilityTier(bool bInUltraGIMode);

	// -------------------------------------------------------------------------
	// Root & Layer Hierarchy
	// -------------------------------------------------------------------------
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hangar|Scene")
	TObjectPtr<USceneComponent> SceneRoot;

	// Layer 1: Wet Industrial Grate & Concrete Floor + Tactical Turntable Plinth
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hangar|Layer1_Floor")
	TObjectPtr<UStaticMeshComponent> WetFloorPlane;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hangar|Layer1_Floor")
	TArray<TObjectPtr<UStaticMeshComponent>> FloorGrateStrips;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hangar|Layer1_Floor")
	TObjectPtr<UStaticMeshComponent> PlinthOuterRing;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hangar|Layer1_Floor")
	TObjectPtr<UStaticMeshComponent> PlinthBevelRing;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hangar|Layer1_Floor")
	TArray<TObjectPtr<UStaticMeshComponent>> PlinthCyanPips;

	// Layer 2: Left-Midground Angled Storm Windows & Structural Ribs (Warm Backlight Zone)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hangar|Layer2_StormWindows")
	TArray<TObjectPtr<UStaticMeshComponent>> LeftStructuralBeams;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hangar|Layer2_StormWindows")
	TArray<TObjectPtr<UStaticMeshComponent>> LeftRainWindowPanes;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hangar|Layer2_StormWindows")
	TArray<TObjectPtr<UStaticMeshComponent>> ExteriorSilhouetteMeshes;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hangar|Layer2_StormWindows")
	TObjectPtr<UStaticMeshComponent> ExteriorFireBackdrop;

	// Layer 3: Right-Midground Tactical Command Consoles (Cyan Tech Zone)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hangar|Layer3_CommandConsoles")
	TArray<TObjectPtr<UStaticMeshComponent>> ConsoleRackFrames;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hangar|Layer3_CommandConsoles")
	TArray<TObjectPtr<UStaticMeshComponent>> ConsoleMonitorScreens;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hangar|Layer3_CommandConsoles")
	TArray<TObjectPtr<UStaticMeshComponent>> ConsoleStatusLEDs;

	// Layer 4: Overhead Gantry, Pipes & Ceiling Skylights
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hangar|Layer4_OverheadGantry")
	TArray<TObjectPtr<UStaticMeshComponent>> OverheadGantryMeshes;

	// Layer 5: Deep Background Wall & Atmospheric Terminus
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hangar|Layer5_DeepBulkhead")
	TArray<TObjectPtr<UStaticMeshComponent>> DeepBulkheadMeshes;

	// Section 5: "Rainburst on Glass" Forefront Rain Streaks & Falling Water Droplets
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hangar|RainFX")
	TArray<TObjectPtr<UStaticMeshComponent>> ForefrontRainStreaks;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hangar|RainFX")
	TArray<TObjectPtr<UStaticMeshComponent>> ForefrontRainDroplets;

	// -------------------------------------------------------------------------
	// Section 4: Cinematic Three-Tone Lighting Rig
	// -------------------------------------------------------------------------
	/** 1A. Primary Left-Rear Warm Sodium-Amber SpotLight shooting through slanted storm windows */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hangar|Lighting")
	TObjectPtr<USpotLightComponent> WarmWindowKeySpotLight;

	/** 1B. Left-Rear Warm Amber Rim & Floor Specular Ribbon Light */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hangar|Lighting")
	TObjectPtr<USpotLightComponent> WarmOperatorRimSpotLight;

	/** 1C. Exterior Industrial Fire & Window Area RectLight */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hangar|Lighting")
	TObjectPtr<URectLightComponent> WarmWindowRectLight;

	/** 1D. Exterior Fire Backdrop Illumination PointLight */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hangar|Lighting")
	TObjectPtr<UPointLightComponent> ExteriorAmberGlowLight;

	/** 2A. Cool Tactical Overhead Fill RectLight (#1B3B52 to #2A5C7A) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hangar|Lighting")
	TObjectPtr<URectLightComponent> CoolOverheadRectLight;

	/** 2B. Hangar Interior SkyLight (Dark Navy lower hemisphere) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hangar|Lighting")
	TObjectPtr<USkyLightComponent> HangarSkyLight;

	/** 3A. Right-Rear Cyan Command Console Bounce & Floor Reflection Light (#00DDF5) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hangar|Lighting")
	TObjectPtr<UPointLightComponent> CyanConsoleBounceLight;

	/** 3B. Right-Rear Cyan Operator Left-Arm Rim Counter-Light (#00DDF5) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hangar|Lighting")
	TObjectPtr<USpotLightComponent> CyanOperatorRimSpotLight;

	/** 3C. Close-range Monitor Screen Self-Illumination Light (boosts #00DDF5 screen pixels to 3.5x HDR) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hangar|Lighting")
	TObjectPtr<URectLightComponent> CyanScreenIllumRectLight;

	/** 3D. Left-Midground Crate Cyan LED Accent Light */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hangar|Lighting")
	TObjectPtr<UPointLightComponent> LeftCrateCyanPipLight;

	/** 4. Operator Front Readability Fill SpotLight (#C8DCF0, subtle low intensity) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hangar|Lighting")
	TObjectPtr<USpotLightComponent> OperatorFrontFillSpotLight;

	/** 4B. Rear Hangar Bulkhead Architectural Wash Light */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hangar|Lighting")
	TObjectPtr<UPointLightComponent> BulkheadWashLight;

	/** 5. Exponential Height Fog + Volumetric Fog (#040A12) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hangar|Atmosphere")
	TObjectPtr<UExponentialHeightFogComponent> HangarHeightFog;

	/** 6. Locked Manual Exposure, Physical DoF & Color Grading Post-Process Volume */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hangar|PostProcess")
	TObjectPtr<UPostProcessComponent> HangarPostProcess;

protected:
	virtual void BeginPlay() override;

private:
	// Modular Construction Helpers
	void ConstructLayer1FloorAndPlinth(UStaticMesh* FloorMesh, UStaticMesh* CubeMesh, UStaticMesh* CylinderMesh, UMaterialInterface* SteelMat);
	void ConstructLayer2LeftStormWindows(UStaticMesh* CubeMesh, UStaticMesh* CylinderMesh, UMaterialInterface* SteelMat);
	void ConstructLayer3RightCommandConsoles(UStaticMesh* CubeMesh, UStaticMesh* CylinderMesh, UMaterialInterface* SteelMat);
	void ConstructLayer4OverheadGantry(UStaticMesh* CubeMesh, UStaticMesh* CylinderMesh, UMaterialInterface* SteelMat);
	void ConstructLayer5DeepBackgroundBulkhead(UStaticMesh* CubeMesh, UStaticMesh* CylinderMesh, UMaterialInterface* SteelMat);
	void ConstructRainburstOpticsAndParticles(UStaticMesh* CubeMesh, UStaticMesh* SphereMesh, UMaterialInterface* BaseMat);
	void ConstructThreeToneLightingRig();
	void BuildAndApplyProceduralPBRMaterials();

	// Runtime Procedural PBR Texture Generators
	static UTexture2D* CreateTransientPBRTexture(int32 Width, int32 Height, const TArray<FColor>& Pixels, bool bSRGB, bool bIsNormalMap);
	static void GenerateWetHangarFloorTextures(TArray<FColor>& OutMask, TArray<FColor>& OutNormal, TArray<FColor>& OutRoughness);
	static void GenerateRainGlassTextures(TArray<FColor>& OutMask, TArray<FColor>& OutNormal, TArray<FColor>& OutRoughness);
	static void GenerateTerminalScreenTextures(int32 ScreenVariant, TArray<FColor>& OutMask, TArray<FColor>& OutNormal, TArray<FColor>& OutRoughness);

	// Dynamic Material Instances kept alive for runtime animation
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> WetFloorMID;

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> RainWindowMIDs;

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> TerminalScreenMIDs;

	UPROPERTY()
	TWeakObjectPtr<AFPSCharacterPreviewActor> LinkedPreviewActor;

	// Animation state for rain droplets, window rivulets, and industrial fire/terminal flicker
	float StageAnimTime = 0.0f;
	bool bUltraGIActive = false;

	struct FRainDropTrack
	{
		FVector BaseLocation;
		float FallSpeed;
		float MinZ;
		float MaxZ;
		float PhaseOffset;
	};
	TArray<FRainDropTrack> RainDropletTracks;
	TArray<FRainDropTrack> RainStreakTracks;
};
