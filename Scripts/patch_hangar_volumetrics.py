import os

cpp_path = os.path.abspath('Source/NobleFermi/Game/FPSMenuHangarStage.cpp')
with open(cpp_path, 'r', encoding='utf-8') as f:
    text = f.read()

fog_target = """	HangarHeightFog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("HangarHeightFog"));
	HangarHeightFog->SetupAttachment(RootComponent);
	HangarHeightFog->SetFogDensity(0.00002f);
	HangarHeightFog->SetFogHeightFalloff(0.25f);
	HangarHeightFog->SetFogInscatteringColor(FLinearColor(0.016f, 0.039f, 0.071f)); // #040A12
	HangarHeightFog->SetVolumetricFog(false); // Enabled dynamically when Ultra GI mode is active
	HangarHeightFog->SetVolumetricFogScatteringDistribution(0.65f);
	HangarHeightFog->SetVolumetricFogAlbedo(FColor(28, 42, 58));
	HangarHeightFog->SetVolumetricFogExtinctionScale(1.8f);"""

fog_replace = """	HangarHeightFog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("HangarHeightFog"));
	HangarHeightFog->SetupAttachment(RootComponent);
	HangarHeightFog->SetFogDensity(0.0028f); // Atmospheric industrial haze
	HangarHeightFog->SetFogHeightFalloff(0.08f);
	HangarHeightFog->SetFogInscatteringColor(FLinearColor(0.02f, 0.05f, 0.08f)); // Deep industrial teal
	HangarHeightFog->SetVolumetricFog(true); // Active Volumetric Fog in Hangar!
	HangarHeightFog->SetVolumetricFogScatteringDistribution(0.68f);
	HangarHeightFog->SetVolumetricFogAlbedo(FColor(35, 60, 85));
	HangarHeightFog->SetVolumetricFogExtinctionScale(1.8f);
	HangarHeightFog->SetVolumetricFogDistance(4500.0f);"""

assert fog_target in text, "fog_target not found in FPSMenuHangarStage.cpp"
text = text.replace(fog_target, fog_replace)

# Boost scattering on key spot and rim lights
key_target = "WarmWindowKeySpotLight->SetVolumetricScatteringIntensity(1.5f);"
key_replace = "WarmWindowKeySpotLight->SetVolumetricScatteringIntensity(3.5f);"
text = text.replace(key_target, key_replace)

rim_target = """	CyanOperatorRimSpotLight->SetIntensity(2400.0f);
	CyanOperatorRimSpotLight->SetLightColor(FLinearColor(0.0f, 0.867f, 0.961f)); // #00DDF5
	CyanOperatorRimSpotLight->SetAttenuationRadius(720.0f);
	CyanOperatorRimSpotLight->SetInnerConeAngle(25.0f);
	CyanOperatorRimSpotLight->SetOuterConeAngle(56.0f);
	CyanOperatorRimSpotLight->SetCastShadows(false);"""

rim_replace = """	CyanOperatorRimSpotLight->SetIntensity(2800.0f);
	CyanOperatorRimSpotLight->SetLightColor(FLinearColor(0.0f, 0.867f, 0.961f)); // #00DDF5
	CyanOperatorRimSpotLight->SetAttenuationRadius(850.0f);
	CyanOperatorRimSpotLight->SetInnerConeAngle(25.0f);
	CyanOperatorRimSpotLight->SetOuterConeAngle(56.0f);
	CyanOperatorRimSpotLight->SetVolumetricScatteringIntensity(3.2f);
	CyanOperatorRimSpotLight->SetCastShadows(false);"""

assert rim_target in text, "rim_target not found in FPSMenuHangarStage.cpp"
text = text.replace(rim_target, rim_replace)

with open(cpp_path, 'w', encoding='utf-8') as f:
    f.write(text)

print("SUCCESS: Patched FPSMenuHangarStage.cpp with volumetric fog and scattering lights!")
