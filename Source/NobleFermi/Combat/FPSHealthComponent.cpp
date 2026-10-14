#include "Combat/FPSHealthComponent.h"

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

	// Bone multiplier calculation
	float FinalDamage = BaseDamage;
	bool bIsHeadshot = false;

	if (HitInfo.BoneName == "head" || HitInfo.BoneName == "neck_01")
	{
		FinalDamage *= HeadshotMultiplier;
		bIsHeadshot = true;
	}
	else if (HitInfo.BoneName.ToString().Contains("arm") || HitInfo.BoneName.ToString().Contains("leg") || HitInfo.BoneName.ToString().Contains("hand"))
	{
		FinalDamage *= LimbMultiplier;
	}

	bool bShieldBroken = false;

	// Deduct from shield first
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

	// Excess damage penetrates to health
	if (FinalDamage > 0.0f)
	{
		CurrentHealth = FMath::Max(0.0f, CurrentHealth - FinalDamage);
	}

	OnHealthShieldChanged.Broadcast(CurrentHealth, CurrentShield);
	OnDamageTaken.Broadcast(BaseDamage, InstigatorActor, HitInfo, bIsHeadshot, bShieldBroken);

	if (CurrentHealth <= 0.0f && !bIsDead)
	{
		bIsDead = true;
		OnDeath.Broadcast(InstigatorActor);
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
