import os

preview_path = 'Source/NobleFermi/Character/FPSCharacterPreviewActor.cpp'
with open(preview_path, 'rb') as f:
    preview_content = f.read().decode('utf-8').replace('\r\n', '\n').replace('\r', '\n')

# 1. Update fill light in preview actor constructor
old_fill = """	StudioFillLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("StudioFillLight"));
	StudioFillLight->SetupAttachment(RootComponent);
	StudioFillLight->SetMobility(EComponentMobility::Movable);
	StudioFillLight->SetRelativeLocation(FVector(160.0f, 140.0f, 90.0f));
	StudioFillLight->SetIntensity(18000.0f);"""

new_fill = """	StudioFillLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("StudioFillLight"));
	StudioFillLight->SetupAttachment(RootComponent);
	StudioFillLight->SetMobility(EComponentMobility::Movable);
	StudioFillLight->SetRelativeLocation(FVector(120.0f, 0.0f, 120.0f));
	StudioFillLight->SetIntensity(65000.0f);
	StudioFillLight->SetLightColor(FLinearColor(1.0f, 0.96f, 0.90f));
	StudioFillLight->SetAttenuationRadius(1500.0f);
	StudioFillLight->SetCastShadows(false);"""

# 2. Update animation mode in constructor
old_anim_mode = """	// SK_BaseMale natural Spec-Ops pose
	BodyMesh->SetAnimationMode(EAnimationMode::AnimationCustomMode);"""

new_anim_mode = """	// SK_BaseMale natural Spec-Ops pose with continuous skinning evaluation
	BodyMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	BodyMesh->SetForcedLOD(1);"""

# 3. Update BeginPlay in preview actor
old_begin_play = """	// Ensure SK_BaseMale is loaded
	if (BodyMesh && !BodyMesh->GetSkeletalMeshAsset())
	{
		USkeletalMesh* LoadedMesh = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Characters/BaseMale/SK_BaseMale.SK_BaseMale"));
		if (!LoadedMesh)
		{
			LoadedMesh = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Characters/BaseMale/SK_BaseMale"));
		}
		if (LoadedMesh)
		{
			BodyMesh->SetSkeletalMesh(LoadedMesh);
		}
	}

	if (BodyMesh)
	{
		BodyMesh->bComponentUseFixedSkelBounds = true;
		BodyMesh->BoundsScale = 5.0f;
		BodyMesh->SetVisibility(true, true);
		BodyMesh->SetHiddenInGame(false, true);
	}

	UE_LOG(LogTemp, Warning, TEXT("[PREVIEW_DIAG] BodyMesh valid=%d, Asset=%s, Bones=%d, BoundsScale=%.1f, Vis=%d, Hidden=%d, Loc=%s"),
		BodyMesh != nullptr,
		(BodyMesh && BodyMesh->GetSkeletalMeshAsset()) ? *BodyMesh->GetSkeletalMeshAsset()->GetName() : TEXT("NULL"),
		(BodyMesh && BodyMesh->GetSkeletalMeshAsset()) ? BodyMesh->GetNumBones() : 0,
		BodyMesh ? BodyMesh->BoundsScale : 0.0f,
		BodyMesh ? BodyMesh->IsVisible() : 0,
		BodyMesh ? BodyMesh->bHiddenInGame : 0,
		BodyMesh ? *BodyMesh->GetComponentLocation().ToString() : TEXT("NONE"));"""

new_begin_play = """	// Ensure SK_BaseMale is loaded
	if (BodyMesh && !BodyMesh->GetSkeletalMeshAsset())
	{
		USkeletalMesh* LoadedMesh = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Characters/BaseMale/SK_BaseMale.SK_BaseMale"));
		if (!LoadedMesh)
		{
			LoadedMesh = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Characters/BaseMale/SK_BaseMale"));
		}
		if (LoadedMesh)
		{
			BodyMesh->SetSkeletalMesh(LoadedMesh);
		}
	}

	if (BodyMesh)
	{
		BodyMesh->bComponentUseFixedSkelBounds = true;
		BodyMesh->BoundsScale = 5.0f;
		BodyMesh->SetForcedLOD(1);
		BodyMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		if (AnimLowReady)
		{
			BodyMesh->PlayAnimation(AnimLowReady, true);
		}
		BodyMesh->SetVisibility(true, true);
		BodyMesh->SetHiddenInGame(false, true);
	}

	UE_LOG(LogTemp, Warning, TEXT("[PREVIEW_DIAG] BodyMesh valid=%d, Asset=%s, Bones=%d, BoundsScale=%.1f, Vis=%d, Hidden=%d, Loc=%s"),
		BodyMesh != nullptr,
		(BodyMesh && BodyMesh->GetSkeletalMeshAsset()) ? *BodyMesh->GetSkeletalMeshAsset()->GetName() : TEXT("NULL"),
		(BodyMesh && BodyMesh->GetSkeletalMeshAsset()) ? BodyMesh->GetNumBones() : 0,
		BodyMesh ? BodyMesh->BoundsScale : 0.0f,
		BodyMesh ? BodyMesh->IsVisible() : 0,
		BodyMesh ? BodyMesh->bHiddenInGame : 0,
		BodyMesh ? *BodyMesh->GetComponentLocation().ToString() : TEXT("NONE"));"""

assert old_fill in preview_content, "old_fill not found"
assert old_anim_mode in preview_content, "old_anim_mode not found"
assert old_begin_play in preview_content, "old_begin_play not found"

preview_content = preview_content.replace(old_fill, new_fill, 1)
preview_content = preview_content.replace(old_anim_mode, new_anim_mode, 1)
preview_content = preview_content.replace(old_begin_play, new_begin_play, 1)

with open(preview_path, 'wb') as f:
    f.write(preview_content.replace('\n', '\r\n').encode('utf-8'))
print("Successfully patched FPSCharacterPreviewActor.cpp")

# 4. Patch Hangar SkyLight in FPSMenuHangarStage.cpp
hangar_path = 'Source/NobleFermi/Game/FPSMenuHangarStage.cpp'
with open(hangar_path, 'rb') as f:
    hangar_content = f.read().decode('utf-8').replace('\r\n', '\n').replace('\r', '\n')

old_sky = """	HangarSkyLight->SetIntensity(2.8f);
	HangarSkyLight->SetLightColor(FLinearColor(0.22f, 0.35f, 0.48f)); // #1B3B52
	HangarSkyLight->bLowerHemisphereIsBlack = false;
	HangarSkyLight->LowerHemisphereColor = FLinearColor(0.08f, 0.12f, 0.16f, 1.0f);"""

new_sky = """	HangarSkyLight->SetIntensity(5.5f);
	HangarSkyLight->SetLightColor(FLinearColor(0.35f, 0.48f, 0.62f)); // Cool Steel Ambient
	HangarSkyLight->bLowerHemisphereIsBlack = false;
	HangarSkyLight->LowerHemisphereColor = FLinearColor(0.15f, 0.22f, 0.28f, 1.0f);"""

assert old_sky in hangar_content, "old_sky not found"
hangar_content = hangar_content.replace(old_sky, new_sky, 1)

with open(hangar_path, 'wb') as f:
    f.write(hangar_content.replace('\n', '\r\n').encode('utf-8'))
print("Successfully patched FPSMenuHangarStage.cpp")
