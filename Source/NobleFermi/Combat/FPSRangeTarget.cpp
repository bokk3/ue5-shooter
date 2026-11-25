#include "Combat/FPSRangeTarget.h"
#include "Character/FPSCharacter.h"
#include "UI/FPSHUDWidget.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

namespace
{
	// Swing spring: stiffness (1/s^2) and damping ratio per kind. Low damping lets a silhouette rock a few times.
	constexpr float SwingStiffness = 55.0f;
	constexpr float PopperDownAngle = 84.0f;  // lying back on its stand
	constexpr float SwingMaxAngle = 88.0f;    // hits the ground and bounces a little
	constexpr float SwingMinAngle = -25.0f;
	constexpr float ReadoutWorldSize = 22.0f;

	float DampingRatio(EFPSRangeTargetKind Kind)
	{
		switch (Kind)
		{
		case EFPSRangeTargetKind::Popper:     return 0.6f;
		case EFPSRangeTargetKind::Silhouette: return 0.18f;
		default:                              return 0.3f;
		}
	}

	float HitImpulse(EFPSRangeTargetKind Kind, bool bHead)
	{
		switch (Kind)
		{
		case EFPSRangeTargetKind::Popper:     return 260.0f;
		case EFPSRangeTargetKind::Silhouette: return bHead ? 150.0f : 110.0f;
		default:                              return 60.0f;
		}
	}
}

AFPSRangeTarget::AFPSRangeTarget()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicatingMovement(false);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	Hinge = CreateDefaultSubobject<USceneComponent>(TEXT("Hinge"));
	Hinge->SetupAttachment(SceneRoot);

	TargetMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TargetMesh"));
	TargetMesh->SetupAttachment(Hinge);
	TargetMesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	TargetMesh->SetMobility(EComponentMobility::Movable);

	Readout = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Readout"));
	Readout->SetupAttachment(SceneRoot);
	Readout->SetHorizontalAlignment(EHTA_Center);
	Readout->SetVerticalAlignment(EVRTA_TextBottom);
	Readout->SetWorldSize(ReadoutWorldSize);
	Readout->SetTextRenderColor(FColor(255, 220, 80));
	Readout->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Readout->SetText(FText::GetEmpty());
}

void AFPSRangeTarget::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (TargetMeshAsset)
	{
		TargetMesh->SetStaticMesh(TargetMeshAsset);
	}

	// The readout floats just above the target and doesn't swing with it.
	float TopZ = 180.0f;
	if (const UStaticMesh* Mesh = TargetMesh->GetStaticMesh())
	{
		TopZ = Mesh->GetBoundingBox().Max.Z;
	}
	Readout->SetRelativeLocation(FVector(0.0f, 0.0f, TopZ + 12.0f));
	Readout->SetVisibility(bShowReadout);
}

void AFPSRangeTarget::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AFPSRangeTarget, HitSerial);
	DOREPLIFETIME(AFPSRangeTarget, HitCount);
	DOREPLIFETIME(AFPSRangeTarget, bKnockedDown);
	DOREPLIFETIME(AFPSRangeTarget, bLastHitHead);
	DOREPLIFETIME(AFPSRangeTarget, LastHitDistance);
}

float AFPSRangeTarget::HeightFraction(const FVector& ImpactPoint) const
{
	const UStaticMesh* Mesh = TargetMesh->GetStaticMesh();
	if (!Mesh)
	{
		return 0.0f;
	}
	const FBox Box = Mesh->GetBoundingBox();
	const float Height = Box.Max.Z - Box.Min.Z;
	if (Height <= KINDA_SMALL_NUMBER)
	{
		return 0.0f;
	}
	// In the mesh's own space, so a target that is still rocking measures the same.
	const FVector Local = TargetMesh->GetComponentTransform().InverseTransformPosition(ImpactPoint);
	return (Local.Z - Box.Min.Z) / Height;
}

EFPSRangeHitResult AFPSRangeTarget::RegisterHit(const FHitResult& Hit, const AActor* Shooter)
{
	if (bKnockedDown)
	{
		return EFPSRangeHitResult::None;
	}

	const float Height = HeightFraction(Hit.ImpactPoint);
	const bool bHead = Kind == EFPSRangeTargetKind::Silhouette && Height >= 1.0f - HeadZoneFraction;
	const EFPSRangeHitResult Result = (Kind == EFPSRangeTargetKind::Popper) ? EFPSRangeHitResult::KnockDown
		: bHead ? EFPSRangeHitResult::Head : EFPSRangeHitResult::Body;

	if (!HasAuthority())
	{
		return Result;  // the server's replicated state drives the reaction here
	}

	UWorld* World = GetWorld();
	const double Now = World ? World->GetTimeSeconds() : 0.0;
	if (Now - LastHitTime > StringTimeout)
	{
		HitCount = 0;
	}
	LastHitTime = Now;
	++HitCount;
	++HitSerial;
	bLastHitHead = bHead;
	LastHitDistance = Shooter ? FVector::Dist(Shooter->GetActorLocation(), Hit.ImpactPoint) : 0.0f;

	if (Kind == EFPSRangeTargetKind::Popper && World)
	{
		bKnockedDown = true;
		World->GetTimerManager().SetTimer(ResetTimer, this, &AFPSRangeTarget::StandUp, ResetDelay, false);
	}

	UE_LOG(LogTemp, Log, TEXT("[Range] %s hit (%s, %.0f%% up) at %.1f m, %d in this string%s"), *GetName(), bHead ? TEXT("head") : TEXT("body"),
		Height * 100.0f, LastHitDistance / 100.0f, HitCount, bKnockedDown ? TEXT(", down") : TEXT(""));
	PlayHitReaction(bHead);
	return Result;
}

void AFPSRangeTarget::ShowHitMarker(AFPSCharacter* Shooter, EFPSRangeHitResult Result)
{
	if (!Shooter || !Shooter->IsLocallyControlled() || Result == EFPSRangeHitResult::None)
	{
		return;
	}
	if (UFPSHUDWidget* HUD = Shooter->GetActiveHUDWidget())
	{
		// Range hits give a hit marker only: no gold, accolades or kill combo.
		HUD->TriggerHitmarker(Result == EFPSRangeHitResult::KnockDown ? EFPSHitmarkerType::Kill
			: Result == EFPSRangeHitResult::Head ? EFPSHitmarkerType::Headshot : EFPSHitmarkerType::Standard);
	}
}

void AFPSRangeTarget::StandUp()
{
	bKnockedDown = false;
	UpdateReadout();
	SetActorTickEnabled(true);
}

void AFPSRangeTarget::OnRep_HitSerial()
{
	PlayHitReaction(bLastHitHead);
}

void AFPSRangeTarget::OnRep_KnockedDown()
{
	UpdateReadout();
	SetActorTickEnabled(true);
}

void AFPSRangeTarget::PlayHitReaction(bool bHead)
{
	SwingVelocity += HitImpulse(Kind, bHead);
	FlashRemaining = 1.0f;
	UpdateReadout();
	SetActorTickEnabled(true);
}

void AFPSRangeTarget::UpdateReadout()
{
	if (!bShowReadout)
	{
		return;
	}
	const float Metres = LastHitDistance / 100.0f;
	FString Line;
	if (Kind == EFPSRangeTargetKind::Popper && bKnockedDown)
	{
		Line = FString::Printf(TEXT("DOWN  %.1f m"), Metres);
	}
	else if (bLastHitHead)
	{
		Line = FString::Printf(TEXT("HEAD  %.1f m  |  %d"), Metres, HitCount);
	}
	else
	{
		Line = FString::Printf(TEXT("%d HIT%s  %.1f m"), HitCount, HitCount == 1 ? TEXT("") : TEXT("S"), Metres);
	}
	Readout->SetText(FText::FromString(Line));
	Readout->SetTextRenderColor(bLastHitHead ? FColor(255, 90, 60) : FColor(255, 220, 80));
}

void AFPSRangeTarget::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const float Target = bKnockedDown ? PopperDownAngle : 0.0f;
	const float Damping = 2.0f * DampingRatio(Kind) * FMath::Sqrt(SwingStiffness);

	// Semi-implicit Euler in small steps so a long frame doesn't blow up the spring.
	float Remaining = FMath::Min(DeltaTime, 0.1f);
	while (Remaining > 0.0f)
	{
		const float Step = FMath::Min(Remaining, 1.0f / 120.0f);
		Remaining -= Step;
		SwingVelocity += (SwingStiffness * (Target - SwingAngle) - Damping * SwingVelocity) * Step;
		SwingAngle += SwingVelocity * Step;
		if (SwingAngle > SwingMaxAngle)
		{
			SwingAngle = SwingMaxAngle;
			SwingVelocity = -0.25f * SwingVelocity;
		}
		else if (SwingAngle < SwingMinAngle)
		{
			SwingAngle = SwingMinAngle;
			SwingVelocity = -0.25f * SwingVelocity;
		}
	}

	// Positive pitch tips the top towards local -X: back, away from a shooter the target faces.
	Hinge->SetRelativeRotation(FRotator(SwingAngle, 0.0f, 0.0f));

	if (FlashRemaining > 0.0f)
	{
		FlashRemaining = FMath::Max(0.0f, FlashRemaining - DeltaTime * 3.0f);
		Readout->SetWorldSize(ReadoutWorldSize * (1.0f + 0.35f * FlashRemaining));
	}

	const bool bSettled = FMath::Abs(Target - SwingAngle) < 0.05f && FMath::Abs(SwingVelocity) < 0.05f && FlashRemaining <= 0.0f;
	if (bSettled)
	{
		SwingAngle = Target;
		SwingVelocity = 0.0f;
		Hinge->SetRelativeRotation(FRotator(SwingAngle, 0.0f, 0.0f));
		SetActorTickEnabled(false);
	}
}
