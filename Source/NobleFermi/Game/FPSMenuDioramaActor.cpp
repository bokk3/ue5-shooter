#include "Game/FPSMenuDioramaActor.h"
#include "Components/SpotLightComponent.h"

AFPSMenuDioramaActor::AFPSMenuDioramaActor()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	MainCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("MainCamera"));
	MainCamera->SetupAttachment(RootComponent);
	MainCamera->SetRelativeLocation(FVector(-350.0f, 0.0f, 120.0f));
	MainCamera->SetRelativeRotation(FRotator(-5.0f, 0.0f, 0.0f));

	LoadoutCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("LoadoutCamera"));
	LoadoutCamera->SetupAttachment(RootComponent);
	LoadoutCamera->SetRelativeLocation(FVector(-120.0f, 60.0f, 100.0f));
	LoadoutCamera->SetRelativeRotation(FRotator(0.0f, -25.0f, 0.0f));

	SettingsCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("SettingsCamera"));
	SettingsCamera->SetupAttachment(RootComponent);
	SettingsCamera->SetRelativeLocation(FVector(-400.0f, -150.0f, 140.0f));
	SettingsCamera->SetRelativeRotation(FRotator(-10.0f, 15.0f, 0.0f));

	WeaponTurntableRoot = CreateDefaultSubobject<USceneComponent>(TEXT("WeaponTurntableRoot"));
	WeaponTurntableRoot->SetupAttachment(RootComponent);
	WeaponTurntableRoot->SetRelativeLocation(FVector(0.0f, 0.0f, 90.0f));

	WeaponKeyLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("WeaponKeyLight"));
	WeaponKeyLight->SetupAttachment(WeaponTurntableRoot);
	WeaponKeyLight->SetRelativeLocation(FVector(-80.0f, 0.0f, 100.0f));
	WeaponKeyLight->SetIntensity(5000.0f);
	WeaponKeyLight->SetLightColor(FLinearColor(0.85f, 0.95f, 1.0f));
}

void AFPSMenuDioramaActor::BeginPlay()
{
	Super::BeginPlay();
}

void AFPSMenuDioramaActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bAutoRotateWeapon && WeaponTurntableRoot)
	{
		WeaponTurntableRoot->AddLocalRotation(FRotator(0.0f, TurntableRotationSpeed * DeltaTime, 0.0f));
	}
}

void AFPSMenuDioramaActor::AddWeaponManualYaw(float DeltaYaw)
{
	if (WeaponTurntableRoot)
	{
		WeaponTurntableRoot->AddLocalRotation(FRotator(0.0f, DeltaYaw, 0.0f));
	}
}

void AFPSMenuDioramaActor::SetLightingMood(bool bFocusOnWeapon)
{
	if (WeaponKeyLight)
	{
		WeaponKeyLight->SetIntensity(bFocusOnWeapon ? 8000.0f : 2000.0f);
	}
}
