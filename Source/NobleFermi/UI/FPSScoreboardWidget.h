#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Multiplayer/FPSPlayerState.h"
#include "FPSScoreboardWidget.generated.h"

USTRUCT(BlueprintType)
struct FFPSScoreboardPlayerEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoreboard")
	FString PlayerName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoreboard")
	EFPSPlayerTeam Team = EFPSPlayerTeam::Alpha;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoreboard")
	int32 Score = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoreboard")
	int32 Kills = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoreboard")
	int32 Deaths = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoreboard")
	int32 Assists = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoreboard")
	int32 Ping = 20;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoreboard")
	bool bIsLocalPlayer = false;
};

/**
 * Classic MW2 Style TAB Multiplayer Scoreboard Widget
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class NOBLEFERMI_API UFPSScoreboardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "FPS Scoreboard")
	void RefreshScoreboard(const TArray<FFPSScoreboardPlayerEntry>& AllPlayers, int32 AlphaScore, int32 BravoScore, float TimeRemaining);

	UFUNCTION(BlueprintImplementableEvent, Category = "FPS Scoreboard")
	void OnScoreboardRefreshed(const TArray<FFPSScoreboardPlayerEntry>& AlphaTeam, const TArray<FFPSScoreboardPlayerEntry>& BravoTeam, int32 AlphaScore, int32 BravoScore, float TimeRemaining);
};
