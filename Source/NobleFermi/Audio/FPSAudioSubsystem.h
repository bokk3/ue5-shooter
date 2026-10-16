#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"
#include "FPSAudioSubsystem.generated.h"

UENUM(BlueprintType)
enum class EFPSAudioChannel : uint8
{
	Master,
	Music,
	SFX,
	Voice,
	UI
};

/**
 * GameInstance Subsystem managing persistent audio settings and SoundClass volume modulation across levels.
 */
UCLASS()
class NOBLEFERMI_API UFPSAudioSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintCallable, Category = "FPS Audio")
	void SetChannelVolume(EFPSAudioChannel Channel, float Volume);

	UFUNCTION(BlueprintPure, Category = "FPS Audio")
	float GetChannelVolume(EFPSAudioChannel Channel) const;

	UFUNCTION(BlueprintCallable, Category = "FPS Audio")
	void ApplyAudioSettings();

	UFUNCTION(BlueprintCallable, Category = "FPS Audio")
	void PlayUISound(USoundBase* SoundToPlay);

	/**
	 * Spatialized audio playback with dynamic physical distance falloff, quadratic air absorption,
	 * channel gain modulation, and auditory occlusion culling beyond MaxDistance.
	 */
	UFUNCTION(BlueprintCallable, Category = "FPS Audio", meta = (WorldContext = "WorldContextObject"))
	static void PlaySpatialSoundWithFalloff(
		const UObject* WorldContextObject,
		USoundBase* Sound,
		FVector Location,
		float BaseVolume = 1.0f,
		float MinDistance = 150.0f,
		float MaxDistance = 3500.0f,
		float PitchMultiplier = 1.0f,
		EFPSAudioChannel Channel = EFPSAudioChannel::SFX
	);

private:
	UPROPERTY()
	TMap<EFPSAudioChannel, float> ChannelVolumes;

	void SaveAudioSettingsToConfig();
	void LoadAudioSettingsFromConfig();
};
