#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "KulusturSave.generated.h"

USTRUCT()
struct FKulusturScore
{
	GENERATED_BODY()

	UPROPERTY()
	FString Name;

	UPROPERTY()
	int32 Meters = 0;

	UPROPERTY()
	int32 Coins = 0;

	UPROPERTY()
	FString Date;
};

/** Oyuncunun ilerlemesi, rekorları ve ayarları. Saved/SaveGames/Kulustur.sav dosyasına yazılır. */
UCLASS()
class KULUSTUR_API UKulusturSave : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FString PlayerName;

	/** Her dünyada açılmış en yüksek bölüm (0..9). -1 = dünya kilitli. */
	UPROPERTY()
	TArray<int32> UnlockedLevel;

	/** 50 bölümün yıldızları (0..3). */
	UPROPERTY()
	TArray<int32> Stars;

	/** 50 bölümün en iyi süresi (sn). 0 = bitirilmedi. */
	UPROPERTY()
	TArray<float> BestTime;

	UPROPERTY()
	TArray<FKulusturScore> Leaderboard;

	UPROPERTY()
	int32 TotalCoins = 0;

	UPROPERTY()
	bool bGameCompleted = false;

	UPROPERTY()
	int32 LastWorld = 0;

	UPROPERTY()
	float MasterVolume = 1.f;

	UPROPERTY()
	float MusicVolume = 0.5f;

	UPROPERTY()
	float MouseSensitivity = 1.f;

	/** Kayıt dosyasını yükler (yoksa oluşturur). Her zaman geçerli bir nesne döner. */
	static UKulusturSave* Get();
	void Write();

	bool IsWorldUnlocked(int32 World) const;
	bool IsLevelUnlocked(int32 World, int32 Level) const;
	int32 GetStars(int32 World, int32 Level) const;
	float GetBestTime(int32 World, int32 Level) const;
	int32 WorldStars(int32 World) const;
	int32 TotalStars() const;

	/** Bölüm bitince çağrılır. Yeni dünya açıldıysa true döner. */
	bool RecordLevel(int32 World, int32 Level, float Time, int32 NewStars);

	/** Sonsuz mod skoru. Liderlikte kaçıncı sıraya girdiğini döner (0 tabanlı), girmediyse -1. */
	int32 RecordEndless(int32 Meters, int32 Coins);
	int32 BestEndless() const;

	static constexpr int32 MaxLeaderboard = 10;

private:
	void FixUp();
};
