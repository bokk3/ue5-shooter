import os

file_path = 'Source/NobleFermi/Character/FPSCharacter.cpp'
with open(file_path, 'r', encoding='utf-8') as f:
    c = f.read()

# 1. Update Constructor
old_c = """	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BaseBodyMatFinder(TEXT("/Game/Characters/BaseMale/Materials/M_Operator_BaseBody.M_Operator_BaseBody"));
	if (BaseBodyMatFinder.Succeeded())
	{
		GetMesh()->SetMaterial(0, BaseBodyMatFinder.Object);
	}"""

new_c = """	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BaseBodyMatFinder(TEXT("/Game/Characters/BaseMale/Materials/M_Operator_BaseBody.M_Operator_BaseBody"));
	if (BaseBodyMatFinder.Succeeded())
	{
		GetMesh()->SetMaterial(0, BaseBodyMatFinder.Object);
	}
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ArmorMatFinder(TEXT("/Game/Characters/BaseMale/Materials/M_Operator_Armor.M_Operator_Armor"));
	if (ArmorMatFinder.Succeeded())
	{
		GetMesh()->SetMaterial(1, ArmorMatFinder.Object);
	}
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> VisorMatFinder(TEXT("/Game/Characters/BaseMale/Materials/M_Soldier_Visor.M_Soldier_Visor"));
	if (VisorMatFinder.Succeeded())
	{
		GetMesh()->SetMaterial(2, VisorMatFinder.Object);
	}
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> PatchMatFinder(TEXT("/Game/Characters/BaseMale/Materials/M_Operator_Patch.M_Operator_Patch"));
	if (PatchMatFinder.Succeeded())
	{
		GetMesh()->SetMaterial(3, PatchMatFinder.Object);
	}"""

c = c.replace(old_c, new_c)

# 2. Update BeginPlay
old_b = """		UMaterialInterface* CurrentMat = GetMesh()->GetMaterial(0);
		if (!CurrentMat || !CurrentMat->GetName().Contains(TEXT("M_Operator_BaseBody")))
		{
			UMaterialInterface* BaseBodyMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Characters/BaseMale/Materials/M_Operator_BaseBody.M_Operator_BaseBody"));
			if (BaseBodyMat)
			{
				GetMesh()->SetMaterial(0, BaseBodyMat);
			}
		}"""

new_b = """		UMaterialInterface* CurrentMat = GetMesh()->GetMaterial(0);
		if (!CurrentMat || !CurrentMat->GetName().Contains(TEXT("M_Operator_BaseBody")))
		{
			UMaterialInterface* BaseBodyMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Characters/BaseMale/Materials/M_Operator_BaseBody.M_Operator_BaseBody"));
			if (BaseBodyMat)
			{
				GetMesh()->SetMaterial(0, BaseBodyMat);
			}
		}
		if (UMaterialInterface* ArmorMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Characters/BaseMale/Materials/M_Operator_Armor.M_Operator_Armor")))
		{
			GetMesh()->SetMaterial(1, ArmorMat);
		}
		if (UMaterialInterface* VisorMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Characters/BaseMale/Materials/M_Soldier_Visor.M_Soldier_Visor")))
		{
			GetMesh()->SetMaterial(2, VisorMat);
		}
		if (UMaterialInterface* PatchMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Characters/BaseMale/Materials/M_Operator_Patch.M_Operator_Patch")))
		{
			GetMesh()->SetMaterial(3, PatchMat);
		}"""

c = c.replace(old_b, new_b)

with open(file_path, 'w', encoding='utf-8') as f:
    f.write(c)

print('Updated FPSCharacter.cpp successfully!')
