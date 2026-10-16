#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FPSBallisticTracer.generated.h"

class UStaticMeshComponent;

/**
 * Physicalized Slow-Motion Ballistic Tracer Actor
 * Spawned when weapons fire (especially inside or through a Fermi Time-Dilation Bubble).
 * Renders a high-caliber brass projectile slug with an elongated cyan/amber wake streak
 * that slices visibly through 3D space at dilated speeds (2,800 cm/s in slow-mo).
 */
UCLASS()
class NOBLEFERMI_API AFPSBallisticTracer : public AActor
{
	GENERATED_BODY()

public:
	AFPSBallisticTracer();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	void InitializeTracer(const FVector& StartLoc, const FVector& EndLoc, bool bInTimeBubble, bool bIsHologramEcho = false);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tracer|Components")
	TObjectPtr<USceneComponent> TracerRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tracer|Components")
	TObjectPtr<UStaticMeshComponent> BrassSlugMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tracer|Components")
	TObjectPtr<UStaticMeshComponent> WakeTrailMesh;

	/** Speed when dilated inside a Fermi Time Bubble (cm/s) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Tracer|Tuning")
	float DilatedSpeed = 2800.0f;

	/** Standard high-velocity supersonic visual tracer speed (cm/s) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Tracer|Tuning")
	float StandardSpeed = 12500.0f;

private:
	FVector TargetLocation = FVector::ZeroVector;
	FVector TravelDirection = FVector::ForwardVector;
	float RemainingDistance = 0.0f;
	float ActiveSpeed = 12500.0f;
	bool bEchoTint = false;
};
