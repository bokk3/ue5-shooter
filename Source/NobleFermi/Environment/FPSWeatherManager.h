#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FPSWeatherManager.generated.h"

class UInstancedStaticMeshComponent;
class UAudioComponent;
class USoundBase;

UENUM(BlueprintType)
enum class EFPSWeatherState : uint8
{
	ClearDaylight UMETA(DisplayName = "Clear Daylight"),
	NorthSeaStorm UMETA(DisplayName = "North Sea Storm"),
	TwilightRain UMETA(DisplayName = "Twilight Maritime Rain"),
	HangarNightHaze UMETA(DisplayName = "Hangar Night Haze")
};

USTRUCT(BlueprintType)
struct FFPSRainParticleInstance
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	FVector RelativeOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float FallSpeed = 2200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float LengthScale = 1.0f;
};

/**
 * AFPSWeatherManager
 *
 * Authoritative Maritime Storm & Atmospheric Weather Controller for NobleFermi.
 * Powers volumetric rain streaks, camera-relative precipitation volumes, wind vectors,
 * and dynamic surface wetness matching the target benchmark in Docs/Screenshots/aim/.
 */
UCLASS()
class NOBLEFERMI_API AFPSWeatherManager : public AActor
{
	GENERATED_BODY()

public:
	AFPSWeatherManager();

	virtual void Tick(float DeltaTime) override;
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weather|Scene")
	TObjectPtr<USceneComponent> SceneRoot;

	/** High-performance instanced rain streaks volume attached to player camera frustum */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weather|Precipitation")
	TObjectPtr<UInstancedStaticMeshComponent> RainStreaksISM;

	/** Ambient storm downpour audio component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weather|Audio")
	TObjectPtr<UAudioComponent> StormAudioComp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Audio")
	TObjectPtr<USoundBase> RainAmbienceSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|State")
	EFPSWeatherState CurrentWeatherState = EFPSWeatherState::NorthSeaStorm;

	/** 0.0 (bone dry) to 1.0 (torrential downpour) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Parameters", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RainIntensity = 0.85f;

	/** 0.0 (dry materials) to 1.0 (water-saturated glossy surfaces and puddles) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Parameters", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float GlobalWetness = 0.90f;

	/** Prevailing wind direction and magnitude */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Wind")
	FVector WindDirection = FVector(0.707f, 0.45f, -0.15f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Wind")
	float WindSpeed = 850.0f; // cm/s

	/** Number of camera-relative instanced rain streaks (default 256 for smooth 60fps) */
	UPROPERTY(EditDefaultsOnly, Category = "Weather|Precipitation")
	int32 NumRainInstances = 256;

	/** Extents of the camera precipitation box (X=forward, Y=lateral, Z=vertical) */
	UPROPERTY(EditDefaultsOnly, Category = "Weather|Precipitation")
	FVector RainVolumeExtents = FVector(1400.0f, 1600.0f, 900.0f);

	UFUNCTION(BlueprintCallable, Category = "Weather")
	void SetWeatherPreset(EFPSWeatherState NewState);

private:
	void InitializeRainInstances();
	void UpdateRainInstances(float DeltaTime);

	TArray<FFPSRainParticleInstance> RainParticleData;
	float ElapsedTime = 0.0f;
};
