#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FPSRangeTarget.generated.h"

class UStaticMesh;
class UStaticMeshComponent;
class UTextRenderComponent;
class AFPSCharacter;

UENUM(BlueprintType)
enum class EFPSRangeTargetKind : uint8
{
	/** Steel popper: falls back on a hit and stands up again after ResetDelay. */
	Popper,
	/** Man-sized steel silhouette: rocks back on a hit; the top of the target counts as the head. */
	Silhouette,
	/** Paper target on a stand: a small shake on a hit. */
	Paper
};

/** What a shot did to a range target, so the shooter's HUD can show the matching hit marker. */
enum class EFPSRangeHitResult : uint8
{
	/** The target was already down; nothing to report. */
	None,
	Body,
	Head,
	/** The shot knocked a popper down. */
	KnockDown
};

/**
 * A reactive target for L_Range. The weapon's hitscan (and the Fermi dilated slug) calls RegisterHit
 * when it hits one. The server owns the hit count and the popper's down state and replicates them;
 * every machine animates the swing locally. A readout above the target shows the hit count in the
 * current string of fire and the distance of the last hit.
 *
 * The mesh is the Trellis target prop (pivot at the bottom, facing +X). The map script spawns these
 * with yaw 180 so they face the spawn line, and the target swings back away from the shooter.
 */
UCLASS()
class NOBLEFERMI_API AFPSRangeTarget : public AActor
{
	GENERATED_BODY()

public:
	AFPSRangeTarget();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/**
	 * Registers a hit at Hit.ImpactPoint by Shooter. Only the server changes state; any machine
	 * gets back what the hit did, so an owning client can predict its hit marker.
	 */
	EFPSRangeHitResult RegisterHit(const FHitResult& Hit, const AActor* Shooter);

	/** Shows the hit marker for a range hit on Shooter's HUD, if Shooter is the local player. */
	static void ShowHitMarker(AFPSCharacter* Shooter, EFPSRangeHitResult Result);

	UFUNCTION(BlueprintPure, Category = "Range Target")
	bool IsKnockedDown() const { return bKnockedDown; }

	UFUNCTION(BlueprintPure, Category = "Range Target")
	int32 GetHitCount() const { return HitCount; }

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Range Target")
	EFPSRangeTargetKind Kind = EFPSRangeTargetKind::Silhouette;

	/** The target's mesh; applied to TargetMesh on construction. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Range Target")
	TObjectPtr<UStaticMesh> TargetMeshAsset;

	/** Seconds a popper stays down before it stands up again. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Range Target", meta = (ClampMin = "0.5"))
	float ResetDelay = 2.5f;

	/** Seconds without a hit after which the next hit starts a new count. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Range Target", meta = (ClampMin = "1.0"))
	float StringTimeout = 6.0f;

	/** Top fraction of a silhouette's height that counts as the head. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Range Target", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float HeadZoneFraction = 0.18f;

	/** Shows the hit readout above the target. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Range Target")
	bool bShowReadout = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Range Target")
	TObjectPtr<USceneComponent> SceneRoot;

	/** Hinge at the target's base; the swing rotates this. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Range Target")
	TObjectPtr<USceneComponent> Hinge;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Range Target")
	TObjectPtr<UStaticMeshComponent> TargetMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Range Target")
	TObjectPtr<UTextRenderComponent> Readout;

private:
	UFUNCTION()
	void OnRep_HitSerial();

	/** Wakes the swing when a popper goes down or stands up on a client. */
	UFUNCTION()
	void OnRep_KnockedDown();

	void PlayHitReaction(bool bHead);
	void StandUp();
	void UpdateReadout();
	/** Height of a point on the target, 0 at the bottom of the mesh and 1 at the top. */
	float HeightFraction(const FVector& ImpactPoint) const;

	/** Bumped by the server on every hit so clients replay the reaction. */
	UPROPERTY(ReplicatedUsing = OnRep_HitSerial)
	int32 HitSerial = 0;

	UPROPERTY(Replicated)
	int32 HitCount = 0;

	UPROPERTY(ReplicatedUsing = OnRep_KnockedDown)
	bool bKnockedDown = false;

	UPROPERTY(Replicated)
	bool bLastHitHead = false;

	UPROPERTY(Replicated)
	float LastHitDistance = 0.0f;

	double LastHitTime = -1000.0;
	FTimerHandle ResetTimer;

	/** Swing angle (degrees, positive leans back) and its angular velocity (degrees per second). */
	float SwingAngle = 0.0f;
	float SwingVelocity = 0.0f;
	float FlashRemaining = 0.0f;
};
