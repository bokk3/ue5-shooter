#include "Combat/FPSFermiTimeBubble.h"
#include "Combat/FPSBallisticTracer.h"
#include "Combat/FPSProjectileBase.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/World.h"
#include "EngineUtils.h"

AFPSFermiTimeBubble::AFPSFermiTimeBubble()
{
	PrimaryActorTick.bCanEverTick = true;

	BubbleSphere = CreateDefaultSubobject<USphereComponent>(TEXT("BubbleSphere"));
	RootComponent = BubbleSphere;
	BubbleSphere->InitSphereRadius(BubbleRadius);
	BubbleSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	BubbleSphere->SetCollisionObjectType(ECC_WorldDynamic);
	BubbleSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	BubbleSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	BubbleSphere->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	BubbleSphere->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Overlap);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BaseMatFinder(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	// Subtle phosphor-cyan chrono horizon shell (kept very thin/low-opacity to respect AEM_Manual exposure lock)
	OuterShellMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("OuterShellMesh"));
	OuterShellMesh->SetupAttachment(BubbleSphere);
	OuterShellMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	OuterShellMesh->SetCastShadow(false);
	OuterShellMesh->bCastDynamicShadow = false;
	if (SphereFinder.Succeeded())
	{
		OuterShellMesh->SetStaticMesh(SphereFinder.Object);
	}
	if (BaseMatFinder.Succeeded())
	{
		OuterShellMesh->SetMaterial(0, BaseMatFinder.Object);
	}
	// Ground ring / chrono boundary indicator (Radius = 450cm -> Scale 9.0 in X/Y, thin 2cm disc in Z)
	OuterShellMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -85.0f));
	OuterShellMesh->SetRelativeScale3D(FVector(9.0f, 9.0f, 0.015f));

	EquatorRingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EquatorRingMesh"));
	EquatorRingMesh->SetupAttachment(BubbleSphere);
	EquatorRingMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	EquatorRingMesh->SetCastShadow(false);
	EquatorRingMesh->bCastDynamicShadow = false;
	if (CylinderFinder.Succeeded())
	{
		EquatorRingMesh->SetStaticMesh(CylinderFinder.Object);
	}
	if (BaseMatFinder.Succeeded())
	{
		EquatorRingMesh->SetMaterial(0, BaseMatFinder.Object);
	}
	EquatorRingMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -84.0f));
	EquatorRingMesh->SetRelativeScale3D(FVector(9.05f, 9.05f, 0.025f));
}

void AFPSFermiTimeBubble::BeginPlay()
{
	Super::BeginPlay();

	if (BubbleSphere)
	{
		BubbleSphere->SetSphereRadius(BubbleRadius);
	}

	if (OuterShellMesh)
	{
		ShellMID = OuterShellMesh->CreateDynamicMaterialInstance(0);
		if (ShellMID)
		{
			// Low-intensity phosphor teal so AEM_Manual never whites out
			ShellMID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.0f, 0.32f, 0.36f, 1.0f));
		}
	}

	if (EquatorRingMesh)
	{
		RingMID = EquatorRingMesh->CreateDynamicMaterialInstance(0);
		if (RingMID)
		{
			RingMID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.0f, 0.78f, 0.72f, 1.0f));
		}
	}
}

void AFPSFermiTimeBubble::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Track un-dilated real-world time so the 2.4s bubble duration is exact regardless of owner dilation
	const float CurrentDilation = FMath::Max(0.05f, GetActorTimeDilation());
	const float RealDeltaTime = DeltaTime / CurrentDilation;
	ElapsedRealTime += RealDeltaTime;

	if (ElapsedRealTime >= BubbleDuration)
	{
		RestoreAllDilatedActors();
		Destroy();
		return;
	}

	// Expanding chrono wave animation in first 0.22s, gentle breathing pulse afterward
	const float ExpandAlpha = FMath::Clamp(ElapsedRealTime / 0.22f, 0.0f, 1.0f);
	const float SmoothScale = FMath::InterpEaseOut(0.15f, 1.0f, ExpandAlpha, 3.0f);
	const float Pulse = 1.0f + 0.025f * FMath::Sin(ElapsedRealTime * 14.0f);
	const float DiameterScale = (BubbleRadius / 50.0f) * SmoothScale * Pulse;

	if (OuterShellMesh)
	{
		OuterShellMesh->SetRelativeScale3D(FVector(DiameterScale * 0.96f, DiameterScale * 0.96f, 0.012f));
	}
	if (EquatorRingMesh)
	{
		EquatorRingMesh->SetRelativeScale3D(FVector(DiameterScale, DiameterScale, 0.022f));
	}

	// Query actors within the 4.5m sphere and apply localized CustomTimeDilation
	UWorld* World = GetWorld();
	if (!World) return;

	AActor* BubbleOwner = GetOwner();
	const FVector BubbleCenter = GetActorLocation();
	const float RadiusSq = FMath::Square(BubbleRadius);

	TSet<TWeakObjectPtr<AActor>> CurrentlyInside;

	// 1. Check all Pawns (Players, Enemies, Decoys)
	for (TActorIterator<APawn> It(World); It; ++It)
	{
		APawn* Pawn = *It;
		if (!Pawn || !IsValid(Pawn)) continue;

		if (FVector::DistSquared(Pawn->GetActorLocation(), BubbleCenter) <= RadiusSq)
		{
			CurrentlyInside.Add(Pawn);
			DilatedActors.Add(Pawn);

			if (Pawn == BubbleOwner)
			{
				Pawn->CustomTimeDilation = OwnerTimeDilation;
			}
			else
			{
				Pawn->CustomTimeDilation = EnemyTimeDilation;
			}
		}
	}

	// 2. Check all Physical Projectiles & Ballistic Tracers
	for (TActorIterator<AFPSProjectileBase> It(World); It; ++It)
	{
		AFPSProjectileBase* Proj = *It;
		if (Proj && IsValid(Proj) && FVector::DistSquared(Proj->GetActorLocation(), BubbleCenter) <= RadiusSq)
		{
			CurrentlyInside.Add(Proj);
			DilatedActors.Add(Proj);
			Proj->CustomTimeDilation = EnemyTimeDilation;
		}
	}

	for (TActorIterator<AFPSBallisticTracer> It(World); It; ++It)
	{
		AFPSBallisticTracer* Tracer = *It;
		if (Tracer && IsValid(Tracer) && FVector::DistSquared(Tracer->GetActorLocation(), BubbleCenter) <= RadiusSq)
		{
			CurrentlyInside.Add(Tracer);
			DilatedActors.Add(Tracer);
			Tracer->CustomTimeDilation = EnemyTimeDilation;
		}
	}

	// Restore any actor that stepped outside the 4.5m bubble radius
	for (auto It = DilatedActors.CreateIterator(); It; ++It)
	{
		if (!It->IsValid())
		{
			It.RemoveCurrent();
			continue;
		}

		AActor* Actor = It->Get();
		if (!CurrentlyInside.Contains(*It))
		{
			Actor->CustomTimeDilation = 1.0f;
			It.RemoveCurrent();
		}
	}
}

void AFPSFermiTimeBubble::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RestoreAllDilatedActors();
	Super::EndPlay(EndPlayReason);
}

void AFPSFermiTimeBubble::RestoreAllDilatedActors()
{
	for (const TWeakObjectPtr<AActor>& WeakActor : DilatedActors)
	{
		if (WeakActor.IsValid())
		{
			WeakActor->CustomTimeDilation = 1.0f;
		}
	}
	DilatedActors.Empty();
}

bool AFPSFermiTimeBubble::IsLocationInsideAnyBubble(const UWorld* World, const FVector& WorldLocation)
{
	if (!World) return false;

	for (TActorIterator<AFPSFermiTimeBubble> It(World); It; ++It)
	{
		const AFPSFermiTimeBubble* Bubble = *It;
		if (Bubble && IsValid(Bubble))
		{
			if (FVector::DistSquared(WorldLocation, Bubble->GetActorLocation()) <= FMath::Square(Bubble->BubbleRadius))
			{
				return true;
			}
		}
	}
	return false;
}

bool AFPSFermiTimeBubble::FindEarliestBubbleEntry(const UWorld* World, const FVector& RayStart, const FVector& RayEnd, float MaxTestDistance, FVector& OutEntryLocation, float& OutEntryDistance)
{
	if (!World) return false;

	const FVector DirVec = RayEnd - RayStart;
	const float SegmentLen = DirVec.Size();
	if (SegmentLen <= KINDA_SMALL_NUMBER) return false;

	const FVector UnitDir = DirVec / SegmentLen;
	const float EffectiveMaxDist = FMath::Min(SegmentLen, MaxTestDistance);

	float ClosestEntryDist = EffectiveMaxDist;
	bool bFoundEntry = false;
	FVector BestEntryLoc = FVector::ZeroVector;

	for (TActorIterator<AFPSFermiTimeBubble> It(World); It; ++It)
	{
		const AFPSFermiTimeBubble* Bubble = *It;
		if (!Bubble || !IsValid(Bubble)) continue;

		const FVector Center = Bubble->GetActorLocation();
		const float Radius = Bubble->BubbleRadius;
		const float RadiusSq = Radius * Radius;

		const FVector M = RayStart - Center;
		const float DistSqFromCenter = M.SizeSquared();

		if (DistSqFromCenter <= RadiusSq)
		{
			// Ray origin is already inside this bubble
			OutEntryLocation = RayStart;
			OutEntryDistance = 0.0f;
			return true;
		}

		// Ray starts outside the sphere
		const float B = FVector::DotProduct(M, UnitDir);
		if (B > 0.0f)
		{
			// Sphere center is strictly behind ray start
			continue;
		}

		const float C = DistSqFromCenter - RadiusSq;
		const float Discriminant = (B * B) - C;
		if (Discriminant < 0.0f)
		{
			// Ray misses sphere
			continue;
		}

		const float TEntry = -B - FMath::Sqrt(Discriminant);
		if (TEntry >= 0.0f && TEntry < ClosestEntryDist)
		{
			ClosestEntryDist = TEntry;
			// Nudge slightly inside the bubble horizon (2cm along flight path)
			BestEntryLoc = RayStart + UnitDir * (TEntry + 2.0f);
			bFoundEntry = true;
		}
	}

	if (bFoundEntry)
	{
		OutEntryLocation = BestEntryLoc;
		OutEntryDistance = ClosestEntryDist;
		return true;
	}

	return false;
}

