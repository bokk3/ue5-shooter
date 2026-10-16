#include "Combat/FPSFermiDecoyActor.h"
#include "Combat/FPSFermiTimeBubble.h"
#include "Combat/FPSBallisticTracer.h"
#include "Radar/FPSRadarSubsystem.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/World.h"

AFPSFermiDecoyActor::AFPSFermiDecoyActor()
{
	PrimaryActorTick.bCanEverTick = true;

	DecoyRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DecoyRoot"));
	RootComponent = DecoyRoot;

	HologramMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HologramMesh"));
	HologramMesh->SetupAttachment(DecoyRoot);
	HologramMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -96.0f));
	HologramMesh->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	HologramMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HologramMesh->SetCastShadow(false);

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> BaseMaleFinder(TEXT("/Game/Characters/BaseMale/SK_BaseMale.SK_BaseMale"));
	if (BaseMaleFinder.Succeeded())
	{
		HologramMesh->SetSkeletalMesh(BaseMaleFinder.Object);
	}

	static ConstructorHelpers::FObjectFinder<UAnimSequence> SprintFinder(TEXT("/Game/Characters/BaseMale/Animations/Anim_BaseMale_Sprint.Anim_BaseMale_Sprint"));
	if (SprintFinder.Succeeded())
	{
		AnimSprint = SprintFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UAnimSequence> RunFinder(TEXT("/Game/Characters/BaseMale/Animations/Anim_BaseMale_Run.Anim_BaseMale_Run"));
	if (RunFinder.Succeeded())
	{
		AnimRun = RunFinder.Object;
	}

	HologramAK47Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HologramAK47Mesh"));
	HologramAK47Mesh->SetupAttachment(HologramMesh, TEXT("weapon_r"));
	HologramAK47Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HologramAK47Mesh->SetCastShadow(false);
	HologramAK47Mesh->SetRelativeLocation(FVector::ZeroVector);
	HologramAK47Mesh->SetRelativeRotation(FRotator::ZeroRotator);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> AK47Finder(TEXT("/Game/Characters/BaseMale/SM_AK47.SM_AK47"));
	if (AK47Finder.Succeeded())
	{
		HologramAK47Mesh->SetStaticMesh(AK47Finder.Object);
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BaseMatFinder(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	BaseEmitterRing = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseEmitterRing"));
	BaseEmitterRing->SetupAttachment(DecoyRoot);
	BaseEmitterRing->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BaseEmitterRing->SetCastShadow(false);
	if (CylinderFinder.Succeeded())
	{
		BaseEmitterRing->SetStaticMesh(CylinderFinder.Object);
	}
	if (BaseMatFinder.Succeeded())
	{
		BaseEmitterRing->SetMaterial(0, BaseMatFinder.Object);
	}
	BaseEmitterRing->SetRelativeLocation(FVector(0.0f, 0.0f, -94.0f));
	BaseEmitterRing->SetRelativeScale3D(FVector(0.85f, 0.85f, 0.02f));
}

void AFPSFermiDecoyActor::BeginPlay()
{
	Super::BeginPlay();

	if (HologramMesh)
	{
		if (!HologramMesh->GetSkeletalMeshAsset())
		{
			if (USkeletalMesh* LoadedMesh = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Characters/BaseMale/SK_BaseMale.SK_BaseMale")))
			{
				HologramMesh->SetSkeletalMesh(LoadedMesh);
			}
		}

		if (!AnimSprint)
		{
			AnimSprint = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Characters/BaseMale/Animations/Anim_BaseMale_Sprint.Anim_BaseMale_Sprint"));
		}
		if (!AnimRun)
		{
			AnimRun = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Characters/BaseMale/Animations/Anim_BaseMale_Run.Anim_BaseMale_Run"));
		}

		UAnimSequence* ActiveAnim = AnimRun ? AnimRun : AnimSprint;
		if (ActiveAnim)
		{
			HologramMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
			HologramMesh->SetAnimation(ActiveAnim);
			HologramMesh->SetPlayRate(1.25f);
			HologramMesh->Play(true);
		}

		// Configure glowing phosphor-green / cyan holographic look on SK_BaseMale
		if (!HologramMesh->GetMaterial(0))
		{
			if (UMaterialInterface* BaseBodyMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Characters/BaseMale/Materials/M_Operator_BaseBody.M_Operator_BaseBody")))
			{
				HologramMesh->SetMaterial(0, BaseBodyMat);
			}
		}
		UMaterialInstanceDynamic* HoloMID = HologramMesh->CreateDynamicMaterialInstance(0);
		if (HoloMID)
		{
			HoloMID->SetVectorParameterValue(TEXT("CamoColorPrimary"), FLinearColor(0.0f, 0.95f, 0.72f, 1.0f));
			HoloMID->SetVectorParameterValue(TEXT("CamoColorSecondary"), FLinearColor(0.0f, 0.45f, 0.38f, 1.0f));
			HoloMID->SetVectorParameterValue(TEXT("CamoColorTertiary"), FLinearColor(0.15f, 1.0f, 0.88f, 1.0f));
			HoloMID->SetVectorParameterValue(TEXT("TorsoColor"), FLinearColor(0.0f, 0.82f, 0.65f, 1.0f));
			HoloMID->SetVectorParameterValue(TEXT("SkinTone"), FLinearColor(0.05f, 0.92f, 0.78f, 1.0f));
			HoloMID->SetVectorParameterValue(TEXT("GearColor"), FLinearColor(0.0f, 0.60f, 0.52f, 1.0f));
			HoloMID->SetScalarParameterValue(TEXT("RoughnessBase"), 0.25f);
		}
	}

	if (HologramAK47Mesh)
	{
		if (!HologramAK47Mesh->GetStaticMesh())
		{
			if (UStaticMesh* LoadedAK = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Characters/BaseMale/SM_AK47.SM_AK47")))
			{
				HologramAK47Mesh->SetStaticMesh(LoadedAK);
			}
		}
		HologramAK47Mesh->AttachToComponent(HologramMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("weapon_r"));
		HologramAK47Mesh->SetRelativeLocation(FVector::ZeroVector);
		HologramAK47Mesh->SetRelativeRotation(FRotator::ZeroRotator);
	}

	if (BaseEmitterRing)
	{
		if (UMaterialInstanceDynamic* RingMID = BaseEmitterRing->CreateDynamicMaterialInstance(0))
		{
			RingMID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.0f, 1.2f, 0.95f, 1.0f));
		}
	}
}

void AFPSFermiDecoyActor::InitializeDecoy(const FVector& MoveDirection, EFPSPlayerTeam InSourceTeam)
{
	SprintDirection = MoveDirection.GetSafeNormal2D();
	if (SprintDirection.IsNearlyZero())
	{
		SprintDirection = GetActorForwardVector().GetSafeNormal2D();
	}
	SetActorRotation(SprintDirection.Rotation());
	SourceTeam = InSourceTeam;
}

void AFPSFermiDecoyActor::SpawnSecondaryTimeBubble()
{
	UWorld* World = GetWorld();
	if (!World) return;

	if (SecondaryBubble && IsValid(SecondaryBubble))
	{
		SecondaryBubble->Destroy();
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SecondaryBubble = World->SpawnActor<AFPSFermiTimeBubble>(AFPSFermiTimeBubble::StaticClass(), GetActorLocation(), FRotator::ZeroRotator, SpawnParams);
	if (SecondaryBubble)
	{
		SecondaryBubble->AttachToActor(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	}
}

void AFPSFermiDecoyActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	ElapsedTime += DeltaTime;
	if (ElapsedTime >= DecoyDuration)
	{
		Destroy();
		return;
	}

	UWorld* World = GetWorld();
	if (!World) return;

	// 1. Forward obstacle avoidance / wall deflection so decoy keeps sprinting down lanes
	const FVector CurrentLoc = GetActorLocation();
	FVector StepOffset = SprintDirection * (DecoySprintSpeed * DeltaTime);
	FHitResult WallHit;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	if (GetOwner()) QueryParams.AddIgnoredActor(GetOwner());

	if (World->LineTraceSingleByChannel(WallHit, CurrentLoc, CurrentLoc + SprintDirection * 75.0f, ECC_Visibility, QueryParams))
	{
		// Deflect along wall surface tangent
		SprintDirection = FVector::VectorPlaneProject(SprintDirection, WallHit.ImpactNormal).GetSafeNormal2D();
		if (SprintDirection.IsNearlyZero())
		{
			SprintDirection = (-WallHit.ImpactNormal).GetSafeNormal2D();
		}
		SetActorRotation(SprintDirection.Rotation());
		StepOffset = SprintDirection * (DecoySprintSpeed * DeltaTime);
	}

	FVector NextLoc = CurrentLoc + StepOffset;

	// 2. Ground height snap trace so hologram stays planted on ramps/catwalks/terrain
	FHitResult GroundHit;
	if (World->LineTraceSingleByChannel(GroundHit, NextLoc + FVector(0.0f, 0.0f, 80.0f), NextLoc - FVector(0.0f, 0.0f, 220.0f), ECC_Visibility, QueryParams))
	{
		NextLoc.Z = GroundHit.ImpactPoint.Z + 96.0f;
	}

	SetActorLocation(NextLoc);

	// 3. Periodic Acoustic Gunfire Bursts (pings UFPSRadarSubsystem + spawns holographic tracer)
	FireBurstTimer += DeltaTime;
	if (FireBurstTimer >= SimulatedFireInterval)
	{
		FireBurstTimer = 0.0f;
		FireAcousticBurst();
	}
}

void AFPSFermiDecoyActor::FireAcousticBurst()
{
	UWorld* World = GetWorld();
	if (!World) return;

	if (UFPSRadarSubsystem* Radar = World->GetSubsystem<UFPSRadarSubsystem>())
	{
		// Report unsuppressed gunfire on Bravo/Enemy channel as well so both enemy teams and local HUD radar show the decoy's acoustic signature
		Radar->ReportGunfire(this, GetActorLocation(), EFPSPlayerTeam::Bravo, false);
		if (SourceTeam != EFPSPlayerTeam::Bravo)
		{
			Radar->ReportGunfire(this, GetActorLocation(), SourceTeam, false);
		}
	}

	// Spawn a visible holographic tracer streak downrange
	const FVector MuzzleStart = GetActorLocation() + SprintDirection * 55.0f + FVector(0.0f, 0.0f, 38.0f);
	const FVector SpreadDir = FMath::VRandCone(SprintDirection, FMath::DegreesToRadians(2.5f));
	const FVector TraceEnd = MuzzleStart + SpreadDir * 2400.0f;

	AFPSBallisticTracer* Tracer = World->SpawnActor<AFPSBallisticTracer>(AFPSBallisticTracer::StaticClass(), MuzzleStart, SpreadDir.Rotation());
	if (Tracer)
	{
		const bool bDilated = AFPSFermiTimeBubble::IsLocationInsideAnyBubble(World, MuzzleStart);
		Tracer->InitializeTracer(MuzzleStart, TraceEnd, bDilated, true);
	}
}

void AFPSFermiDecoyActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (SecondaryBubble && IsValid(SecondaryBubble))
	{
		SecondaryBubble->Destroy();
		SecondaryBubble = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}
