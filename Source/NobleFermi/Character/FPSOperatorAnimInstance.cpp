// FPSOperatorAnimInstance.cpp

#include "Character/FPSOperatorAnimInstance.h"

#include "Animation/AnimMontage.h"
#include "Animation/AnimSequenceBase.h"
#include "Character/FPSCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Weapons/FPSWeaponBase.h"
#include "Weapons/FPSWeaponDataAsset.h"

namespace
{
	/** Retargeted Lyra content (Scripts/ThirdPartyContent/README.md). */
	TSoftObjectPtr<UAnimSequenceBase> LyraAnim(const TCHAR* SubPath)
	{
		const FString Name = FPaths::GetBaseFilename(SubPath);
		return TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(
			FString::Printf(TEXT("/Game/Characters/UEFN_Mannequin/Animations/Lyra/%s.%s"), SubPath, *Name)));
	}

	/** Lyra has fire/reload/melee for every family, equip for rifle and pistol, grenade toss for rifle only. */
	FFPSOperatorWeaponAnims MakeWeaponAnims(const FString& Family, bool bHasEquip)
	{
		FFPSOperatorWeaponAnims Anims;
		Anims.Fire = LyraAnim(*(TEXT("Actions/MM_") + Family + TEXT("_Fire")));
		Anims.Reload = LyraAnim(*(TEXT("Actions/MM_") + Family + TEXT("_Reload")));
		Anims.Melee = LyraAnim(*(TEXT("Actions/MM_") + Family + TEXT("_Melee")));
		Anims.Equip = LyraAnim(*(TEXT("Actions/MM_") + (bHasEquip ? Family : FString(TEXT("Rifle"))) + TEXT("_Equip")));
		Anims.GrenadeToss = LyraAnim(TEXT("Actions/MM_Rifle_GrenadeToss"));
		return Anims;
	}

	void LoadInto(const TSoftObjectPtr<UAnimSequenceBase>& Soft, TArray<TObjectPtr<UAnimSequenceBase>>& Out)
	{
		if (!Soft.IsNull())
		{
			if (UAnimSequenceBase* Anim = Soft.LoadSynchronous())
			{
				Out.AddUnique(Anim);
			}
		}
	}
}

UFPSOperatorAnimInstance::UFPSOperatorAnimInstance()
{
	RifleAnims = MakeWeaponAnims(TEXT("Rifle"), true);
	PistolAnims = MakeWeaponAnims(TEXT("Pistol"), true);
	ShotgunAnims = MakeWeaponAnims(TEXT("Shotgun"), false);

	HitReactsFront = { LyraAnim(TEXT("Actions/MM_HitReact_Front_Lgt_01")), LyraAnim(TEXT("Actions/MM_HitReact_Front_Lgt_02")),
		LyraAnim(TEXT("Actions/MM_HitReact_Front_Med_01")), LyraAnim(TEXT("Actions/MM_HitReact_Front_Med_02")) };
	HitReactsBack = { LyraAnim(TEXT("Actions/MM_HitReact_Back_Lgt_01")), LyraAnim(TEXT("Actions/MM_HitReact_Back_Med_01")) };
	HitReactsLeft = { LyraAnim(TEXT("Actions/MM_HitReact_Left_Lgt_01")), LyraAnim(TEXT("Actions/MM_HitReact_Left_Med_01")) };
	HitReactsRight = { LyraAnim(TEXT("Actions/MM_HitReact_Right_Lgt_01")), LyraAnim(TEXT("Actions/MM_HitReact_Right_Med_01")) };
}

UFPSOperatorAnimInstance* UFPSOperatorAnimInstance::Get(const ACharacter* Character)
{
	const USkeletalMeshComponent* Mesh = Character ? Character->GetMesh() : nullptr;
	return Mesh ? Cast<UFPSOperatorAnimInstance>(Mesh->GetAnimInstance()) : nullptr;
}

void UFPSOperatorAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	OwnerCharacter = Cast<AFPSCharacter>(TryGetPawnOwner());

	if (!OwnerCharacter.IsValid())
	{
		return; // editor preview: nothing to aim or fire
	}
	LoadedAnims.Reset();
	for (const FFPSOperatorWeaponAnims* Family : { &RifleAnims, &PistolAnims, &ShotgunAnims })
	{
		for (const TSoftObjectPtr<UAnimSequenceBase>* Soft : { &Family->Fire, &Family->Reload, &Family->Equip, &Family->Melee, &Family->GrenadeToss })
		{
			LoadInto(*Soft, LoadedAnims);
		}
	}
	for (const TArray<TSoftObjectPtr<UAnimSequenceBase>>* Set : { &HitReactsFront, &HitReactsBack, &HitReactsLeft, &HitReactsRight })
	{
		for (const TSoftObjectPtr<UAnimSequenceBase>& Soft : *Set)
		{
			LoadInto(Soft, LoadedAnims);
		}
	}
}

void UFPSOperatorAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	AFPSCharacter* Character = OwnerCharacter.Get();
	if (!Character)
	{
		return;
	}

	// Remote players' pitch arrives through RemoteViewPitch inside GetBaseAimRotation.
	const FRotator Delta = (Character->GetBaseAimRotation() - Character->GetActorRotation()).GetNormalized();
	AimPitch = FMath::Clamp(Delta.Pitch, -MaxAimAngle, MaxAimAngle);
	AimYaw = FMath::Clamp(Delta.Yaw, -MaxAimAngle, MaxAimAngle);

	const AFPSWeaponBase* Weapon = Character->GetCurrentWeapon();
	const UFPSWeaponDataAsset* Data = Weapon ? Weapon->WeaponDataAsset.Get() : nullptr;
	bUsePistolPose = Data && Data->Archetype == EFPSWeaponArchetype::Pistol;

	// Weapon switches go through the inventory component; play the equip when the weapon changes
	// (not for the first weapon after spawning).
	if (Weapon != LastWeapon.Get())
	{
		const bool bHadWeapon = LastWeapon.IsValid();
		LastWeapon = Weapon;
		if (bHadWeapon && Weapon)
		{
			PlayWeaponAction(EFPSOperatorAction::Equip);
		}
	}

	const bool bArmed = Weapon != nullptr && Data && Data->Archetype != EFPSWeaponArchetype::Melee;
	const float Target = (bArmed && !Character->IsDead() && !IsForeignMontagePlaying()) ? 1.0f : 0.0f;
	WeaponLayerAlpha = FMath::FInterpConstantTo(WeaponLayerAlpha, Target, DeltaSeconds, WeaponLayerBlendSpeed);

	// Left hand on the foregrip for two-handed weapons, except while an action animates it.
	const bool bHandFree = GetWorld() && GetWorld()->GetTimeSeconds() >= LeftHandBusyUntil;
	const float IKTarget = (bArmed && !bUsePistolPose && bHandFree && WeaponLayerAlpha > 0.99f) ? 1.0f : 0.0f;
	LeftHandIKAlpha = FMath::FInterpConstantTo(LeftHandIKAlpha, IKTarget, DeltaSeconds, LeftHandIKBlendSpeed);
	if (Weapon)
	{
		LeftHandIKLocation = Weapon->GetForegripWorldLocation() + Weapon->GetActorQuat().RotateVector(LeftWristFromForegrip);
	}
}

bool UFPSOperatorAnimInstance::IsForeignMontagePlaying() const
{
	// The sample plays traversal (vault, mantle, hurdle) as full-body montages; hands must leave the weapon pose.
	for (const FAnimMontageInstance* Instance : MontageInstances)
	{
		const UAnimMontage* Montage = Instance ? Instance->Montage.Get() : nullptr;
		if (Montage && Instance->IsActive() && !Montage->IsValidSlot(UpperBodySlot) && !Montage->IsValidSlot(FullBodySlot))
		{
			return true;
		}
	}
	return false;
}

const FFPSOperatorWeaponAnims& UFPSOperatorAnimInstance::AnimsForEquippedWeapon() const
{
	const AFPSCharacter* Character = OwnerCharacter.Get();
	const AFPSWeaponBase* Weapon = Character ? Character->GetCurrentWeapon() : nullptr;
	const UFPSWeaponDataAsset* Data = Weapon ? Weapon->WeaponDataAsset.Get() : nullptr;
	switch (Data ? Data->Archetype : EFPSWeaponArchetype::AssaultRifle)
	{
	case EFPSWeaponArchetype::Pistol:  return PistolAnims;
	case EFPSWeaponArchetype::Shotgun: return ShotgunAnims;
	default:                           return RifleAnims;
	}
}

void UFPSOperatorAnimInstance::PlayWeaponAction(EFPSOperatorAction Action, float DurationSeconds)
{
	const FFPSOperatorWeaponAnims& Anims = AnimsForEquippedWeapon();
	const TSoftObjectPtr<UAnimSequenceBase>* Soft = nullptr;
	switch (Action)
	{
	case EFPSOperatorAction::Fire:        Soft = &Anims.Fire; break;
	case EFPSOperatorAction::Reload:      Soft = &Anims.Reload; break;
	case EFPSOperatorAction::Equip:       Soft = &Anims.Equip; break;
	case EFPSOperatorAction::Melee:       Soft = &Anims.Melee; break;
	case EFPSOperatorAction::GrenadeToss: Soft = &Anims.GrenadeToss; break;
	}
	UAnimSequenceBase* Anim = Soft ? Soft->Get() : nullptr;
	if (!Anim)
	{
		return; // content not imported on this machine, or still loading
	}
	const float Length = Anim->GetPlayLength();
	const float Rate = (DurationSeconds > 0.05f && Length > 0.05f) ? Length / DurationSeconds : 1.0f;
	const float BlendIn = Action == EFPSOperatorAction::Fire ? 0.03f : 0.15f;
	PlaySlotAnimationAsDynamicMontage(Anim, UpperBodySlot, BlendIn, 0.2f, Rate);
	if (Action != EFPSOperatorAction::Fire)
	{
		UE_LOG(LogTemp, Log, TEXT("[OperatorAnim] %s plays %s in %s at rate %.2f"),
			*GetNameSafe(OwnerCharacter.Get()), *Anim->GetName(), *UpperBodySlot.ToString(), Rate);
	}
	if (Action != EFPSOperatorAction::Fire && GetWorld())
	{
		LeftHandBusyUntil = GetWorld()->GetTimeSeconds() + Length / FMath::Max(Rate, 0.01f);
	}
}

void UFPSOperatorAnimInstance::PlayHitReact(const FVector& WorldHitDirection, float Damage)
{
	const AFPSCharacter* Character = OwnerCharacter.Get();
	if (!Character || Character->IsDead() || WorldHitDirection.IsNearlyZero())
	{
		return;
	}
	// The hit travels along WorldHitDirection, so it came from the opposite side.
	const FVector From = -WorldHitDirection.GetSafeNormal2D();
	const float Forward = FVector::DotProduct(From, Character->GetActorForwardVector());
	const float Right = FVector::DotProduct(From, Character->GetActorRightVector());
	const TArray<TSoftObjectPtr<UAnimSequenceBase>>& Set =
		FMath::Abs(Forward) >= FMath::Abs(Right) ? (Forward >= 0.0f ? HitReactsFront : HitReactsBack)
		                                         : (Right >= 0.0f ? HitReactsRight : HitReactsLeft);
	if (Set.IsEmpty())
	{
		return;
	}
	// Light variants first in each list; heavier hits pick from the back half.
	const int32 Half = FMath::Max(1, Set.Num() / 2);
	const int32 Index = Damage >= 35.0f ? FMath::RandRange(Set.Num() - Half, Set.Num() - 1) : FMath::RandRange(0, Half - 1);
	if (UAnimSequenceBase* Anim = Set[Index].Get())
	{
		PlaySlotAnimationAsDynamicMontage(Anim, FullBodySlot, 0.05f, 0.25f, 1.0f);
	}
}
