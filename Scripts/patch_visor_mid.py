import os

preview_path = "Source/NobleFermi/Character/FPSCharacterPreviewActor.cpp"

with open(preview_path, 'r', encoding='utf-8') as f:
    content = f.read()

lf = content.replace('\r\n', '\n')

old_part = """		if (UMaterialInstanceDynamic* VisorMID = Cast<UMaterialInstanceDynamic>(OperatorMesh->GetMaterial(1)))
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
		}"""

new_part = """		if (UMaterialInstanceDynamic* VisorMID = Cast<UMaterialInstanceDynamic>(OperatorMesh->GetMaterial(1)))
		{
			VisorMID->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.01f, 0.08f, 0.12f));
			VisorMID->SetVectorParameterValue(TEXT("EmissiveColor"), FLinearColor(0.0f, 0.898f, 1.0f));
			VisorMID->SetScalarParameterValue(TEXT("EmissiveIntensity"), 1.6f);
		}
		else if (UMaterialInstanceDynamic* NewVisorMID = OperatorMesh->CreateDynamicMaterialInstance(1))
		{
			NewVisorMID->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.01f, 0.08f, 0.12f));
			NewVisorMID->SetVectorParameterValue(TEXT("EmissiveColor"), FLinearColor(0.0f, 0.898f, 1.0f));
			NewVisorMID->SetScalarParameterValue(TEXT("EmissiveIntensity"), 1.6f);
		}"""

assert old_part in lf, "old_part not found!"
lf = lf.replace(old_part, new_part)

crlf = lf.replace('\n', '\r\n')
with open(preview_path, 'wb') as f:
    f.write(crlf.encode('utf-8'))

print("Updated VisorMID in FPSCharacterPreviewActor.cpp!")
