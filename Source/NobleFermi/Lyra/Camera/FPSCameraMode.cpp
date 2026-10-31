#include "Lyra/Camera/FPSCameraMode.h"
#include "GameFramework/Character.h"

void FFPSCameraModeView::Blend(const FFPSCameraModeView& Other, float OtherWeight)
{
	OtherWeight = FMath::Clamp(OtherWeight, 0.0f, 1.0f);
	Location = FMath::Lerp(Location, Other.Location, OtherWeight);
	Rotation = FMath::Lerp(Rotation, Other.Rotation, OtherWeight);
	ControlRotation = FMath::Lerp(ControlRotation, Other.ControlRotation, OtherWeight);
	FieldOfView = FMath::Lerp(FieldOfView, Other.FieldOfView, OtherWeight);
}

UFPSCameraMode::UFPSCameraMode()
{
}

void UFPSCameraMode::UpdateCameraMode(float DeltaTime, AActor* TargetActor, FFPSCameraModeView& OutCameraModeView)
{
	if (!TargetActor) return;

	FRotator ViewRot = TargetActor->GetActorRotation();
	if (APawn* TargetPawn = Cast<APawn>(TargetActor))
	{
		ViewRot = TargetPawn->GetViewRotation();
	}

	FVector PivotLoc = TargetActor->GetActorLocation() + FVector(0.0f, 0.0f, 64.0f);
	FVector LocalOffset = ViewRot.RotateVector(PivotOffset);
	FVector ArmOffset = -ViewRot.Vector() * TargetArmLength;

	OutCameraModeView.Location = PivotLoc + LocalOffset + ArmOffset;
	OutCameraModeView.Rotation = ViewRot;
	OutCameraModeView.ControlRotation = ViewRot;
	OutCameraModeView.FieldOfView = FieldOfView;
}

UFPSCameraMode_ThirdPersonOTS::UFPSCameraMode_ThirdPersonOTS()
{
	FieldOfView = 80.0f;
	PivotOffset = FVector(0.0f, 44.0f, 18.0f);
	TargetArmLength = 165.0f;
	BlendTime = 0.25f;
}

void UFPSCameraMode_ThirdPersonOTS::UpdateCameraMode(float DeltaTime, AActor* TargetActor, FFPSCameraModeView& OutCameraModeView)
{
	Super::UpdateCameraMode(DeltaTime, TargetActor, OutCameraModeView);
}

UFPSCameraMode_ADS::UFPSCameraMode_ADS()
{
	FieldOfView = 68.0f;
	PivotOffset = FVector(0.0f, 30.0f, 14.0f);
	TargetArmLength = 85.0f;
	BlendTime = 0.18f;
}

void UFPSCameraMode_ADS::UpdateCameraMode(float DeltaTime, AActor* TargetActor, FFPSCameraModeView& OutCameraModeView)
{
	Super::UpdateCameraMode(DeltaTime, TargetActor, OutCameraModeView);
}

UFPSCameraMode_FirstPersonOptic::UFPSCameraMode_FirstPersonOptic()
{
	FieldOfView = 58.0f;
	PivotOffset = FVector(18.0f, 0.0f, 68.0f);
	TargetArmLength = 0.0f;
	BlendTime = 0.15f;
}

void UFPSCameraMode_FirstPersonOptic::UpdateCameraMode(float DeltaTime, AActor* TargetActor, FFPSCameraModeView& OutCameraModeView)
{
	Super::UpdateCameraMode(DeltaTime, TargetActor, OutCameraModeView);
}
