with open("Source/NobleFermi/Character/FPSCharacterPreviewActor.cpp", "r", encoding="utf-8") as f:
    text = f.read()

old_code = """\tRootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
\tRootComponent = RootScene;

\t// 2. Camera: Calibrated to frame the full Task Force Echo Operator & Hangar stage
\tPreviewCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("PreviewCamera"));
\tPreviewCamera->SetupAttachment(RootComponent);
\tPreviewCamera->SetRelativeLocation(FVector(400.0f, 0.0f, 90.0f));
\tPreviewCamera->SetRelativeRotation(FRotator(-1.5f, 180.0f, 0.0f));
\tPreviewCamera->FieldOfView = 50.0f;
\tPreviewCamera->PostProcessSettings.bOverride_AutoExposureMethod = true;
\tPreviewCamera->PostProcessSettings.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
\tPreviewCamera->PostProcessSettings.bOverride_AutoExposureBias = true;
\tPreviewCamera->PostProcessSettings.AutoExposureBias = 10.0f;
\tPreviewCamera->PostProcessSettings.bOverride_BloomIntensity = true;
\tPreviewCamera->PostProcessSettings.BloomIntensity = 0.25f;

\t// 3. Studio Lighting: 3-point tactical dynamic lighting
\tStudioKeyLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("StudioKeyLight"));
\tStudioKeyLight->SetupAttachment(RootComponent);
\tStudioKeyLight->SetMobility(EComponentMobility::Movable);
\tStudioKeyLight->SetRelativeLocation(FVector(190.0f, -120.0f, 210.0f));
\tStudioKeyLight->SetRelativeRotation(FRotator(-35.0f, 145.0f, 0.0f));
\tStudioKeyLight->SetIntensity(1500.0f);
\tStudioKeyLight->SetLightColor(FLinearColor(1.0f, 0.78f, 0.40f)); // Warm amber key light
\tStudioKeyLight->SetAttenuationRadius(1500.0f);
\tStudioKeyLight->SetInnerConeAngle(25.0f);
\tStudioKeyLight->SetOuterConeAngle(65.0f);

\tStudioRimLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("StudioRimLight"));
\tStudioRimLight->SetupAttachment(RootComponent);
\tStudioRimLight->SetMobility(EComponentMobility::Movable);
\tStudioRimLight->SetRelativeLocation(FVector(-160.0f, 120.0f, 175.0f));
\tStudioRimLight->SetRelativeRotation(FRotator(-25.0f, -35.0f, 0.0f));
\tStudioRimLight->SetIntensity(1800.0f);
\tStudioRimLight->SetLightColor(FLinearColor(0.0f, 0.898f, 1.0f)); // Tactical cyan rim light (#00E5FF)
\tStudioRimLight->SetAttenuationRadius(1400.0f);
\tStudioRimLight->SetInnerConeAngle(30.0f);
\tStudioRimLight->SetOuterConeAngle(70.0f);
\tStudioRimLight->SetCastShadows(false);

\tStudioFillLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("StudioFillLight"));
\tStudioFillLight->SetupAttachment(RootComponent);
\tStudioFillLight->SetMobility(EComponentMobility::Movable);
\tStudioFillLight->SetRelativeLocation(FVector(150.0f, 0.0f, 120.0f));
\tStudioFillLight->SetIntensity(2200.0f);
\tStudioFillLight->SetLightColor(FLinearColor(0.85f, 0.92f, 1.0f)); // Clean fill
\tStudioFillLight->SetAttenuationRadius(1500.0f);
\tStudioFillLight->SetCastShadows(false);

\tStudioAmbientLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("StudioAmbientLight"));
\tStudioAmbientLight->SetupAttachment(RootComponent);
\tStudioAmbientLight->SetMobility(EComponentMobility::Movable);
\tStudioAmbientLight->SetRelativeLocation(FVector(0.0f, 0.0f, 150.0f));
\tStudioAmbientLight->SetIntensity(800.0f);
\tStudioAmbientLight->SetLightColor(FLinearColor(0.35f, 0.45f, 0.55f));
\tStudioAmbientLight->SetAttenuationRadius(1500.0f);
\tStudioAmbientLight->SetCastShadows(false);

\tStudioSunLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("StudioSunLight"));
\tStudioSunLight->SetupAttachment(RootComponent);
\tStudioSunLight->SetMobility(EComponentMobility::Movable);
\tStudioSunLight->SetRelativeRotation(FRotator(-30.0f, 165.0f, 0.0f));
\tStudioSunLight->SetIntensity(10000.0f); // Disabled inside indoor hangar stage"""

new_code = """\tRootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
\tRootScene->SetMobility(EComponentMobility::Movable);
\tRootComponent = RootScene;

\t// 2. Camera: Calibrated to frame the full Task Force Echo Operator & Hangar stage
\tPreviewCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("PreviewCamera"));
\tPreviewCamera->SetupAttachment(RootComponent);
\tPreviewCamera->SetRelativeLocation(FVector(400.0f, 0.0f, 90.0f));
\tPreviewCamera->SetRelativeRotation(FRotator(-1.5f, 180.0f, 0.0f));
\tPreviewCamera->FieldOfView = 50.0f;
\tPreviewCamera->PostProcessSettings.bOverride_AutoExposureMethod = true;
\tPreviewCamera->PostProcessSettings.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
\tPreviewCamera->PostProcessSettings.bOverride_AutoExposureBias = true;
\tPreviewCamera->PostProcessSettings.AutoExposureBias = 0.0f;
\tPreviewCamera->PostProcessSettings.bOverride_BloomIntensity = true;
\tPreviewCamera->PostProcessSettings.BloomIntensity = 0.25f;

\t// 3. Studio Lighting: 3-point tactical dynamic lighting
\tStudioKeyLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("StudioKeyLight"));
\tStudioKeyLight->SetupAttachment(RootComponent);
\tStudioKeyLight->SetMobility(EComponentMobility::Movable);
\tStudioKeyLight->SetRelativeLocation(FVector(190.0f, -120.0f, 210.0f));
\tStudioKeyLight->SetRelativeRotation(FRotator(-35.0f, 145.0f, 0.0f));
\tStudioKeyLight->SetIntensity(8500.0f);
\tStudioKeyLight->SetLightColor(FLinearColor(1.0f, 0.78f, 0.40f)); // Warm amber key light
\tStudioKeyLight->SetAttenuationRadius(1500.0f);
\tStudioKeyLight->SetInnerConeAngle(25.0f);
\tStudioKeyLight->SetOuterConeAngle(65.0f);

\tStudioRimLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("StudioRimLight"));
\tStudioRimLight->SetupAttachment(RootComponent);
\tStudioRimLight->SetMobility(EComponentMobility::Movable);
\tStudioRimLight->SetRelativeLocation(FVector(-160.0f, 120.0f, 175.0f));
\tStudioRimLight->SetRelativeRotation(FRotator(-25.0f, -35.0f, 0.0f));
\tStudioRimLight->SetIntensity(9500.0f);
\tStudioRimLight->SetLightColor(FLinearColor(0.0f, 0.898f, 1.0f)); // Tactical cyan rim light (#00E5FF)
\tStudioRimLight->SetAttenuationRadius(1400.0f);
\tStudioRimLight->SetInnerConeAngle(30.0f);
\tStudioRimLight->SetOuterConeAngle(70.0f);
\tStudioRimLight->SetCastShadows(false);

\tStudioFillLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("StudioFillLight"));
\tStudioFillLight->SetupAttachment(RootComponent);
\tStudioFillLight->SetMobility(EComponentMobility::Movable);
\tStudioFillLight->SetRelativeLocation(FVector(150.0f, 0.0f, 120.0f));
\tStudioFillLight->SetIntensity(6500.0f);
\tStudioFillLight->SetLightColor(FLinearColor(0.85f, 0.92f, 1.0f)); // Clean fill
\tStudioFillLight->SetAttenuationRadius(1500.0f);
\tStudioFillLight->SetCastShadows(false);

\tStudioAmbientLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("StudioAmbientLight"));
\tStudioAmbientLight->SetupAttachment(RootComponent);
\tStudioAmbientLight->SetMobility(EComponentMobility::Movable);
\tStudioAmbientLight->SetRelativeLocation(FVector(0.0f, 0.0f, 150.0f));
\tStudioAmbientLight->SetIntensity(2500.0f);
\tStudioAmbientLight->SetLightColor(FLinearColor(0.35f, 0.45f, 0.55f));
\tStudioAmbientLight->SetAttenuationRadius(1500.0f);
\tStudioAmbientLight->SetCastShadows(false);

\tStudioSunLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("StudioSunLight"));
\tStudioSunLight->SetupAttachment(RootComponent);
\tStudioSunLight->SetMobility(EComponentMobility::Movable);
\tStudioSunLight->SetRelativeRotation(FRotator(-30.0f, 165.0f, 0.0f));
\tStudioSunLight->SetIntensity(0.0f); // Disabled inside indoor hangar stage"""

old_norm = old_code.replace("\r\n", "\n")
text_norm = text.replace("\r\n", "\n")
if old_norm in text_norm:
    text_norm = text_norm.replace(old_norm, new_code.replace("\r\n", "\n"))
    with open("Source/NobleFermi/Character/FPSCharacterPreviewActor.cpp", "w", encoding="utf-8") as f:
        f.write(text_norm)
    print("SUCCESS: Updated FPSCharacterPreviewActor.cpp")
else:
    print("ERROR: old_code not found in text_norm")
