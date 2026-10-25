#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Multiplayer/FPSPlayerState.h"
#include "FPSRadarSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FFPSRadarPing
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Radar")
	FVector WorldLocation = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Radar")
	EFPSPlayerTeam SourceTeam = EFPSPlayerTeam::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Radar")
	float LifetimeRemaining = 2.5f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Radar")
	float InitialLifetime = 2.5f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Radar")
	bool bIsGunfire = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Radar")
	TWeakObjectPtr<AActor> SourceActor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Radar")
	float AcousticRadius = 1500.0f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRadarPingAddedSignature, const FFPSRadarPing&, NewPing);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnUAVSweepSignature, EFPSPlayerTeam, Team, float, SweepAngle);

/**
 * AAA FPS World Radar & Acoustic Gunfire Subsystem
 * Tracks unsuppressed gunfire pings, UAV sweeps, and radar stealth mechanics.
 */
UCLASS()
class NOBLEFERMI_API UFPSRadarSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	UPROPERTY(BlueprintAssignable, Category = "FPS Radar|Events")
	FOnRadarPingAddedSignature OnRadarPingAdded;

	UPROPERTY(BlueprintAssignable, Category = "FPS Radar|Events")
	FOnUAVSweepSignature OnUAVSweep;

	// Acoustic Gunfire Detection
	UFUNCTION(BlueprintCallable, Category = "FPS Radar")
	void ReportGunfire(AActor* ShooterActor, FVector Location, EFPSPlayerTeam ShooterTeam, bool bIsSuppressed);

	// Surface-Aware Acoustic Footstep Detection (e.g. Coastal Wooden Pier & Shallow Water)
	UFUNCTION(BlueprintCallable, Category = "FPS Radar")
	void ReportAcousticPing(AActor* InSourceActor, FVector Location, EFPSPlayerTeam InSourceTeam, float PingRadius = 1500.0f, float Duration = 1.8f);

	UFUNCTION(BlueprintCallable, Category = "FPS Radar")
	void ReportAcousticFootstep(AActor* InSourceActor, FVector Location, EFPSPlayerTeam InSourceTeam, float PingRadius = 1500.0f);

	// UAV Reconnaissance
	UFUNCTION(BlueprintCallable, Category = "FPS Radar")
	void ActivateUAV(EFPSPlayerTeam Team, float Duration = 30.0f);

	UFUNCTION(BlueprintPure, Category = "FPS Radar")
	bool IsUAVActiveForTeam(EFPSPlayerTeam Team) const;

	// Ping queries
	UFUNCTION(BlueprintPure, Category = "FPS Radar")
	TArray<FFPSRadarPing> GetVisiblePingsForTeam(EFPSPlayerTeam ViewerTeam) const;

	UPROPERTY(EditDefaultsOnly, Category = "FPS Radar|Tuning")
	float GunfirePingDuration = 2.5f;

	UPROPERTY(EditDefaultsOnly, Category = "FPS Radar|Tuning")
	float UAVSweepInterval = 3.5f;

private:
	UPROPERTY()
	TArray<FFPSRadarPing> ActivePings;

	float AlphaTeamUAVTimer = 0.0f;
	float BravoTeamUAVTimer = 0.0f;

	float AlphaSweepTimer = 0.0f;
	float BravoSweepTimer = 0.0f;

	void PerformUAVSweep(EFPSPlayerTeam Team);
};
