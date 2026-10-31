import os

file_path = 'Source/NobleFermi/Character/FPSCharacterPreviewActor.cpp'
with open(file_path, 'r', encoding='utf-8') as f:
    c = f.read()

# Replace constructor material finders
old_block1 = """	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BaseBodyMatFinder(TEXT("/Game/Characters/BaseMale/Materials/M_Operator_BaseBody.M_Operator_BaseBody"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> VisorMatFinder(TEXT("/Game/Characters/BaseMale/Materials/M_Soldier_Visor.M_Soldier_Visor"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> AKSteelMatFinder(TEXT("/Game/Characters/BaseMale/Materials/M_AK47_Steel.M_AK47_Steel"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> AKWoodMatFinder(TEXT("/Game/Characters/BaseMale/Materials/M_AK47_Wood.M_AK47_Wood"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> AKBakeMatFinder(TEXT("/Game/Characters/BaseMale/Materials/M_AK47_Bakelite.M_AK47_Bakelite"));

	if (BaseBodyMatFinder.Succeeded()) OperatorMesh->SetMaterial(0, BaseBodyMatFinder.Object);
	if (VisorMatFinder.Succeeded()) OperatorMesh->SetMaterial(1, VisorMatFinder.Object);
	if (AKSteelMatFinder.Succeeded()) OperatorMesh->SetMaterial(2, AKSteelMatFinder.Object);
	if (AKWoodMatFinder.Succeeded()) OperatorMesh->SetMaterial(3, AKWoodMatFinder.Object);
	if (AKBakeMatFinder.Succeeded()) OperatorMesh->SetMaterial(4, AKBakeMatFinder.Object);"""

new_block1 = """	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BaseBodyMatFinder(TEXT("/Game/Characters/BaseMale/Materials/M_Operator_BaseBody.M_Operator_BaseBody"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ArmorMatFinder(TEXT("/Game/Characters/BaseMale/Materials/M_Operator_Armor.M_Operator_Armor"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> VisorMatFinder(TEXT("/Game/Characters/BaseMale/Materials/M_Soldier_Visor.M_Soldier_Visor"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> PatchMatFinder(TEXT("/Game/Characters/BaseMale/Materials/M_Operator_Patch.M_Operator_Patch"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> AKSteelMatFinder(TEXT("/Game/Characters/BaseMale/Materials/M_AK47_Steel.M_AK47_Steel"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> AKWoodMatFinder(TEXT("/Game/Characters/BaseMale/Materials/M_AK47_Wood.M_AK47_Wood"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> AKBakeMatFinder(TEXT("/Game/Characters/BaseMale/Materials/M_AK47_Bakelite.M_AK47_Bakelite"));

	if (BaseBodyMatFinder.Succeeded()) OperatorMesh->SetMaterial(0, BaseBodyMatFinder.Object);
	if (ArmorMatFinder.Succeeded()) OperatorMesh->SetMaterial(1, ArmorMatFinder.Object);
	if (VisorMatFinder.Succeeded()) OperatorMesh->SetMaterial(2, VisorMatFinder.Object);
	if (PatchMatFinder.Succeeded()) OperatorMesh->SetMaterial(3, PatchMatFinder.Object);
	if (AKSteelMatFinder.Succeeded()) OperatorMesh->SetMaterial(4, AKSteelMatFinder.Object);
	if (AKWoodMatFinder.Succeeded()) OperatorMesh->SetMaterial(5, AKWoodMatFinder.Object);
	if (AKBakeMatFinder.Succeeded()) OperatorMesh->SetMaterial(6, AKBakeMatFinder.Object);"""

c = c.replace(old_block1, new_block1)

# Replace BeginPlay materials
old_block2 = """		if (UMaterialInterface* BodyMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Characters/BaseMale/Materials/M_Operator_BaseBody.M_Operator_BaseBody")))
			OperatorMesh->SetMaterial(0, BodyMat);
		if (UMaterialInterface* VisorMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Characters/BaseMale/Materials/M_Soldier_Visor.M_Soldier_Visor")))
			OperatorMesh->SetMaterial(1, VisorMat);
		if (UMaterialInterface* AKSteel = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Characters/BaseMale/Materials/M_AK47_Steel.M_AK47_Steel")))
			OperatorMesh->SetMaterial(2, AKSteel);
		if (UMaterialInterface* AKWood = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Characters/BaseMale/Materials/M_AK47_Wood.M_AK47_Wood")))
			OperatorMesh->SetMaterial(3, AKWood);
		if (UMaterialInterface* AKBake = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Characters/BaseMale/Materials/M_AK47_Bakelite.M_AK47_Bakelite")))
			OperatorMesh->SetMaterial(4, AKBake);"""

new_block2 = """		if (UMaterialInterface* BodyMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Characters/BaseMale/Materials/M_Operator_BaseBody.M_Operator_BaseBody")))
			OperatorMesh->SetMaterial(0, BodyMat);
		if (UMaterialInterface* ArmorMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Characters/BaseMale/Materials/M_Operator_Armor.M_Operator_Armor")))
			OperatorMesh->SetMaterial(1, ArmorMat);
		if (UMaterialInterface* VisorMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Characters/BaseMale/Materials/M_Soldier_Visor.M_Soldier_Visor")))
			OperatorMesh->SetMaterial(2, VisorMat);
		if (UMaterialInterface* PatchMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Characters/BaseMale/Materials/M_Operator_Patch.M_Operator_Patch")))
			OperatorMesh->SetMaterial(3, PatchMat);
		if (UMaterialInterface* AKSteel = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Characters/BaseMale/Materials/M_AK47_Steel.M_AK47_Steel")))
			OperatorMesh->SetMaterial(4, AKSteel);
		if (UMaterialInterface* AKWood = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Characters/BaseMale/Materials/M_AK47_Wood.M_AK47_Wood")))
			OperatorMesh->SetMaterial(5, AKWood);
		if (UMaterialInterface* AKBake = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Characters/BaseMale/Materials/M_AK47_Bakelite.M_AK47_Bakelite")))
			OperatorMesh->SetMaterial(6, AKBake);"""

c = c.replace(old_block2, new_block2)

# Replace ApplyCamoColors visor MID
old_block3 = """		if (UMaterialInstanceDynamic* VisorMID = Cast<UMaterialInstanceDynamic>(OperatorMesh->GetMaterial(1)))
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

new_block3 = """		if (UMaterialInstanceDynamic* VisorMID = Cast<UMaterialInstanceDynamic>(OperatorMesh->GetMaterial(2)))
		{
			VisorMID->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.005f, 0.008f, 0.012f));
			VisorMID->SetVectorParameterValue(TEXT("EmissiveColor"), FLinearColor(0.0f, 1.4f, 0.35f));
			VisorMID->SetScalarParameterValue(TEXT("EmissiveIntensity"), 2.5f);
		}
		else if (UMaterialInstanceDynamic* NewVisorMID = OperatorMesh->CreateDynamicMaterialInstance(2))
		{
			NewVisorMID->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.005f, 0.008f, 0.012f));
			NewVisorMID->SetVectorParameterValue(TEXT("EmissiveColor"), FLinearColor(0.0f, 1.4f, 0.35f));
			NewVisorMID->SetScalarParameterValue(TEXT("EmissiveIntensity"), 2.5f);
		}"""

c = c.replace(old_block3, new_block3)

# Replace ApplyWeaponFinish steel MID (slot 4)
old_block4 = """		UMaterialInstanceDynamic* SteelMID = Cast<UMaterialInstanceDynamic>(OperatorMesh->GetMaterial(2));
		if (!SteelMID)
		{
			SteelMID = OperatorMesh->CreateDynamicMaterialInstance(2);
		}"""

new_block4 = """		UMaterialInstanceDynamic* SteelMID = Cast<UMaterialInstanceDynamic>(OperatorMesh->GetMaterial(4));
		if (!SteelMID)
		{
			SteelMID = OperatorMesh->CreateDynamicMaterialInstance(4);
		}"""

c = c.replace(old_block4, new_block4)

with open(file_path, 'w', encoding='utf-8') as f:
    f.write(c)

print('Updated FPSCharacterPreviewActor.cpp successfully!')
