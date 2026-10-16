#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Multiplayer/FPSPlayerState.h"
#include "FPSFermiDecoyActor.generated.h"

class USkeletalMeshComponent;
class UStaticMeshComponent;
class UAnimSequence;
class AFPSFermiTimeBubble;

/**
 * "Fermi Echo" Acoustic Hologram Decoy & Quantum Phase-Swap Partner Actor
 * Deployed via [Q] key. Spawns an autonomous phosphor-green/cyan holographic clone
 * of the 21-bone SK_BaseMale operator holding SM_AK47 that sprints forward, fires simulated
 * unsuppressed bursts to flood enemy minimaps/compass tapes via UFPSRadarSubsystem,
 * projects a secondary Fermi Time Bubble when the owner dives, and serves as the anchor
 * for mid-air Quantum Phase-Swapping.
 */
UCLASS()
class NOBLEFERMI_API AFPSFermiDecoyActor : public AActor
{
	GENERATED_BODY()

public:
	AFPSFermiDecoyActor();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Fermi Echo|Components")
	TObjectPtr<USceneComponent> DecoyRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Fermi Echo|Components")
	TObjectPtr<USkeletalMeshComponent> HologramMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Fermi Echo|Components")
	TObjectPtr<UStaticMeshComponent> HologramAK47Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Fermi Echo|Components")
	TObjectPtr<UStaticMeshComponent> BaseEmitterRing;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fermi Echo|Animations")
	TObjectPtr<UAnimSequence> AnimSprint;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fermi Echo|Animations")
	TObjectPtr<UAnimSequence> AnimRun;

	/** Sprinting velocity of the autonomous hologram clone (cm/s) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Fermi Echo|Tuning")
	float DecoySprintSpeed = 680.0f;

	/** Active lifetime of the Fermi Echo decoy (seconds) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Fermi Echo|Tuning")
	float DecoyDuration = 6.0f;

	/** Interval between simulated unsuppressed gunfire radar bursts (seconds) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Fermi Echo|Tuning")
	float SimulatedFireInterval = 0.28f;

	void InitializeDecoy(const FVector& MoveDirection, EFPSPlayerTeam InSourceTeam);
	void SpawnSecondaryTimeBubble();

	UFUNCTION(BlueprintPure, Category = "Fermi Echo|State")
	float GetRemainingLifetime() const { return FMath::Max(0.0f, DecoyDuration - ElapsedTime); }

private:
	FVector SprintDirection = FVector::ForwardVector;
	EFPSPlayerTeam SourceTeam = EFPSPlayerTeam::Bravo; // Broadcasts on opposite team if solo so local player also sees decoy radar test blips, or owner's team in MP
	float ElapsedTime = 0.0f;
	float FireBurstTimer = 0.0f;

	UPROPERTY()
	TObjectPtr<AFPSFermiTimeBubble> SecondaryBubble;

	void FireAcousticBurst();
};
