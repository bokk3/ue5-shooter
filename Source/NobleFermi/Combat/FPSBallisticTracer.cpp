#include "Combat/FPSBallisticTracer.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AFPSBallisticTracer::AFPSBallisticTracer()
{
	PrimaryActorTick.bCanEverTick = true;

	TracerRoot = CreateDefaultSubobject<USceneComponent>(TEXT("TracerRoot"));
	RootComponent = TracerRoot;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BaseMatFinder(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	BrassSlugMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BrassSlugMesh"));
	BrassSlugMesh->SetupAttachment(TracerRoot);
	BrassSlugMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BrassSlugMesh->SetCastShadow(false);
	BrassSlugMesh->bCastDynamicShadow = false;
	if (CylinderFinder.Succeeded())
	{
		BrassSlugMesh->SetStaticMesh(CylinderFinder.Object);
	}
	if (BaseMatFinder.Succeeded())
	{
		BrassSlugMesh->SetMaterial(0, BaseMatFinder.Object);
	}
	// Align cylinder along +X forward travel axis (3.5cm diameter slug, 14cm long for clear 3rd-person visibility)
	BrassSlugMesh->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
	BrassSlugMesh->SetRelativeScale3D(FVector(0.032f, 0.032f, 0.14f));

	WakeTrailMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WakeTrailMesh"));
	WakeTrailMesh->SetupAttachment(TracerRoot);
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
	// Elongated 110cm wake streak trailing behind the brass slug
	WakeTrailMesh->SetRelativeLocation(FVector(-58.0f, 0.0f, 0.0f));
	WakeTrailMesh->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
	WakeTrailMesh->SetRelativeScale3D(FVector(0.016f, 0.016f, 1.10f));

	InitialLifeSpan = 2.0f;
}

void AFPSBallisticTracer::BeginPlay()
{
	Super::BeginPlay();
}

void AFPSBallisticTracer::InitializeTracer(const FVector& StartLoc, const FVector& EndLoc, bool bInTimeBubble, bool bIsHologramEcho)
{
	SetActorLocation(StartLoc);
	TargetLocation = EndLoc;
	const FVector Delta = EndLoc - StartLoc;
	RemainingDistance = Delta.Size();
	TravelDirection = RemainingDistance > 1.0f ? (Delta / RemainingDistance) : GetActorForwardVector();
	SetActorRotation(TravelDirection.Rotation());

	ActiveSpeed = bInTimeBubble ? DilatedSpeed : StandardSpeed;
	bEchoTint = bIsHologramEcho;

	if (BrassSlugMesh)
	{
		UMaterialInstanceDynamic* SlugMID = BrassSlugMesh->CreateDynamicMaterialInstance(0);
		if (SlugMID)
		{
			const FLinearColor SlugCol = bIsHologramEcho
				? FLinearColor(0.1f, 2.5f, 1.8f, 1.0f)
				: FLinearColor(2.4f, 1.55f, 0.35f, 1.0f); // High-caliber glowing brass
			SlugMID->SetVectorParameterValue(TEXT("Color"), SlugCol);
		}
	}

	if (WakeTrailMesh)
	{
		UMaterialInstanceDynamic* TrailMID = WakeTrailMesh->CreateDynamicMaterialInstance(0);
		if (TrailMID)
		{
			const FLinearColor TrailCol = (bInTimeBubble || bIsHologramEcho)
				? FLinearColor(0.0f, 1.8f, 1.65f, 1.0f)  // Electric phosphor-cyan chrono wake
				: FLinearColor(2.2f, 0.85f, 0.15f, 1.0f); // Warm supersonic tracer wake
			TrailMID->SetVectorParameterValue(TEXT("Color"), TrailCol);
		}
	}
}

void AFPSBallisticTracer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const float StepDist = ActiveSpeed * DeltaTime;
	if (StepDist >= RemainingDistance || RemainingDistance <= 1.0f)
	{
		SetActorLocation(TargetLocation);
		Destroy();
		return;
	}

	AddActorWorldOffset(TravelDirection * StepDist);
	RemainingDistance -= StepDist;
}
