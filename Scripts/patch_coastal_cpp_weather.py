import os

cpp_path = os.path.abspath('Source/NobleFermi/Map/FPSCoastalOperationsMapActor.cpp')
with open(cpp_path, 'r', encoding='utf-8') as f:
    text = f.read()

# Add includes
inc_target = '#include "Components/PointLightComponent.h"'
inc_replace = '''#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Environment/FPSWeatherManager.h"'''

assert inc_target in text, "inc_target not found in FPSCoastalOperationsMapActor.cpp"
text = text.replace(inc_target, inc_replace)

# Replace lighting & post-processing setup
light_target = """	// 1. Cinematic Golden-Hour Brazilian Coastline Favela Lighting
	SunLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("SunLight"));
	SunLight->SetupAttachment(RootComponent);
	SunLight->SetMobility(EComponentMobility::Movable);
	SunLight->SetRelativeRotation(FRotator(-32.0f, 55.0f, 0.0f));
	SunLight->SetIntensity(11.5f); // Tropical maritime sun lux
	SunLight->SetLightColor(FLinearColor(1.0f, 0.94f, 0.84f)); // Golden tropical sun
	SunLight->bAtmosphereSunLight = true;
	SunLight->bCastVolumetricShadow = false;
	SunLight->CastShadows = 1;

	SkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
	SkyLight->SetupAttachment(RootComponent);
	SkyLight->SetMobility(EComponentMobility::Movable);
	SkyLight->SetIntensity(3.6f);
	SkyLight->SetLightColor(FLinearColor(0.82f, 0.90f, 1.0f)); // Tropical sky ambient
	SkyLight->bRealTimeCapture = true;

	SkyAtmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("SkyAtmosphere"));
	SkyAtmosphere->SetupAttachment(RootComponent);

	HeightFog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("HeightFog"));
	HeightFog->SetupAttachment(RootComponent);
	HeightFog->SetFogDensity(0.0012f);
	HeightFog->SetFogInscatteringColor(FLinearColor(0.82f, 0.88f, 0.96f)); // Volumetric maritime sea-mist

	PierHeadNavBeacon = CreateDefaultSubobject<UPointLightComponent>(TEXT("PierHeadNavBeacon"));
	PierHeadNavBeacon->SetupAttachment(RootComponent);
	PierHeadNavBeacon->SetMobility(EComponentMobility::Movable);
	PierHeadNavBeacon->SetRelativeLocation(FVector(0.0f, -5800.0f, 450.0f));
	PierHeadNavBeacon->SetIntensity(25.0f);
	PierHeadNavBeacon->SetLightColor(FLinearColor(0.0f, 0.90f, 1.0f)); // Tactical cyan navigational beacon
	PierHeadNavBeacon->SetAttenuationRadius(2400.0f);
	PierHeadNavBeacon->SetCastShadows(false);

	DikeHazardBeacon = CreateDefaultSubobject<UPointLightComponent>(TEXT("DikeHazardBeacon"));
	DikeHazardBeacon->SetupAttachment(RootComponent);
	DikeHazardBeacon->SetMobility(EComponentMobility::Movable);
	DikeHazardBeacon->SetRelativeLocation(FVector(0.0f, -720.0f, 620.0f));
	DikeHazardBeacon->SetIntensity(30.0f);
	DikeHazardBeacon->SetLightColor(FLinearColor(1.0f, 0.25f, 0.05f)); // Amber hazard beacon on Dike Flag B
	DikeHazardBeacon->SetAttenuationRadius(2000.0f);
	DikeHazardBeacon->SetCastShadows(false);

	// Post Process: Balanced Exposure to showcase coastal favela textures
	PostProcess = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcess"));
	PostProcess->SetupAttachment(RootComponent);
	PostProcess->bUnbound = true;
	PostProcess->Settings.bOverride_AutoExposureMethod = true;
	PostProcess->Settings.AutoExposureMethod = EAutoExposureMethod::AEM_Basic;
	PostProcess->Settings.bOverride_AutoExposureBias = true;
	PostProcess->Settings.AutoExposureBias = 0.5f;
	PostProcess->Settings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
	PostProcess->Settings.AutoExposureApplyPhysicalCameraExposure = 0;
	PostProcess->Settings.bOverride_AutoExposureMinBrightness = true;
	PostProcess->Settings.AutoExposureMinBrightness = 0.35f;
	PostProcess->Settings.bOverride_AutoExposureMaxBrightness = true;
	PostProcess->Settings.AutoExposureMaxBrightness = 3.0f;
	PostProcess->Settings.bOverride_BloomIntensity = true;
	PostProcess->Settings.BloomIntensity = 0.15f;"""

light_replace = """	// 1. Cinematic Overcast North Sea Storm Atmosphere (Docs/Screenshots/aim/coastal_operations_gameplay.jpg)
	SunLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("SunLight"));
	SunLight->SetupAttachment(RootComponent);
	SunLight->SetMobility(EComponentMobility::Movable);
	SunLight->SetRelativeRotation(FRotator(-22.0f, 65.0f, 0.0f));
	SunLight->SetIntensity(3.5f); // Cool overcast daylight lux
	SunLight->SetLightColor(FLinearColor(0.72f, 0.80f, 0.90f)); // Overcast slate-silver sun
	SunLight->bAtmosphereSunLight = true;
	SunLight->bCastVolumetricShadow = true;
	SunLight->SetVolumetricScatteringIntensity(2.8f);
	SunLight->CastShadows = 1;

	SkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
	SkyLight->SetupAttachment(RootComponent);
	SkyLight->SetMobility(EComponentMobility::Movable);
	SkyLight->SetIntensity(2.2f);
	SkyLight->SetLightColor(FLinearColor(0.55f, 0.65f, 0.78f)); // Deep maritime slate ambient
	SkyLight->bRealTimeCapture = true;
	SkyLight->bLowerHemisphereIsBlack = false;
	SkyLight->LowerHemisphereColor = FLinearColor(0.08f, 0.10f, 0.14f, 1.0f);

	SkyAtmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("SkyAtmosphere"));
	SkyAtmosphere->SetupAttachment(RootComponent);

	HeightFog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("HeightFog"));
	HeightFog->SetupAttachment(RootComponent);
	HeightFog->SetFogDensity(0.022f); // Moody maritime storm mist
	HeightFog->SetFogHeightFalloff(0.0015f);
	HeightFog->SetFogInscatteringColor(FLinearColor(0.24f, 0.30f, 0.38f)); // Slate overcast storm cloud
	HeightFog->SetVolumetricFog(true);
	HeightFog->SetVolumetricFogScatteringDistribution(0.68f);
	HeightFog->SetVolumetricFogExtinctionScale(1.8f);
	HeightFog->SetVolumetricFogDistance(7500.0f);
	HeightFog->SetVolumetricFogAlbedo(FColor(170, 195, 220));

	PierHeadNavBeacon = CreateDefaultSubobject<UPointLightComponent>(TEXT("PierHeadNavBeacon"));
	PierHeadNavBeacon->SetupAttachment(RootComponent);
	PierHeadNavBeacon->SetMobility(EComponentMobility::Movable);
	PierHeadNavBeacon->SetRelativeLocation(FVector(0.0f, -5800.0f, 450.0f));
	PierHeadNavBeacon->SetIntensity(45.0f);
	PierHeadNavBeacon->SetLightColor(FLinearColor(0.0f, 0.90f, 1.0f)); // Tactical cyan navigational beacon
	PierHeadNavBeacon->SetAttenuationRadius(3200.0f);
	PierHeadNavBeacon->SetVolumetricScatteringIntensity(3.0f);
	PierHeadNavBeacon->SetCastShadows(false);

	DikeHazardBeacon = CreateDefaultSubobject<UPointLightComponent>(TEXT("DikeHazardBeacon"));
	DikeHazardBeacon->SetupAttachment(RootComponent);
	DikeHazardBeacon->SetMobility(EComponentMobility::Movable);
	DikeHazardBeacon->SetRelativeLocation(FVector(0.0f, -720.0f, 620.0f));
	DikeHazardBeacon->SetIntensity(50.0f);
	DikeHazardBeacon->SetLightColor(FLinearColor(1.0f, 0.45f, 0.08f)); // Amber hazard beacon on Dike Flag B
	DikeHazardBeacon->SetAttenuationRadius(2500.0f);
	DikeHazardBeacon->SetVolumetricScatteringIntensity(3.5f);
	DikeHazardBeacon->SetCastShadows(false);

	// Seawall Stanchion Floodlights with conical volumetric light beams
	for (int32 i = 0; i < 5; ++i)
	{
		const float FloodX = -1800.0f + i * 900.0f;
		USpotLightComponent* Flood = CreateDefaultSubobject<USpotLightComponent>(*FString::Printf(TEXT("SeawallFlood_%d"), i));
		Flood->SetupAttachment(RootComponent);
		Flood->SetMobility(EComponentMobility::Movable);
		Flood->SetRelativeLocation(FVector(FloodX, -520.0f, 580.0f));
		Flood->SetRelativeRotation(FRotator(-40.0f, -90.0f, 0.0f));
		Flood->SetIntensity(6500.0f);
		Flood->SetLightColor(FLinearColor(0.92f, 0.96f, 1.0f));
		Flood->SetAttenuationRadius(2200.0f);
		Flood->SetInnerConeAngle(30.0f);
		Flood->SetOuterConeAngle(55.0f);
		Flood->SetVolumetricScatteringIntensity(3.8f);
		Flood->SetCastShadows(true);
		SeawallFloodlights.Add(Flood);
	}

	// Post Process: High-contrast bodycam / cinematic storm grading
	PostProcess = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcess"));
	PostProcess->SetupAttachment(RootComponent);
	PostProcess->bUnbound = true;
	PostProcess->Settings.bOverride_AutoExposureMethod = true;
	PostProcess->Settings.AutoExposureMethod = EAutoExposureMethod::AEM_Basic;
	PostProcess->Settings.bOverride_AutoExposureBias = true;
	PostProcess->Settings.AutoExposureBias = 0.15f;
	PostProcess->Settings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
	PostProcess->Settings.AutoExposureApplyPhysicalCameraExposure = 0;
	PostProcess->Settings.bOverride_AutoExposureMinBrightness = true;
	PostProcess->Settings.AutoExposureMinBrightness = 0.40f;
	PostProcess->Settings.bOverride_AutoExposureMaxBrightness = true;
	PostProcess->Settings.AutoExposureMaxBrightness = 2.2f;
	PostProcess->Settings.bOverride_BloomIntensity = true;
	PostProcess->Settings.BloomIntensity = 0.22f;
	PostProcess->Settings.bOverride_FilmSaturation = true;
	PostProcess->Settings.FilmSaturation = 0.84f; // Gritty desaturated military realism
	PostProcess->Settings.bOverride_FilmContrast = true;
	PostProcess->Settings.FilmContrast = 1.18f; // Deep rich blacks
	PostProcess->Settings.bOverride_FilmShadowTint = true;
	PostProcess->Settings.FilmShadowTint = FLinearColor(0.92f, 0.95f, 1.05f); // Cool shadows
	PostProcess->Settings.bOverride_VignetteIntensity = true;
	PostProcess->Settings.VignetteIntensity = 0.35f;
	PostProcess->Settings.bOverride_AmbientOcclusionIntensity = true;
	PostProcess->Settings.AmbientOcclusionIntensity = 0.85f;
	PostProcess->Settings.bOverride_AmbientOcclusionRadius = true;
	PostProcess->Settings.AmbientOcclusionRadius = 120.0f;"""

assert light_target in text, "light_target not found in FPSCoastalOperationsMapActor.cpp"
text = text.replace(light_target, light_replace)

# In BeginPlay, spawn WeatherManager
begin_target = """void AFPSCoastalOperationsMapActor::BeginPlay()
{
	Super::BeginPlay();

	if (SkyLight)
	{
		SkyLight->RecaptureSky();
	}"""

begin_replace = """void AFPSCoastalOperationsMapActor::BeginPlay()
{
	Super::BeginPlay();

	if (SkyLight)
	{
		SkyLight->RecaptureSky();
	}

	// Spawn Dynamic Storm Weather Manager
	if (GetWorld() && !WeatherManager)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		WeatherManager = GetWorld()->SpawnActor<AFPSWeatherManager>(AFPSWeatherManager::StaticClass(), GetActorLocation(), FRotator::ZeroRotator, SpawnParams);
		if (WeatherManager)
		{
			WeatherManager->SetWeatherPreset(EFPSWeatherState::NorthSeaStorm);
		}
	}"""

assert begin_target in text, "begin_target not found in FPSCoastalOperationsMapActor.cpp"
text = text.replace(begin_target, begin_replace)

with open(cpp_path, 'w', encoding='utf-8') as f:
    f.write(text)

print("SUCCESS: Patched FPSCoastalOperationsMapActor.cpp with North Sea Storm lighting, floodlights, and WeatherManager!")
