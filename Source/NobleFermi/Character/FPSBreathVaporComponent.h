#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FPSBreathVaporComponent.generated.h"

class USkeletalMeshComponent;
class UProceduralMeshComponent;
class UMaterialInstanceDynamic;

/**
 * Individual cold condensation vapor puff drifting in 3D space
 */
struct FFPSVaporPuff
{
	FVector Position;
	FVector Velocity;
	float Age = 0.0f;
	float MaxLifetime = 1.35f;
	float InitialRadius = 3.2f;
	float MaxRadius = 15.5f;
	float MaxOpacity = 0.70f;
	float RotationDeg = 0.0f;
	float RotSpeed = 0.0f;
};

/**
 * Procedural Cold Breath Vapor Component for Noble Fermi Operators
 * Emits lifelike condensing breath vapor puffs from the helmet visor/mouth grill
 * in synchronization with the thoracic breathing exhalation cycle.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class NOBLEFERMI_API UFPSBreathVaporComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFPSBreathVaporComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Initialize vapor emitter with owner skeletal mesh */
	UFUNCTION(BlueprintCallable, Category = "Tactical|Effects")
	void InitializeBreathVapor(USkeletalMeshComponent* InBodyMesh);

	/** Set whether cold environment condensation is active (default: true in hangar & cold missions) */
	UFUNCTION(BlueprintCallable, Category = "Tactical|Effects")
	void SetColdEnvironmentActive(bool bActive) { bColdEnvironment = bActive; }

	/** Set breathing intensity multiplier (e.g. 1.0 at rest, 1.8 after sprint/exertion) */
	UFUNCTION(BlueprintCallable, Category = "Tactical|Effects")
	void SetExertionMultiplier(float Multiplier) { ExertionMultiplier = FMath::Clamp(Multiplier, 0.5f, 2.5f); }

	/** Directly trigger a manual puff (useful for testing or event triggers) */
	UFUNCTION(BlueprintCallable, Category = "Tactical|Effects")
	void TriggerPuff();

private:
	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> TargetBodyMesh;

	UPROPERTY(Transient)
	TObjectPtr<UProceduralMeshComponent> VaporMeshComp;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> VaporMID;

	UPROPERTY(EditAnywhere, Category = "Tactical|Effects")
	bool bColdEnvironment = true;

	UPROPERTY(EditAnywhere, Category = "Tactical|Effects")
	float ExertionMultiplier = 1.0f;

	// Breathing phase accumulator (0.32 Hz base = ~3.125s per full cycle)
	float BreathCyclePhase = 0.0f;
	float LastPuffSpawnTime = 0.0f;
	int32 PuffsEmittedInCurrentBreath = 0;

	// Pool of active vapor puffs
	TArray<FFPSVaporPuff> ActivePuffs;

	void UpdateBreathingAndSpawnPuffs(float DeltaTime);
	void UpdatePuffPhysics(float DeltaTime);
	void RenderVaporMesh();

	FVector ComputeMouthWorldLocation(FVector& OutHeadFwd, FVector& OutHeadUp) const;
};
