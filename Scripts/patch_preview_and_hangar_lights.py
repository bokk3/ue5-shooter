import os

# 2. Update FPSCharacterPreviewActor.cpp
cpp_path = 'Source/NobleFermi/Character/FPSCharacterPreviewActor.cpp'
with open(cpp_path, 'rb') as f:
    cpp_content = f.read().decode('utf-8').replace('\r\n', '\n').replace('\r', '\n')

old_inc = """#include "Components/PointLightComponent.h"
#include "Camera/CameraComponent.h\""""

new_inc = """#include "Components/PointLightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Camera/CameraComponent.h\""""

old_ctor_light = """	StudioAmbientLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("StudioAmbientLight"));
	StudioAmbientLight->SetupAttachment(RootComponent);
	StudioAmbientLight->SetMobility(EComponentMobility::Movable);
	StudioAmbientLight->SetRelativeLocation(FVector(0.0f, 0.0f, 150.0f));
	StudioAmbientLight->SetIntensity(3000.0f);
	StudioAmbientLight->SetLightColor(FLinearColor(0.08f, 0.12f, 0.18f));
	StudioAmbientLight->SetAttenuationRadius(700.0f);
	StudioAmbientLight->SetCastShadows(false);"""

new_ctor_light = """	StudioAmbientLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("StudioAmbientLight"));
	StudioAmbientLight->SetupAttachment(RootComponent);
	StudioAmbientLight->SetMobility(EComponentMobility::Movable);
	StudioAmbientLight->SetRelativeLocation(FVector(0.0f, 0.0f, 150.0f));
	StudioAmbientLight->SetIntensity(5000.0f);
	StudioAmbientLight->SetLightColor(FLinearColor(0.15f, 0.22f, 0.32f));
	StudioAmbientLight->SetAttenuationRadius(900.0f);
	StudioAmbientLight->SetCastShadows(false);

	StudioSunLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("StudioSunLight"));
	StudioSunLight->SetupAttachment(RootComponent);
	StudioSunLight->SetMobility(EComponentMobility::Movable);
	StudioSunLight->SetRelativeRotation(FRotator(-25.0f, 165.0f, 0.0f));
	StudioSunLight->SetIntensity(6.5f);
	StudioSunLight->SetLightColor(FLinearColor(1.0f, 0.94f, 0.85f));
	StudioSunLight->SetCastShadows(false);"""

old_ped_setup = """	// 6. Turntable Pedestal
	SetupMeshComp(PedestalMesh, TEXT("PedestalMesh"), CylinderMesh, FVector(0.0f, 0.0f, 4.0f), FRotator::ZeroRotator, FVector(1.3f, 1.3f, 0.08f));"""

new_ped_setup = """	// 6. Turntable Pedestal
	SetupMeshComp(PedestalMesh, TEXT("PedestalMesh"), CylinderMesh, FVector(0.0f, 0.0f, 6.0f), FRotator::ZeroRotator, FVector(1.4f, 1.4f, 0.12f));"""

old_ped_mat = """	// Pedestal dark metallic material
	if (PedestalMesh)
	{
		if (UMaterialInstanceDynamic* PedestalMID = PedestalMesh->CreateDynamicMaterialInstance(0))
		{
			PedestalMID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.0f, 0.85f, 1.0f));
		}
	}"""

new_ped_mat = """	// Pedestal prominent cyan display plinth
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

		if (UMaterialInstanceDynamic* PedMID = PedestalMesh->CreateDynamicMaterialInstance(0))
		{
			PedMID->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.0f, 0.85f, 1.0f));
			PedMID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.0f, 0.85f, 1.0f));
		}
	}"""

old_ak_block = """		// Attach real 3D AK-47 to weapon_r socket (or TurntableRoot fallback)
		BodyMesh->SetVisibility(true, true);
		if (AK47Mesh)
		{
			if (BodyMesh && BodyMesh->DoesSocketExist(TEXT("weapon_r")))
			{
				AK47Mesh->AttachToComponent(BodyMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("weapon_r"));
				AK47Mesh->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
				AK47Mesh->SetRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));
			}
			else
			{
				AK47Mesh->AttachToComponent(TurntableRoot, FAttachmentTransformRules::KeepRelativeTransform);
				AK47Mesh->SetRelativeLocation(FVector(20.0f, 0.0f, 115.0f));
				AK47Mesh->SetRelativeRotation(FRotator(-15.0f, 45.0f, 0.0f));
			}
			AK47Mesh->SetVisibility(true, true);
			AK47Mesh->SetHiddenInGame(false, true);
		}"""

new_ak_block = """		// Position real 3D AK-47 in authoritative Low-Ready Carbine stance
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
			AK47Mesh->AttachToComponent(TurntableRoot, FAttachmentTransformRules::KeepRelativeTransform);
			AK47Mesh->SetRelativeLocation(FVector(26.0f, 8.0f, 108.0f));
			AK47Mesh->SetRelativeRotation(FRotator(-20.0f, 55.0f, -8.0f));
			AK47Mesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
			AK47Mesh->SetVisibility(true, true);
			AK47Mesh->SetHiddenInGame(false, true);
		}"""

old_camo = """void AFPSCharacterPreviewActor::ApplyCamoColors()
{
	if (BodyMesh)
	{
		UMaterialInstanceDynamic* BodyMID = Cast<UMaterialInstanceDynamic>(BodyMesh->GetMaterial(0));
		if (!BodyMID)
		{
			BodyMID = BodyMesh->CreateDynamicMaterialInstance(0);
		}
		if (BodyMID)
		{
			FLinearColor CamoTint(1.0f, 1.0f, 1.0f, 1.0f); // Default / Desert Tan
			if (CurrentCamo == EOperatorCamo::WoodlandTactical)
			{
				CamoTint = FLinearColor(0.55f, 0.70f, 0.48f, 1.0f); // Tactical Olive Drab
			}
			else if (CurrentCamo == EOperatorCamo::DesertSpecOps)
			{
				CamoTint = FLinearColor(1.10f, 0.98f, 0.82f, 1.0f); // Desert Sand
			}
			else if (CurrentCamo == EOperatorCamo::StealthBlackout)
			{
				CamoTint = FLinearColor(0.32f, 0.35f, 0.40f, 1.0f); // Stealth Blackout Slate
			}
			BodyMID->SetVectorParameterValue(TEXT("CamoColorPrimary"), CamoTint);
			BodyMID->SetScalarParameterValue(TEXT("RoughnessBase"), 0.55f);
		}
	}
}"""

new_camo = """void AFPSCharacterPreviewActor::ApplyCamoColors()
{
	if (BodyMesh)
	{
		UMaterialInstanceDynamic* BodyMID = Cast<UMaterialInstanceDynamic>(BodyMesh->GetMaterial(0));
		if (!BodyMID)
		{
			BodyMID = BodyMesh->CreateDynamicMaterialInstance(0);
		}
		if (BodyMID)
		{
			// Bright tactical tint ensuring crisp in-engine visibility
			FLinearColor CamoTint = FLinearColor(2.2f, 2.0f, 1.7f, 1.0f); // Desert Sand SpecOps
			if (CurrentCamo == EOperatorCamo::WoodlandTactical)
			{
				CamoTint = FLinearColor(1.2f, 1.8f, 1.1f, 1.0f); // Tactical Olive Drab
			}
			else if (CurrentCamo == EOperatorCamo::StealthBlackout)
			{
				CamoTint = FLinearColor(1.2f, 1.3f, 1.5f, 1.0f); // Stealth Slate
			}
			BodyMID->SetVectorParameterValue(TEXT("CamoColorPrimary"), CamoTint);
			BodyMID->SetScalarParameterValue(TEXT("RoughnessBase"), 0.45f);
		}

		if (UMaterialInstanceDynamic* VisorMID = BodyMesh->CreateDynamicMaterialInstance(1))
		{
			VisorMID->SetVectorParameterValue(TEXT("CamoColorPrimary"), FLinearColor(0.0f, 0.90f, 1.0f));
			VisorMID->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.0f, 0.90f, 1.0f));
		}
	}
}"""

assert old_inc in cpp_content, "old_inc not found"
assert old_ctor_light in cpp_content, "old_ctor_light not found"
assert old_ped_setup in cpp_content, "old_ped_setup not found"
assert old_ped_mat in cpp_content, "old_ped_mat not found"
assert old_ak_block in cpp_content, "old_ak_block not found"
assert old_camo in cpp_content, "old_camo not found"

cpp_content = cpp_content.replace(old_inc, new_inc, 1)
cpp_content = cpp_content.replace(old_ctor_light, new_ctor_light, 1)
cpp_content = cpp_content.replace(old_ped_setup, new_ped_setup, 1)
cpp_content = cpp_content.replace(old_ped_mat, new_ped_mat, 1)
cpp_content = cpp_content.replace(old_ak_block, new_ak_block, 1)
cpp_content = cpp_content.replace(old_camo, new_camo, 1)

with open(cpp_path, 'wb') as f:
    f.write(cpp_content.replace('\n', '\r\n').encode('utf-8'))
print("Successfully patched FPSCharacterPreviewActor.cpp")

# 3. Update FPSMenuHangarStage.cpp ConfigurePreviewActorAndCamera
hangar_path = 'Source/NobleFermi/Game/FPSMenuHangarStage.cpp'
with open(hangar_path, 'rb') as f:
    hangar_content = f.read().decode('utf-8').replace('\r\n', '\n').replace('\r', '\n')

old_h_lights_tune = """	if (InPreviewActor->StudioKeyLight)
	{
		// Warm Sodium-Amber Key/Rim from Screen Left (+Y) hitting Operator's right helmet, shoulder, and forearm
		InPreviewActor->StudioKeyLight->SetRelativeLocation(FVector(110.0f, 155.0f, 185.0f));
		InPreviewActor->StudioKeyLight->SetRelativeRotation(FRotator(-22.0f, -125.0f, 0.0f));
		InPreviewActor->StudioKeyLight->SetIntensity(20000.0f);
		InPreviewActor->StudioKeyLight->SetLightColor(FLinearColor(1.0f, 0.58f, 0.20f)); // Warm Amber
	}

	if (InPreviewActor->StudioRimLight)
	{
		// Noble Fermi Cyan Rim Counter-Light from Screen Right (-Y) hitting Operator's left arm & vest
		InPreviewActor->StudioRimLight->SetRelativeLocation(FVector(-95.0f, -155.0f, 160.0f));
		InPreviewActor->StudioRimLight->SetRelativeRotation(FRotator(-16.0f, 55.0f, 0.0f));
		InPreviewActor->StudioRimLight->SetIntensity(16000.0f);
		InPreviewActor->StudioRimLight->SetLightColor(FLinearColor(0.0f, 0.867f, 0.961f)); // #00DDF5 Cyan
	}

	if (InPreviewActor->StudioFillLight)
	{
		// Subtle cool steel-blue fill from front-center
		InPreviewActor->StudioFillLight->SetRelativeLocation(FVector(180.0f, -60.0f, 105.0f));
		InPreviewActor->StudioFillLight->SetIntensity(4200.0f);
		InPreviewActor->StudioFillLight->SetLightColor(FLinearColor(0.14f, 0.28f, 0.42f));
	}"""

new_h_lights_tune = """	if (InPreviewActor->StudioKeyLight)
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
	}"""

assert old_h_lights_tune in hangar_content, "old_h_lights_tune not found"
hangar_content = hangar_content.replace(old_h_lights_tune, new_h_lights_tune, 1)

with open(hangar_path, 'wb') as f:
    f.write(hangar_content.replace('\n', '\r\n').encode('utf-8'))
print("Successfully patched FPSMenuHangarStage.cpp")
