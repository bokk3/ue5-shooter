#include "Weapons/FPSDilatedBulletActor.h"
#include "Combat/FPSFermiTimeBubble.h"
#include "Combat/FPSBallisticTracer.h"
#include "Combat/FPSHealthComponent.h"
#include "Character/FPSCharacter.h"
#include "UI/FPSHUDWidget.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/World.h"

AFPSDilatedBulletActor::AFPSDilatedBulletActor()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	RootComponent = CollisionSphere;
	CollisionSphere->InitSphereRadius(SweepRadius);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BaseMatFinder(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	SlugMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SlugMesh"));
	SlugMesh->SetupAttachment(RootComponent);
	SlugMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SlugMesh->SetCastShadow(false);
	SlugMesh->bCastDynamicShadow = false;
	if (CylinderFinder.Succeeded())
	{
		SlugMesh->SetStaticMesh(CylinderFinder.Object);
	}
	if (BaseMatFinder.Succeeded())
	{
		SlugMesh->SetMaterial(0, BaseMatFinder.Object);
	}
	SlugMesh->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
	SlugMesh->SetRelativeScale3D(FVector(0.045f, 0.045f, 0.18f));

	WakeTrailMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WakeTrailMesh"));
	WakeTrailMesh->SetupAttachment(RootComponent);
	WakeTrailMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WakeTrailMesh->SetCastShadow(false);
	WakeTrailMesh->bCastDynamicShadow = false;
	if (CylinderFinder.Succeeded())
	{
		WakeTrailMesh->SetStaticMesh(CylinderFinder.Object);
	}
	if (BaseMatFinder.Succeeded())
	{
		WakeTrailMesh->SetMaterial(0, BaseMatFinder.Object);
	}
	WakeTrailMesh->SetRelativeLocation(FVector(-45.0f, 0.0f, 0.0f));
	WakeTrailMesh->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
	WakeTrailMesh->SetRelativeScale3D(FVector(0.022f, 0.022f, 0.90f));

	InitialLifeSpan = 6.0f;
}

void AFPSDilatedBulletActor::BeginPlay()
{
	Super::BeginPlay();
}

void AFPSDilatedBulletActor::InitializeSlug(const FVector& InStartLoc, const FVector& InDirection, float InDamage, float InRemainingRange, AActor* InShooter)
{
	SetActorLocation(InStartLoc);
	MoveDirection = InDirection.GetSafeNormal();
	SetActorRotation(MoveDirection.Rotation());
	PreviousLocation = InStartLoc;
	BulletDamage = InDamage;
	MaxAllowedDistance = FMath::Max(100.0f, InRemainingRange);
	ShooterActor = InShooter;
	TraveledDistance = 0.0f;
	LifeTimer = 0.0f;

	if (SlugMesh)
	{
		SlugMID = SlugMesh->CreateDynamicMaterialInstance(0);
		if (SlugMID)
		{
			// High-caliber glowing brass slug (respecting AEM_Manual exposure)
			SlugMID->SetVectorParameterValue(TEXT("Color"), FLinearColor(2.2f, 1.4f, 0.3f, 1.0f));
		}
	}

	if (WakeTrailMesh)
	{
		WakeMID = WakeTrailMesh->CreateDynamicMaterialInstance(0);
		if (WakeMID)
		{
			// Phosphor-cyan dilated chrono wake streak
			WakeMID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.0f, 1.6f, 1.5f, 1.0f));
		}
	}
}

void AFPSDilatedBulletActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	LifeTimer += DeltaTime;
	if (LifeTimer > 6.0f)
	{
		Destroy();
		return;
	}

	const float StepDist = DilatedSpeed * DeltaTime;
	const FVector CurrentLocation = GetActorLocation();
	const FVector NextLocation = CurrentLocation + (MoveDirection * StepDist);

	UWorld* World = GetWorld();
	if (!World)
	{
		Destroy();
		return;
	}

	FHitResult HitResult;
	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(this);
	if (ShooterActor.IsValid())
	{
		TraceParams.AddIgnoredActor(ShooterActor.Get());
	}
	TraceParams.bReturnPhysicalMaterial = true;

	// Sphere sweep collision test between previous frame and current frame positions
	const bool bHit = World->SweepSingleByChannel(
		HitResult,
		CurrentLocation,
		NextLocation,
		FQuat::Identity,
		ECC_Visibility,
		FCollisionShape::MakeSphere(SweepRadius),
		TraceParams
	);

	if (bHit)
	{
		AActor* HitActor = HitResult.GetActor();
		if (HitActor)
		{
			UFPSHealthComponent* HealthComp = HitActor->FindComponentByClass<UFPSHealthComponent>();
			if (HealthComp)
			{
				HealthComp->ApplyDamage(BulletDamage, ShooterActor.Get(), HitResult);

				AFPSCharacter* ShooterChar = Cast<AFPSCharacter>(ShooterActor.Get());
				if (ShooterChar && ShooterChar->GetActiveHUDWidget())
				{
					const bool bIsHead = (HitResult.BoneName == FName(TEXT("head")));
					ShooterChar->GetActiveHUDWidget()->TriggerHitmarker(bIsHead ? EFPSHitmarkerType::Headshot : EFPSHitmarkerType::Standard);
				}
			}
		}

		Destroy();
		return;
	}

	SetActorLocation(NextLocation);
	TraveledDistance += StepDist;

	if (TraveledDistance >= MaxAllowedDistance)
	{
		Destroy();
		return;
	}

	// Verify if the bullet is still inside a localized Fermi Bubble
	const bool bInsideBubble = AFPSFermiTimeBubble::IsLocationInsideAnyBubble(World, NextLocation);
	if (!bInsideBubble)
	{
		// Exited the Fermi Bubble: Piecewise convert back into supersonic hitscan remainder trace
		ExecuteRemainderHitscan(NextLocation);
		Destroy();
		return;
	}
}

void AFPSDilatedBulletActor::ExecuteRemainderHitscan(const FVector& StartPoint)
{
	UWorld* World = GetWorld();
	if (!World) return;

	const float RemainingRange = FMath::Max(0.0f, MaxAllowedDistance - TraveledDistance);
	if (RemainingRange <= 1.0f) return;

	const FVector HitscanEnd = StartPoint + (MoveDirection * RemainingRange);

	FHitResult RemainderHit;
	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(this);
	if (ShooterActor.IsValid())
	{
		TraceParams.AddIgnoredActor(ShooterActor.Get());
	}
	TraceParams.bReturnPhysicalMaterial = true;

	FVector ImpactPoint = HitscanEnd;
	if (World->LineTraceSingleByChannel(RemainderHit, StartPoint, HitscanEnd, ECC_Visibility, TraceParams))
	{
		ImpactPoint = RemainderHit.ImpactPoint;
		AActor* HitActor = RemainderHit.GetActor();
		if (HitActor)
		{
			UFPSHealthComponent* HealthComp = HitActor->FindComponentByClass<UFPSHealthComponent>();
			if (HealthComp)
			{
				HealthComp->ApplyDamage(BulletDamage, ShooterActor.Get(), RemainderHit);

				AFPSCharacter* ShooterChar = Cast<AFPSCharacter>(ShooterActor.Get());
				if (ShooterChar && ShooterChar->GetActiveHUDWidget())
				{
					const bool bIsHead = (RemainderHit.BoneName == FName(TEXT("head")));
					ShooterChar->GetActiveHUDWidget()->TriggerHitmarker(bIsHead ? EFPSHitmarkerType::Headshot : EFPSHitmarkerType::Standard);
				}
			}
		}
	}

	// Visual tracer streak across the remainder hitscan path
	AFPSBallisticTracer* Tracer = World->SpawnActor<AFPSBallisticTracer>(AFPSBallisticTracer::StaticClass(), StartPoint, MoveDirection.Rotation());
	if (Tracer)
	{
		Tracer->InitializeTracer(StartPoint, ImpactPoint, false, false);
	}
}
