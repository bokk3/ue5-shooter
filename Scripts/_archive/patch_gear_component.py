import os

filepath = 'Source/NobleFermi/Character/FPSOperatorGearComponent.cpp'
with open(filepath, 'r', encoding='utf-8') as f:
    content = f.read()

old_block = """	for (UProceduralMeshComponent* Comp : AllGearMeshes)
	{
		if (Comp && UnscaledAttachParent)
		{
			if (!Comp->IsRegistered())
			{
				Comp->RegisterComponent();
			}
			Comp->AttachToComponent(UnscaledAttachParent, FAttachmentTransformRules::KeepRelativeTransform);
			Comp->SetWorldScale3D(FVector(1.0f, 1.0f, 1.0f));
			Comp->SetCastShadow(true);
			Comp->bCastDynamicShadow = true;
			Comp->SetHiddenInGame(false);
			Comp->SetVisibility(true, true);
		}
	}

	if (CarbineProcMesh)
	{
		CarbineProcMesh->SetVisibility(bIncludeCarbine, true);
		CarbineProcMesh->SetHiddenInGame(!bIncludeCarbine);
	}

	// 1. Generate low-contrast micro-ripstop weave & seamless body mask textures and calibrate M_Operator_BaseBody
	GenerateRuntimeOperatorTextures();
	CalibrateOperatorBaseBodyMaterial();

	// 2. Build all 7 Socket-Driven 3D Tactical Gear Zones + Spec-Ops Carbine
	BuildZone1_HeadAndComms();
	BuildZone2_PlateCarrierAndCollar();
	BuildZone3_ShoulderPauldrons();
	BuildZone4_ForearmsAndGloves();
	BuildZone5_BattleBeltAndGroin();
	BuildZone6_ThighHolsterAndRig();
	BuildZone7_KneePadsAndBoots();
	BuildSpecOpsCarbine();"""

new_block = """	for (UProceduralMeshComponent* Comp : AllGearMeshes)
	{
		if (Comp)
		{
			Comp->SetVisibility(false, true);
			Comp->SetHiddenInGame(true);
		}
	}

	if (CarbineProcMesh)
	{
		CarbineProcMesh->SetVisibility(false, true);
		CarbineProcMesh->SetHiddenInGame(true);
	}

	// High-fidelity SK_BaseMale model contains full integrated 3D tactical gear.
	// Procedural boxes are disabled to prevent floating disjointed parts.
	CalibrateOperatorBaseBodyMaterial();"""

assert old_block in content, 'old_block not found in FPSOperatorGearComponent.cpp!'
content = content.replace(old_block, new_block, 1)

# In CalibrateOperatorBaseBodyMaterial, change CamoPrimary default to (1, 1, 1, 1) and Roughness to 0.55
old_calib = """	// Default Task Force Echo Slate / Deep Charcoal Spec-Ops Calibration (matching menu-prop.jpeg)
	FLinearColor CamoPrimary(0.016f, 0.022f, 0.030f, 1.0f);   // #12161B Deep Charcoal
	FLinearColor CamoSecondary(0.010f, 0.014f, 0.020f, 1.0f); // #0E1217 Midnight Slate
	FLinearColor CamoTertiary(0.026f, 0.034f, 0.045f, 1.0f);  // #1A2026 Micro-Camo Contrast
	FLinearColor TorsoColor(0.014f, 0.019f, 0.026f, 1.0f);    // #11151B Combat Shirt
	FLinearColor BalaclavaBase(0.009f, 0.012f, 0.016f, 1.0f); // #0D1014 Matte Black Balaclava Under-Layer
	FLinearColor GloveBootBase(0.012f, 0.016f, 0.022f, 1.0f); // #0F1317 Reinforced Carbon-Knuckle Gloves
	float RoughnessBase = 0.84f;"""

new_calib = """	// Default Task Force Echo Full-Fidelity Textured Soldier
	FLinearColor CamoPrimary(1.0f, 1.0f, 1.0f, 1.0f);
	FLinearColor CamoSecondary(0.85f, 0.85f, 0.85f, 1.0f);
	FLinearColor CamoTertiary(0.65f, 0.65f, 0.65f, 1.0f);
	FLinearColor TorsoColor(1.0f, 1.0f, 1.0f, 1.0f);
	FLinearColor BalaclavaBase(0.12f, 0.14f, 0.18f, 1.0f);
	FLinearColor GloveBootBase(0.15f, 0.18f, 0.22f, 1.0f);
	float RoughnessBase = 0.55f;"""

assert old_calib in content, 'old_calib not found!'
content = content.replace(old_calib, new_calib, 1)

with open(filepath, 'w', encoding='utf-8', newline='\r\n') as f:
    f.write(content)
print('SUCCESSFULLY PATCHED FPSOperatorGearComponent.cpp')
