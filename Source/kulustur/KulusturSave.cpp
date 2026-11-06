#include "KulusturSave.h"

#include "KulusturTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/DateTime.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
	const TCHAR* SlotName = TEXT("Kulustur");
	TStrongObjectPtr<UKulusturSave> GSave;
}

UKulusturSave* UKulusturSave::Get()
{
	if (!GSave.IsValid())
	{
		UKulusturSave* Loaded = nullptr;
		if (UGameplayStatics::DoesSaveGameExist(SlotName, 0))
		{
			Loaded = Cast<UKulusturSave>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
		}
		if (!Loaded)
		{
			Loaded = Cast<UKulusturSave>(UGameplayStatics::CreateSaveGameObject(UKulusturSave::StaticClass()));
		}
		GSave.Reset(Loaded);
		GSave->FixUp();
	}
	return GSave.Get();
}

void UKulusturSave::FixUp()
{
	const int32 Total = KulusturNumWorlds * KulusturLevelsPerWorld;
	if (UnlockedLevel.Num() != KulusturNumWorlds)
	{
		UnlockedLevel.Init(-1, KulusturNumWorlds);
	}
	if (UnlockedLevel[0] < 0)
	{
		UnlockedLevel[0] = 0; // ilk dünyanın ilk bölümü her zaman açık
	}
	if (Stars.Num() != Total) Stars.SetNumZeroed(Total);
	if (BestTime.Num() != Total) BestTime.SetNumZeroed(Total);
	LastWorld = FMath::Clamp(LastWorld, 0, KulusturNumWorlds - 1);
}

void UKulusturSave::Write()
{
	UGameplayStatics::SaveGameToSlot(this, SlotName, 0);
}

bool UKulusturSave::IsWorldUnlocked(int32 World) const
{
	return UnlockedLevel.IsValidIndex(World) && UnlockedLevel[World] >= 0;
}

bool UKulusturSave::IsLevelUnlocked(int32 World, int32 Level) const
{
	return IsWorldUnlocked(World) && Level <= UnlockedLevel[World];
}

int32 UKulusturSave::GetStars(int32 World, int32 Level) const
{
	const int32 I = KulusturLevelIndex(World, Level);
	return Stars.IsValidIndex(I) ? Stars[I] : 0;
}

float UKulusturSave::GetBestTime(int32 World, int32 Level) const
{
	const int32 I = KulusturLevelIndex(World, Level);
	return BestTime.IsValidIndex(I) ? BestTime[I] : 0.f;
}

int32 UKulusturSave::WorldStars(int32 World) const
{
	int32 S = 0;
	for (int32 L = 0; L < KulusturLevelsPerWorld; ++L) S += GetStars(World, L);
	return S;
}

int32 UKulusturSave::TotalStars() const
{
	int32 S = 0;
	for (int32 V : Stars) S += V;
	return S;
}

bool UKulusturSave::RecordLevel(int32 World, int32 Level, float Time, int32 NewStars)
{
	const int32 I = KulusturLevelIndex(World, Level);
	if (!Stars.IsValidIndex(I)) return false;

	Stars[I] = FMath::Max(Stars[I], NewStars);
	if (BestTime[I] <= 0.f || Time < BestTime[I])
	{
		BestTime[I] = Time;
	}

	bool bNewWorld = false;
	if (Level + 1 < KulusturLevelsPerWorld)
	{
		UnlockedLevel[World] = FMath::Max(UnlockedLevel[World], Level + 1);
	}
	else if (World + 1 < KulusturNumWorlds)
	{
		if (UnlockedLevel[World + 1] < 0)
		{
			UnlockedLevel[World + 1] = 0;
			bNewWorld = true;
		}
	}
	else
	{
		bGameCompleted = true;
	}
	Write();
	return bNewWorld;
}

int32 UKulusturSave::RecordEndless(int32 Meters, int32 Coins)
{
	FKulusturScore S;
	S.Name = PlayerName.IsEmpty() ? TEXT("Oyuncu") : PlayerName;
	S.Meters = Meters;
	S.Coins = Coins;
	S.Date = FDateTime::Now().ToString(TEXT("%d.%m.%Y"));

	int32 Pos = 0;
	while (Pos < Leaderboard.Num() && Leaderboard[Pos].Meters >= Meters) ++Pos;
	if (Pos >= MaxLeaderboard)
	{
		return -1;
	}
	Leaderboard.Insert(S, Pos);
	if (Leaderboard.Num() > MaxLeaderboard)
	{
		Leaderboard.SetNum(MaxLeaderboard);
	}
	Write();
	return Pos;
}

int32 UKulusturSave::BestEndless() const
{
	return Leaderboard.Num() > 0 ? Leaderboard[0].Meters : 0;
}
