#include "Combat/FPSProjectileBase.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "TimerManager.h"
#include "Combat/FPSHealthComponent.h"

AFPSProjectileBase::AFPSProjectileBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->InitSphereRadius(8.0f);
	CollisionSphere->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	RootComponent = CollisionSphere;

	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	ProjectileMesh->SetupAttachment(CollisionSphere);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = CollisionSphere;
	ProjectileMovement->InitialSpeed = 1600.0f;
	ProjectileMovement->MaxSpeed = 2200.0f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = true;
	ProjectileMovement->Bounciness = 0.35f;
	ProjectileMovement->Friction = 0.6f;
}

void AFPSProjectileBase::BeginPlay()
{
	Super::BeginPlay();

	CollisionSphere->OnComponentHit.AddDynamic(this, &AFPSProjectileBase::OnProjectileHit);

	if (HasAuthority() && FuseTimeRemaining > 0.0f)
	{
		GetWorldTimerManager().SetTimer(FuseTimerHandle, this, &AFPSProjectileBase::Detonate, FuseTimeRemaining, false);
	}
}

void AFPSProjectileBase::InitializeFuse(float CustomFuseTime, AController* InstigatingController)
{
	SetInstigator(InstigatingController ? InstigatingController->GetPawn() : nullptr);
	FuseTimeRemaining = CustomFuseTime;

	if (HasAuthority())
	{
		GetWorldTimerManager().ClearTimer(FuseTimerHandle);
		GetWorldTimerManager().SetTimer(FuseTimerHandle, this, &AFPSProjectileBase::Detonate, FuseTimeRemaining, false);

		if (ProjectileType == EFPSProjectileType::Semtex)
		{
			GetWorldTimerManager().SetTimer(SemtexBeepTimerHandle, this, &AFPSProjectileBase::PlaySemtexBeep, 0.4f, true);
		}
	}
}

void AFPSProjectileBase::OnProjectileHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (ProjectileType == EFPSProjectileType::Semtex && !bHasStuck)
	{
		bHasStuck = true;
		ProjectileMovement->StopMovementImmediately();
		ProjectileMovement->ProjectileGravityScale = 0.0f;
		ProjectileMovement->Deactivate();

		if (OtherComp)
		{
			AttachToComponent(OtherComp, FAttachmentTransformRules::KeepWorldTransform);
		}
	}
	else if (ProjectileType == EFPSProjectileType::ThrowingKnife)
	{
		if (OtherActor && OtherActor != GetInstigator())
		{
			UFPSHealthComponent* HealthComp = OtherActor->FindComponentByClass<UFPSHealthComponent>();
			if (HealthComp)
			{
				HealthComp->ApplyDamage(200.0f, GetInstigator(), Hit);
			}
		}
		Detonate();
	}
}

void AFPSProjectileBase::PlaySemtexBeep()
{
	if (SemtexBeepSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, SemtexBeepSound, GetActorLocation(), 1.0f, 1.2f);
	}
}

void AFPSProjectileBase::ProcessFlashbangConcussion()
{
	UWorld* World = GetWorld();
	if (!World) return;

	FVector FlashLocation = GetActorLocation();
	TArray<FOverlapResult> Overlaps;
	FCollisionShape QueryShape = FCollisionShape::MakeSphere(OuterDamageRadius);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	if (World->OverlapMultiByChannel(Overlaps, FlashLocation, FQuat::Identity, ECC_Pawn, QueryShape, QueryParams))
	{
		for (const FOverlapResult& Overlap : Overlaps)
		{
			AActor* Victim = Overlap.GetActor();
			if (!Victim || Victim == this) continue;

			// Check line of sight from flash to victim
			FHitResult LosHit;
			FCollisionQueryParams TraceParams;
			TraceParams.AddIgnoredActor(this);
			TraceParams.AddIgnoredActor(Victim);

			FVector VictimEyes = Victim->GetActorLocation() + FVector(0, 0, 50.0f);
			bool bBlocked = World->LineTraceSingleByChannel(LosHit, FlashLocation, VictimEyes, ECC_Visibility, TraceParams);

			if (!bBlocked)
			{
				// Compute angle between victim look direction and flash
				FVector VictimForward = Victim->GetActorForwardVector();
				FVector DirToFlash = (FlashLocation - VictimEyes).GetSafeNormal();
				float FacingDot = FVector::DotProduct(VictimForward, DirToFlash);

				// Facing flash (Dot > 0.2): full blind & stun
				// Facing away (Dot <= 0.2): partial concussion
				float StunDuration = (FacingDot > 0.2f) ? 3.5f : 1.2f;
				UE_LOG(LogTemp, Log, TEXT("[Flashbang] Victim %s stunned for %.1fs (Facing Dot: %.2f)"), *Victim->GetName(), StunDuration, FacingDot);
			}
		}
	}
}

void AFPSProjectileBase::Detonate()
{
	if (bHasDetonated) return;
	bHasDetonated = true;

	GetWorldTimerManager().ClearTimer(FuseTimerHandle);
	GetWorldTimerManager().ClearTimer(SemtexBeepTimerHandle);

	if (DetonationSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, DetonationSound, GetActorLocation());
	}

	if (HasAuthority())
	{
		if (ProjectileType == EFPSProjectileType::Flashbang)
		{
			ProcessFlashbangConcussion();
		}
		else if (ProjectileType != EFPSProjectileType::ThrowingKnife)
		{
			TArray<AActor*> IgnoreActors;
			IgnoreActors.Add(this);

			UGameplayStatics::ApplyRadialDamageWithFalloff(
				this,
				BaseExplosionDamage,
				MinimumDamage,
				GetActorLocation(),
				InnerDamageRadius,
				OuterDamageRadius,
				1.0f,
				nullptr,
				IgnoreActors,
				this,
				GetInstigatorController(),
				ECC_Visibility
			);
		}
	}

	OnDetonated.Broadcast(GetActorLocation());
	Destroy();
}
