import re

# 1. Update Source/NobleFermi/Character/FPSCharacterPreviewActor.cpp
cpp_path = 'Source/NobleFermi/Character/FPSCharacterPreviewActor.cpp'
with open(cpp_path, 'r', encoding='utf-8') as f:
    cpp_text = f.read()

# Replace camera & lighting in constructor
new_cam_and_lights = """// 2. Camera: Calibrated to frame the full Task Force Echo Operator & Hangar stage
	PreviewCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("PreviewCamera"));
	PreviewCamera->SetupAttachment(RootComponent);
	PreviewCamera->SetRelativeLocation(FVector(350.0f, 0.0f, 100.0f));
	PreviewCamera->SetRelativeRotation(FRotator(-2.0f, 180.0f, 0.0f));
	PreviewCamera->FieldOfView = 52.0f;
	PreviewCamera->PostProcessSettings.bOverride_AutoExposureMethod = true;
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
	StudioSunLight->SetCastShadows(false);
	"""

cpp_text = re.sub(r'// 2\. Camera:.*?(?=// 4\. Turntable Root)', new_cam_and_lights, cpp_text, flags=re.DOTALL)

# Replace BodyMesh in BeginPlay
new_bodymesh_beginplay = """	if (BodyMesh)
	{
		BodyMesh->bComponentUseFixedSkelBounds = false;
		BodyMesh->BoundsScale = 10.0f;
		BodyMesh->SetForcedLOD(0);

		// Run C++ UFPSAnimInstance (21-bone Low-Ready Carbine + Two-Bone IK solver)
		BodyMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		BodyMesh->SetAnimInstanceClass(UFPSAnimInstance::StaticClass());
		BodyMesh->InitAnim(true);

		if (UFPSAnimInstance* FPSAnim = Cast<UFPSAnimInstance>(BodyMesh->GetAnimInstance()))
		{
			FPSAnim->bIsMenuPreview = true;
			FPSAnim->MenuPoseIndex = CurrentStanceIndex; // 0 = Low-Ready Carbine
		}

		BodyMesh->SetVisibility(true, true);
		BodyMesh->SetHiddenInGame(false, true);
	}

	if (BodyMesh)
	{
		UMaterialInterface* CurrentMat = BodyMesh->GetMaterial(0);
		if (!CurrentMat || !CurrentMat->GetName().Contains(TEXT("M_Operator_BaseBody")))
		{
			if (UMaterialInterface* LoadedMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Characters/BaseMale/Materials/M_Operator_BaseBody.M_Operator_BaseBody")))
			{
				BodyMesh->SetMaterial(0, LoadedMat);
			}
		}
	}

	FBoxSphereBounds MeshBounds = BodyMesh ? BodyMesh->CalcBounds(BodyMesh->GetComponentTransform()) : FBoxSphereBounds();
	UE_LOG(LogTemp, Warning, TEXT("[PREVIEW_DIAG] BodyMesh valid=%d, Asset=%s, Bones=%d, BoundsScale=%.1f, Radius=%.1f, Vis=%d, Hidden=%d, Loc=%s, AnimClass=%s"),
		BodyMesh != nullptr,
		(BodyMesh && BodyMesh->GetSkeletalMeshAsset()) ? *BodyMesh->GetSkeletalMeshAsset()->GetName() : TEXT("NULL"),
		(BodyMesh && BodyMesh->GetSkeletalMeshAsset()) ? BodyMesh->GetNumBones() : 0,
		BodyMesh ? BodyMesh->BoundsScale : 0.0f,
		MeshBounds.SphereRadius,
		BodyMesh ? BodyMesh->IsVisible() : 0,
		BodyMesh ? BodyMesh->bHiddenInGame : 0,
		BodyMesh ? *BodyMesh->GetComponentLocation().ToString() : TEXT("NONE"),
		(BodyMesh && BodyMesh->GetAnimInstance()) ? *BodyMesh->GetAnimInstance()->GetClass()->GetName() : TEXT("NONE"));"""

cpp_text = re.sub(r'if \(BodyMesh\)\s*\{\s*BodyMesh->bComponentUseFixedSkelBounds = true;.*?if \(BodyMesh && BodyMesh->GetSkeletalMeshAsset\(\)\)',
    new_bodymesh_beginplay + "\n\n\tif (BodyMesh && BodyMesh->GetSkeletalMeshAsset())", cpp_text, flags=re.DOTALL)

# Replace Pedestal in BeginPlay
new_ped_beginplay = """// Pedestal prominent cyan display plinth
	if (PedestalMesh)
	{
		if (!PedestalMesh->GetStaticMesh())
		{
			if (UStaticMesh* Cyl = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")))
			{
				PedestalMesh->SetStaticMesh(Cyl);
			}
		}
		PedestalMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 6.0f));
		PedestalMesh->SetRelativeScale3D(FVector(1.4f, 1.4f, 0.12f));
		PedestalMesh->SetVisibility(true, true);
		PedestalMesh->SetHiddenInGame(false, true);

		if (UMaterialInterface* MasterMultiMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Characters/BaseMale/Materials/M_Operator_BaseBody.M_Operator_BaseBody")))
		{
			PedestalMesh->SetMaterial(0, MasterMultiMat);
		}
		if (UMaterialInstanceDynamic* PedMID = PedestalMesh->CreateDynamicMaterialInstance(0))
		{
			PedMID->SetVectorParameterValue(TEXT("CamoColorPrimary"), FLinearColor(0.0f, 0.85f, 1.0f));
			PedMID->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.0f, 0.85f, 1.0f));
			PedMID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.0f, 0.85f, 1.0f));
		}
	}"""

cpp_text = re.sub(r'// Pedestal prominent cyan display plinth.*?// Sync persistent equipped loadout',
    new_ped_beginplay + "\n\n\t// Sync persistent equipped loadout", cpp_text, flags=re.DOTALL)

# Replace AK47 placement
new_ak_beginplay = """// Position real 3D AK-47 in authoritative Low-Ready Carbine stance
		BodyMesh->SetVisibility(true, true);
		if (AK47Mesh)
		{
			if (!AK47Mesh->GetStaticMesh())
			{
				if (UStaticMesh* LoadedAK = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Characters/BaseMale/SM_AK47.SM_AK47")))
				{
					AK47Mesh->SetStaticMesh(LoadedAK);
				}
			}
			if (BodyMesh && BodyMesh->DoesSocketExist(TEXT("weapon_r")))
			{
				AK47Mesh->AttachToComponent(BodyMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("weapon_r"));
				AK47Mesh->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
				AK47Mesh->SetRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));
			}
			else
			{
				AK47Mesh->AttachToComponent(TurntableRoot, FAttachmentTransformRules::KeepRelativeTransform);
				AK47Mesh->SetRelativeLocation(FVector(26.0f, 8.0f, 108.0f));
				AK47Mesh->SetRelativeRotation(FRotator(-20.0f, 55.0f, -8.0f));
			}
			AK47Mesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
			AK47Mesh->SetVisibility(true, true);
			AK47Mesh->SetHiddenInGame(false, true);
		}"""

cpp_text = re.sub(r'// Position real 3D AK-47 in authoritative Low-Ready Carbine stance.*?(?=\t\}\s*// Pedestal)',
    new_ak_beginplay + "\n", cpp_text, flags=re.DOTALL)

with open(cpp_path, 'wb') as f:
    f.write(cpp_text.replace('\r\n', '\n').replace('\n', '\r\n').encode('utf-8'))
print("Successfully patched FPSCharacterPreviewActor.cpp")


# 2. Update Source/NobleFermi/Game/FPSMenuHangarStage.cpp
hangar_path = 'Source/NobleFermi/Game/FPSMenuHangarStage.cpp'
with open(hangar_path, 'r', encoding='utf-8') as f:
    hangar_text = f.read()

new_hangar_cfg = """	if (InPreviewActor->StudioAmbientLight)
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

hangar_text = re.sub(r'if \(InPreviewActor->StudioAmbientLight\)\s*\{\s*InPreviewActor->StudioAmbientLight->SetIntensity\(1800\.0f\);.*?CamPP\.AutoExposureMaxBrightness = 10\.0f;',
    new_hangar_cfg, hangar_text, flags=re.DOTALL)

with open(hangar_path, 'wb') as f:
    f.write(hangar_text.replace('\r\n', '\n').replace('\n', '\r\n').encode('utf-8'))
print("Successfully patched FPSMenuHangarStage.cpp")


# 3. Update Source/NobleFermi/Game/FPSMenuGameMode.cpp
gm_path = 'Source/NobleFermi/Game/FPSMenuGameMode.cpp'
with open(gm_path, 'r', encoding='utf-8') as f:
    gm_text = f.read()

new_gm_shot = """			// Diagnostic dump right before screenshot
			if (ActivePreviewActor)
			{
				USkeletalMeshComponent* Body = ActivePreviewActor->BodyMesh;
				FBoxSphereBounds B = Body ? Body->CalcBounds(Body->GetComponentTransform()) : FBoxSphereBounds();
				UE_LOG(LogTemp, Warning, TEXT("[SCREENSHOT_DIAG] ActivePreviewActor=%s, BodyMesh valid=%d, Asset=%s, Vis=%d, Hidden=%d, Radius=%.1f, Loc=%s"),
					*ActivePreviewActor->GetName(),
					Body != nullptr,
					(Body && Body->GetSkeletalMeshAsset()) ? *Body->GetSkeletalMeshAsset()->GetName() : TEXT("NONE"),
					Body ? Body->IsVisible() : 0,
					Body ? Body->bHiddenInGame : 0,
					B.SphereRadius,
					Body ? *Body->GetComponentLocation().ToString() : TEXT("NONE"));
			}
			if (APlayerController* PCLocal = UGameplayStatics::GetPlayerController(this, 0))
			{
				UE_LOG(LogTemp, Warning, TEXT("[SCREENSHOT_DIAG] ViewTarget=%s, CameraMgr=%s, CamLoc=%s, CamRot=%s, CamFOV=%.1f"),
					PCLocal->GetViewTarget() ? *PCLocal->GetViewTarget()->GetName() : TEXT("NONE"),
					PCLocal->PlayerCameraManager ? *PCLocal->PlayerCameraManager->GetName() : TEXT("NONE"),
					PCLocal->PlayerCameraManager ? *PCLocal->PlayerCameraManager->GetCameraLocation().ToString() : TEXT("NONE"),
					PCLocal->PlayerCameraManager ? *PCLocal->PlayerCameraManager->GetCameraRotation().ToString() : TEXT("NONE"),
					PCLocal->PlayerCameraManager ? PCLocal->PlayerCameraManager->GetFOVAngle() : 0.0f);
			}

			const FString ScreenshotFile = FPaths::Combine(ScreenshotDir, TEXT("menu_real_actual.png"));
			FScreenshotRequest::RequestScreenshot(ScreenshotFile, true, false);
			UE_LOG(LogTemp, Warning, TEXT("[SCREENSHOT] Real in-engine screenshot queued to: %s"), *ScreenshotFile);"""

gm_text = re.sub(r'const FString ScreenshotFile = FPaths::Combine\(ScreenshotDir, TEXT\("menu_real_actual\.png"\)\);\s*FScreenshotRequest::RequestScreenshot\(ScreenshotFile, true, false\);\s*UE_LOG\(LogTemp, Warning, TEXT\("\[SCREENSHOT\] Real in-engine screenshot queued to: %s"\), \*ScreenshotFile\);',
    new_gm_shot, gm_text)

with open(gm_path, 'wb') as f:
    f.write(gm_text.replace('\r\n', '\n').replace('\n', '\r\n').encode('utf-8'))
print("Successfully patched FPSMenuGameMode.cpp")
