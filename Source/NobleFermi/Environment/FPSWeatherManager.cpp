#include "Environment/FPSWeatherManager.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/AudioComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"
#include "UObject/ConstructorHelpers.h"

AFPSWeatherManager::AFPSWeatherManager()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	RainStreaksISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("RainStreaksISM"));
	RainStreaksISM->SetupAttachment(RootComponent);
	RainStreaksISM->SetMobility(EComponentMobility::Movable);
	RainStreaksISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RainStreaksISM->SetCastShadow(false);
	RainStreaksISM->bCastDynamicShadow = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeFinder.Succeeded())
	{
		RainStreaksISM->SetStaticMesh(CubeFinder.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BaseMatFinder(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (BaseMatFinder.Succeeded())
	{
		RainStreaksISM->SetMaterial(0, BaseMatFinder.Object);
	}

	StormAudioComp = CreateDefaultSubobject<UAudioComponent>(TEXT("StormAudioComp"));
	StormAudioComp->SetupAttachment(RootComponent);
	StormAudioComp->bAutoActivate = false;

	WindDirection = FVector(0.65f, 0.45f, -0.2f).GetSafeNormal();
}

void AFPSWeatherManager::BeginPlay()
{
	Super::BeginPlay();

	// Create dynamic material instance with cool translucent rain streak tint
	if (RainStreaksISM && RainStreaksISM->GetMaterial(0))
	{
		UMaterialInstanceDynamic* RainMID = UMaterialInstanceDynamic::Create(RainStreaksISM->GetMaterial(0), this);
		if (RainMID)
		{
			RainMID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.72f, 0.82f, 0.94f, 0.35f));
			RainMID->SetScalarParameterValue(TEXT("Roughness"), 0.05f);
			RainStreaksISM->SetMaterial(0, RainMID);
		}
	}

	InitializeRainInstances();

	if (RainAmbienceSound && StormAudioComp)
	{
		StormAudioComp->SetSound(RainAmbienceSound);
		StormAudioComp->FadeIn(2.0f, RainIntensity);
	}
}

void AFPSWeatherManager::InitializeRainInstances()
{
	if (!RainStreaksISM)
	{
		return;
	}

	RainStreaksISM->ClearInstances();
	RainParticleData.Empty();
	RainParticleData.Reserve(NumRainInstances);

	for (int32 i = 0; i < NumRainInstances; ++i)
	{
		FFPSRainParticleInstance Particle;
		Particle.RelativeOffset = FVector(
			FMath::FRandRange(-RainVolumeExtents.X * 0.5f, RainVolumeExtents.X * 0.8f),
			FMath::FRandRange(-RainVolumeExtents.Y * 0.5f, RainVolumeExtents.Y * 0.5f),
			FMath::FRandRange(-RainVolumeExtents.Z * 0.5f, RainVolumeExtents.Z * 0.5f)
		);
		Particle.FallSpeed = FMath::FRandRange(2400.0f, 3200.0f);
		Particle.LengthScale = FMath::FRandRange(0.85f, 1.35f);

		RainParticleData.Add(Particle);

		FTransform T;
		T.SetLocation(Particle.RelativeOffset);
		T.SetScale3D(FVector(0.0028f, 0.0028f, 0.30f * Particle.LengthScale));
		RainStreaksISM->AddInstance(T);
	}
}

void AFPSWeatherManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	ElapsedTime += DeltaTime;

	if (RainIntensity > 0.01f)
	{
		UpdateRainInstances(DeltaTime);
	}
	else if (RainStreaksISM && RainStreaksISM->GetInstanceCount() > 0)
	{
		RainStreaksISM->ClearInstances();
	}
}

void AFPSWeatherManager::UpdateRainInstances(float DeltaTime)
{
	if (!RainStreaksISM || RainParticleData.Num() == 0)
	{
		return;
	}

	APlayerCameraManager* CamMgr = UGameplayStatics::GetPlayerCameraManager(this, 0);
	const FVector CamLoc = CamMgr ? CamMgr->GetCameraLocation() : GetActorLocation();
	const FRotator CamRot = CamMgr ? CamMgr->GetCameraRotation() : GetActorRotation();

	const FVector WindVelocity = WindDirection * WindSpeed * RainIntensity;
	const FVector FallVelocity = FVector(0.0f, 0.0f, -1.0f);

	// Compute wind tilt rotation for streaks
	const FVector CombinedVelocity = (FallVelocity * 2800.0f) + WindVelocity;
	const FRotator StreakRotation = CombinedVelocity.Rotation() + FRotator(-90.0f, 0.0f, 0.0f);

	const float HalfX = RainVolumeExtents.X * 0.5f;
	const float HalfY = RainVolumeExtents.Y * 0.5f;
	const float HalfZ = RainVolumeExtents.Z * 0.5f;

	for (int32 i = 0; i < RainParticleData.Num(); ++i)
	{
		FFPSRainParticleInstance& P = RainParticleData[i];

		// Integrate velocity
		P.RelativeOffset.Z -= P.FallSpeed * DeltaTime;
		P.RelativeOffset += WindVelocity * DeltaTime;

		// Wrap Z boundaries
		if (P.RelativeOffset.Z < -HalfZ)
		{
			P.RelativeOffset.Z += RainVolumeExtents.Z;
			P.RelativeOffset.X = FMath::FRandRange(-HalfX, HalfX);
			P.RelativeOffset.Y = FMath::FRandRange(-HalfY, HalfY);
		}

		// Wrap lateral boundaries
		if (P.RelativeOffset.X < -HalfX) P.RelativeOffset.X += RainVolumeExtents.X;
		if (P.RelativeOffset.X > HalfX) P.RelativeOffset.X -= RainVolumeExtents.X;
		if (P.RelativeOffset.Y < -HalfY) P.RelativeOffset.Y += RainVolumeExtents.Y;
		if (P.RelativeOffset.Y > HalfY) P.RelativeOffset.Y -= RainVolumeExtents.Y;

		// Transform relative to camera with forward bias
		const FVector ForwardBias = CamRot.Vector() * 350.0f;
		const FVector WorldPos = CamLoc + ForwardBias + P.RelativeOffset;

		FTransform T;
		T.SetLocation(WorldPos);
		T.SetRotation(StreakRotation.Quaternion());
		T.SetScale3D(FVector(0.0028f, 0.0028f, 0.32f * P.LengthScale * RainIntensity));

		RainStreaksISM->UpdateInstanceTransform(i, T, false, false, false);
	}

	RainStreaksISM->MarkRenderStateDirty();
}

void AFPSWeatherManager::SetWeatherPreset(EFPSWeatherState NewState)
{
	CurrentWeatherState = NewState;

	switch (NewState)
	{
	case EFPSWeatherState::NorthSeaStorm:
		RainIntensity = 0.88f;
		GlobalWetness = 0.95f;
		WindSpeed = 920.0f;
		WindDirection = FVector(0.707f, 0.5f, -0.15f).GetSafeNormal();
		break;

	case EFPSWeatherState::TwilightRain:
		RainIntensity = 0.60f;
		GlobalWetness = 0.80f;
		WindSpeed = 450.0f;
		WindDirection = FVector(0.4f, 0.8f, -0.1f).GetSafeNormal();
		break;

	case EFPSWeatherState::HangarNightHaze:
		RainIntensity = 0.0f;
		GlobalWetness = 0.85f; // Wet polished hangar floor
		WindSpeed = 0.0f;
		break;

	case EFPSWeatherState::ClearDaylight:
	default:
		RainIntensity = 0.0f;
		GlobalWetness = 0.0f;
		WindSpeed = 200.0f;
		break;
	}

	if (StormAudioComp)
	{
		if (RainIntensity > 0.05f)
		{
			StormAudioComp->FadeIn(1.5f, RainIntensity);
		}
		else
		{
			StormAudioComp->FadeOut(1.5f, 0.0f);
		}
	}
}
