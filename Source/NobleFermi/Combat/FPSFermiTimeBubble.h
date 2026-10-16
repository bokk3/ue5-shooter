#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FPSFermiTimeBubble.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;

/**
 * Localized "Fermi Time-Dilation Field" (Superhot / Quantum Break Bubble)
 * Deployed automatically during an omnidirectional Ballistic Dive (and on active Fermi Echo decoys).
 * Slows enemy pawns and projectiles inside its 4.5m radius to 0.25x while granting the diving
 * operator 0.55x reflex superiority, and cleanly restores CustomTimeDilation = 1.0f on exit/expiry.
 */
UCLASS()
class NOBLEFERMI_API AFPSFermiTimeBubble : public AActor
{
	GENERATED_BODY()

public:
	AFPSFermiTimeBubble();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Fermi Bubble|Components")
	TObjectPtr<USphereComponent> BubbleSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Fermi Bubble|Components")
	TObjectPtr<UStaticMeshComponent> OuterShellMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Fermi Bubble|Components")
	TObjectPtr<UStaticMeshComponent> EquatorRingMesh;

	/** Radius of the localized chrono field in Unreal units (cm). 450.0f = 4.5 meters */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Fermi Bubble|Tuning")
	float BubbleRadius = 450.0f;

	/** Total lifetime of the chrono field in real-world seconds */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Fermi Bubble|Tuning")
	float BubbleDuration = 2.4f;

	/** Time dilation applied to hostile/other characters and projectiles inside the bubble */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Fermi Bubble|Tuning")
	float EnemyTimeDilation = 0.25f;

	/** Time dilation applied to the diving owner operator inside the bubble */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Fermi Bubble|Tuning")
	float OwnerTimeDilation = 0.55f;

	UFUNCTION(BlueprintPure, Category = "Fermi Bubble|State")
	float GetRemainingTime() const { return FMath::Max(0.0f, BubbleDuration - ElapsedRealTime); }

	UFUNCTION(BlueprintPure, Category = "Fermi Bubble|State")
	float GetNormalizedRemainingTime() const { return BubbleDuration > 0.0f ? FMath::Clamp((BubbleDuration - ElapsedRealTime) / BubbleDuration, 0.0f, 1.0f) : 0.0f; }

	/** Static helper to check if any active Fermi Time Bubble contains a given world location */
	static bool IsLocationInsideAnyBubble(const UWorld* World, const FVector& WorldLocation);

	/** Static helper to find the earliest sphere entry point for a ray directed into any active Fermi Bubble */
	static bool FindEarliestBubbleEntry(const UWorld* World, const FVector& RayStart, const FVector& RayEnd, float MaxTestDistance, FVector& OutEntryLocation, float& OutEntryDistance);

private:
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> ShellMID;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> RingMID;

	TSet<TWeakObjectPtr<AActor>> DilatedActors;
	float ElapsedRealTime = 0.0f;

	void RestoreAllDilatedActors();
};
