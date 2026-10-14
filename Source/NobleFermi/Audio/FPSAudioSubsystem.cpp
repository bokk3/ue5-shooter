#include "Audio/FPSAudioSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/ConfigCacheIni.h"

void UFPSAudioSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Default channel volumes
	ChannelVolumes.Add(EFPSAudioChannel::Master, 1.0f);
	ChannelVolumes.Add(EFPSAudioChannel::Music, 0.7f);
	ChannelVolumes.Add(EFPSAudioChannel::SFX, 1.0f);
	ChannelVolumes.Add(EFPSAudioChannel::Voice, 1.0f);
	ChannelVolumes.Add(EFPSAudioChannel::UI, 0.8f);

	LoadAudioSettingsFromConfig();
	ApplyAudioSettings();
}

void UFPSAudioSubsystem::SetChannelVolume(EFPSAudioChannel Channel, float Volume)
{
	ChannelVolumes.FindOrAdd(Channel) = FMath::Clamp(Volume, 0.0f, 1.0f);
	ApplyAudioSettings();
	SaveAudioSettingsToConfig();
}

float UFPSAudioSubsystem::GetChannelVolume(EFPSAudioChannel Channel) const
{
	if (const float* FoundVal = ChannelVolumes.Find(Channel))
	{
		return *FoundVal;
	}
	return 1.0f;
}

void UFPSAudioSubsystem::ApplyAudioSettings()
{
	// In UE5, SoundClasses can be adjusted via SoundMix or Audio Modulation
	// Here we prepare the volume multiplier for SoundMix override
}

void UFPSAudioSubsystem::PlayUISound(USoundBase* SoundToPlay)
{
	if (!SoundToPlay) return;

	UWorld* World = GetWorld();
	if (World)
	{
		float UiVol = GetChannelVolume(EFPSAudioChannel::UI) * GetChannelVolume(EFPSAudioChannel::Master);
		UGameplayStatics::PlaySound2D(World, SoundToPlay, UiVol);
	}
}

void UFPSAudioSubsystem::SaveAudioSettingsToConfig()
{
	if (!GConfig) return;

	FString ConfigPath = GGameUserSettingsIni;
	for (const auto& Pair : ChannelVolumes)
	{
		FString KeyName = FString::Printf(TEXT("AudioVol_%d"), static_cast<int32>(Pair.Key));
		GConfig->SetFloat(TEXT("FPSAudioSettings"), *KeyName, Pair.Value, ConfigPath);
	}
	GConfig->Flush(false, ConfigPath);
}

void UFPSAudioSubsystem::LoadAudioSettingsFromConfig()
{
	if (!GConfig) return;

	FString ConfigPath = GGameUserSettingsIni;
	for (auto& Pair : ChannelVolumes)
	{
		FString KeyName = FString::Printf(TEXT("AudioVol_%d"), static_cast<int32>(Pair.Key));
		float ReadVal = Pair.Value;
		if (GConfig->GetFloat(TEXT("FPSAudioSettings"), *KeyName, ReadVal, ConfigPath))
		{
			Pair.Value = FMath::Clamp(ReadVal, 0.0f, 1.0f);
		}
	}
}
