with open("Source/NobleFermi/Game/FPSMenuHangarStage.cpp", "r", encoding="utf-8") as f:
    text = f.read()

old_block = """\tif (InPreviewActor->StudioKeyLight)
\t{
\t\tInPreviewActor->StudioKeyLight->SetRelativeLocation(FVector(110.0f, 155.0f, 185.0f));
\t\tInPreviewActor->StudioKeyLight->SetRelativeRotation(FRotator(-22.0f, -125.0f, 0.0f));
\t\tInPreviewActor->StudioKeyLight->SetIntensity(1500.0f);
\t\tInPreviewActor->StudioKeyLight->SetLightColor(FLinearColor(1.0f, 0.70f, 0.35f)); // Warm Amber
\t}

\tif (InPreviewActor->StudioRimLight)
\t{
\t\tInPreviewActor->StudioRimLight->SetRelativeLocation(FVector(-95.0f, -155.0f, 160.0f));
\t\tInPreviewActor->StudioRimLight->SetRelativeRotation(FRotator(-16.0f, 55.0f, 0.0f));
\t\tInPreviewActor->StudioRimLight->SetIntensity(1800.0f);
\t\tInPreviewActor->StudioRimLight->SetLightColor(FLinearColor(0.0f, 0.898f, 1.0f)); // Tactical Cyan
\t}

\tif (InPreviewActor->StudioFillLight)
\t{
\t\tInPreviewActor->StudioFillLight->SetRelativeLocation(FVector(150.0f, 0.0f, 115.0f));
\t\tInPreviewActor->StudioFillLight->SetIntensity(2200.0f);
\t\tInPreviewActor->StudioFillLight->SetLightColor(FLinearColor(1.0f, 0.96f, 0.90f));
\t}

\tif (InPreviewActor->StudioAmbientLight)
\t{
\t\tInPreviewActor->StudioAmbientLight->SetIntensity(800.0f);
\t\tInPreviewActor->StudioAmbientLight->SetLightColor(FLinearColor(0.35f, 0.45f, 0.55f));
\t}

\tif (InPreviewActor->StudioSunLight)
\t{
\t\tInPreviewActor->StudioSunLight->SetIntensity(10000.0f);
\t}

\t// Lock PreviewCamera Exposure & Vignette
\tif (InPreviewActor->PreviewCamera)
\t{
\t\tFPostProcessSettings& CamPP = InPreviewActor->PreviewCamera->PostProcessSettings;
\t\tCamPP.bOverride_AutoExposureMethod = true;
\t\tCamPP.AutoExposureMethod = EAutoExposureMethod::AEM_Histogram;
\t\tCamPP.bOverride_AutoExposureBias = true;
\t\tCamPP.AutoExposureBias = 0.0f;
\t\tCamPP.bOverride_AutoExposureMinBrightness = false;
\t\tCamPP.bOverride_AutoExposureMaxBrightness = false;

\t\tCamPP.bOverride_BloomIntensity = true;
\t\tCamPP.BloomIntensity = 0.28f;"""

new_block = """\tif (InPreviewActor->StudioKeyLight)
\t{
\t\tInPreviewActor->StudioKeyLight->SetRelativeLocation(FVector(110.0f, 155.0f, 185.0f));
\t\tInPreviewActor->StudioKeyLight->SetRelativeRotation(FRotator(-22.0f, -125.0f, 0.0f));
\t\tInPreviewActor->StudioKeyLight->SetIntensity(8500.0f);
\t\tInPreviewActor->StudioKeyLight->SetLightColor(FLinearColor(1.0f, 0.70f, 0.35f)); // Warm Amber
\t}

\tif (InPreviewActor->StudioRimLight)
\t{
\t\tInPreviewActor->StudioRimLight->SetRelativeLocation(FVector(-95.0f, -155.0f, 160.0f));
\t\tInPreviewActor->StudioRimLight->SetRelativeRotation(FRotator(-16.0f, 55.0f, 0.0f));
\t\tInPreviewActor->StudioRimLight->SetIntensity(9500.0f);
\t\tInPreviewActor->StudioRimLight->SetLightColor(FLinearColor(0.0f, 0.898f, 1.0f)); // Tactical Cyan
\t}

\tif (InPreviewActor->StudioFillLight)
\t{
\t\tInPreviewActor->StudioFillLight->SetRelativeLocation(FVector(150.0f, 0.0f, 115.0f));
\t\tInPreviewActor->StudioFillLight->SetIntensity(6500.0f);
\t\tInPreviewActor->StudioFillLight->SetLightColor(FLinearColor(1.0f, 0.96f, 0.90f));
\t}

\tif (InPreviewActor->StudioAmbientLight)
\t{
\t\tInPreviewActor->StudioAmbientLight->SetIntensity(2500.0f);
\t\tInPreviewActor->StudioAmbientLight->SetLightColor(FLinearColor(0.35f, 0.45f, 0.55f));
\t}

\tif (InPreviewActor->StudioSunLight)
\t{
\t\tInPreviewActor->StudioSunLight->SetIntensity(0.0f);
\t}

\t// Lock PreviewCamera Exposure & Vignette
\tif (InPreviewActor->PreviewCamera)
\t{
\t\tFPostProcessSettings& CamPP = InPreviewActor->PreviewCamera->PostProcessSettings;
\t\tCamPP.bOverride_AutoExposureMethod = true;
\t\tCamPP.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
\t\tCamPP.bOverride_AutoExposureBias = true;
\t\tCamPP.AutoExposureBias = 0.0f;
\t\tCamPP.bOverride_AutoExposureMinBrightness = false;
\t\tCamPP.bOverride_AutoExposureMaxBrightness = false;

\t\tCamPP.bOverride_BloomIntensity = true;
\t\tCamPP.BloomIntensity = 0.28f;"""

old_block_norm = old_block.replace("\r\n", "\n")
text_norm = text.replace("\r\n", "\n")
if old_block_norm in text_norm:
    text_norm = text_norm.replace(old_block_norm, new_block.replace("\r\n", "\n"))
    with open("Source/NobleFermi/Game/FPSMenuHangarStage.cpp", "w", encoding="utf-8") as f:
        f.write(text_norm)
    print("SUCCESS: Updated FPSMenuHangarStage.cpp")
else:
    print("ERROR: old_block not found in text_norm")
