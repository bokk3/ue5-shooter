#include "Combat/FPSHealthComponent.h"
#include "BodyMechanics/FPSHumanBones.h"
#include "BodyMechanics/FPSBodyMechanicsComponent.h"
#include "Net/UnrealNetwork.h"

// Networking model:
//  - Health, shield and death change only where HasHealthAuthority() is true (server or standalone).
//    Calls on clients are ignored, so client-side shot prediction can't deal damage.
//  - Vitals and death replicate to everyone; OnRep_* re-broadcast the same delegates clients already bind to.
//  - Per-hit feedback (flinch, HUD damage indicator) reaches clients through an unreliable multicast.

UFPSHealthComponent::UFPSHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void UFPSHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UFPSHealthComponent, CurrentHealth);
	DOREPLIFETIME(UFPSHealthComponent, CurrentShield);
	DOREPLIFETIME(UFPSHealthComponent, bIsDead);
	DOREPLIFETIME(UFPSHealthComponent, LastKiller);
	DOREPLIFETIME(UFPSHealthComponent, DeathImpulse);
	DOREPLIFETIME(UFPSHealthComponent, DeathImpactPoint);
	DOREPLIFETIME(UFPSHealthComponent, DeathBoneName);
}

void UFPSHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	if (HasHealthAuthority())
	{
		CurrentHealth = MaxHealth;
		CurrentShield = MaxShield;
		bIsDead = false;
	}
	TimeSinceLastDamage = 0.0f;
}

bool UFPSHealthComponent::HasHealthAuthority() const
{
	const AActor* Owner = GetOwner();
	return Owner && Owner->HasAuthority();
}

void UFPSHealthComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Shield regen is server-driven; clients receive the result through replication
	if (bIsDead || !HasHealthAuthority()) return;

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
	if (!HasHealthAuthority()) return;
	if (bIsDead || BaseDamage <= 0.0f) return;

	TimeSinceLastDamage = 0.0f;

	// Anatomical human bone or capsule hit multiplier calculation
	float BoneMultiplier = 1.0f;
	bool bIsHeadshot = false;
	bool bIsArmorPlateHit = false;

	if (HitInfo.BoneName != NAME_None)
	{
		BoneMultiplier = FFPSHumanSkeleton::GetDamageMultiplierForBone(HitInfo.BoneName);
		bIsHeadshot = (FFPSHumanSkeleton::GetDamageZoneForBone(HitInfo.BoneName) == EFPSBodyDamageZone::Head);
		bIsArmorPlateHit = FFPSHumanSkeleton::IsBoneArmorProtected(HitInfo.BoneName);
	}
	else
	{
		// Height-based evaluation when hit on the capsule or without skeletal bone data
		const float RelativeHitZ = HitInfo.ImpactPoint.Z - (GetOwner() ? GetOwner()->GetActorLocation().Z : 0.0f);
		if (RelativeHitZ > 55.0f)
		{
			BoneMultiplier = 2.0f; // Headshot
			bIsHeadshot = true;
		}
		else if (RelativeHitZ > 15.0f)
		{
			BoneMultiplier = 1.0f; // Chest / Torso armor
			bIsArmorPlateHit = true;
		}
		else
		{
			BoneMultiplier = 0.85f; // Limbs
		}
	}
	float FinalDamage = BaseDamage * BoneMultiplier;
	bool bShieldBroken = false;

	// Headshots cleanly bypass body armor plates directly to biological vitals
	if (bIsHeadshot)
	{
		CurrentHealth = FMath::Max(0.0f, CurrentHealth - FinalDamage);
	}
	else
	{
		// Tactical Body Armor: Chest plate absorbs 30% of incoming bullet kinetic energy
		if (bIsArmorPlateHit && CurrentShield > 0.0f)
		{
			FinalDamage *= 0.70f;
		}

		// Deduct from shield/vest plates first
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
	}

	OnHealthShieldChanged.Broadcast(CurrentHealth, CurrentShield);
	OnDamageTaken.Broadcast(BaseDamage, InstigatorActor, HitInfo, bIsHeadshot, bShieldBroken);

	// Notify Body Mechanics for Aim Punch, Flinch, and Physical Ragdoll
	UFPSBodyMechanicsComponent* BodyMechanics = GetOwner() ? GetOwner()->FindComponentByClass<UFPSBodyMechanicsComponent>() : nullptr;
	const FVector ShotDir = (HitInfo.TraceEnd - HitInfo.TraceStart).GetSafeNormal();

	if (CurrentHealth <= 0.0f && !bIsDead)
	{
		// Fill replicated death details before flipping bIsDead so they arrive in the same update
		LastKiller = InstigatorActor;
		DeathImpulse = ShotDir * (BaseDamage + 25.0f);
		DeathImpactPoint = HitInfo.ImpactPoint;
		DeathBoneName = HitInfo.BoneName;
		bIsDead = true;

		OnDeath.Broadcast(InstigatorActor);

		if (BodyMechanics)
		{
			BodyMechanics->TriggerDeathRagdoll(DeathImpulse, DeathImpactPoint, DeathBoneName);
		}
	}
	else if (BodyMechanics)
	{
		BodyMechanics->ApplyBoneHitImpulse(HitInfo.BoneName, ShotDir, FinalDamage);
	}

	if (GetNetMode() != NM_Standalone)
	{
		MulticastDamageTaken(BaseDamage, InstigatorActor, HitInfo, bIsHeadshot, bShieldBroken, FinalDamage);
	}
}

void UFPSHealthComponent::MulticastDamageTaken_Implementation(float BaseDamage, AActor* InstigatorActor, const FHitResult& HitInfo, bool bIsHeadshot, bool bShieldBroken, float AppliedDamage)
{
	// The server already broadcast locally inside ApplyDamage
	if (HasHealthAuthority()) return;

	OnDamageTaken.Broadcast(BaseDamage, InstigatorActor, HitInfo, bIsHeadshot, bShieldBroken);

	// Death ragdoll comes from OnRep_IsDead; only flinch living characters here
	if (!bIsDead)
	{
		if (UFPSBodyMechanicsComponent* BodyMechanics = GetOwner() ? GetOwner()->FindComponentByClass<UFPSBodyMechanicsComponent>() : nullptr)
		{
			const FVector ShotDir = (HitInfo.TraceEnd - HitInfo.TraceStart).GetSafeNormal();
			BodyMechanics->ApplyBoneHitImpulse(HitInfo.BoneName, ShotDir, AppliedDamage);
		}
	}
}

void UFPSHealthComponent::OnRep_Vitals()
{
	OnHealthShieldChanged.Broadcast(CurrentHealth, CurrentShield);
}

void UFPSHealthComponent::OnRep_IsDead()
{
	if (!bIsDead) return;

	OnDeath.Broadcast(LastKiller);

	if (UFPSBodyMechanicsComponent* BodyMechanics = GetOwner() ? GetOwner()->FindComponentByClass<UFPSBodyMechanicsComponent>() : nullptr)
	{
		BodyMechanics->TriggerDeathRagdoll(DeathImpulse, DeathImpactPoint, DeathBoneName);
	}
}

void UFPSHealthComponent::Heal(float HealAmount)
{
	if (!HasHealthAuthority() || bIsDead) return;

	CurrentHealth = FMath::Min(MaxHealth, CurrentHealth + HealAmount);
	OnHealthShieldChanged.Broadcast(CurrentHealth, CurrentShield);
}

void UFPSHealthComponent::RechargeShield(float ShieldAmount)
{
	if (!HasHealthAuthority() || bIsDead) return;

	CurrentShield = FMath::Min(MaxShield, CurrentShield + ShieldAmount);
	OnHealthShieldChanged.Broadcast(CurrentHealth, CurrentShield);
}
