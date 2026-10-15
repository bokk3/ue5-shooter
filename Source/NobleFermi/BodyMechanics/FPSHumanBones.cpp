#include "BodyMechanics/FPSHumanBones.h"

TArray<FFPSHumanBoneData> FFPSHumanSkeleton::Registry;
TMap<FName, int32> FFPSHumanSkeleton::BoneNameToIndexMap;
bool FFPSHumanSkeleton::bIsInitialized = false;

const TArray<FFPSHumanBoneData>& FFPSHumanSkeleton::GetMaleBoneRegistry()
{
	if (!bIsInitialized)
	{
		InitializeRegistry();
	}
	return Registry;
}

const FFPSHumanBoneData* FFPSHumanSkeleton::FindBoneDataByName(FName InBoneName)
{
	if (!bIsInitialized)
	{
		InitializeRegistry();
	}

	const int32* FoundIndex = BoneNameToIndexMap.Find(InBoneName);
	if (FoundIndex && Registry.IsValidIndex(*FoundIndex))
	{
		return &Registry[*FoundIndex];
	}
	return nullptr;
}

float FFPSHumanSkeleton::GetDamageMultiplierForBone(FName InBoneName)
{
	const FFPSHumanBoneData* Bone = FindBoneDataByName(InBoneName);
	if (Bone)
	{
		return Bone->DamageMultiplier;
	}

	// Fallback heuristic based on standard Unreal humanoid bone naming
	const FString BoneStr = InBoneName.ToString().ToLower();
	if (BoneStr.Contains(TEXT("head")))
	{
		return 2.5f;
	}
	if (BoneStr.Contains(TEXT("neck")))
	{
		return 2.0f;
	}
	if (BoneStr.Contains(TEXT("spine_03")) || BoneStr.Contains(TEXT("chest")))
	{
		return 1.25f;
	}
	if (BoneStr.Contains(TEXT("arm")) || BoneStr.Contains(TEXT("hand")))
	{
		return 0.8f;
	}
	if (BoneStr.Contains(TEXT("thigh")) || BoneStr.Contains(TEXT("leg")) || BoneStr.Contains(TEXT("foot")) || BoneStr.Contains(TEXT("calf")))
	{
		return 0.85f;
	}

	return 1.0f;
}

EFPSBodyDamageZone FFPSHumanSkeleton::GetDamageZoneForBone(FName InBoneName)
{
	const FFPSHumanBoneData* Bone = FindBoneDataByName(InBoneName);
	if (Bone)
	{
		return Bone->DamageZone;
	}

	const FString BoneStr = InBoneName.ToString().ToLower();
	if (BoneStr.Contains(TEXT("head"))) return EFPSBodyDamageZone::Head;
	if (BoneStr.Contains(TEXT("neck"))) return EFPSBodyDamageZone::Neck;
	if (BoneStr.Contains(TEXT("spine_03")) || BoneStr.Contains(TEXT("chest"))) return EFPSBodyDamageZone::UpperTorso;
	if (BoneStr.Contains(TEXT("spine"))) return EFPSBodyDamageZone::LowerTorso;
	if (BoneStr.Contains(TEXT("pelvis"))) return EFPSBodyDamageZone::Pelvis;
	if (BoneStr.Contains(TEXT("upperarm"))) return EFPSBodyDamageZone::UpperArms;
	if (BoneStr.Contains(TEXT("lowerarm"))) return EFPSBodyDamageZone::LowerArms;
	if (BoneStr.Contains(TEXT("hand"))) return EFPSBodyDamageZone::Hands;
	if (BoneStr.Contains(TEXT("thigh"))) return EFPSBodyDamageZone::Thighs;
	if (BoneStr.Contains(TEXT("calf"))) return EFPSBodyDamageZone::Calves;
	if (BoneStr.Contains(TEXT("foot"))) return EFPSBodyDamageZone::Feet;

	return EFPSBodyDamageZone::LowerTorso;
}

bool FFPSHumanSkeleton::IsBoneArmorProtected(FName InBoneName)
{
	const FFPSHumanBoneData* Bone = FindBoneDataByName(InBoneName);
	return Bone ? Bone->bIsPlateCarrierProtected : false;
}

float FFPSHumanSkeleton::GetTotalHumanMaleMassKg()
{
	if (!bIsInitialized)
	{
		InitializeRegistry();
	}

	float TotalMass = 0.0f;
	for (const FFPSHumanBoneData& Bone : Registry)
	{
		TotalMass += Bone.MassKg;
	}
	return TotalMass;
}

void FFPSHumanSkeleton::InitializeRegistry()
{
	if (bIsInitialized)
	{
		return;
	}

	Registry.Reset();
	BoneNameToIndexMap.Reset();

	auto AddBone = [](const FFPSHumanBoneData& Data)
	{
		const int32 NewIndex = Registry.Add(Data);
		BoneNameToIndexMap.Add(Data.BoneName, NewIndex);
	};

	// 0. Root (Ground Reference)
	{
		FFPSHumanBoneData B;
		B.BoneName = TEXT("root");
		B.ParentBoneName = NAME_None;
		B.BoneType = EFPSHumanBoneType::Root;
		B.DamageZone = EFPSBodyDamageZone::LowerTorso;
		B.MassKg = 0.0f;
		B.LocalOffsetCm = FVector::ZeroVector;
		B.CollisionRadiusCm = 1.0f;
		B.CollisionHalfHeightCm = 1.0f;
		B.DamageMultiplier = 1.0f;
		B.bIsPlateCarrierProtected = false;
		AddBone(B);
	}

	// 1. Pelvis / Sacrum (Locomotion Center of Gravity, 96cm above ground)
	{
		FFPSHumanBoneData B;
		B.BoneName = TEXT("pelvis");
		B.ParentBoneName = TEXT("root");
		B.BoneType = EFPSHumanBoneType::Pelvis;
		B.DamageZone = EFPSBodyDamageZone::Pelvis;
		B.MassKg = 14.5f;
		B.LocalOffsetCm = FVector(0.0f, 0.0f, 96.0f);
		B.CollisionRadiusCm = 15.0f;
		B.CollisionHalfHeightCm = 14.0f;
		B.DamageMultiplier = 1.0f;
		B.bIsPlateCarrierProtected = false;
		B.JointLimits.MinAngularLimit = FRotator(-20.0f, -20.0f, -15.0f);
		B.JointLimits.MaxAngularLimit = FRotator(20.0f, 20.0f, 15.0f);
		B.JointLimits.AngularStiffness = 3000.0f;
		B.JointLimits.AngularDamping = 250.0f;
		AddBone(B);
	}

	// 2. Spine Lumbar (L1-L5 lower abdomen)
	{
		FFPSHumanBoneData B;
		B.BoneName = TEXT("spine_01");
		B.ParentBoneName = TEXT("pelvis");
		B.BoneType = EFPSHumanBoneType::SpineLumbar;
		B.DamageZone = EFPSBodyDamageZone::LowerTorso;
		B.MassKg = 8.0f;
		B.LocalOffsetCm = FVector(0.0f, 0.0f, 14.0f);
		B.CollisionRadiusCm = 14.0f;
		B.CollisionHalfHeightCm = 12.0f;
		B.DamageMultiplier = 1.0f;
		B.bIsPlateCarrierProtected = false;
		B.JointLimits.MinAngularLimit = FRotator(-25.0f, -20.0f, -20.0f);
		B.JointLimits.MaxAngularLimit = FRotator(35.0f, 20.0f, 20.0f);
		B.JointLimits.AngularStiffness = 2500.0f;
		B.JointLimits.AngularDamping = 200.0f;
		AddBone(B);
	}

	// 3. Spine Thoracic Mid (T7-T12 mid-torso, lower ribcage)
	{
		FFPSHumanBoneData B;
		B.BoneName = TEXT("spine_02");
		B.ParentBoneName = TEXT("spine_01");
		B.BoneType = EFPSHumanBoneType::SpineThoracicMid;
		B.DamageZone = EFPSBodyDamageZone::UpperTorso;
		B.MassKg = 12.0f;
		B.LocalOffsetCm = FVector(0.0f, 0.0f, 16.0f);
		B.CollisionRadiusCm = 16.0f;
		B.CollisionHalfHeightCm = 14.0f;
		B.DamageMultiplier = 1.15f;
		B.bIsPlateCarrierProtected = true;
		B.JointLimits.MinAngularLimit = FRotator(-15.0f, -25.0f, -15.0f);
		B.JointLimits.MaxAngularLimit = FRotator(20.0f, 25.0f, 15.0f);
		B.JointLimits.AngularStiffness = 2500.0f;
		B.JointLimits.AngularDamping = 200.0f;
		AddBone(B);
	}

	// 4. Spine Thoracic Upper (T1-T6 chest, heart, lungs, pectorals)
	{
		FFPSHumanBoneData B;
		B.BoneName = TEXT("spine_03");
		B.ParentBoneName = TEXT("spine_02");
		B.BoneType = EFPSHumanBoneType::SpineThoracicUpper;
		B.DamageZone = EFPSBodyDamageZone::UpperTorso;
		B.MassKg = 14.0f;
		B.LocalOffsetCm = FVector(0.0f, 0.0f, 16.0f);
		B.CollisionRadiusCm = 17.5f;
		B.CollisionHalfHeightCm = 15.0f;
		B.DamageMultiplier = 1.25f;
		B.bIsPlateCarrierProtected = true;
		B.JointLimits.MinAngularLimit = FRotator(-15.0f, -20.0f, -12.0f);
		B.JointLimits.MaxAngularLimit = FRotator(15.0f, 20.0f, 12.0f);
		B.JointLimits.AngularStiffness = 2800.0f;
		B.JointLimits.AngularDamping = 220.0f;
		AddBone(B);
	}

	// 5. Neck (C1-C7 cervical column, carotid artery)
	{
		FFPSHumanBoneData B;
		B.BoneName = TEXT("neck_01");
		B.ParentBoneName = TEXT("spine_03");
		B.BoneType = EFPSHumanBoneType::Neck;
		B.DamageZone = EFPSBodyDamageZone::Neck;
		B.MassKg = 2.2f;
		B.LocalOffsetCm = FVector(0.0f, 0.0f, 12.0f);
		B.CollisionRadiusCm = 7.0f;
		B.CollisionHalfHeightCm = 8.0f;
		B.DamageMultiplier = 2.0f;
		B.bIsPlateCarrierProtected = false;
		B.JointLimits.MinAngularLimit = FRotator(-50.0f, -75.0f, -35.0f);
		B.JointLimits.MaxAngularLimit = FRotator(60.0f, 75.0f, 35.0f);
		B.JointLimits.AngularStiffness = 1800.0f;
		B.JointLimits.AngularDamping = 150.0f;
		AddBone(B);
	}

	// 6. Head (Cranium, brain stem, facial bones, eye line ~168cm, vertex ~180cm)
	{
		FFPSHumanBoneData B;
		B.BoneName = TEXT("head");
		B.ParentBoneName = TEXT("neck_01");
		B.BoneType = EFPSHumanBoneType::Head;
		B.DamageZone = EFPSBodyDamageZone::Head;
		B.MassKg = 4.8f;
		B.LocalOffsetCm = FVector(0.0f, 2.0f, 15.0f);
		B.CollisionRadiusCm = 9.5f;
		B.CollisionHalfHeightCm = 11.0f;
		B.DamageMultiplier = 2.5f;
		B.bIsPlateCarrierProtected = false;
		B.JointLimits.MinAngularLimit = FRotator(-35.0f, -50.0f, -25.0f);
		B.JointLimits.MaxAngularLimit = FRotator(40.0f, 50.0f, 25.0f);
		B.JointLimits.AngularStiffness = 1600.0f;
		B.JointLimits.AngularDamping = 140.0f;
		AddBone(B);
	}

	// 7. Left Clavicle
	{
		FFPSHumanBoneData B;
		B.BoneName = TEXT("clavicle_l");
		B.ParentBoneName = TEXT("spine_03");
		B.BoneType = EFPSHumanBoneType::ClavicleLeft;
		B.DamageZone = EFPSBodyDamageZone::UpperTorso;
		B.MassKg = 1.2f;
		B.LocalOffsetCm = FVector(-14.0f, -2.0f, 10.0f);
		B.CollisionRadiusCm = 5.0f;
		B.CollisionHalfHeightCm = 8.0f;
		B.DamageMultiplier = 1.0f;
		B.bIsPlateCarrierProtected = true;
		B.JointLimits.MinAngularLimit = FRotator(-20.0f, -30.0f, -20.0f);
		B.JointLimits.MaxAngularLimit = FRotator(20.0f, 30.0f, 20.0f);
		AddBone(B);
	}

	// 8. Right Clavicle
	{
		FFPSHumanBoneData B;
		B.BoneName = TEXT("clavicle_r");
		B.ParentBoneName = TEXT("spine_03");
		B.BoneType = EFPSHumanBoneType::ClavicleRight;
		B.DamageZone = EFPSBodyDamageZone::UpperTorso;
		B.MassKg = 1.2f;
		B.LocalOffsetCm = FVector(14.0f, -2.0f, 10.0f);
		B.CollisionRadiusCm = 5.0f;
		B.CollisionHalfHeightCm = 8.0f;
		B.DamageMultiplier = 1.0f;
		B.bIsPlateCarrierProtected = true;
		B.JointLimits.MinAngularLimit = FRotator(-20.0f, -30.0f, -20.0f);
		B.JointLimits.MaxAngularLimit = FRotator(20.0f, 30.0f, 20.0f);
		AddBone(B);
	}

	// 9. Left Upper Arm (Humerus)
	{
		FFPSHumanBoneData B;
		B.BoneName = TEXT("upperarm_l");
		B.ParentBoneName = TEXT("clavicle_l");
		B.BoneType = EFPSHumanBoneType::UpperArmLeft;
		B.DamageZone = EFPSBodyDamageZone::UpperArms;
		B.MassKg = 3.2f;
		B.LocalOffsetCm = FVector(-18.0f, 0.0f, -3.0f);
		B.CollisionRadiusCm = 5.5f;
		B.CollisionHalfHeightCm = 15.0f;
		B.DamageMultiplier = 0.8f;
		B.bIsPlateCarrierProtected = false;
		B.JointLimits.MinAngularLimit = FRotator(-180.0f, -130.0f, -90.0f);
		B.JointLimits.MaxAngularLimit = FRotator(60.0f, 40.0f, 90.0f);
		B.JointLimits.AngularStiffness = 1400.0f;
		B.JointLimits.AngularDamping = 120.0f;
		AddBone(B);
	}

	// 10. Right Upper Arm (Humerus)
	{
		FFPSHumanBoneData B;
		B.BoneName = TEXT("upperarm_r");
		B.ParentBoneName = TEXT("clavicle_r");
		B.BoneType = EFPSHumanBoneType::UpperArmRight;
		B.DamageZone = EFPSBodyDamageZone::UpperArms;
		B.MassKg = 3.2f;
		B.LocalOffsetCm = FVector(18.0f, 0.0f, -3.0f);
		B.CollisionRadiusCm = 5.5f;
		B.CollisionHalfHeightCm = 15.0f;
		B.DamageMultiplier = 0.8f;
		B.bIsPlateCarrierProtected = false;
		B.JointLimits.MinAngularLimit = FRotator(-180.0f, -40.0f, -90.0f);
		B.JointLimits.MaxAngularLimit = FRotator(60.0f, 130.0f, 90.0f);
		B.JointLimits.AngularStiffness = 1400.0f;
		B.JointLimits.AngularDamping = 120.0f;
		AddBone(B);
	}

	// 11. Left Forearm (Radius & Ulna)
	{
		FFPSHumanBoneData B;
		B.BoneName = TEXT("lowerarm_l");
		B.ParentBoneName = TEXT("upperarm_l");
		B.BoneType = EFPSHumanBoneType::ForearmLeft;
		B.DamageZone = EFPSBodyDamageZone::LowerArms;
		B.MassKg = 1.8f;
		B.LocalOffsetCm = FVector(0.0f, 0.0f, -27.0f);
		B.CollisionRadiusCm = 4.8f;
		B.CollisionHalfHeightCm = 14.0f;
		B.DamageMultiplier = 0.75f;
		B.bIsPlateCarrierProtected = false;
		B.JointLimits.MinAngularLimit = FRotator(0.0f, 0.0f, -90.0f);
		B.JointLimits.MaxAngularLimit = FRotator(145.0f, 0.0f, 80.0f);
		B.JointLimits.AngularStiffness = 1200.0f;
		B.JointLimits.AngularDamping = 100.0f;
		AddBone(B);
	}

	// 12. Right Forearm (Radius & Ulna)
	{
		FFPSHumanBoneData B;
		B.BoneName = TEXT("lowerarm_r");
		B.ParentBoneName = TEXT("upperarm_r");
		B.BoneType = EFPSHumanBoneType::ForearmRight;
		B.DamageZone = EFPSBodyDamageZone::LowerArms;
		B.MassKg = 1.8f;
		B.LocalOffsetCm = FVector(0.0f, 0.0f, -27.0f);
		B.CollisionRadiusCm = 4.8f;
		B.CollisionHalfHeightCm = 14.0f;
		B.DamageMultiplier = 0.75f;
		B.bIsPlateCarrierProtected = false;
		B.JointLimits.MinAngularLimit = FRotator(0.0f, 0.0f, -80.0f);
		B.JointLimits.MaxAngularLimit = FRotator(145.0f, 0.0f, 90.0f);
		B.JointLimits.AngularStiffness = 1200.0f;
		B.JointLimits.AngularDamping = 100.0f;
		AddBone(B);
	}

	// 13. Left Hand & Wrist
	{
		FFPSHumanBoneData B;
		B.BoneName = TEXT("hand_l");
		B.ParentBoneName = TEXT("lowerarm_l");
		B.BoneType = EFPSHumanBoneType::HandLeft;
		B.DamageZone = EFPSBodyDamageZone::Hands;
		B.MassKg = 0.6f;
		B.LocalOffsetCm = FVector(0.0f, 0.0f, -24.0f);
		B.CollisionRadiusCm = 4.5f;
		B.CollisionHalfHeightCm = 9.0f;
		B.DamageMultiplier = 0.7f;
		B.bIsPlateCarrierProtected = false;
		B.JointLimits.MinAngularLimit = FRotator(-70.0f, -25.0f, -15.0f);
		B.JointLimits.MaxAngularLimit = FRotator(75.0f, 35.0f, 15.0f);
		AddBone(B);
	}

	// 14. Right Hand & Wrist (Weapon trigger hand)
	{
		FFPSHumanBoneData B;
		B.BoneName = TEXT("hand_r");
		B.ParentBoneName = TEXT("lowerarm_r");
		B.BoneType = EFPSHumanBoneType::HandRight;
		B.DamageZone = EFPSBodyDamageZone::Hands;
		B.MassKg = 0.6f;
		B.LocalOffsetCm = FVector(0.0f, 0.0f, -24.0f);
		B.CollisionRadiusCm = 4.5f;
		B.CollisionHalfHeightCm = 9.0f;
		B.DamageMultiplier = 0.7f;
		B.bIsPlateCarrierProtected = false;
		B.JointLimits.MinAngularLimit = FRotator(-70.0f, -35.0f, -15.0f);
		B.JointLimits.MaxAngularLimit = FRotator(75.0f, 25.0f, 15.0f);
		AddBone(B);
	}

	// 15. Left Thigh (Femur, hip ball joint, femoral artery)
	{
		FFPSHumanBoneData B;
		B.BoneName = TEXT("thigh_l");
		B.ParentBoneName = TEXT("pelvis");
		B.BoneType = EFPSHumanBoneType::ThighLeft;
		B.DamageZone = EFPSBodyDamageZone::Thighs;
		B.MassKg = 9.6f;
		B.LocalOffsetCm = FVector(-10.5f, 0.0f, -10.0f);
		B.CollisionRadiusCm = 8.5f;
		B.CollisionHalfHeightCm = 21.0f;
		B.DamageMultiplier = 0.9f;
		B.bIsPlateCarrierProtected = false;
		B.JointLimits.MinAngularLimit = FRotator(-125.0f, -40.0f, -45.0f);
		B.JointLimits.MaxAngularLimit = FRotator(20.0f, 45.0f, 45.0f);
		B.JointLimits.AngularStiffness = 3200.0f;
		B.JointLimits.AngularDamping = 300.0f;
		AddBone(B);
	}

	// 16. Right Thigh (Femur)
	{
		FFPSHumanBoneData B;
		B.BoneName = TEXT("thigh_r");
		B.ParentBoneName = TEXT("pelvis");
		B.BoneType = EFPSHumanBoneType::ThighRight;
		B.DamageZone = EFPSBodyDamageZone::Thighs;
		B.MassKg = 9.6f;
		B.LocalOffsetCm = FVector(10.5f, 0.0f, -10.0f);
		B.CollisionRadiusCm = 8.5f;
		B.CollisionHalfHeightCm = 21.0f;
		B.DamageMultiplier = 0.9f;
		B.bIsPlateCarrierProtected = false;
		B.JointLimits.MinAngularLimit = FRotator(-125.0f, -45.0f, -45.0f);
		B.JointLimits.MaxAngularLimit = FRotator(20.0f, 40.0f, 45.0f);
		B.JointLimits.AngularStiffness = 3200.0f;
		B.JointLimits.AngularDamping = 300.0f;
		AddBone(B);
	}

	// 17. Left Calf (Tibia & Fibula, Knee Hinge)
	{
		FFPSHumanBoneData B;
		B.BoneName = TEXT("calf_l");
		B.ParentBoneName = TEXT("thigh_l");
		B.BoneType = EFPSHumanBoneType::CalfLeft;
		B.DamageZone = EFPSBodyDamageZone::Calves;
		B.MassKg = 4.2f;
		B.LocalOffsetCm = FVector(0.0f, 0.0f, -42.0f);
		B.CollisionRadiusCm = 6.0f;
		B.CollisionHalfHeightCm = 19.5f;
		B.DamageMultiplier = 0.75f;
		B.bIsPlateCarrierProtected = false;
		B.JointLimits.MinAngularLimit = FRotator(0.0f, -10.0f, 0.0f);
		B.JointLimits.MaxAngularLimit = FRotator(142.0f, 10.0f, 0.0f);
		B.JointLimits.AngularStiffness = 2600.0f;
		B.JointLimits.AngularDamping = 220.0f;
		AddBone(B);
	}

	// 18. Right Calf (Tibia & Fibula, Knee Hinge)
	{
		FFPSHumanBoneData B;
		B.BoneName = TEXT("calf_r");
		B.ParentBoneName = TEXT("thigh_r");
		B.BoneType = EFPSHumanBoneType::CalfRight;
		B.DamageZone = EFPSBodyDamageZone::Calves;
		B.MassKg = 4.2f;
		B.LocalOffsetCm = FVector(0.0f, 0.0f, -42.0f);
		B.CollisionRadiusCm = 6.0f;
		B.CollisionHalfHeightCm = 19.5f;
		B.DamageMultiplier = 0.75f;
		B.bIsPlateCarrierProtected = false;
		B.JointLimits.MinAngularLimit = FRotator(0.0f, -10.0f, 0.0f);
		B.JointLimits.MaxAngularLimit = FRotator(142.0f, 10.0f, 0.0f);
		B.JointLimits.AngularStiffness = 2600.0f;
		B.JointLimits.AngularDamping = 220.0f;
		AddBone(B);
	}

	// 19. Left Foot & Ankle
	{
		FFPSHumanBoneData B;
		B.BoneName = TEXT("foot_l");
		B.ParentBoneName = TEXT("calf_l");
		B.BoneType = EFPSHumanBoneType::FootLeft;
		B.DamageZone = EFPSBodyDamageZone::Feet;
		B.MassKg = 1.2f;
		B.LocalOffsetCm = FVector(0.0f, 6.0f, -38.0f);
		B.CollisionRadiusCm = 5.0f;
		B.CollisionHalfHeightCm = 11.0f;
		B.DamageMultiplier = 0.65f;
		B.bIsPlateCarrierProtected = false;
		B.JointLimits.MinAngularLimit = FRotator(-20.0f, -15.0f, -25.0f);
		B.JointLimits.MaxAngularLimit = FRotator(50.0f, 15.0f, 25.0f);
		B.JointLimits.AngularStiffness = 1800.0f;
		B.JointLimits.AngularDamping = 160.0f;
		AddBone(B);
	}

	// 20. Right Foot & Ankle
	{
		FFPSHumanBoneData B;
		B.BoneName = TEXT("foot_r");
		B.ParentBoneName = TEXT("calf_r");
		B.BoneType = EFPSHumanBoneType::FootRight;
		B.DamageZone = EFPSBodyDamageZone::Feet;
		B.MassKg = 1.2f;
		B.LocalOffsetCm = FVector(0.0f, 6.0f, -38.0f);
		B.CollisionRadiusCm = 5.0f;
		B.CollisionHalfHeightCm = 11.0f;
		B.DamageMultiplier = 0.65f;
		B.bIsPlateCarrierProtected = false;
		B.JointLimits.MinAngularLimit = FRotator(-20.0f, -15.0f, -25.0f);
		B.JointLimits.MaxAngularLimit = FRotator(50.0f, 15.0f, 25.0f);
		B.JointLimits.AngularStiffness = 1800.0f;
		B.JointLimits.AngularDamping = 160.0f;
		AddBone(B);
	}

	bIsInitialized = true;
}
