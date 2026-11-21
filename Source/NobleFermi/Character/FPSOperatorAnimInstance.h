// FPSOperatorAnimInstance.h
// Parent class of the operator's animation blueprint (ABP_FPSOperator, generated from the Game Animation
// Sample's motion-matching ABP by Scripts/ThirdPartyContent/create_operator_anim_blueprint.py).
// The sample graph drives locomotion; this class feeds the weapon layer that the generator inserts on top
// (hold pose + aim offset above spine_01) and plays Lyra's weapon and hit-react animations in its slots.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "FPSOperatorAnimInstance.generated.h"

class UAnimSequenceBase;
class AFPSCharacter;

UENUM(BlueprintType)
enum class EFPSOperatorAction : uint8
{
	Fire,
	Reload,
	Equip,
	Melee,
	GrenadeToss
};

/** Action animations for one weapon family (rifle, pistol, shotgun). */
USTRUCT(BlueprintType)
struct FFPSOperatorWeaponAnims
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Operator Anims")
	TSoftObjectPtr<UAnimSequenceBase> Fire;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Operator Anims")
	TSoftObjectPtr<UAnimSequenceBase> Reload;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Operator Anims")
	TSoftObjectPtr<UAnimSequenceBase> Equip;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Operator Anims")
	TSoftObjectPtr<UAnimSequenceBase> Melee;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Operator Anims")
	TSoftObjectPtr<UAnimSequenceBase> GrenadeToss;
};

UCLASS(Blueprintable)
class NOBLEFERMI_API UFPSOperatorAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	UFPSOperatorAnimInstance();

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	/** The operator anim instance on a character's mesh, or null when it runs another anim class. */
	static UFPSOperatorAnimInstance* Get(const ACharacter* Character);

	/**
	 * Plays the action animation for the equipped weapon family in the upper-body slot.
	 * DurationSeconds > 0 stretches the animation to fit (e.g. the weapon's reload time).
	 */
	UFUNCTION(BlueprintCallable, Category = "Operator Anims")
	void PlayWeaponAction(EFPSOperatorAction Action, float DurationSeconds = 0.0f);

	/** Plays a directional hit react (additive) in the full-body slot. WorldHitDirection points from attacker to victim. */
	UFUNCTION(BlueprintCallable, Category = "Operator Anims")
	void PlayHitReact(const FVector& WorldHitDirection, float Damage);

	// --- Read by the generated weapon layer in ABP_FPSOperator (names must match FFPSWeaponLayerSettings) ---

	/** Aim relative to the body, degrees: + is up. */
	UPROPERTY(BlueprintReadOnly, Category = "Operator Anims|Weapon Layer")
	float AimPitch = 0.0f;

	/** Aim relative to the body, degrees: + is right. */
	UPROPERTY(BlueprintReadOnly, Category = "Operator Anims|Weapon Layer")
	float AimYaw = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Operator Anims|Weapon Layer")
	bool bUsePistolPose = false;

	/** Weight of the weapon layer: 0 when unarmed, dead, or while the sample plays a traversal montage. */
	UPROPERTY(BlueprintReadOnly, Category = "Operator Anims|Weapon Layer")
	float WeaponLayerAlpha = 0.0f;

	/** World-space target for the left wrist: under the weapon's foregrip. */
	UPROPERTY(BlueprintReadOnly, Category = "Operator Anims|Weapon Layer")
	FVector LeftHandIKLocation = FVector::ZeroVector;

	/** 0 for pistols and while a weapon action (reload, equip, melee, grenade) needs the left hand. */
	UPROPERTY(BlueprintReadOnly, Category = "Operator Anims|Weapon Layer")
	float LeftHandIKAlpha = 0.0f;

	/** Left wrist relative to the foregrip, in weapon space (X forward, Y right, Z up), cm. */
	UPROPERTY(EditDefaultsOnly, Category = "Operator Anims|Weapon Layer")
	FVector LeftWristFromForegrip = FVector(-4.0f, -3.0f, -7.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Operator Anims|Weapon Layer")
	float LeftHandIKBlendSpeed = 6.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Operator Anims|Slots")
	FName UpperBodySlot = TEXT("NF_UpperBody");

	UPROPERTY(EditDefaultsOnly, Category = "Operator Anims|Slots")
	FName FullBodySlot = TEXT("NF_FullBody");

	UPROPERTY(EditDefaultsOnly, Category = "Operator Anims")
	FFPSOperatorWeaponAnims RifleAnims;

	UPROPERTY(EditDefaultsOnly, Category = "Operator Anims")
	FFPSOperatorWeaponAnims PistolAnims;

	UPROPERTY(EditDefaultsOnly, Category = "Operator Anims")
	FFPSOperatorWeaponAnims ShotgunAnims;

	/** Hit reacts by direction the hit came from: Front, Back, Left, Right. Light and medium variants. */
	UPROPERTY(EditDefaultsOnly, Category = "Operator Anims")
	TArray<TSoftObjectPtr<UAnimSequenceBase>> HitReactsFront;

	UPROPERTY(EditDefaultsOnly, Category = "Operator Anims")
	TArray<TSoftObjectPtr<UAnimSequenceBase>> HitReactsBack;

	UPROPERTY(EditDefaultsOnly, Category = "Operator Anims")
	TArray<TSoftObjectPtr<UAnimSequenceBase>> HitReactsLeft;

	UPROPERTY(EditDefaultsOnly, Category = "Operator Anims")
	TArray<TSoftObjectPtr<UAnimSequenceBase>> HitReactsRight;

	/** Aim offsets in the Lyra set cover +-90 degrees of pitch and yaw. */
	UPROPERTY(EditDefaultsOnly, Category = "Operator Anims|Weapon Layer")
	float MaxAimAngle = 90.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Operator Anims|Weapon Layer")
	float WeaponLayerBlendSpeed = 8.0f;

private:
	const FFPSOperatorWeaponAnims& AnimsForEquippedWeapon() const;
	bool IsForeignMontagePlaying() const;

	TWeakObjectPtr<AFPSCharacter> OwnerCharacter;
	TWeakObjectPtr<const class AFPSWeaponBase> LastWeapon;

	/** World time until which a non-fire weapon action owns the left hand. */
	float LeftHandBusyUntil = 0.0f;

	/** Hard references to everything above once loaded, so playback never hitches mid-match. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UAnimSequenceBase>> LoadedAnims;
};
