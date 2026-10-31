#include "Lyra/Camera/FPSCameraComponent.h"

UFPSCameraComponent::UFPSCameraComponent()
{
	DefaultCameraModeClass = UFPSCameraMode_ThirdPersonOTS::StaticClass();
}

void UFPSCameraComponent::SetDesiredCameraMode(TSubclassOf<UFPSCameraMode> CameraModeClass)
{
	if (!CameraModeClass || CameraModeClass == CurrentCameraModeClass) return;

	PreviousCameraMode = CurrentCameraMode;
	CurrentCameraModeClass = CameraModeClass;
	CurrentCameraMode = NewObject<UFPSCameraMode>(this, CameraModeClass);

	if (CurrentCameraMode)
	{
		CurrentBlendDuration = FMath::Max(0.01f, CurrentCameraMode->BlendTime);
		BlendProgress = (PreviousCameraMode != nullptr) ? 0.0f : 1.0f;
	}
}

void UFPSCameraComponent::GetCameraView(float DeltaTime, FMinimalViewInfo& DesiredView)
{
	if (!CurrentCameraMode)
	{
		SetDesiredCameraMode(DefaultCameraModeClass);
	}

	AActor* TargetActor = GetOwner();
	if (CurrentCameraMode && TargetActor)
	{
		FFPSCameraModeView TargetView;
		CurrentCameraMode->UpdateCameraMode(DeltaTime, TargetActor, TargetView);

		if (PreviousCameraMode && BlendProgress < 1.0f)
		{
			BlendProgress += (DeltaTime / CurrentBlendDuration);
			BlendProgress = FMath::Clamp(BlendProgress, 0.0f, 1.0f);

			FFPSCameraModeView PrevView;
			PreviousCameraMode->UpdateCameraMode(DeltaTime, TargetActor, PrevView);

			// Smooth S-curve (cubic hermite) interpolation
			float SmoothWeight = FMath::SmoothStep(0.0f, 1.0f, BlendProgress);
			PrevView.Blend(TargetView, SmoothWeight);
			CurrentView = PrevView;

			if (BlendProgress >= 1.0f)
			{
				PreviousCameraMode = nullptr;
			}
		}
		else
		{
			CurrentView = TargetView;
		}

		DesiredView.Location = CurrentView.Location;
		DesiredView.Rotation = CurrentView.Rotation;
		DesiredView.FOV = CurrentView.FieldOfView;
		DesiredView.OrthoWidth = OrthoWidth;
		DesiredView.OrthoNearClipPlane = OrthoNearClipPlane;
		DesiredView.OrthoFarClipPlane = OrthoFarClipPlane;
		DesiredView.AspectRatio = AspectRatio;
		DesiredView.bConstrainAspectRatio = bConstrainAspectRatio;
		DesiredView.bUseFieldOfViewForLOD = bUseFieldOfViewForLOD;
		DesiredView.ProjectionMode = ProjectionMode;
		DesiredView.PostProcessSettings = PostProcessSettings;
		DesiredView.PostProcessBlendWeight = PostProcessBlendWeight;
		return;
	}

	Super::GetCameraView(DeltaTime, DesiredView);
}
