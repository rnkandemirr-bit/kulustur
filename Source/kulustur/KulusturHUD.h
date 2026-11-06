#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "KulusturHUD.generated.h"

class AKulusturDirector;
class UFont;

/**
 * Tüm arayüz Canvas ile çiziliyor: oyun içi göstergeler, ana menü, dünya haritası,
 * bölüm yolu, liderlik tablosu, ayarlar, duraklatma, sonuç ekranları.
 */
UCLASS()
class KULUSTUR_API AKulusturHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

	/** Fare bu noktadaysa hangi düğmenin üstünde? */
	FName HitTest(const FVector2D& P) const;

private:
	struct FButton
	{
		FBox2D Box;
		FName Id;
	};

	// Çizim yardımcıları (U = ekran ölçeği, 1080p'ye göre)
	void Rect(float X, float Y, float W, float H, const FLinearColor& C);
	void Frame(float X, float Y, float W, float H, float T, const FLinearColor& C);
	void Circle(float X, float Y, float R, const FLinearColor& C, int32 Sides = 32);
	void Star(float X, float Y, float R, const FLinearColor& C);
	void Line(float X1, float Y1, float X2, float Y2, float T, const FLinearColor& C);
	void Lock(float X, float Y, float S, const FLinearColor& C);
	FVector2D Text(const FString& S, float X, float Y, float Scale, const FLinearColor& C, int32 Align = 0, bool bShadow = true, UFont* Font = nullptr);
	FVector2D Measure(const FString& S, float Scale, UFont* Font = nullptr);
	bool Button(FName Id, const FString& Label, float X, float Y, float W, float H, const FLinearColor& C, bool bEnabled = true, float TextScale = 1.f);
	bool IsHover(float X, float Y, float W, float H) const;

	void DrawGameplay(AKulusturDirector* D);
	void DrawBanners(AKulusturDirector* D);
	void DrawMain(AKulusturDirector* D);
	void DrawWorlds(AKulusturDirector* D);
	void DrawLevels(AKulusturDirector* D);
	void DrawLeaderboard(AKulusturDirector* D);
	void DrawSettings(AKulusturDirector* D);
	void DrawHelp(AKulusturDirector* D);
	void DrawNameEntry(AKulusturDirector* D);
	void DrawPause(AKulusturDirector* D);
	void DrawResults(AKulusturDirector* D);
	void DrawGameComplete(AKulusturDirector* D);
	void DrawEndlessOver(AKulusturDirector* D);
	void DrawVolumeRow(const FString& Label, float Value, FName Down, FName Up, float X, float Y);
	void Title(const FString& S, float Y, const FLinearColor& C);

	TWeakObjectPtr<AKulusturDirector> Director;
	TArray<FButton> Buttons;
	FVector2D Mouse = FVector2D(-1.f, -1.f);
	float U = 1.f;
	float Time = 0.f;
};
