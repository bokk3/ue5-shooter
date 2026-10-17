#include "Audio/FPSAudioSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/ConfigCacheIni.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Character/FPSCharacter.h"
#include "UI/FPSHUDWidget.h"

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

void UFPSAudioSubsystem::PlaySpatialSoundWithFalloff(
	const UObject* WorldContextObject,
	USoundBase* Sound,
	FVector Location,
	float BaseVolume,
	float MinDistance,
	float MaxDistance,
	float PitchMultiplier,
	EFPSAudioChannel Channel)
{
	if (!Sound || !WorldContextObject) return;

	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!World && GEngine)
	{
		World = GEngine->GetCurrentPlayWorld();
	}
	if (!World) return;

	// Calculate listener distance from active local player camera
	FVector ListenerLoc = Location;
	bool bFoundListener = false;

	APlayerController* PC = World->GetFirstPlayerController();
	if (PC && PC->PlayerCameraManager)
	{
		ListenerLoc = PC->PlayerCameraManager->GetCameraLocation();
		bFoundListener = true;
	}
	else if (PC && PC->GetPawn())
	{
		ListenerLoc = PC->GetPawn()->GetActorLocation();
		bFoundListener = true;
	}

	float Distance = bFoundListener ? FVector::Dist(Location, ListenerLoc) : 0.0f;

	// Auditory culling: if outside max audible radius, skip playback entirely
	if (bFoundListener && Distance > MaxDistance)
	{
		return;
	}

	// Dynamic physical falloff calculation (Quadratic inverse-square air dissipation)
	float DistanceFalloff = 1.0f;
	if (bFoundListener && Distance > MinDistance)
	{
		const float Alpha = (Distance - MinDistance) / FMath::Max(1.0f, MaxDistance - MinDistance);
		const float LinearFalloff = FMath::Clamp(1.0f - Alpha, 0.0f, 1.0f);
		DistanceFalloff = LinearFalloff * LinearFalloff; // Air absorption curve
	}

	// Channel volume modulation
	float ChannelGain = 1.0f;
	if (UGameInstance* GI = World->GetGameInstance())
	{
		if (UFPSAudioSubsystem* AudioSys = GI->GetSubsystem<UFPSAudioSubsystem>())
		{
			ChannelGain = AudioSys->GetChannelVolume(Channel) * AudioSys->GetChannelVolume(EFPSAudioChannel::Master);
		}
	}

	const float EffectiveVolume = FMath::Max(0.0f, BaseVolume * DistanceFalloff * ChannelGain);
	if (EffectiveVolume > 0.001f)
	{
		UGameplayStatics::PlaySoundAtLocation(WorldContextObject, Sound, Location, EffectiveVolume, PitchMultiplier);

		// Deaf Accessibility / Visual Audio: Route spatial sounds to directional HUD radar
		if (PC)
		{
			if (AFPSCharacter* LocalChar = Cast<AFPSCharacter>(PC->GetPawn()))
			{
				const AActor* ContextActor = Cast<AActor>(WorldContextObject);
				const APawn* InstigatingPawn = Cast<APawn>(ContextActor);
				if (!InstigatingPawn && ContextActor)
				{
					InstigatingPawn = Cast<APawn>(ContextActor->GetOwner());
				}

				if (InstigatingPawn != LocalChar)
				{
					if (UFPSHUDWidget* LocalHUD = LocalChar->GetActiveHUDWidget())
					{
						const FString SoundName = Sound->GetName();
						if (SoundName.Contains(TEXT("FS_")) || SoundName.Contains(TEXT("Footstep")) || SoundName.Contains(TEXT("Step")))
						{
							LocalHUD->AddDirectionalIndicator(Location, FMath::Clamp(EffectiveVolume * 1.5f, 0.4f, 1.4f), EThreatIndicatorType::Footstep);
						}
						else if (SoundName.Contains(TEXT("Fire")) || SoundName.Contains(TEXT("Shot")) || SoundName.Contains(TEXT("A_Carbine")))
						{
							LocalHUD->AddDirectionalIndicator(Location, FMath::Clamp(EffectiveVolume * 1.8f, 0.6f, 2.2f), EThreatIndicatorType::Gunfire);
						}
					}
				}
			}
		}
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
