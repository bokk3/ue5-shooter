#include "Settings/FPSBenchmarkSubsystem.h"

FFPSHardwareReport UFPSBenchmarkSubsystem::RunAutoBenchmark()
{
	FFPSHardwareReport Report;

	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	if (Settings)
	{
		Settings->RunHardwareBenchmark();
		Settings->ApplyHardwareBenchmarkResults();

		Report.CPUScore = Settings->GetBenchmarkFallbackCPUMark();
		Report.GPUScore = Settings->GetBenchmarkFallbackGPUMark();
		Report.RecommendedQualityPreset = Settings->GetOverallScalabilityLevel();

		switch (Report.RecommendedQualityPreset)
		{
		case 0:
			Report.RecommendationSummary = TEXT("Low Preset (Optimized for Maximum Competitive FPS)");
			break;
		case 1:
			Report.RecommendationSummary = TEXT("Medium Preset (Balanced 144+ FPS)");
			break;
		case 2:
			Report.RecommendationSummary = TEXT("High Preset (Optimal Visual Fidelity & High Framerates)");
			break;
		case 3:
		default:
			Report.RecommendationSummary = TEXT("Epic Preset (Ultra Lumen & High-Res Textures)");
			break;
		}

		Settings->SaveSettings();
	}

	OnBenchmarkCompleted.Broadcast(Report);
	return Report;
}
