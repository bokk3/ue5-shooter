// FPSAnimInstance.h
// Native C++ AnimInstance and AnimInstanceProxy for AFPSCharacter & AFPSCharacterPreviewActor.
// Evaluates procedural Two-Bone Leg IK (tall 180cm military posture + 0.18 Hz weight-transfer oscillation),
// thoracic breathing (0.32 Hz), situational head/neck micro-look, Low-Ready Patrol Cradle vs High-Ready/ADS arm IK,
// and realistic belt-grab reload (STANAG mag from BeltMagComp_1) & grenade throw (M67/M84 from belt) trajectories.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "BoneIndices.h"
#include "FPSBeltGearComponent.h"
#include "FPSAnimInstance.generated.h"

class AFPSCharacter;
class UFPSAnimInstance;
class UAnimSequenceBase;

/**
 * Worker-thread-safe animation proxy that computes procedural bone transforms
 * inside Evaluate(FPoseContext& Output) using FComponentSpacePoseContext.
 */
USTRUCT()
struct NOBLEFERMI_API FFPSAnimInstanceProxy : public FAnimInstanceProxy
{
	GENERATED_BODY()

public:
	FFPSAnimInstanceProxy()
		: FAnimInstanceProxy()
	{}

	FFPSAnimInstanceProxy(UAnimInstance* InAnimInstance)
		: FAnimInstanceProxy(InAnimInstance)
	{}

	virtual void Initialize(UAnimInstance* InAnimInstance) override;
	virtual void CacheBones() override;
	virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) override;
	virtual bool Evaluate(FPoseContext& Output) override;

private:
	// Cached gameplay state copied from UFPSAnimInstance in PreUpdate
	float Speed = 0.0f;
	float Direction = 0.0f;
	float ForwardSpeed = 0.0f;
	float RightSpeed = 0.0f;
	float AimPitch = 0.0f;
	float AimYaw = 0.0f;
	bool bIsInAir = false;
	bool bIsCrouching = false;
	bool bIsSprinting = false;
	bool bIsADS = false;
	bool bIsSliding = false;
	bool bIsFiring = false;
	float RecoilImpulseAlpha = 0.0f;
	float LocomotionCyclePhase = 0.0f;
	float IdleBreathPhase = 0.0f;
	float WeightTransferPhase = 0.0f;
	float HeadLookTime = 0.0f;
	float SmoothCrouchAlpha = 0.0f;
	float SmoothSprintAlpha = 0.0f;
	float SmoothADSAlpha = 0.0f;
	float SmoothSlideAlpha = 0.0f;
	float HighReadyAlpha = 0.0f;
	bool bIsMenuPreview = false;
	bool bUnarmedRelaxedIdle = false;

	// Reload & Belt-Grab state
	bool bIsReloading = false;
	float ReloadProgressAlpha = 0.0f;
	FVector BeltMag1TargetCS = FVector(-14.5f, 12.0f, 94.0f);
	FVector BeltFragTargetCS = FVector(14.0f, 12.5f, 95.0f);
	FVector BeltFlashTargetCS = FVector(19.5f, 4.5f, 94.5f);

	// Grenade throw belt-grab state
	bool bIsThrowingGrenade = false;
	float ThrowProgressAlpha = 0.0f;
	EFPSBeltItemType ActiveThrowItemType = EFPSBeltItemType::FragGrenade;

	// Optional active mocap sequence when moving/jumping/crouching in-game
	TObjectPtr<UAnimSequenceBase> ActiveSequence = nullptr;
	float SequencePlayTime = 0.0f;

	// Cached compact pose bone indices
	FCompactPoseBoneIndex PelvisIdx{INDEX_NONE};
	FCompactPoseBoneIndex Spine01Idx{INDEX_NONE};
	FCompactPoseBoneIndex Spine02Idx{INDEX_NONE};
	FCompactPoseBoneIndex Spine03Idx{INDEX_NONE};
	FCompactPoseBoneIndex Neck01Idx{INDEX_NONE};
	FCompactPoseBoneIndex HeadIdx{INDEX_NONE};
	FCompactPoseBoneIndex ClavicleLIdx{INDEX_NONE};
	FCompactPoseBoneIndex UpperArmLIdx{INDEX_NONE};
	FCompactPoseBoneIndex LowerArmLIdx{INDEX_NONE};
	FCompactPoseBoneIndex HandLIdx{INDEX_NONE};
	FCompactPoseBoneIndex ClavicleRIdx{INDEX_NONE};
	FCompactPoseBoneIndex UpperArmRIdx{INDEX_NONE};
	FCompactPoseBoneIndex LowerArmRIdx{INDEX_NONE};
	FCompactPoseBoneIndex HandRIdx{INDEX_NONE};
	FCompactPoseBoneIndex WeaponRIdx{INDEX_NONE};
	FCompactPoseBoneIndex ThighLIdx{INDEX_NONE};
	FCompactPoseBoneIndex CalfLIdx{INDEX_NONE};
	FCompactPoseBoneIndex FootLIdx{INDEX_NONE};
	FCompactPoseBoneIndex BallLIdx{INDEX_NONE};
	FCompactPoseBoneIndex ThighRIdx{INDEX_NONE};
	FCompactPoseBoneIndex CalfRIdx{INDEX_NONE};
	FCompactPoseBoneIndex FootRIdx{INDEX_NONE};
	FCompactPoseBoneIndex BallRIdx{INDEX_NONE};

	bool bBonesResolved = false;

	// Anatomical basis vectors derived in component space from rest pose
	FVector CompUpDir = FVector(0.0f, 0.0f, 1.0f);
	FVector CompRightDir = FVector(-1.0f, 0.0f, 0.0f);
	FVector CompForwardDir = FVector(0.0f, 1.0f, 0.0f);

	// Helper methods executed during Evaluate()
	void ResolveSkeletonBones(const FBoneContainer& BoneContainer);
	void ApplyRestingIdleAndLocomotion(FComponentSpacePoseContext& CSPose, const TArray<FTransform>& RestCompBases);
	void ApplySpineBreathingAndHeadLook(FComponentSpacePoseContext& CSPose, const TArray<FTransform>& RestCompBases);
	void SolveRightArmRifleIK(FComponentSpacePoseContext& CSPose, const TArray<FTransform>& RestCompBases);
	void SolveLeftHandTwoBoneIK(FComponentSpacePoseContext& CSPose, const TArray<FTransform>& RestCompBases);
	void SolveLeftHandReloadIK(FComponentSpacePoseContext& CSPose, const TArray<FTransform>& RestCompBases);
	void SolveGrenadeThrowBeltIK(FComponentSpacePoseContext& CSPose, const TArray<FTransform>& RestCompBases);
	void PropagateHandChildBones(
		FComponentSpacePoseContext& CSPose,
		const TArray<FTransform>& RestCompBases,
		FCompactPoseBoneIndex HandBoneIdx,
		const FTransform& NewHandCS);
};

/**
 * Native C++ Animation Instance for AFPSCharacter & AFPSCharacterPreviewActor.
 * Works autonomously without requiring a UAnimBlueprint asset.
 */
UCLASS(Transient, Blueprintable)
class NOBLEFERMI_API UFPSAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	UFPSAnimInstance();

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	/** Notifies the anim instance that a shot was fired so upper body recoil impulse plays. */
	UFUNCTION(BlueprintCallable, Category = "Animation|Combat")
	void NotifyWeaponFired(float Strength = 1.0f);

	/** Alerts the character into High-Ready posture for the specified duration (e.g. on mouse look, ADS, or fire). */
	UFUNCTION(BlueprintCallable, Category = "Animation|Combat")
	void TriggerAlertHighReady(float HoldDurationSeconds = 3.0f);

	/** Starts the procedural belt-grab grenade throw animation (0-25% reach belt, 25-45% pull pin, 45-60% hurl, 60-100% return). */
	UFUNCTION(BlueprintCallable, Category = "Animation|Combat")
	void NotifyGrenadeThrowStarted(EFPSBeltItemType ThrowItemType, float DurationSeconds = 0.55f);

	/** Sets an optional mocap sequence to evaluate when moving/jumping/crouching in-game (nullptr = procedural resting idle). */
	void SetActiveMocapSequence(UAnimSequenceBase* InSequence, float InPlayRate = 1.0f);

	/** Returns true while the grenade throw animation is playing. */
	bool IsThrowingGrenade() const { return bIsThrowingGrenade; }

	// ---- Locomotion Properties ----

	/** Horizontal ground speed (cm/s). */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Animation|Locomotion")
	float Speed = 0.0f;

	/** Movement direction relative to actor rotation [-180, 180]. */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Animation|Locomotion")
	float Direction = 0.0f;

	/** Signed forward speed along character forward vector (cm/s). */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Animation|Locomotion")
	float ForwardSpeed = 0.0f;

	/** Signed strafe speed along character right vector (cm/s). */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Animation|Locomotion")
	float RightSpeed = 0.0f;

	/** Aim pitch angle [-90, 90] for Spine/Head AimOffset. */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Animation|Aim")
	float AimPitch = 0.0f;

	/** Relative Aim Yaw angle [-180, 180]. */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Animation|Aim")
	float AimYaw = 0.0f;

	/** True when character is falling or jumping. */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Animation|Locomotion")
	bool bIsInAir = false;

	/** True when character is crouched. */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Animation|Locomotion")
	bool bIsCrouching = false;

	/** True when character is sprinting. */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Animation|Locomotion")
	bool bIsSprinting = false;

	/** True when player is Aiming Down Sights. */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Animation|Combat")
	bool bIsADS = false;

	/** True when character is power-sliding. */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Animation|Locomotion")
	bool bIsSliding = false;

	/** True when weapon is actively firing. */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Animation|Combat")
	bool bIsFiring = false;

	/** True when current weapon is reloading. */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Animation|Combat")
	bool bIsReloading = false;

	/** Normalized reload progress [0..1] for procedural left-hand belt-mag grab IK. */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Animation|Combat")
	float ReloadProgressAlpha = 0.0f;

	/** True when throwing a frag or flashbang grenade from the belt. */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Animation|Combat")
	bool bIsThrowingGrenade = false;

	/** Normalized throw progress [0..1]. */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Animation|Combat")
	float ThrowProgressAlpha = 0.0f;

	/** Which belt grenade is currently being thrown. */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Animation|Combat")
	EFPSBeltItemType ActiveThrowItemType = EFPSBeltItemType::FragGrenade;

	/** True when driving the Hangar menu preview character (`AFPSCharacterPreviewActor`). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation|Preview")
	bool bIsMenuPreview = false;

	/** Stance index in Hangar preview (0 = Unarmed Relaxed Tactical Idle with rifle on table, 1 = High-Ready ADS, 2 = Walk Cycle). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation|Preview")
	int32 MenuPoseIndex = 0;

	/** True when arms should rest in a relaxed unarmed tactical readiness stance (elbows bent ~22 deg at sides). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation|Preview")
	bool bUnarmedRelaxedIdle = false;

	/** Blend weight [0 = Low-Ready Patrol Cradle, 1 = High-Ready / ADS]. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation|Combat")
	float HighReadyAlpha = 0.0f;

	/** Target High-Ready state requested by Hangar preview button or combat alert timer. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation|Combat")
	bool bForceHighReady = false;

	// ---- Smoothed Blend Alphas ----
	float SmoothCrouchAlpha = 0.0f;
	float SmoothSprintAlpha = 0.0f;
	float SmoothADSAlpha = 0.0f;
	float SmoothSlideAlpha = 0.0f;
	float RecoilImpulseAlpha = 0.0f;
	float LocomotionCyclePhase = 0.0f;
	float IdleBreathPhase = 0.0f;
	float WeightTransferPhase = 0.0f;
	float HeadLookTime = 0.0f;

	// Component-space belt targets updated on Game Thread from UFPSBeltGearComponent
	FVector BeltMag1TargetCS = FVector(-14.5f, 12.0f, 94.0f);
	FVector BeltFragTargetCS = FVector(14.0f, 12.5f, 95.0f);
	FVector BeltFlashTargetCS = FVector(19.5f, 4.5f, 94.5f);

	// Optional active mocap sequence for moving/jumping/crouching
	UPROPERTY(Transient)
	TObjectPtr<UAnimSequenceBase> ActiveMocapSequence = nullptr;

	float MocapPlayRate = 1.0f;
	float MocapAccumulatedTime = 0.0f;

protected:
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override
	{
		return &Proxy;
	}

	virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy) override
	{
	}

private:
	friend struct FFPSAnimInstanceProxy;

	UPROPERTY(Transient)
	FFPSAnimInstanceProxy Proxy;

	UPROPERTY(Transient)
	TObjectPtr<AFPSCharacter> OwningFPSCharacter = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UFPSBeltGearComponent> OwningBeltGear = nullptr;

	float AlertHighReadyTimer = 0.0f;
	float ThrowElapsedSeconds = 0.0f;
	float ThrowTotalDuration = 0.55f;
};
