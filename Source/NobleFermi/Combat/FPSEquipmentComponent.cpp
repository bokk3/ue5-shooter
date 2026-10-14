#include "Combat/FPSEquipmentComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/ProjectileMovementComponent.h"

UFPSEquipmentComponent::UFPSEquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void UFPSEquipmentComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UFPSEquipmentComponent, CurrentLethalCount);
	DOREPLIFETIME(UFPSEquipmentComponent, CurrentTacticalCount);
}

void UFPSEquipmentComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UFPSEquipmentComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bIsCookingFrag)
	{
		FragCookElapsed += DeltaTime;
		OnFragCookingTick.Broadcast(FMath::Max(0.0f, MaxFragFuseTime - FragCookElapsed));

		if (FragCookElapsed >= MaxFragFuseTime)
		{
			DetonateFragInHand();
		}
	}
}

void UFPSEquipmentComponent::StartCookingLethal()
{
	if (CurrentLethalCount <= 0) return;

	if (EquippedLethal == EFPSProjectileType::FragGrenade)
	{
		bIsCookingFrag = true;
		FragCookElapsed = 0.0f;
	}
}

void UFPSEquipmentComponent::ReleaseAndThrowLethal()
{
	if (CurrentLethalCount <= 0) return;

	float CustomFuse = MaxFragFuseTime;
	if (bIsCookingFrag)
	{
		CustomFuse = FMath::Max(0.35f, MaxFragFuseTime - FragCookElapsed);
		bIsCookingFrag = false;
		FragCookElapsed = 0.0f;
	}
	else if (EquippedLethal == EFPSProjectileType::Semtex)
	{
		CustomFuse = 3.0f;
	}

	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character) return;

	APlayerController* PC = Cast<APlayerController>(Character->GetController());
	if (!PC || !PC->PlayerCameraManager) return;

	FVector CameraLoc = PC->PlayerCameraManager->GetCameraLocation();
	FRotator CameraRot = PC->PlayerCameraManager->GetCameraRotation();
	FVector SpawnLoc = CameraLoc + (CameraRot.Vector() * 50.0f);
	FVector LaunchVel = (CameraRot.Vector() * ThrowImpulseSpeed) + (Character->GetVelocity() * 0.4f);

	if (GetOwner()->HasAuthority())
	{
		Server_ThrowLethal_Implementation(SpawnLoc, LaunchVel, CustomFuse);
	}
	else
	{
		Server_ThrowLethal(SpawnLoc, LaunchVel, CustomFuse);
	}
}

void UFPSEquipmentComponent::ThrowTactical()
{
	if (CurrentTacticalCount <= 0) return;

	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character) return;

	APlayerController* PC = Cast<APlayerController>(Character->GetController());
	if (!PC || !PC->PlayerCameraManager) return;

	FVector CameraLoc = PC->PlayerCameraManager->GetCameraLocation();
	FRotator CameraRot = PC->PlayerCameraManager->GetCameraRotation();
	FVector SpawnLoc = CameraLoc + (CameraRot.Vector() * 50.0f);
	FVector LaunchVel = (CameraRot.Vector() * ThrowImpulseSpeed) + (Character->GetVelocity() * 0.4f);

	if (GetOwner()->HasAuthority())
	{
		Server_ThrowTactical_Implementation(SpawnLoc, LaunchVel);
	}
	else
	{
		Server_ThrowTactical(SpawnLoc, LaunchVel);
	}
}

void UFPSEquipmentComponent::ResupplyEquipment(int32 LethalsToAdd, int32 TacticalsToAdd)
{
	CurrentLethalCount = FMath::Clamp(CurrentLethalCount + LethalsToAdd, 0, 2);
	CurrentTacticalCount = FMath::Clamp(CurrentTacticalCount + TacticalsToAdd, 0, 2);
	OnRep_EquipmentCounts();
}

void UFPSEquipmentComponent::DetonateFragInHand()
{
	bIsCookingFrag = false;
	FragCookElapsed = 0.0f;

	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character || !Character->HasAuthority()) return;

	FVector SpawnLoc = Character->GetActorLocation() + FVector(0, 0, 40.0f);
	Server_ThrowLethal_Implementation(SpawnLoc, FVector::ZeroVector, 0.01f);
}

void UFPSEquipmentComponent::OnRep_EquipmentCounts()
{
	OnEquipmentCountChanged.Broadcast(CurrentLethalCount, CurrentTacticalCount);
}

void UFPSEquipmentComponent::Server_ThrowLethal_Implementation(FVector SpawnLocation, FVector LaunchVelocity, float CustomFuse)
{
	if (CurrentLethalCount <= 0) return;

	UWorld* World = GetWorld();
	if (!World) return;

	ACharacter* Character = Cast<ACharacter>(GetOwner());
	AController* InstigatorController = Character ? Character->GetController() : nullptr;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Character;
	SpawnParams.Instigator = Character;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	TSubclassOf<AFPSProjectileBase> ClassToSpawn = ProjectileClass ? ProjectileClass : TSubclassOf<AFPSProjectileBase>(AFPSProjectileBase::StaticClass());
	AFPSProjectileBase* Projectile = World->SpawnActor<AFPSProjectileBase>(ClassToSpawn, SpawnLocation, LaunchVelocity.Rotation(), SpawnParams);
	if (Projectile)
	{
		Projectile->ProjectileType = EquippedLethal;
		if (Projectile->ProjectileMovement)
		{
			Projectile->ProjectileMovement->Velocity = LaunchVelocity;
		}
		Projectile->InitializeFuse(CustomFuse, InstigatorController);
	}

	CurrentLethalCount = FMath::Max(0, CurrentLethalCount - 1);
	OnRep_EquipmentCounts();
}

bool UFPSEquipmentComponent::Server_ThrowLethal_Validate(FVector SpawnLocation, FVector LaunchVelocity, float CustomFuse)
{
	return true;
}

void UFPSEquipmentComponent::Server_ThrowTactical_Implementation(FVector SpawnLocation, FVector LaunchVelocity)
{
	if (CurrentTacticalCount <= 0) return;

	UWorld* World = GetWorld();
	if (!World) return;

	ACharacter* Character = Cast<ACharacter>(GetOwner());
	AController* InstigatorController = Character ? Character->GetController() : nullptr;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Character;
	SpawnParams.Instigator = Character;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	TSubclassOf<AFPSProjectileBase> ClassToSpawn = ProjectileClass ? ProjectileClass : TSubclassOf<AFPSProjectileBase>(AFPSProjectileBase::StaticClass());
	AFPSProjectileBase* Projectile = World->SpawnActor<AFPSProjectileBase>(ClassToSpawn, SpawnLocation, LaunchVelocity.Rotation(), SpawnParams);
	if (Projectile)
	{
		Projectile->ProjectileType = EquippedTactical;
		if (Projectile->ProjectileMovement)
		{
			Projectile->ProjectileMovement->Velocity = LaunchVelocity;
		}
		Projectile->InitializeFuse(2.2f, InstigatorController);
	}

	CurrentTacticalCount = FMath::Max(0, CurrentTacticalCount - 1);
	OnRep_EquipmentCounts();
}

bool UFPSEquipmentComponent::Server_ThrowTactical_Validate(FVector SpawnLocation, FVector LaunchVelocity)
{
	return true;
}
