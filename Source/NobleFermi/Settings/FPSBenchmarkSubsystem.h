#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameFramework/GameUserSettings.h"
#include "FPSBenchmarkSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FFPSHardwareReport
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Benchmark")
	float CPUScore = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Benchmark")
	float GPUScore = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Benchmark")
	int32 RecommendedQualityPreset = 2; // 0=Low, 1=Med, 2=High, 3=Epic

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Benchmark")
	FString RecommendationSummary = TEXT("High (1080p 144Hz)");
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBenchmarkCompletedSignature, const FFPSHardwareReport&, Report);

/**
 * AAA Hardware Benchmark & Auto-Optimization Subsystem
 */
UCLASS()
class NOBLEFERMI_API UFPSBenchmarkSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "FPS Benchmark")
	FOnBenchmarkCompletedSignature OnBenchmarkCompleted;

	UFUNCTION(BlueprintCallable, Category = "FPS Benchmark")
	FFPSHardwareReport RunAutoBenchmark();
};
