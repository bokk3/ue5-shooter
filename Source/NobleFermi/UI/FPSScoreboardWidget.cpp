#include "UI/FPSScoreboardWidget.h"

void UFPSScoreboardWidget::RefreshScoreboard(const TArray<FFPSScoreboardPlayerEntry>& AllPlayers, int32 AlphaScore, int32 BravoScore, float TimeRemaining)
{
	TArray<FFPSScoreboardPlayerEntry> AlphaTeam;
	TArray<FFPSScoreboardPlayerEntry> BravoTeam;

	for (const FFPSScoreboardPlayerEntry& Entry : AllPlayers)
	{
		if (Entry.Team == EFPSPlayerTeam::Alpha)
		{
			AlphaTeam.Add(Entry);
		}
		else if (Entry.Team == EFPSPlayerTeam::Bravo)
		{
			BravoTeam.Add(Entry);
		}
	}

	// Sort each team descending by score (like classic MW2)
	AlphaTeam.Sort([](const FFPSScoreboardPlayerEntry& A, const FFPSScoreboardPlayerEntry& B) {
		return A.Score > B.Score;
	});

	BravoTeam.Sort([](const FFPSScoreboardPlayerEntry& A, const FFPSScoreboardPlayerEntry& B) {
		return A.Score > B.Score;
	});

	OnScoreboardRefreshed(AlphaTeam, BravoTeam, AlphaScore, BravoScore, TimeRemaining);
}
