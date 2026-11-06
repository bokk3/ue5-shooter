#include "Character/FPSBreathVaporComponent.h"
#include "ProceduralMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/Texture2D.h"

UFPSBreathVaporComponent::UFPSBreathVaporComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bTickEvenWhenPaused = false;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;

	bColdEnvironment = true;
	ExertionMultiplier = 1.0f;
	BreathCyclePhase = 0.0f;
	LastPuffSpawnTime = 0.0f;
	PuffsEmittedInCurrentBreath = 0;
}

void UFPSBreathVaporComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UFPSBreathVaporComponent::InitializeBreathVapor(USkeletalMeshComponent* InBodyMesh)
{
	TargetBodyMesh = InBodyMesh;
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor) return;

	if (!VaporMeshComp)
	{
		VaporMeshComp = NewObject<UProceduralMeshComponent>(OwnerActor, TEXT("VaporMeshComp"));
		if (VaporMeshComp)
		{
			VaporMeshComp->SetMobility(EComponentMobility::Movable);
			VaporMeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			VaporMeshComp->SetCastShadow(false);
			VaporMeshComp->bCastDynamicShadow = false;

			USceneComponent* AttachTarget = InBodyMesh ? static_cast<USceneComponent*>(InBodyMesh) : OwnerActor->GetRootComponent();
			if (AttachTarget)
			{
				VaporMeshComp->AttachToComponent(AttachTarget, FAttachmentTransformRules::KeepWorldTransform);
			}

			VaporMeshComp->RegisterComponent();

			// Load Cooked Translucent / Additive Vapor Material (fallback to M_Soldier_Visor if M_BreathVapor lacks cooked shaders)
			UMaterialInterface* VaporMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Characters/BaseMale/Materials/M_BreathVapor.M_BreathVapor"));
			if (!VaporMat || VaporMat->GetMaterial() == UMaterial::GetDefaultMaterial(MD_Surface))
			{
				VaporMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Characters/BaseMale/Materials/M_Soldier_Visor.M_Soldier_Visor"));
			}
			if (!VaporMat)
			{
				VaporMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
			}

			if (VaporMat)
			{
				VaporMID = VaporMeshComp->CreateDynamicMaterialInstance(0, VaporMat);
				if (VaporMID)
				{
					VaporMID->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.90f, 0.96f, 1.0f, 0.5f));
					VaporMID->SetVectorParameterValue(TEXT("EmissiveColor"), FLinearColor(0.25f, 0.35f, 0.45f, 0.5f));
					VaporMID->SetVectorParameterValue(TEXT("VaporColor"), FLinearColor(0.88f, 0.94f, 1.0f, 1.0f));
					VaporMID->SetScalarParameterValue(TEXT("VaporDensity"), 0.65f);
					VaporMID->SetScalarParameterValue(TEXT("Opacity"), 0.55f);
					VaporMeshComp->SetMaterial(0, VaporMID);
				}
			}
		}
	}
}

FVector UFPSBreathVaporComponent::ComputeMouthWorldLocation(FVector& OutHeadFwd, FVector& OutHeadUp) const
{
	if (!TargetBodyMesh)
	{
		OutHeadFwd = FVector::ForwardVector;
		OutHeadUp = FVector::UpVector;
		return GetOwner() ? GetOwner()->GetActorLocation() + FVector(0.0f, 0.0f, 165.0f) : FVector::ZeroVector;
	}

	const FTransform MeshTransform = TargetBodyMesh->GetComponentTransform();
	// In SK_BaseMale / Mixamo Vanguard, character faces +Y in component space:
	OutHeadFwd = MeshTransform.GetUnitAxis(EAxis::Y);
	OutHeadUp  = MeshTransform.GetUnitAxis(EAxis::Z);

	const FVector HeadPos = TargetBodyMesh->GetSocketLocation(FName(TEXT("head")));
	const FVector BasePos = HeadPos.IsNearlyZero()
		? (MeshTransform.GetLocation() + OutHeadUp * 162.0f)
		: HeadPos;

	// Visor / mouth respirator grill sits approx +13.0 cm forward and +2.5 cm up from skull center
	return BasePos + OutHeadFwd * 13.0f + OutHeadUp * 2.5f;
}

void UFPSBreathVaporComponent::TriggerPuff()
{
	FVector HeadFwd, HeadUp;
	const FVector MouthLoc = ComputeMouthWorldLocation(HeadFwd, HeadUp);
	const FVector HeadRight = FVector::CrossProduct(HeadUp, HeadFwd).GetSafeNormal();

	FFPSVaporPuff Puff;
	Puff.Position = MouthLoc + HeadRight * FMath::FRandRange(-0.3f, 0.3f) + HeadUp * FMath::FRandRange(-0.2f, 0.2f);
	// Authentic forward breath propulsion with gentle buoyancy
	Puff.Velocity =
		HeadFwd   * FMath::FRandRange(28.0f, 40.0f) +
		HeadUp    * FMath::FRandRange(4.0f, 8.5f) +
		HeadRight * FMath::FRandRange(-2.2f, 2.2f);

	Puff.Age = 0.0f;
	Puff.MaxLifetime = FMath::FRandRange(1.10f, 1.45f);
	Puff.InitialRadius = FMath::FRandRange(1.8f, 2.4f);
	Puff.MaxRadius = FMath::FRandRange(6.5f, 9.2f);
	Puff.MaxOpacity = FMath::FRandRange(0.35f, 0.48f);
	Puff.RotationDeg = FMath::FRandRange(0.0f, 360.0f);
	Puff.RotSpeed = FMath::FRandRange(-35.0f, 35.0f);

	ActivePuffs.Add(Puff);
}

void UFPSBreathVaporComponent::UpdateBreathingAndSpawnPuffs(float DeltaTime)
{
	if (!bColdEnvironment)
	{
		return;
	}

	// 0.32 Hz breathing base = ~3.125s per full breath cycle
	const float BaseFreq = 0.32f * ExertionMultiplier;
	BreathCyclePhase += DeltaTime * BaseFreq * 2.0f * PI;

	if (BreathCyclePhase >= 2.0f * PI)
	{
		BreathCyclePhase = FMath::Fmod(BreathCyclePhase, 2.0f * PI);
		PuffsEmittedInCurrentBreath = 0;
	}

	// Exhalation occurs during phase [PI .. 1.85 * PI] (approx 1.56s to 2.89s of the cycle)
	const float ExhaleStart = PI;
	const float ExhaleEnd   = 1.85f * PI;

	if (BreathCyclePhase >= ExhaleStart && BreathCyclePhase <= ExhaleEnd)
	{
		// Emit 3 micro-puffs spaced across the exhalation window for a delicate billowing mist
		const float ExhaleProgress = (BreathCyclePhase - ExhaleStart) / (ExhaleEnd - ExhaleStart);
		const int32 DesiredPuffs = FMath::Clamp(FMath::FloorToInt(ExhaleProgress * 3.0f) + 1, 0, 3);

		if (DesiredPuffs > PuffsEmittedInCurrentBreath)
		{
			TriggerPuff();
			PuffsEmittedInCurrentBreath = DesiredPuffs;
		}
	}
}

void UFPSBreathVaporComponent::UpdatePuffPhysics(float DeltaTime)
{
	for (int32 i = ActivePuffs.Num() - 1; i >= 0; --i)
	{
		FFPSVaporPuff& Puff = ActivePuffs[i];
		Puff.Age += DeltaTime;

		if (Puff.Age >= Puff.MaxLifetime)
		{
			ActivePuffs.RemoveAtSwap(i, EAllowShrinking::No);
			continue;
		}

		// Advection: Position along velocity
		Puff.Position += Puff.Velocity * DeltaTime;

		// Gentle thermal buoyancy: warm condensing breath rises in cold ambient air
		Puff.Velocity.Z += 7.0f * DeltaTime;

		// Aerodynamic drag: air resistance decelerates forward momentum
		Puff.Velocity *= FMath::Max(0.0f, 1.0f - 1.25f * DeltaTime);

		// Turbulent micro-spin
		Puff.RotationDeg += Puff.RotSpeed * DeltaTime;
	}
}

void UFPSBreathVaporComponent::RenderVaporMesh()
{
	if (!VaporMeshComp)
	{
		return;
	}

	if (ActivePuffs.Num() == 0)
	{
		VaporMeshComp->ClearMeshSection(0);
		return;
	}

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FColor> Colors;
	TArray<FProcMeshTangent> Tangents;

	// View normal / Billboard axes
	APlayerCameraManager* CamMgr = UGameplayStatics::GetPlayerCameraManager(this, 0);
	const FVector CamLoc = CamMgr ? CamMgr->GetCameraLocation() : (VaporMeshComp->GetComponentLocation() + FVector(435.0f, 0.0f, 96.0f));

	for (const FFPSVaporPuff& Puff : ActivePuffs)
	{
		const float NormAge = FMath::Clamp(Puff.Age / Puff.MaxLifetime, 0.0f, 1.0f);

		// Billowy expansion as moisture vapor spreads into cold air
		const float CurrentRadius = FMath::Lerp(Puff.InitialRadius, Puff.MaxRadius, FMath::Pow(NormAge, 0.48f));

		// Condensation curve: quick fog-in (0.0 to 0.18), peak saturation, smooth evaporation fade (0.18 to 1.0)
		float OpacityAlpha = 0.0f;
		if (NormAge < 0.18f)
		{
			OpacityAlpha = (NormAge / 0.18f) * Puff.MaxOpacity;
		}
		else
		{
			const float Fade = 1.0f - (NormAge - 0.18f) / 0.82f;
			OpacityAlpha = FMath::Max(0.0f, Fade * Fade * Puff.MaxOpacity);
		}

		const uint8 AlphaByte = static_cast<uint8>(FMath::Clamp(OpacityAlpha * 255.0f, 0.0f, 255.0f));
		const FColor PuffColor(230, 242, 255, AlphaByte);

		// Camera-facing basis vectors
		FVector ToCam = (CamLoc - Puff.Position).GetSafeNormal();
		if (ToCam.IsNearlyZero()) ToCam = FVector::ForwardVector;

		FVector Right = FVector::CrossProduct(FVector::UpVector, ToCam).GetSafeNormal();
		if (Right.IsNearlyZero()) Right = FVector::RightVector;
		FVector Up = FVector::CrossProduct(ToCam, Right).GetSafeNormal();

		// Cross-billboard representation (2 intersecting quads per puff) for full 3D volumetric depth
		const float Angles[2] = { 0.0f, 90.0f };
		for (int32 q = 0; q < 2; ++q)
		{
			const float AngleRad = FMath::DegreesToRadians(Puff.RotationDeg + Angles[q]);
			const float CosA = FMath::Cos(AngleRad);
			const float SinA = FMath::Sin(AngleRad);

			const FVector QRight = (Right * CosA - Up * SinA) * CurrentRadius;
			const FVector QUp    = (Right * SinA + Up * CosA) * CurrentRadius;

			const int32 BaseIdx = Vertices.Num();

			Vertices.Add(Puff.Position - QRight - QUp); // 0: Bottom-Left
			Vertices.Add(Puff.Position + QRight - QUp); // 1: Bottom-Right
			Vertices.Add(Puff.Position + QRight + QUp); // 2: Top-Right
			Vertices.Add(Puff.Position - QRight + QUp); // 3: Top-Left

			UVs.Add(FVector2D(0.0f, 1.0f));
			UVs.Add(FVector2D(1.0f, 1.0f));
			UVs.Add(FVector2D(1.0f, 0.0f));
			UVs.Add(FVector2D(0.0f, 0.0f));

			for (int32 k = 0; k < 4; ++k)
			{
				Normals.Add(ToCam);
				Colors.Add(PuffColor);
				Tangents.Add(FProcMeshTangent(QRight.GetSafeNormal(), false));
			}

			Triangles.Add(BaseIdx + 0);
			Triangles.Add(BaseIdx + 2);
			Triangles.Add(BaseIdx + 1);
			Triangles.Add(BaseIdx + 0);
			Triangles.Add(BaseIdx + 3);
			Triangles.Add(BaseIdx + 2);
		}
	}

	VaporMeshComp->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, Colors, Tangents, false);
}

void UFPSBreathVaporComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateBreathingAndSpawnPuffs(DeltaTime);
	UpdatePuffPhysics(DeltaTime);
	RenderVaporMesh();
}
