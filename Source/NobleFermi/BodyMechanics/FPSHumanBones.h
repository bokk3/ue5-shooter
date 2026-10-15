#pragma once

#include "CoreMinimal.h"
#include "FPSHumanBones.generated.h"

/**
 * Anatomical bone classification for general adult human male (~180cm, ~82kg).
 */
UENUM(BlueprintType)
enum class EFPSHumanBoneType : uint8
{
	Root UMETA(DisplayName = "Root Ground"),
	Pelvis UMETA(DisplayName = "Pelvis / Sacrum"),
	SpineLumbar UMETA(DisplayName = "Lumbar Spine (Lower Back)"),
	SpineThoracicMid UMETA(DisplayName = "Mid-Thoracic Spine (Abdomen/Ribs)"),
	SpineThoracicUpper UMETA(DisplayName = "Upper Thoracic Spine (Chest/Pectoral)"),
	Neck UMETA(DisplayName = "Cervical Spine (Neck)"),
	Head UMETA(DisplayName = "Cranium & Facial Skeleton (Head)"),
	ClavicleLeft UMETA(DisplayName = "Left Clavicle (Collarbone)"),
	ClavicleRight UMETA(DisplayName = "Right Clavicle (Collarbone)"),
	UpperArmLeft UMETA(DisplayName = "Left Upper Arm (Humerus)"),
	UpperArmRight UMETA(DisplayName = "Right Upper Arm (Humerus)"),
	ForearmLeft UMETA(DisplayName = "Left Forearm (Radius & Ulna)"),
	ForearmRight UMETA(DisplayName = "Right Forearm (Radius & Ulna)"),
	HandLeft UMETA(DisplayName = "Left Hand & Fingers"),
	HandRight UMETA(DisplayName = "Right Hand & Fingers"),
	ThighLeft UMETA(DisplayName = "Left Thigh (Femur)"),
	ThighRight UMETA(DisplayName = "Right Thigh (Femur)"),
	CalfLeft UMETA(DisplayName = "Left Calf / Shin (Tibia & Fibula)"),
	CalfRight UMETA(DisplayName = "Right Calf / Shin (Tibia & Fibula)"),
	FootLeft UMETA(DisplayName = "Left Foot & Ankle"),
	FootRight UMETA(DisplayName = "Right Foot & Ankle"),
	ToesLeft UMETA(DisplayName = "Left Toes / Ball"),
	ToesRight UMETA(DisplayName = "Right Toes / Ball")
};

/**
 * Ballistic damage zoning for precise hitbox multipliers and armor protection.
 */
UENUM(BlueprintType)
enum class EFPSBodyDamageZone : uint8
{
	Head UMETA(DisplayName = "Head / Critical Lethal"),
	Neck UMETA(DisplayName = "Neck / Vulnerable"),
	UpperTorso UMETA(DisplayName = "Upper Torso (Vital Heart/Lungs)"),
	LowerTorso UMETA(DisplayName = "Lower Torso (Stomach/Abdomen)"),
	Pelvis UMETA(DisplayName = "Pelvic Girdle"),
	UpperArms UMETA(DisplayName = "Upper Arm Extremities"),
	LowerArms UMETA(DisplayName = "Forearms"),
	Hands UMETA(DisplayName = "Hands & Wrists"),
	Thighs UMETA(DisplayName = "Thighs / Femoral"),
	Calves UMETA(DisplayName = "Calves & Shins"),
	Feet UMETA(DisplayName = "Feet & Ankles")
};

/**
 * Human joint constraint configuration for physical animation and ragdoll.
 */
USTRUCT(BlueprintType)
struct FFPSJointLimitData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Joint Physics")
	FRotator MinAngularLimit = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Joint Physics")
	FRotator MaxAngularLimit = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Joint Physics")
	float AngularStiffness = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Joint Physics")
	float AngularDamping = 100.0f;
};

/**
 * Comprehensive anatomical metadata for a single bone in the human male skeleton.
 */
USTRUCT(BlueprintType)
struct FFPSHumanBoneData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bone Identity")
	FName BoneName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bone Identity")
	FName ParentBoneName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bone Identity")
	EFPSHumanBoneType BoneType = EFPSHumanBoneType::Root;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Zoning")
	EFPSBodyDamageZone DamageZone = EFPSBodyDamageZone::LowerTorso;

	// Anthropometric mass distribution for adult human male (~82kg total)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physical Properties")
	float MassKg = 5.0f;

	// Local anatomical reference height/offset in cm relative to parent
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physical Properties")
	FVector LocalOffsetCm = FVector::ZeroVector;

	// Collision capsule dimensions for precise physical raycasting and ragdoll
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physical Properties")
	float CollisionRadiusCm = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physical Properties")
	float CollisionHalfHeightCm = 15.0f;

	// Damage multiplier relative to base bullet kinetic energy
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Zoning")
	float DamageMultiplier = 1.0f;

	// Whether this bone is sheltered by standard Level IV tactical body armor plate
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Zoning")
	bool bIsPlateCarrierProtected = false;

	// Anatomical angular movement constraints
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Joint Physics")
	FFPSJointLimitData JointLimits;
};

/**
 * Skeletal registry representing a general adult human male.
 * Height: 180cm | Mass: 82.0kg | Head Height: ~170-180cm | Eye Level: ~168cm.
 */
class NOBLEFERMI_API FFPSHumanSkeleton
{
public:
	static const TArray<FFPSHumanBoneData>& GetMaleBoneRegistry();
	static const FFPSHumanBoneData* FindBoneDataByName(FName InBoneName);
	static float GetDamageMultiplierForBone(FName InBoneName);
	static EFPSBodyDamageZone GetDamageZoneForBone(FName InBoneName);
	static bool IsBoneArmorProtected(FName InBoneName);
	static float GetTotalHumanMaleMassKg();
	static FName GetRootBoneName() { return FName(TEXT("root")); }
	static FName GetPelvisBoneName() { return FName(TEXT("pelvis")); }
	static FName GetHeadBoneName() { return FName(TEXT("head")); }

private:
	static void InitializeRegistry();
	static TArray<FFPSHumanBoneData> Registry;
	static TMap<FName, int32> BoneNameToIndexMap;
	static bool bIsInitialized;
};
