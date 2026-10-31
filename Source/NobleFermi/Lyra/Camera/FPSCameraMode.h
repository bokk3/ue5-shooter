#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "FPSCameraMode.generated.h"

class AActor;

/**
 * Result of evaluating a camera mode (Lyra pattern)
 */
USTRUCT(BlueprintType)
struct NOBLEFERMI_API FFPSCameraModeView
{
	GENERATED_BODY()

	FFPSCameraModeView()
		: Location(ForceInit)
		, Rotation(ForceInit)
		, ControlRotation(ForceInit)
		, FieldOfView(90.0f)
	{
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	FVector Location;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	FRotator Rotation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	FRotator ControlRotation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float FieldOfView;

	void Blend(const FFPSCameraModeView& Other, float OtherWeight);
};

/**
 * Modular Camera Mode (Lyra pattern)
 */
UCLASS(Abstract, Blueprintable)
class NOBLEFERMI_API UFPSCameraMode : public UObject
{
	GENERATED_BODY()

public:
	UFPSCameraMode();

	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float FieldOfView = 80.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	FVector PivotOffset = FVector(0.0f, 44.0f, 18.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float TargetArmLength = 165.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float BlendTime = 0.25f;

	virtual void UpdateCameraMode(float DeltaTime, AActor* TargetActor, FFPSCameraModeView& OutCameraModeView);
};

/**
 * 3rd Person Over-The-Shoulder Camera Mode
 */
UCLASS()
class NOBLEFERMI_API UFPSCameraMode_ThirdPersonOTS : public UFPSCameraMode
{
	GENERATED_BODY()

public:
	UFPSCameraMode_ThirdPersonOTS();
	virtual void UpdateCameraMode(float DeltaTime, AActor* TargetActor, FFPSCameraModeView& OutCameraModeView) override;
};

/**
 * High-Precision Tight ADS Camera Mode
 */
UCLASS()
class NOBLEFERMI_API UFPSCameraMode_ADS : public UFPSCameraMode
{
	GENERATED_BODY()

public:
	UFPSCameraMode_ADS();
	virtual void UpdateCameraMode(float DeltaTime, AActor* TargetActor, FFPSCameraModeView& OutCameraModeView) override;
};

/**
 * True 1st-Person Optic Camera Mode
 */
UCLASS()
class NOBLEFERMI_API UFPSCameraMode_FirstPersonOptic : public UFPSCameraMode
{
	GENERATED_BODY()

public:
	UFPSCameraMode_FirstPersonOptic();
	virtual void UpdateCameraMode(float DeltaTime, AActor* TargetActor, FFPSCameraModeView& OutCameraModeView) override;
};
