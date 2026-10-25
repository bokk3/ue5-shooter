#include "Radar/FPSRadarSubsystem.h"
#include "Multiplayer/FPSPerkComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

void UFPSRadarSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	ActivePings.Empty();
}

TStatId UFPSRadarSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UFPSRadarSubsystem, STATGROUP_Tickables);
}

void UFPSRadarSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Update Active Ping Lifetimes
	for (int32 i = ActivePings.Num() - 1; i >= 0; --i)
	{
		ActivePings[i].LifetimeRemaining -= DeltaTime;
		if (ActivePings[i].LifetimeRemaining <= 0.0f)
		{
			ActivePings.RemoveAt(i);
		}
	}

	// Update UAV Durations & Sweeps
	if (AlphaTeamUAVTimer > 0.0f)
	{
		AlphaTeamUAVTimer -= DeltaTime;
		AlphaSweepTimer += DeltaTime;
		if (AlphaSweepTimer >= UAVSweepInterval)
		{
			AlphaSweepTimer = 0.0f;
			PerformUAVSweep(EFPSPlayerTeam::Alpha);
		}
	}

	if (BravoTeamUAVTimer > 0.0f)
	{
		BravoTeamUAVTimer -= DeltaTime;
		BravoSweepTimer += DeltaTime;
		if (BravoSweepTimer >= UAVSweepInterval)
		{
			BravoSweepTimer = 0.0f;
			PerformUAVSweep(EFPSPlayerTeam::Bravo);
		}
	}
}

void UFPSRadarSubsystem::ReportGunfire(AActor* ShooterActor, FVector Location, EFPSPlayerTeam ShooterTeam, bool bIsSuppressed)
{
	// Suppressor stealth check: completely suppressed shots generate no radar acoustic signature!
	if (bIsSuppressed)
	{
		return;
	}

	FFPSRadarPing NewPing;
	NewPing.WorldLocation = Location;
	NewPing.SourceTeam = ShooterTeam;
	NewPing.LifetimeRemaining = GunfirePingDuration;
	NewPing.InitialLifetime = GunfirePingDuration;
	NewPing.bIsGunfire = true;
	NewPing.SourceActor = ShooterActor;

	ActivePings.Add(NewPing);
	OnRadarPingAdded.Broadcast(NewPing);
}

void UFPSRadarSubsystem::ReportAcousticPing(AActor* InSourceActor, FVector Location, EFPSPlayerTeam InSourceTeam, float PingRadius, float Duration)
{
	FFPSRadarPing NewPing;
	NewPing.WorldLocation = Location;
	NewPing.SourceTeam = InSourceTeam;
	NewPing.LifetimeRemaining = Duration;
	NewPing.InitialLifetime = Duration;
	NewPing.bIsGunfire = false;
	NewPing.SourceActor = InSourceActor;
	NewPing.AcousticRadius = PingRadius;

	ActivePings.Add(NewPing);
	OnRadarPingAdded.Broadcast(NewPing);
}

void UFPSRadarSubsystem::ReportAcousticFootstep(AActor* InSourceActor, FVector Location, EFPSPlayerTeam InSourceTeam, float PingRadius)
{
	ReportAcousticPing(InSourceActor, Location, InSourceTeam, PingRadius, 1.8f);
}

void UFPSRadarSubsystem::ActivateUAV(EFPSPlayerTeam Team, float Duration)
{
	if (Team == EFPSPlayerTeam::Alpha)
	{
		AlphaTeamUAVTimer = FMath::Max(AlphaTeamUAVTimer, Duration);
		PerformUAVSweep(EFPSPlayerTeam::Alpha);
	}
	else if (Team == EFPSPlayerTeam::Bravo)
	{
		BravoTeamUAVTimer = FMath::Max(BravoTeamUAVTimer, Duration);
		PerformUAVSweep(EFPSPlayerTeam::Bravo);
	}
}

bool UFPSRadarSubsystem::IsUAVActiveForTeam(EFPSPlayerTeam Team) const
{
	if (Team == EFPSPlayerTeam::Alpha) return AlphaTeamUAVTimer > 0.0f;
	if (Team == EFPSPlayerTeam::Bravo) return BravoTeamUAVTimer > 0.0f;
	return false;
}

void UFPSRadarSubsystem::PerformUAVSweep(EFPSPlayerTeam Team)
{
	UWorld* World = GetWorld();
	if (!World) return;

	OnUAVSweep.Broadcast(Team, 0.0f);

	TArray<AActor*> AllCharacters;
	UGameplayStatics::GetAllActorsOfClass(World, ACharacter::StaticClass(), AllCharacters);

	for (AActor* Actor : AllCharacters)
	{
		ACharacter* Char = Cast<ACharacter>(Actor);
		if (!Char) continue;

		APlayerState* PS = Char->GetPlayerState();
		AFPSPlayerState* FPSPS = Cast<AFPSPlayerState>(PS);
		if (!FPSPS) continue;

		// Only ping enemy players
		if (FPSPS->GetPlayerTeam() == Team || FPSPS->GetPlayerTeam() == EFPSPlayerTeam::None)
		{
			continue;
		}

		// Cold-Blooded perk stealth check: immune to UAV sweep
		UFPSPerkComponent* PerkComp = Char->FindComponentByClass<UFPSPerkComponent>();
		if (PerkComp && PerkComp->IsHiddenFromRadar())
		{
			continue;
		}

		FFPSRadarPing UAVPing;
		UAVPing.WorldLocation = Char->GetActorLocation();
		UAVPing.SourceTeam = FPSPS->GetPlayerTeam();
		UAVPing.LifetimeRemaining = 2.0f;
		UAVPing.InitialLifetime = 2.0f;
		UAVPing.bIsGunfire = false;
		UAVPing.SourceActor = Char;

		ActivePings.Add(UAVPing);
		OnRadarPingAdded.Broadcast(UAVPing);
	}
}

TArray<FFPSRadarPing> UFPSRadarSubsystem::GetVisiblePingsForTeam(EFPSPlayerTeam ViewerTeam) const
{
	TArray<FFPSRadarPing> OutPings;
	for (const FFPSRadarPing& Ping : ActivePings)
	{
		// Show pings if they are from the opposing team
		if (Ping.SourceTeam != ViewerTeam)
		{
			OutPings.Add(Ping);
		}
	}
	return OutPings;
}
