import os

print("=== APPLYING CAMERA FRAMING AND CAMO COLOR FIXES ===")

preview_path = "Source/NobleFermi/Character/FPSCharacterPreviewActor.cpp"

with open(preview_path, 'r', encoding='utf-8') as f:
    content = f.read()

lf = content.replace('\r\n', '\n')

# 1. Update PreviewCamera Location to frame the full 180cm soldier including helmet & visor
old_cam_loc = """	PreviewCamera->SetRelativeLocation(FVector(340.0f, 0.0f, 95.0f));
	PreviewCamera->SetRelativeRotation(FRotator(-2.0f, 180.0f, 0.0f));
	PreviewCamera->FieldOfView = 50.0f;"""

new_cam_loc = """	PreviewCamera->SetRelativeLocation(FVector(400.0f, 0.0f, 90.0f));
	PreviewCamera->SetRelativeRotation(FRotator(-1.5f, 180.0f, 0.0f));
	PreviewCamera->FieldOfView = 50.0f;"""

assert old_cam_loc in lf, "old_cam_loc not found!"
lf = lf.replace(old_cam_loc, new_cam_loc)

# 2. Update ApplyCamoColors to use neutral 1.0 for UrbanDigital and configure Visor emissive
old_camo = """void AFPSCharacterPreviewActor::ApplyCamoColors()
{
	if (OperatorMesh)
	{
		UMaterialInstanceDynamic* BodyMID = Cast<UMaterialInstanceDynamic>(OperatorMesh->GetMaterial(0));
		if (!BodyMID)
		{
			BodyMID = OperatorMesh->CreateDynamicMaterialInstance(0);
		}
		if (BodyMID)
		{
			// Tactical tint ensuring crisp in-engine visibility
			FLinearColor CamoTint = FLinearColor(1.8f, 1.7f, 1.5f, 1.0f); // Desert Sand SpecOps
			if (CurrentCamo == EOperatorCamo::WoodlandTactical)
			{
				CamoTint = FLinearColor(1.2f, 1.6f, 1.1f, 1.0f); // Tactical Olive Drab
			}
			else if (CurrentCamo == EOperatorCamo::StealthBlackout)
			{
				CamoTint = FLinearColor(1.1f, 1.2f, 1.4f, 1.0f); // Stealth Slate
			}
			BodyMID->SetVectorParameterValue(TEXT("CamoColorPrimary"), CamoTint);
			BodyMID->SetScalarParameterValue(TEXT("RoughnessBase"), 0.45f);
		}

		if (UMaterialInstanceDynamic* VisorMID = OperatorMesh->CreateDynamicMaterialInstance(1))
		{
			VisorMID->SetVectorParameterValue(TEXT("CamoColorPrimary"), FLinearColor(0.0f, 0.90f, 1.0f));
			VisorMID->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.0f, 0.90f, 1.0f));
		}
	}
}"""

new_camo = """void AFPSCharacterPreviewActor::ApplyCamoColors()
{
	if (OperatorMesh)
	{
		UMaterialInstanceDynamic* BodyMID = Cast<UMaterialInstanceDynamic>(OperatorMesh->GetMaterial(0));
		if (!BodyMID)
		{
			BodyMID = OperatorMesh->CreateDynamicMaterialInstance(0);
		}
		if (BodyMID)
		{
			// Clean tactical tints for Vanguard Spec-Ops Soldier
			FLinearColor CamoTint = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f); // UrbanDigital (Task Force Echo Slate / Default)
			if (CurrentCamo == EOperatorCamo::WoodlandTactical)
			{
				CamoTint = FLinearColor(0.85f, 1.05f, 0.85f, 1.0f); // Tactical Woodland Olive
			}
			else if (CurrentCamo == EOperatorCamo::DesertSpecOps)
			{
				CamoTint = FLinearColor(1.15f, 1.05f, 0.90f, 1.0f); // Desert Sand SpecOps
			}
			else if (CurrentCamo == EOperatorCamo::StealthBlackout)
			{
				CamoTint = FLinearColor(0.70f, 0.72f, 0.76f, 1.0f); // Stealth Blackout Dark Slate
			}
			BodyMID->SetVectorParameterValue(TEXT("CamoColorPrimary"), CamoTint);
			BodyMID->SetScalarParameterValue(TEXT("RoughnessBase"), 0.45f);
		}

		if (UMaterialInstanceDynamic* VisorMID = Cast<UMaterialInstanceDynamic>(OperatorMesh->GetMaterial(1)))
		{
			VisorMID->SetVectorParameterValue(TEXT("CamoColorPrimary"), FLinearColor(0.0f, 0.898f, 1.0f));
			VisorMID->SetVectorParameterValue(TEXT("EmissiveColor"), FLinearColor(0.0f, 0.898f, 1.0f));
			VisorMID->SetScalarParameterValue(TEXT("EmissiveIntensity"), 4.0f);
		}
		else if (UMaterialInstanceDynamic* NewVisorMID = OperatorMesh->CreateDynamicMaterialInstance(1))
		{
			NewVisorMID->SetVectorParameterValue(TEXT("CamoColorPrimary"), FLinearColor(0.0f, 0.898f, 1.0f));
			NewVisorMID->SetVectorParameterValue(TEXT("EmissiveColor"), FLinearColor(0.0f, 0.898f, 1.0f));
			NewVisorMID->SetScalarParameterValue(TEXT("EmissiveIntensity"), 4.0f);
		}
	}
}"""

assert old_camo in lf, "old_camo not found!"
lf = lf.replace(old_camo, new_camo)

# Write with 100% CRLF
crlf = lf.replace('\n', '\r\n')
with open(preview_path, 'wb') as f:
    f.write(crlf.encode('utf-8'))

print("=== SUCCESSFULLY UPDATED FPSCharacterPreviewActor.cpp ===")
