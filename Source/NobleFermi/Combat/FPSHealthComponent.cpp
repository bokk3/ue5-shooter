#include "Combat/FPSHealthComponent.h"
#include "BodyMechanics/FPSHumanBones.h"
#include "BodyMechanics/FPSBodyMechanicsComponent.h"

UFPSHealthComponent::UFPSHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UFPSHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;
	CurrentShield = MaxShield;
	bIsDead = false;
	TimeSinceLastDamage = 0.0f;
}

void UFPSHealthComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bIsDead) return;

	TimeSinceLastDamage += DeltaTime;

	// Regenerate shield if cooldown elapsed
	if (TimeSinceLastDamage >= ShieldRegenDelay && CurrentShield < MaxShield)
	{
		CurrentShield = FMath::Min(MaxShield, CurrentShield + (ShieldRegenRate * DeltaTime));
		OnHealthShieldChanged.Broadcast(CurrentHealth, CurrentShield);
	}
}

void UFPSHealthComponent::ApplyDamage(float BaseDamage, AActor* InstigatorActor, const FHitResult& HitInfo)
{
	if (bIsDead || BaseDamage <= 0.0f) return;

	TimeSinceLastDamage = 0.0f;

	// Anatomical human bone multiplier calculation
	const float BoneMultiplier = FFPSHumanSkeleton::GetDamageMultiplierForBone(HitInfo.BoneName);
	float FinalDamage = BaseDamage * BoneMultiplier;

	const bool bIsHeadshot = (FFPSHumanSkeleton::GetDamageZoneForBone(HitInfo.BoneName) == EFPSBodyDamageZone::Head);
	const bool bIsArmorPlateHit = FFPSHumanSkeleton::IsBoneArmorProtected(HitInfo.BoneName);

	// Tactical Body Armor: Chest plate absorbs 40% more bullet damage before health penetration
	if (bIsArmorPlateHit && CurrentShield > 0.0f)
	{
		FinalDamage *= 0.70f;
	}

	bool bShieldBroken = false;

	// Deduct from shield/vest first
	if (CurrentShield > 0.0f)
	{
		if (CurrentShield >= FinalDamage)
		{
			CurrentShield -= FinalDamage;
			FinalDamage = 0.0f;
		}
		else
		{
			FinalDamage -= CurrentShield;
			CurrentShield = 0.0f;
			bShieldBroken = true;
		}
	}

	// Excess damage penetrates to biological human vitals
	if (FinalDamage > 0.0f)
	{
		CurrentHealth = FMath::Max(0.0f, CurrentHealth - FinalDamage);
	}

	OnHealthShieldChanged.Broadcast(CurrentHealth, CurrentShield);
	OnDamageTaken.Broadcast(BaseDamage, InstigatorActor, HitInfo, bIsHeadshot, bShieldBroken);

	// Notify Body Mechanics for Aim Punch, Flinch, and Physical Ragdoll
	UFPSBodyMechanicsComponent* BodyMechanics = GetOwner() ? GetOwner()->FindComponentByClass<UFPSBodyMechanicsComponent>() : nullptr;

	if (CurrentHealth <= 0.0f && !bIsDead)
	{
		bIsDead = true;
		OnDeath.Broadcast(InstigatorActor);

		if (BodyMechanics)
		{
			const FVector ShotDir = (HitInfo.TraceEnd - HitInfo.TraceStart).GetSafeNormal();
			BodyMechanics->TriggerDeathRagdoll(ShotDir * (BaseDamage + 25.0f), HitInfo.ImpactPoint, HitInfo.BoneName);
		}
	}
	else if (BodyMechanics)
	{
		const FVector ShotDir = (HitInfo.TraceEnd - HitInfo.TraceStart).GetSafeNormal();
		BodyMechanics->ApplyBoneHitImpulse(HitInfo.BoneName, ShotDir, FinalDamage);
	}
}

void UFPSHealthComponent::Heal(float HealAmount)
{
	if (bIsDead) return;

	CurrentHealth = FMath::Min(MaxHealth, CurrentHealth + HealAmount);
	OnHealthShieldChanged.Broadcast(CurrentHealth, CurrentShield);
}

void UFPSHealthComponent::RechargeShield(float ShieldAmount)
{
	if (bIsDead) return;

	CurrentShield = FMath::Min(MaxShield, CurrentShield + ShieldAmount);
	OnHealthShieldChanged.Broadcast(CurrentHealth, CurrentShield);
}
