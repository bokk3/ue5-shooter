#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FPSHealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FOnDamageTakenSignature, float, Damage, AActor*, Attacker, const FHitResult&, HitResult, bool, bIsHeadshot, bool, bShieldBroken);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDeathSignature, AActor*, KillerActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthShieldChangedSignature, float, CurrentHealth, float, CurrentShield);

/**
 * AAA FPS Health and Regenerating Shield Component
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class NOBLEFERMI_API UFPSHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFPSHealthComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(BlueprintAssignable, Category = "FPS Combat|Events")
	FOnDamageTakenSignature OnDamageTaken;

	UPROPERTY(BlueprintAssignable, Category = "FPS Combat|Events")
	FOnDeathSignature OnDeath;

	UPROPERTY(BlueprintAssignable, Category = "FPS Combat|Events")
	FOnHealthShieldChangedSignature OnHealthShieldChanged;

	UFUNCTION(BlueprintCallable, Category = "FPS Combat")
	void ApplyDamage(float BaseDamage, AActor* InstigatorActor, const FHitResult& HitInfo);

	UFUNCTION(BlueprintCallable, Category = "FPS Combat")
	void Heal(float HealAmount);

	UFUNCTION(BlueprintCallable, Category = "FPS Combat")
	void RechargeShield(float ShieldAmount);

	UFUNCTION(BlueprintPure, Category = "FPS Combat")
	float GetHealthPercent() const { return MaxHealth > 0.0f ? CurrentHealth / MaxHealth : 0.0f; }

	UFUNCTION(BlueprintPure, Category = "FPS Combat")
	float GetShieldPercent() const { return MaxShield > 0.0f ? CurrentShield / MaxShield : 0.0f; }

	UFUNCTION(BlueprintPure, Category = "FPS Combat")
	bool IsDead() const { return bIsDead; }

	// Configuration
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Combat|Health")
	float MaxHealth = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Combat|Shield")
	float MaxShield = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Combat|Shield")
	float ShieldRegenRate = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Combat|Shield")
	float ShieldRegenDelay = 4.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Combat|Multipliers")
	float HeadshotMultiplier = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Combat|Multipliers")
	float LimbMultiplier = 0.75f;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleInstanceOnly, Category = "FPS Combat|State")
	float CurrentHealth = 100.0f;

	UPROPERTY(VisibleInstanceOnly, Category = "FPS Combat|State")
	float CurrentShield = 100.0f;

	UPROPERTY(VisibleInstanceOnly, Category = "FPS Combat|State")
	bool bIsDead = false;

	float TimeSinceLastDamage = 0.0f;
};
