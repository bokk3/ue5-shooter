#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FPSDilatedBulletActor.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;

/**
 * Physicalized Dodgeable Dilated Bullet Actor
 * Spawned when weapons fire inside or into a localized Fermi Time-Dilation Bubble.
 * Converts infinite-speed hitscan rays into physical 3D slugs cruising at 1,400 cm/s
 * with sweep collision, bone damage zoning, and piecewise conversion back into
 * remainder hitscan upon exiting the bubble horizon.
 */
UCLASS()
class NOBLEFERMI_API AFPSDilatedBulletActor : public AActor
{
	GENERATED_BODY()

public:
	AFPSDilatedBulletActor();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	void InitializeSlug(const FVector& InStartLoc, const FVector& InDirection, float InDamage, float InRemainingRange, AActor* InShooter);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dilated Bullet|Components")
	TObjectPtr<USphereComponent> CollisionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dilated Bullet|Components")
	TObjectPtr<UStaticMeshComponent> SlugMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dilated Bullet|Components")
	TObjectPtr<UStaticMeshComponent> WakeTrailMesh;

	/** Dilated speed inside the Fermi Bubble (cm/s) - 1,400 cm/s (14 m/s) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Dilated Bullet|Tuning")
	float DilatedSpeed = 1400.0f;

	/** Collision sweep radius in Unreal units (cm) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Dilated Bullet|Tuning")
	float SweepRadius = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Dilated Bullet|Tuning")
	float BulletDamage = 32.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Dilated Bullet|Tuning")
	float MaxRange = 10000.0f;

private:
	FVector MoveDirection = FVector::ForwardVector;
	FVector PreviousLocation = FVector::ZeroVector;
	float TraveledDistance = 0.0f;
	float MaxAllowedDistance = 10000.0f;
	float LifeTimer = 0.0f;

	UPROPERTY()
	TWeakObjectPtr<AActor> ShooterActor;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> SlugMID;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> WakeMID;

	void ExecuteRemainderHitscan(const FVector& StartPoint);
};
