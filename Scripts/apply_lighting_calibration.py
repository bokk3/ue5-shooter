import os
import re

print("=== APPLYING LIGHTING CALIBRATION WITH CRLF ENFORCEMENT ===")

preview_path = "Source/NobleFermi/Character/FPSCharacterPreviewActor.cpp"
hangar_path = "Source/NobleFermi/Game/FPSMenuHangarStage.cpp"

# 1. Patch FPSCharacterPreviewActor.cpp
with open(preview_path, 'r', encoding='utf-8') as f:
    preview_content = f.read()

# Normalize to LF for matching
preview_lf = preview_content.replace('\r\n', '\n')

old_preview_lights = """	PreviewCamera->PostProcessSettings.bOverride_AutoExposureMethod = true;
	PreviewCamera->PostProcessSettings.AutoExposureMethod = EAutoExposureMethod::AEM_Histogram;
	PreviewCamera->PostProcessSettings.bOverride_AutoExposureBias = true;
	PreviewCamera->PostProcessSettings.AutoExposureBias = 0.8f;
	PreviewCamera->PostProcessSettings.bOverride_AutoExposureMinBrightness = true;
	PreviewCamera->PostProcessSettings.AutoExposureMinBrightness = 0.5f;
	PreviewCamera->PostProcessSettings.bOverride_AutoExposureMaxBrightness = true;
	PreviewCamera->PostProcessSettings.AutoExposureMaxBrightness = 8.0f;
	PreviewCamera->PostProcessSettings.bOverride_BloomIntensity = true;
	PreviewCamera->PostProcessSettings.BloomIntensity = 0.2f;

	// 3. Studio Lighting: 3-point tactical dynamic lighting + directional sun
	StudioKeyLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("StudioKeyLight"));
	StudioKeyLight->SetupAttachment(RootComponent);
	StudioKeyLight->SetMobility(EComponentMobility::Movable);
	StudioKeyLight->SetRelativeLocation(FVector(190.0f, -120.0f, 210.0f));
	StudioKeyLight->SetRelativeRotation(FRotator(-35.0f, 145.0f, 0.0f));
	StudioKeyLight->SetIntensity(75000.0f);
	StudioKeyLight->SetLightColor(FLinearColor(1.0f, 0.78f, 0.40f)); // Warm amber key light
	StudioKeyLight->SetAttenuationRadius(1500.0f);
	StudioKeyLight->SetInnerConeAngle(25.0f);
	StudioKeyLight->SetOuterConeAngle(65.0f);

	StudioRimLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("StudioRimLight"));
	StudioRimLight->SetupAttachment(RootComponent);
	StudioRimLight->SetMobility(EComponentMobility::Movable);
	StudioRimLight->SetRelativeLocation(FVector(-160.0f, 120.0f, 175.0f));
	StudioRimLight->SetRelativeRotation(FRotator(-25.0f, -35.0f, 0.0f));
	StudioRimLight->SetIntensity(55000.0f);
	StudioRimLight->SetLightColor(FLinearColor(0.0f, 0.898f, 1.0f)); // Tactical cyan rim light (#00E5FF)
	StudioRimLight->SetAttenuationRadius(1400.0f);
	StudioRimLight->SetInnerConeAngle(30.0f);
	StudioRimLight->SetOuterConeAngle(70.0f);
	StudioRimLight->SetCastShadows(false);

	StudioFillLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("StudioFillLight"));
	StudioFillLight->SetupAttachment(RootComponent);
	StudioFillLight->SetMobility(EComponentMobility::Movable);
	StudioFillLight->SetRelativeLocation(FVector(150.0f, 0.0f, 120.0f));
	StudioFillLight->SetIntensity(65000.0f);
	StudioFillLight->SetLightColor(FLinearColor(0.85f, 0.92f, 1.0f)); // Clean fill
	StudioFillLight->SetAttenuationRadius(1500.0f);
	StudioFillLight->SetCastShadows(false);

	StudioAmbientLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("StudioAmbientLight"));
	StudioAmbientLight->SetupAttachment(RootComponent);
	StudioAmbientLight->SetMobility(EComponentMobility::Movable);
	StudioAmbientLight->SetRelativeLocation(FVector(0.0f, 0.0f, 150.0f));
	StudioAmbientLight->SetIntensity(10000.0f);
	StudioAmbientLight->SetLightColor(FLinearColor(0.35f, 0.45f, 0.55f));
	StudioAmbientLight->SetAttenuationRadius(1500.0f);
	StudioAmbientLight->SetCastShadows(false);

	StudioSunLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("StudioSunLight"));
	StudioSunLight->SetupAttachment(RootComponent);
	StudioSunLight->SetMobility(EComponentMobility::Movable);
	StudioSunLight->SetRelativeRotation(FRotator(-30.0f, 165.0f, 0.0f));
	StudioSunLight->SetIntensity(30000.0f); // 30,000 Lux Studio Direct Illumination
	StudioSunLight->SetLightColor(FLinearColor(1.0f, 0.95f, 0.88f));
	StudioSunLight->SetCastShadows(false);"""

new_preview_lights = """	PreviewCamera->PostProcessSettings.bOverride_AutoExposureMethod = true;
	PreviewCamera->PostProcessSettings.AutoExposureMethod = EAutoExposureMethod::AEM_Histogram;
	PreviewCamera->PostProcessSettings.bOverride_AutoExposureBias = true;
	PreviewCamera->PostProcessSettings.AutoExposureBias = 0.0f;
	PreviewCamera->PostProcessSettings.bOverride_AutoExposureMinBrightness = false;
	PreviewCamera->PostProcessSettings.bOverride_AutoExposureMaxBrightness = false;
	PreviewCamera->PostProcessSettings.bOverride_BloomIntensity = true;
	PreviewCamera->PostProcessSettings.BloomIntensity = 0.25f;

	// 3. Studio Lighting: 3-point tactical dynamic lighting
	StudioKeyLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("StudioKeyLight"));
	StudioKeyLight->SetupAttachment(RootComponent);
	StudioKeyLight->SetMobility(EComponentMobility::Movable);
	StudioKeyLight->SetRelativeLocation(FVector(190.0f, -120.0f, 210.0f));
	StudioKeyLight->SetRelativeRotation(FRotator(-35.0f, 145.0f, 0.0f));
	StudioKeyLight->SetIntensity(1500.0f);
	StudioKeyLight->SetLightColor(FLinearColor(1.0f, 0.78f, 0.40f)); // Warm amber key light
	StudioKeyLight->SetAttenuationRadius(1500.0f);
	StudioKeyLight->SetInnerConeAngle(25.0f);
	StudioKeyLight->SetOuterConeAngle(65.0f);

	StudioRimLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("StudioRimLight"));
	StudioRimLight->SetupAttachment(RootComponent);
	StudioRimLight->SetMobility(EComponentMobility::Movable);
	StudioRimLight->SetRelativeLocation(FVector(-160.0f, 120.0f, 175.0f));
	StudioRimLight->SetRelativeRotation(FRotator(-25.0f, -35.0f, 0.0f));
	StudioRimLight->SetIntensity(1800.0f);
	StudioRimLight->SetLightColor(FLinearColor(0.0f, 0.898f, 1.0f)); // Tactical cyan rim light (#00E5FF)
	StudioRimLight->SetAttenuationRadius(1400.0f);
	StudioRimLight->SetInnerConeAngle(30.0f);
	StudioRimLight->SetOuterConeAngle(70.0f);
	StudioRimLight->SetCastShadows(false);

	StudioFillLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("StudioFillLight"));
	StudioFillLight->SetupAttachment(RootComponent);
	StudioFillLight->SetMobility(EComponentMobility::Movable);
	StudioFillLight->SetRelativeLocation(FVector(150.0f, 0.0f, 120.0f));
	StudioFillLight->SetIntensity(600.0f);
	StudioFillLight->SetLightColor(FLinearColor(0.85f, 0.92f, 1.0f)); // Clean fill
	StudioFillLight->SetAttenuationRadius(1500.0f);
	StudioFillLight->SetCastShadows(false);

	StudioAmbientLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("StudioAmbientLight"));
	StudioAmbientLight->SetupAttachment(RootComponent);
	StudioAmbientLight->SetMobility(EComponentMobility::Movable);
	StudioAmbientLight->SetRelativeLocation(FVector(0.0f, 0.0f, 150.0f));
	StudioAmbientLight->SetIntensity(200.0f);
	StudioAmbientLight->SetLightColor(FLinearColor(0.35f, 0.45f, 0.55f));
	StudioAmbientLight->SetAttenuationRadius(1500.0f);
	StudioAmbientLight->SetCastShadows(false);

	StudioSunLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("StudioSunLight"));
	StudioSunLight->SetupAttachment(RootComponent);
	StudioSunLight->SetMobility(EComponentMobility::Movable);
	StudioSunLight->SetRelativeRotation(FRotator(-30.0f, 165.0f, 0.0f));
	StudioSunLight->SetIntensity(0.0f); // Disabled inside indoor hangar stage
	StudioSunLight->SetLightColor(FLinearColor(1.0f, 0.95f, 0.88f));
	StudioSunLight->SetCastShadows(false);"""

assert old_preview_lights in preview_lf, "old_preview_lights pattern not found in FPSCharacterPreviewActor.cpp!"
preview_lf = preview_lf.replace(old_preview_lights, new_preview_lights)

# Ensure 100% CRLF
preview_crlf = preview_lf.replace('\n', '\r\n')
with open(preview_path, 'wb') as f:
    f.write(preview_crlf.encode('utf-8'))
print("Successfully patched FPSCharacterPreviewActor.cpp with calibrated lighting!")


# 2. Patch FPSMenuHangarStage.cpp
with open(hangar_path, 'r', encoding='utf-8') as f:
    hangar_content = f.read()

hangar_lf = hangar_content.replace('\r\n', '\n')

# Patch ThreeToneLightingRig
old_hangar_rig = """	WarmWindowKeySpotLight->SetIntensity(68000.0f);
	WarmWindowKeySpotLight->SetLightColor(FLinearColor(1.0f, 0.52f, 0.16f)); // Saturated Fiery Sodium Amber (#FF7A1A)
	WarmWindowKeySpotLight->SetAttenuationRadius(2800.0f);
	WarmWindowKeySpotLight->SetInnerConeAngle(22.0f);
	WarmWindowKeySpotLight->SetOuterConeAngle(52.0f);
	WarmWindowKeySpotLight->SetVolumetricScatteringIntensity(2.5f);
	WarmWindowKeySpotLight->SetCastShadows(true);

	WarmOperatorRimSpotLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("WarmOperatorRimSpotLight"));
	WarmOperatorRimSpotLight->SetupAttachment(RootComponent);
	WarmOperatorRimSpotLight->SetMobility(EComponentMobility::Movable);
	// Positioned at Left-Rear (+Y, -X) pointing at Operator's right helmet/shoulder/leg and wet floor
	WarmOperatorRimSpotLight->SetRelativeLocation(FVector(-145.0f, 165.0f, 168.0f));
	WarmOperatorRimSpotLight->SetRelativeRotation(FRotator(-22.0f, -46.0f, 0.0f));
	WarmOperatorRimSpotLight->SetIntensity(28000.0f);
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
	WarmWindowRectLight->SetIntensity(28000.0f);
	WarmWindowRectLight->SetLightColor(FLinearColor(1.0f, 0.48f, 0.12f));
	WarmWindowRectLight->SetSourceWidth(320.0f);
	WarmWindowRectLight->SetSourceHeight(200.0f);
	WarmWindowRectLight->SetAttenuationRadius(2200.0f);
	WarmWindowRectLight->SetCastShadows(false);

	ExteriorAmberGlowLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("ExteriorAmberGlowLight"));
	ExteriorAmberGlowLight->SetupAttachment(RootComponent);
	ExteriorAmberGlowLight->SetMobility(EComponentMobility::Movable);
	ExteriorAmberGlowLight->SetRelativeLocation(FVector(-540.0f, 310.0f, 140.0f));
	ExteriorAmberGlowLight->SetIntensity(36000.0f);
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
	CoolOverheadRectLight->SetIntensity(24000.0f);
	CoolOverheadRectLight->SetLightColor(FLinearColor(0.20f, 0.38f, 0.54f)); // Deep Steel Blue (#2A5C7A)
	CoolOverheadRectLight->SetSourceWidth(400.0f);
	CoolOverheadRectLight->SetSourceHeight(300.0f);
	CoolOverheadRectLight->SetAttenuationRadius(2600.0f);
	CoolOverheadRectLight->SetCastShadows(false);

	HangarSkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("HangarSkyLight"));
	HangarSkyLight->SetupAttachment(RootComponent);
	HangarSkyLight->SetMobility(EComponentMobility::Movable);
	HangarSkyLight->SetIntensity(5.5f);
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
	CyanConsoleBounceLight->SetIntensity(26000.0f);
	CyanConsoleBounceLight->SetLightColor(FLinearColor(0.0f, 0.867f, 0.961f)); // Noble Fermi Cyan (#00DDF5)
	CyanConsoleBounceLight->SetAttenuationRadius(2000.0f);
	CyanConsoleBounceLight->SetCastShadows(false);

	CyanOperatorRimSpotLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("CyanOperatorRimSpotLight"));
	CyanOperatorRimSpotLight->SetupAttachment(RootComponent);
	CyanOperatorRimSpotLight->SetMobility(EComponentMobility::Movable);
	// Positioned behind Operator's left side (-Y) casting cool turquoise rim on left arm/vest & right floor puddles
	CyanOperatorRimSpotLight->SetRelativeLocation(FVector(-150.0f, -135.0f, 148.0f));
	CyanOperatorRimSpotLight->SetRelativeRotation(FRotator(-18.0f, 44.0f, 0.0f));
	CyanOperatorRimSpotLight->SetIntensity(20000.0f);
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
	CyanScreenIllumRectLight->SetIntensity(9500.0f);
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
	LeftCrateCyanPipLight->SetIntensity(1800.0f);
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
	OperatorFrontFillSpotLight->SetIntensity(14000.0f);
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
	BulkheadWashLight->SetIntensity(48000.0f);"""

new_hangar_rig = """	WarmWindowKeySpotLight->SetIntensity(4500.0f);
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
	BulkheadWashLight->SetIntensity(3000.0f);"""

assert old_hangar_rig in hangar_lf, "old_hangar_rig not found in FPSMenuHangarStage.cpp!"
hangar_lf = hangar_lf.replace(old_hangar_rig, new_hangar_rig)

# Patch HangarPostProcess
old_hangar_pp = """	PP.bOverride_AutoExposureMethod = true;
	PP.AutoExposureMethod = EAutoExposureMethod::AEM_Histogram;
	PP.bOverride_AutoExposureBias = true;
	PP.AutoExposureBias = 0.5f;
	PP.bOverride_AutoExposureMinBrightness = true;
	PP.AutoExposureMinBrightness = 0.01f;
	PP.bOverride_AutoExposureMaxBrightness = true;
	PP.AutoExposureMaxBrightness = 10.0f;"""

new_hangar_pp = """	PP.bOverride_AutoExposureMethod = true;
	PP.AutoExposureMethod = EAutoExposureMethod::AEM_Histogram;
	PP.bOverride_AutoExposureBias = true;
	PP.AutoExposureBias = 0.0f;
	PP.bOverride_AutoExposureMinBrightness = false;
	PP.bOverride_AutoExposureMaxBrightness = false;"""

assert old_hangar_pp in hangar_lf, "old_hangar_pp not found in FPSMenuHangarStage.cpp!"
hangar_lf = hangar_lf.replace(old_hangar_pp, new_hangar_pp)

# Patch ConfigurePreviewActorAndCamera
old_configure = """	if (InPreviewActor->StudioKeyLight)
	{
		InPreviewActor->StudioKeyLight->SetRelativeLocation(FVector(110.0f, 155.0f, 185.0f));
		InPreviewActor->StudioKeyLight->SetRelativeRotation(FRotator(-22.0f, -125.0f, 0.0f));
		InPreviewActor->StudioKeyLight->SetIntensity(65000.0f);
		InPreviewActor->StudioKeyLight->SetLightColor(FLinearColor(1.0f, 0.70f, 0.35f)); // Warm Amber
	}

	if (InPreviewActor->StudioRimLight)
	{
		InPreviewActor->StudioRimLight->SetRelativeLocation(FVector(-95.0f, -155.0f, 160.0f));
		InPreviewActor->StudioRimLight->SetRelativeRotation(FRotator(-16.0f, 55.0f, 0.0f));
		InPreviewActor->StudioRimLight->SetIntensity(45000.0f);
		InPreviewActor->StudioRimLight->SetLightColor(FLinearColor(0.0f, 0.898f, 1.0f)); // Tactical Cyan
	}

	if (InPreviewActor->StudioFillLight)
	{
		InPreviewActor->StudioFillLight->SetRelativeLocation(FVector(150.0f, 0.0f, 115.0f));
		InPreviewActor->StudioFillLight->SetIntensity(50000.0f);
		InPreviewActor->StudioFillLight->SetLightColor(FLinearColor(1.0f, 0.96f, 0.90f));
	}

		if (InPreviewActor->StudioAmbientLight)
	{
		InPreviewActor->StudioAmbientLight->SetIntensity(10000.0f);
		InPreviewActor->StudioAmbientLight->SetLightColor(FLinearColor(0.35f, 0.45f, 0.55f));
	}

	if (InPreviewActor->StudioSunLight)
	{
		InPreviewActor->StudioSunLight->SetIntensity(30000.0f);
	}

	// Lock PreviewCamera Exposure & Vignette
	if (InPreviewActor->PreviewCamera)
	{
		FPostProcessSettings& CamPP = InPreviewActor->PreviewCamera->PostProcessSettings;
		CamPP.bOverride_AutoExposureMethod = true;
		CamPP.AutoExposureMethod = EAutoExposureMethod::AEM_Histogram;
		CamPP.bOverride_AutoExposureBias = true;
		CamPP.AutoExposureBias = 0.8f;
		CamPP.bOverride_AutoExposureMinBrightness = true;
		CamPP.AutoExposureMinBrightness = 0.5f;
		CamPP.bOverride_AutoExposureMaxBrightness = true;
		CamPP.AutoExposureMaxBrightness = 8.0f;"""

new_configure = """	if (InPreviewActor->StudioKeyLight)
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
		CamPP.AutoExposureMaxBrightness = 8.0f;"""

assert old_configure in hangar_lf, "old_configure not found in FPSMenuHangarStage.cpp!"
hangar_lf = hangar_lf.replace(old_configure, new_configure)

# Patch WetFloorPlane material application to use SteelBaseMat
old_floor_mat = """	if (WetFloorPlane && MasterMultiMat)
	{
		WetFloorPlane->SetMaterial(0, MasterMultiMat);
		WetFloorMID = WetFloorPlane->CreateDynamicMaterialInstance(0);
		if (WetFloorMID)
		{
			WetFloorMID->SetTextureParameterValue(TEXT("Texture_Camo"), HangarPBRCache::FloorMaskTex);
			WetFloorMID->SetTextureParameterValue(TEXT("Texture_BodyMask"), HangarPBRCache::ZeroBodyMaskTex);
			WetFloorMID->SetTextureParameterValue(TEXT("Texture_RipstopNormal"), HangarPBRCache::FloorNormalTex);
			WetFloorMID->SetTextureParameterValue(TEXT("Texture_RipstopRoughness"), HangarPBRCache::FloorRoughnessTex);

			// Tactical concrete floor & wet specular puddles
			WetFloorMID->SetVectorParameterValue(TEXT("CamoColorPrimary"), FLinearColor(0.12f, 0.16f, 0.20f, 1.0f));
			WetFloorMID->SetVectorParameterValue(TEXT("CamoColorSecondary"), FLinearColor(0.22f, 0.28f, 0.35f, 1.0f));
			WetFloorMID->SetVectorParameterValue(TEXT("CamoColorTertiary"), FLinearColor(0.08f, 0.11f, 0.14f, 1.0f));
			WetFloorMID->SetScalarParameterValue(TEXT("RoughnessBase"), 1.0f);
			WetFloorMID->SetScalarParameterValue(TEXT("SubsurfaceIntensity"), 0.0f);
			WetFloorMID->SetScalarParameterValue(TEXT("CamoTiling"), 14.0f);
			WetFloorMID->SetScalarParameterValue(TEXT("RipstopTiling"), 14.0f);
		}
	}"""

new_floor_mat = """	if (WetFloorPlane)
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
	}"""

assert old_floor_mat in hangar_lf, "old_floor_mat not found in FPSMenuHangarStage.cpp!"
hangar_lf = hangar_lf.replace(old_floor_mat, new_floor_mat)

# Ensure 100% CRLF
hangar_crlf = hangar_lf.replace('\n', '\r\n')
with open(hangar_path, 'wb') as f:
    f.write(hangar_crlf.encode('utf-8'))
print("Successfully patched FPSMenuHangarStage.cpp with calibrated lighting and floor material!")

print("=== ALL LIGHTING & EXPOSURE CALIBRATIONS APPLIED SUCCESSFULLY ===")
