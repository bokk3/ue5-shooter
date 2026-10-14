#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FPSProjectileBase.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UParticleSystem;
class USoundBase;

UENUM(BlueprintType)
enum class EFPSProjectileType : uint8
{
	FragGrenade     UMETA(DisplayName = "Cookable Frag Grenade"),
	Semtex          UMETA(DisplayName = "Sticky Semtex"),
	Flashbang       UMETA(DisplayName = "Concussive Flashbang"),
	SmokeGrenade    UMETA(DisplayName = "Volumetric Smoke Grenade"),
	ThrowingKnife   UMETA(DisplayName = "Throwing Knife")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProjectileDetonatedSignature, const FVector&, ExplosionLocation);

/**
 * AAA FPS Tactical Projectile Base Actor
 * Handles cookable grenade fuses, sticky Semtex physics, flashbang LOS blinding, and radial damage.
 */
UCLASS()
class NOBLEFERMI_API AFPSProjectileBase : public AActor
{
	GENERATED_BODY()

public:
	AFPSProjectileBase();

	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile|Components")
	TObjectPtr<USphereComponent> CollisionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile|Components")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile|Components")
	TObjectPtr<UStaticMeshComponent> ProjectileMesh;

	UPROPERTY(BlueprintAssignable, Category = "Projectile|Events")
	FOnProjectileDetonatedSignature OnDetonated;

	// Projectile Parameters
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Type")
	EFPSProjectileType ProjectileType = EFPSProjectileType::FragGrenade;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Fuse")
	float FuseTimeRemaining = 4.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Damage")
	float BaseExplosionDamage = 135.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Damage")
	float MinimumDamage = 25.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Damage")
	float InnerDamageRadius = 150.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Damage")
	float OuterDamageRadius = 600.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Effects")
	TObjectPtr<USoundBase> DetonationSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Effects")
	TObjectPtr<USoundBase> SemtexBeepSound;

	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void InitializeFuse(float CustomFuseTime, AController* InstigatingController);

	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void Detonate();

protected:
	UFUNCTION()
	void OnProjectileHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	void PlaySemtexBeep();
	void ProcessFlashbangConcussion();

private:
	FTimerHandle FuseTimerHandle;
	FTimerHandle SemtexBeepTimerHandle;
	bool bHasStuck = false;
	bool bHasDetonated = false;
};
