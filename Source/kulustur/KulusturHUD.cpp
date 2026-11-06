#include "KulusturHUD.h"

#include "KulusturDirector.h"
#include "KulusturSave.h"
#include "KulusturTrack.h"
#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Misc/CommandLine.h"

namespace
{
	// Sanal ekran 1920x1080; gerçek ekrana ölçeklenir.
	float GOX = 0.f;
	// Motorun büyük fontu küçük kalıyor; tüm yazıları büyüt.
	constexpr float TextBoost = 1.65f;
	const FLinearColor Panel(0.02f, 0.03f, 0.06f, 0.82f);
	const FLinearColor PanelLight(0.08f, 0.10f, 0.16f, 0.9f);
	const FLinearColor Gold(1.f, 0.78f, 0.15f);
	const FLinearColor Grey(0.35f, 0.37f, 0.42f);
	const FLinearColor Accent(1.f, 0.42f, 0.12f);
	const FLinearColor TextC(0.95f, 0.96f, 1.f);
	const FLinearColor Dim(0.65f, 0.68f, 0.75f);
	const FLinearColor Good(0.35f, 0.95f, 0.45f);
	const FLinearColor Bad(1.f, 0.25f, 0.2f);

	FLinearColor WithA(FLinearColor C, float A) { C.A = A; return C; }
}

// ---------------------------------------------------------------------------------------------
// Yardımcılar

void AKulusturHUD::Rect(float X, float Y, float W, float H, const FLinearColor& C)
{
	FCanvasTileItem T(FVector2D(GOX + X * U, Y * U), FVector2D(W * U, H * U), C);
	T.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(T);
}

void AKulusturHUD::Frame(float X, float Y, float W, float H, float T, const FLinearColor& C)
{
	Rect(X, Y, W, T, C);
	Rect(X, Y + H - T, W, T, C);
	Rect(X, Y, T, H, C);
	Rect(X + W - T, Y, T, H, C);
}

void AKulusturHUD::Circle(float X, float Y, float R, const FLinearColor& C, int32 Sides)
{
	TArray<FCanvasUVTri> Tris;
	const FVector2D Ctr(GOX + X * U, Y * U);
	for (int32 i = 0; i < Sides; ++i)
	{
		const float A0 = 2.f * PI * i / Sides;
		const float A1 = 2.f * PI * (i + 1) / Sides;
		FCanvasUVTri T;
		T.V0_Pos = Ctr;
		T.V1_Pos = Ctr + FVector2D(FMath::Cos(A0), FMath::Sin(A0)) * R * U;
		T.V2_Pos = Ctr + FVector2D(FMath::Cos(A1), FMath::Sin(A1)) * R * U;
		T.V0_Color = T.V1_Color = T.V2_Color = C;
		Tris.Add(T);
	}
	FCanvasTriangleItem Item(Tris, GWhiteTexture);
	Item.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Item);
}

void AKulusturHUD::Star(float X, float Y, float R, const FLinearColor& C)
{
	TArray<FCanvasUVTri> Tris;
	const FVector2D Ctr(GOX + X * U, Y * U);
	auto P = [&](int32 i, float Rad)
	{
		const float A = -PI * 0.5f + i * PI / 5.f;
		return Ctr + FVector2D(FMath::Cos(A), FMath::Sin(A)) * Rad * U;
	};
	for (int32 i = 0; i < 10; ++i)
	{
		FCanvasUVTri T;
		T.V0_Pos = Ctr;
		T.V1_Pos = P(i, (i % 2) ? R * 0.45f : R);
		T.V2_Pos = P(i + 1, ((i + 1) % 2) ? R * 0.45f : R);
		T.V0_Color = T.V1_Color = T.V2_Color = C;
		Tris.Add(T);
	}
	FCanvasTriangleItem Item(Tris, GWhiteTexture);
	Item.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Item);
}

void AKulusturHUD::Line(float X1, float Y1, float X2, float Y2, float T, const FLinearColor& C)
{
	FCanvasLineItem L(FVector2D(GOX + X1 * U, Y1 * U), FVector2D(GOX + X2 * U, Y2 * U));
	L.LineThickness = T * U;
	L.SetColor(C);
	Canvas->DrawItem(L);
}

void AKulusturHUD::Lock(float X, float Y, float S, const FLinearColor& C)
{
	// Asma kilit: gövde + kemer
	Rect(X - S * 0.5f, Y - S * 0.1f, S, S * 0.75f, C);
	Frame(X - S * 0.32f, Y - S * 0.55f, S * 0.64f, S * 0.5f, S * 0.13f, C);
	Rect(X - S * 0.07f, Y + S * 0.1f, S * 0.14f, S * 0.3f, FLinearColor(0.f, 0.f, 0.f, 0.6f));
}

FVector2D AKulusturHUD::Measure(const FString& S, float Scale, UFont* Font)
{
	if (!Font) Font = GEngine->GetLargeFont();
	float XL = 0.f, YL = 0.f;
	Canvas->TextSize(Font, S, XL, YL, Scale * U * TextBoost, Scale * U * TextBoost);
	return FVector2D(XL / U, YL / U);
}

FVector2D AKulusturHUD::Text(const FString& S, float X, float Y, float Scale, const FLinearColor& C, int32 Align, bool bShadow, UFont* Font)
{
	if (!Font) Font = GEngine->GetLargeFont();
	const FVector2D Sz = Measure(S, Scale, Font);
	float PX = X;
	if (Align == 1) PX = X - Sz.X * 0.5f;
	if (Align == 2) PX = X - Sz.X;
	FCanvasTextItem T(FVector2D(GOX + PX * U, Y * U), FText::FromString(S), Font, C);
	T.Scale = FVector2D(Scale * U * TextBoost, Scale * U * TextBoost);
	if (bShadow) T.EnableShadow(FLinearColor(0.f, 0.f, 0.f, C.A * 0.8f), FVector2D(2.f * U, 2.f * U));
	Canvas->DrawItem(T);
	return Sz;
}

bool AKulusturHUD::IsHover(float X, float Y, float W, float H) const
{
	const float SX = GOX + X * U, SY = Y * U;
	return Mouse.X >= SX && Mouse.X <= SX + W * U && Mouse.Y >= SY && Mouse.Y <= SY + H * U;
}

bool AKulusturHUD::Button(FName Id, const FString& Label, float X, float Y, float W, float H, const FLinearColor& C, bool bEnabled, float TextScale)
{
	const bool bHover = bEnabled && IsHover(X, Y, W, H);
	FLinearColor Bg = bEnabled ? C : Grey;
	Bg.A = bHover ? 1.f : 0.85f;
	// Eğik "yol tabelası" görünümü: alt gölge + gövde + üst parlak çizgi
	Rect(X + 6.f, Y + 6.f, W, H, FLinearColor(0.f, 0.f, 0.f, 0.45f));
	Rect(X, Y, W, H, bHover ? Bg * 1.25f : Bg);
	Rect(X, Y, W, 4.f, FLinearColor(1.f, 1.f, 1.f, bHover ? 0.6f : 0.25f));
	if (bHover) Frame(X - 3.f, Y - 3.f, W + 6.f, H + 6.f, 3.f, FLinearColor::White);
	const FVector2D Sz = Measure(Label, TextScale);
	Text(Label, X + W * 0.5f, Y + (H - Sz.Y) * 0.5f, TextScale, bEnabled ? FLinearColor::White : Dim, 1);
	if (bEnabled)
	{
		FButton B;
		B.Box = FBox2D(FVector2D(GOX + X * U, Y * U), FVector2D(GOX + (X + W) * U, (Y + H) * U));
		B.Id = Id;
		Buttons.Add(B);
	}
	return bHover;
}

FName AKulusturHUD::HitTest(const FVector2D& P) const
{
	for (int32 i = Buttons.Num() - 1; i >= 0; --i)
	{
		if (Buttons[i].Box.IsInside(P)) return Buttons[i].Id;
	}
	return NAME_None;
}

void AKulusturHUD::Title(const FString& S, float Y, const FLinearColor& C)
{
	Text(S, 960.f, Y, 2.6f, C, 1);
	const float W = Measure(S, 2.6f).X;
	Rect(960.f - W * 0.5f, Y + Measure(S, 2.6f).Y + 4.f, W, 6.f, C);
}

// ---------------------------------------------------------------------------------------------

void AKulusturHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas) return;

	if (!Director.IsValid())
	{
		for (TActorIterator<AKulusturDirector> It(GetWorld()); It; ++It)
		{
			Director = *It;
			break;
		}
	}
	AKulusturDirector* D = Director.Get();
	static const bool bNoUI = FParse::Param(FCommandLine::Get(), TEXT("KulusturNoUI"));
	if (bNoUI) return;
	if (!D) return;

	U = Canvas->ClipY / 1080.f;
	GOX = (Canvas->ClipX - 1920.f * U) * 0.5f;
	Time += GetWorld()->GetDeltaSeconds() > 0.f ? FMath::Min(GetWorld()->DeltaRealTimeSeconds, 0.1f) : 0.016f;
	Buttons.Reset();
	Mouse = FVector2D(-1.f, -1.f);
	if (PlayerOwner)
	{
		float MX, MY;
		if (PlayerOwner->GetMousePosition(MX, MY)) Mouse = FVector2D(MX, MY);
	}

	if (D->GetMode() != EKulusturMode::Menu)
	{
		DrawGameplay(D);
	}
	DrawBanners(D);

	switch (D->GetScreen())
	{
	case EKulusturScreen::Main: DrawMain(D); break;
	case EKulusturScreen::Worlds: DrawWorlds(D); break;
	case EKulusturScreen::Levels: DrawLevels(D); break;
	case EKulusturScreen::Leaderboard: DrawLeaderboard(D); break;
	case EKulusturScreen::Settings: DrawSettings(D); break;
	case EKulusturScreen::Help: DrawHelp(D); break;
	case EKulusturScreen::NameEntry: DrawNameEntry(D); break;
	case EKulusturScreen::Pause: DrawPause(D); break;
	case EKulusturScreen::Results: DrawResults(D); break;
	case EKulusturScreen::GameComplete: DrawGameComplete(D); break;
	case EKulusturScreen::EndlessOver: DrawEndlessOver(D); break;
	default: break;
	}
}

// ---------------------------------------------------------------------------------------------
// Oyun içi

void AKulusturHUD::DrawGameplay(AKulusturDirector* D)
{
	const FKulusturLevelConfig& Cfg = D->GetConfig();
	const FKulusturBiomeStyle& S = KulusturGetBiomeStyle(D->GetCurrentBiome());
	const bool bEndless = D->GetMode() == EKulusturMode::Endless;

	// Şimşek parlaması
	if (D->GetFlash() > 0.01f)
	{
		Rect(-GOX / U, 0.f, Canvas->ClipX / U, 1080.f, FLinearColor(1.f, 1.f, 1.f, D->GetFlash() * 0.35f));
	}
	// Ters kontrol çerçevesi
	if (D->IsControlsInverted())
	{
		const float A = 0.5f + 0.25f * FMath::Sin(Time * 8.f);
		Frame(-GOX / U, 0.f, Canvas->ClipX / U, 1080.f, 14.f, FLinearColor(1.f, 0.15f, 0.6f, A));
		Text(TEXT("< > TERS KONTROL < >"), 960.f, 1000.f, 1.2f, FLinearColor(1.f, 0.4f, 0.8f), 1);
	}
	if (D->GetInvertWarning() > 0.f)
	{
		Text(FString::Printf(TEXT("KONTROLLER TERS DÖNÜYOR... %d"), FMath::CeilToInt(D->GetInvertWarning())), 960.f, 1000.f, 1.1f, FLinearColor(1.f, 0.6f, 0.85f), 1);
	}

	// Sol üst: bölüm / mesafe
	Rect(30.f, 26.f, 470.f, bEndless ? 150.f : 112.f, Panel);
	Rect(30.f, 26.f, 8.f, bEndless ? 150.f : 112.f, S.UIColor);
	if (bEndless)
	{
		Text(TEXT("SONSUZ MOD"), 56.f, 34.f, 0.8f, S.UIColor);
		Text(FString::Printf(TEXT("%d m"), D->GetMeters()), 56.f, 62.f, 2.2f, TextC);
		const int32 Best = UKulusturSave::Get()->BestEndless();
		Text(FString::Printf(TEXT("%s   |   Rekor: %d m"), *S.Name.ToString(), Best), 56.f, 138.f, 0.7f, Dim);
	}
	else
	{
		Text(FString::Printf(TEXT("DÜNYA %d  |  BÖLÜM %d"), D->GetWorldIndex() + 1, D->GetLevelIndex() + 1), 56.f, 34.f, 0.8f, S.UIColor);
		Text(Cfg.Title.ToString(), 56.f, 62.f, 1.2f, TextC);
		Text(FString::Printf(TEXT("Ölüm: %d"), D->GetDeaths()), 56.f, 104.f, 0.7f, Dim);
	}

	// Sağ üst: süre, altın, hız
	Rect(1520.f, 26.f, 370.f, 150.f, Panel);
	Text(AKulusturDirector::FormatTime(D->GetElapsed()), 1866.f, 30.f, 1.6f, TextC, 2);
	Circle(1560.f, 112.f, 15.f, Gold);
	Circle(1560.f, 112.f, 9.f, FLinearColor(1.f, 0.9f, 0.4f));
	Text(bEndless ? FString::Printf(TEXT("%d"), D->GetCoins()) : FString::Printf(TEXT("%d / %d"), D->GetCoins(), D->GetTotalCoins()), 1588.f, 96.f, 1.f, Gold);
	Text(FString::Printf(TEXT("%.0f km/s"), D->GetSpeedKmh()), 1866.f, 96.f, 1.f, Dim, 2);
	if (!bEndless)
	{
		Text(FString::Printf(TEXT("Yıldız süresi: %s"), *AKulusturDirector::FormatTime(Cfg.ParTime * 0.95f)), 1866.f, 140.f, 0.65f,
			D->GetElapsed() <= Cfg.ParTime * 0.95f ? Good : Bad, 2);
	}

	// İlerleme çubuğu (yol şeklinde)
	if (!bEndless)
	{
		const float X0 = 640.f, W = 640.f, Y = 40.f;
		Rect(X0, Y, W, 22.f, FLinearColor(0.05f, 0.05f, 0.06f, 0.85f));
		for (int32 i = 0; i < 16; ++i) Rect(X0 + 10.f + i * 40.f, Y + 10.f, 20.f, 3.f, FLinearColor(1.f, 1.f, 1.f, 0.5f));
		const float P = D->GetProgress();
		Rect(X0, Y, W * P, 22.f, WithA(S.UIColor, 0.55f));
		// Bitiş bayrağı
		for (int32 r = 0; r < 3; ++r) for (int32 c = 0; c < 3; ++c)
			Rect(X0 + W + 4.f + c * 8.f, Y - 2.f + r * 8.f, 8.f, 8.f, ((r + c) % 2) ? FLinearColor::Black : FLinearColor::White);
		// Oyuncu (tır simgesi)
		const float PX = X0 + W * P;
		Rect(PX - 16.f, Y - 6.f, 24.f, 20.f, Accent);
		Rect(PX + 8.f, Y - 1.f, 10.f, 15.f, FLinearColor::White);
		Circle(PX - 9.f, Y + 16.f, 4.f, FLinearColor::Black, 10);
		Circle(PX + 11.f, Y + 16.f, 4.f, FLinearColor::Black, 10);
	}
	else
	{
		// Sonraki bölgeye kalan
		const float X = FMath::Fmod(D->GetMeters() * 100.f, KulusturEndlessBiomeLength) / KulusturEndlessBiomeLength;
		const EKulusturBiome Next = KulusturEndlessBiomeAt((D->GetMeters() * 100.f) + KulusturEndlessBiomeLength * (1.f - X) + 10.f);
		const FKulusturBiomeStyle& NS = KulusturGetBiomeStyle(Next);
		Rect(640.f, 40.f, 640.f, 16.f, FLinearColor(0.05f, 0.05f, 0.06f, 0.85f));
		Rect(640.f, 40.f, 640.f * X, 16.f, WithA(S.UIColor, 0.7f));
		Text(FString::Printf(TEXT("Sonraki bölge: %s  (%d m)"), *NS.Name.ToString(), FMath::RoundToInt(KulusturEndlessBiomeLength * (1.f - X) / 100.f)), 960.f, 62.f, 0.65f, NS.UIColor, 1);
	}

	// Aktif zorluklar
	{
		float X = 30.f;
		const float Y = bEndless ? 190.f : 152.f;
		const uint32 Ch = D->GetActiveChallenges();
		struct FC { uint32 Bit; const TCHAR* Name; FLinearColor C; };
		const FC List[] = {
			{ KChallenge::Lasers, TEXT("LAZER"), FLinearColor(1.f, 0.2f, 0.2f) },
			{ KChallenge::InvertControls, TEXT("TERS KONTROL"), FLinearColor(1.f, 0.3f, 0.7f) },
			{ KChallenge::Wind, TEXT("RÜZGAR"), FLinearColor(0.6f, 0.85f, 1.f) },
			{ KChallenge::Ice, TEXT("BUZ"), FLinearColor(0.7f, 0.95f, 1.f) },
			{ KChallenge::FallingTrucks, TEXT("GÖKTEN ARAÇ"), FLinearColor(1.f, 0.55f, 0.2f) },
			{ KChallenge::Fog, TEXT("SİS"), FLinearColor(0.8f, 0.8f, 0.85f) },
			{ KChallenge::GapWalls, TEXT("ŞERİT DUVARI"), FLinearColor(0.3f, 1.f, 0.4f) },
			{ KChallenge::LaneSwerve, TEXT("MAKAS"), FLinearColor(1.f, 0.85f, 0.2f) },
			{ KChallenge::SpeedBurst, TEXT("HIZLANMA"), FLinearColor(1.f, 0.7f, 0.1f) },
			{ KChallenge::Darkness, TEXT("KARANLIK"), FLinearColor(0.6f, 0.5f, 1.f) } };
		for (const FC& C : List)
		{
			if (!(Ch & C.Bit)) continue;
			const float W = Measure(C.Name, 0.6f).X + 24.f;
			Rect(X, Y, W, 30.f, WithA(C.C * 0.35f, 0.85f));
			Rect(X, Y, 4.f, 30.f, C.C);
			Text(C.Name, X + 12.f, Y + 4.f, 0.6f, C.C, 0, false);
			X += W + 8.f;
		}
	}

	// Uyarılar
	float WarnY = 300.f;
	const float Laser = D->GetLaserWarning();
	if (Laser > 0.f && Laser < 3500.f)
	{
		const float A = 0.6f + 0.4f * FMath::Sin(Time * 14.f);
		Text(FString::Printf(TEXT("LAZER  %d m - ZIPLA!"), FMath::RoundToInt(Laser / 100.f)), 960.f, WarnY, 1.3f, FLinearColor(1.f, 0.2f, 0.15f, A), 1);
		WarnY += 60.f;
	}
	float OpenY = 0.f;
	const float Gap = D->GetGapWallInfo(OpenY);
	if (Gap > 0.f && Gap < 11000.f)
	{
		const float Dy = OpenY - D->GetPlayerY();
		FString Msg;
		if (FMath::Abs(Dy) < AKulusturTrack::LaneSpacing * 0.6f) Msg = TEXT("DOĞRU ŞERİTTESİN - DEVAM!");
		else if (Dy < 0.f) Msg = TEXT("<<  SOLA GEÇ!");
		else Msg = TEXT("SAĞA GEÇ!  >>");
		const bool bOk = FMath::Abs(Dy) < AKulusturTrack::LaneSpacing * 0.6f;
		const float A = bOk ? 1.f : 0.6f + 0.4f * FMath::Sin(Time * 12.f);
		Text(Msg, 960.f, WarnY, 1.4f, bOk ? WithA(Good, A) : FLinearColor(1.f, 0.85f, 0.1f, A), 1);
		Text(FString::Printf(TEXT("Duvar: %d m"), FMath::RoundToInt(Gap / 100.f)), 960.f, WarnY + 50.f, 0.8f, Dim, 1);
		WarnY += 100.f;
	}
	if (D->GetWindStrength() > 0.f)
	{
		const float Dir = D->GetWindDir();
		const float X = 960.f;
		const float Y = 1040.f;
		Text(TEXT("RÜZGAR"), X, Y - 40.f, 0.6f, FLinearColor(0.7f, 0.9f, 1.f), 1);
		for (int32 i = 0; i < 3; ++i)
		{
			const float Off = FMath::Fmod(Time * 60.f + i * 30.f, 90.f) - 45.f;
			Text(Dir > 0.f ? TEXT(">") : TEXT("<"), X + Off * Dir, Y - 18.f, 1.2f, FLinearColor(0.7f, 0.9f, 1.f, 0.8f), 1, false);
		}
	}

	// Ağır çekim: mavi ton
	if (D->IsSlowMo())
	{
		Rect(-GOX / U, 0.f, Canvas->ClipX / U, 1080.f, FLinearColor(0.1f, 0.35f, 0.9f, 0.13f));
		Text(TEXT("AĞIR ÇEKİM"), 960.f, 230.f, 1.0f, FLinearColor(0.6f, 0.85f, 1.f, 0.6f + 0.3f * FMath::Sin(Time * 6.f)), 1);
	}
	// Enerji çubuğu ve yetenekler
	if (D->GetState() == EKulusturState::Playing)
	{
		const float E = D->GetEnergy() / 100.f;
		const float BX = 40.f, BY = 1000.f, BW = 420.f;
		Rect(BX - 10.f, BY - 52.f, BW + 20.f, 92.f, FLinearColor(0.f, 0.f, 0.f, 0.45f));
		Text(TEXT("ENERJİ"), BX, BY - 46.f, 0.6f, TextC, 0, false);
		Rect(BX, BY - 14.f, BW, 18.f, FLinearColor(0.05f, 0.05f, 0.06f, 0.9f));
		const FLinearColor EC = D->IsSlowMo() ? FLinearColor(0.4f, 0.75f, 1.f) : (E >= 0.3f ? FLinearColor(1.f, 0.45f, 0.9f) : FLinearColor(0.6f, 0.3f, 0.5f));
		Rect(BX, BY - 14.f, BW * E, 18.f, EC);
		Rect(BX + BW * 0.3f, BY - 14.f, 2.f, 18.f, FLinearColor(1.f, 1.f, 1.f, 0.5f)); // atılma eşiği
		Text(TEXT("E: atıl"), BX, BY + 10.f, 0.55f, D->CanDash() ? TextC : Dim, 0, false);
		Text(TEXT("Q: ağır çekim"), BX + 120.f, BY + 10.f, 0.55f, E >= 0.15f ? TextC : Dim, 0, false);
		Text(TEXT("Ctrl: çakıl"), BX + 300.f, BY + 10.f, 0.55f, TextC, 0, false);
	}

	// Ölüm
	if (D->GetState() == EKulusturState::Dead)
	{
		Rect(-GOX / U, 0.f, Canvas->ClipX / U, 1080.f, FLinearColor(0.5f, 0.f, 0.f, 0.35f));
		Text(TEXT("ÖLDÜN!"), 960.f, 400.f, 4.f, FLinearColor(1.f, 0.25f, 0.2f), 1);
		Text(TEXT("Yeniden başlıyor..."), 960.f, 540.f, 1.f, TextC, 1);
	}
	else if (D->GetState() == EKulusturState::Won && D->GetScreen() == EKulusturScreen::None)
	{
		Text(TEXT("BİTİŞ!"), 960.f, 380.f, 4.f, Gold, 1);
	}
	else if (D->GetState() == EKulusturState::Playing && D->GetElapsed() < 8.f)
	{
		// Türkçe kontrol ipucu (paketin İngilizce yardım ekranı yerine)
		const float A = FMath::Clamp(8.f - D->GetElapsed(), 0.f, 1.f);
		Rect(480.f, 880.f, 1300.f, 170.f, FLinearColor(0.f, 0.f, 0.f, 0.5f * A));
		Text(bEndless ? TEXT("Yere değme! Ne kadar uzağa gidebilirsin?") : TEXT("Yere değme! Araçların üstünden atlayarak BİTİŞ kapısından geç."), 1130.f, 890.f, 0.85f, WithA(Gold, A), 1);
		Text(TEXT("WASD: koş   |   Fare: bak   |   Boşluk: zıpla, havada tekrar: çift zıpla   |   Shift: depar"), 1130.f, 935.f, 0.7f, WithA(TextC, A), 1);
		Text(TEXT("Sol tık: rayda yön değiştir   |   Sağ tık: halatta in/çık   |   Tab: bakışı ters çevir"), 1130.f, 970.f, 0.7f, WithA(TextC, A), 1);
		Text(TEXT("E: atıl   |   Q (basılı): ağır çekim   |   Ctrl: aşağı çakıl   |   Havada hareket yap, tıra in: enerji kazan!"), 1130.f, 1005.f, 0.65f, WithA(FLinearColor(1.f, 0.6f, 0.95f), A), 1);
	}
}

void AKulusturHUD::DrawBanners(AKulusturDirector* D)
{
	float Y = 200.f;
	if (D->GetMode() == EKulusturMode::Menu) return;
	for (const FKulusturBanner& B : D->GetBanners())
	{
		const float In = FMath::Clamp(B.Time / 0.2f, 0.f, 1.f);
		const float Out = FMath::Clamp((B.Duration - B.Time) / 0.4f, 0.f, 1.f);
		const float A = FMath::Min(In, Out);
		const float Scale = 1.6f + (1.f - In) * 0.6f;
		const FVector2D Sz = Measure(B.Text, Scale);
		const float W = FMath::Max(Sz.X, B.Sub.IsEmpty() ? 0.f : Measure(B.Sub, 0.85f).X) + 80.f;
		const float H = B.Sub.IsEmpty() ? Sz.Y + 20.f : Sz.Y + 60.f;
		Rect(960.f - W * 0.5f, Y, W, H, FLinearColor(0.f, 0.f, 0.f, 0.55f * A));
		Rect(960.f - W * 0.5f, Y, W, 5.f, WithA(B.Color, A));
		Text(B.Text, 960.f, Y + 10.f, Scale, WithA(B.Color, A), 1);
		if (!B.Sub.IsEmpty()) Text(B.Sub, 960.f, Y + Sz.Y + 16.f, 0.85f, WithA(TextC, A), 1);
		Y += H + 12.f;
	}
}

// ---------------------------------------------------------------------------------------------
// Menüler

void AKulusturHUD::DrawMain(AKulusturDirector* D)
{
	UKulusturSave* Save = UKulusturSave::Get();
	// Sol panel
	Rect(0.f - GOX / U, 0.f, 640.f + GOX / U, 1080.f, FLinearColor(0.01f, 0.02f, 0.05f, 0.78f));
	Rect(640.f, 0.f, 6.f, 1080.f, Accent);

	// Logo: yol çizgileri üstünde KULÜSTÜR
	const float Bob = FMath::Sin(Time * 2.f) * 4.f;
	Text(TEXT("KULÜSTÜR"), 320.f, 90.f + Bob, 3.4f, Accent, 1);
	for (int32 i = 0; i < 7; ++i) Rect(80.f + i * 72.f, 205.f, 44.f, 8.f, Gold);
	Text(TEXT("Tırdan tıra atla. Yere değme."), 320.f, 225.f, 0.95f, Dim, 1);

	const float BX = 120.f, BW = 400.f, BH = 74.f;
	float Y = 330.f;
	Button(TEXT("Play"), TEXT("OYNA"), BX, Y, BW, BH, Accent, true, 1.4f); Y += BH + 22.f;
	Button(TEXT("Endless"), TEXT("SONSUZ MOD"), BX, Y, BW, BH, FLinearColor(0.75f, 0.15f, 0.45f), true, 1.2f); Y += BH + 22.f;
	Button(TEXT("Leaderboard"), TEXT("LİDERLİK TABLOSU"), BX, Y, BW, BH, FLinearColor(0.15f, 0.4f, 0.75f), true, 1.1f); Y += BH + 22.f;
	Button(TEXT("Help"), TEXT("NASIL OYNANIR"), BX, Y, BW, BH, FLinearColor(0.2f, 0.5f, 0.35f), true, 1.1f); Y += BH + 22.f;
	Button(TEXT("Settings"), TEXT("AYARLAR"), BX, Y, BW, BH, FLinearColor(0.3f, 0.32f, 0.4f), true, 1.1f); Y += BH + 22.f;
	Button(TEXT("Quit"), TEXT("ÇIKIŞ"), BX, Y, BW, BH, FLinearColor(0.35f, 0.12f, 0.12f), true, 1.1f);

	// Sağ altta oyuncu kartı
	Rect(1440.f, 900.f, 440.f, 140.f, Panel);
	Rect(1440.f, 900.f, 440.f, 5.f, Gold);
	Text(FString::Printf(TEXT("Sürücü: %s"), Save->PlayerName.IsEmpty() ? TEXT("?") : *Save->PlayerName), 1465.f, 915.f, 1.f, TextC);
	Star(1480.f, 985.f, 20.f, Gold);
	Text(FString::Printf(TEXT("%d / 150"), Save->TotalStars()), 1510.f, 968.f, 1.f, Gold);
	Circle(1690.f, 985.f, 16.f, Gold);
	Text(FString::Printf(TEXT("%d"), Save->TotalCoins), 1715.f, 968.f, 1.f, Gold);
	if (Save->BestEndless() > 0) Text(FString::Printf(TEXT("Sonsuz rekor: %d m"), Save->BestEndless()), 1465.f, 1005.f, 0.7f, Dim);
	Text(TEXT("Enter: Oyna"), 320.f, 1030.f, 0.65f, Dim, 1);
}

void AKulusturHUD::DrawWorlds(AKulusturDirector* D)
{
	UKulusturSave* Save = UKulusturSave::Get();
	Rect(-GOX / U, 0.f, Canvas->ClipX / U, 1080.f, FLinearColor(0.01f, 0.02f, 0.05f, 0.62f));
	Title(TEXT("DÜNYALAR"), 50.f, TextC);
	Text(TEXT("Bir dünyanın 10 bölümünü bitir, sonraki dünyanın kilidi açılsın."), 960.f, 150.f, 0.85f, Dim, 1);

	// Kartları birleştiren otoyol
	Line(130.f, 560.f, 1790.f, 560.f, 26.f, FLinearColor(0.05f, 0.05f, 0.07f, 0.9f));
	for (int32 i = 0; i < 40; ++i) Rect(140.f + i * 42.f, 557.f, 22.f, 5.f, FLinearColor(1.f, 0.85f, 0.2f, 0.6f));

	const float CW = 320.f, CH = 520.f, Gap = 28.f;
	const float X0 = 960.f - (5 * CW + 4 * Gap) * 0.5f;
	for (int32 W = 0; W < KulusturNumWorlds; ++W)
	{
		const FKulusturBiomeStyle& S = KulusturGetBiomeStyle((EKulusturBiome)W);
		const bool bOpen = Save->IsWorldUnlocked(W);
		const float X = X0 + W * (CW + Gap);
		const bool bSel = D->GetSelectedWorld() == W;
		const bool bHover = IsHover(X, 280.f, CW, CH);
		const float Lift = (bHover || bSel) ? -12.f : 0.f;
		const float Y = 280.f + Lift;

		Rect(X + 8.f, Y + 8.f, CW, CH, FLinearColor(0.f, 0.f, 0.f, 0.5f));
		Rect(X, Y, CW, CH, bOpen ? PanelLight : FLinearColor(0.06f, 0.06f, 0.08f, 0.92f));
		// Üst: biyom "manzara" şeridi
		const FLinearColor Sky = bOpen ? S.FogColor : Grey * 0.6f;
		Rect(X, Y, CW, 170.f, Sky);
		Rect(X, Y + 120.f, CW, 50.f, bOpen ? S.Ground : Grey * 0.4f);
		Rect(X, Y + 140.f, CW, 16.f, bOpen ? S.Road : FLinearColor::Black);
		for (int32 i = 0; i < 6; ++i) Rect(X + 10.f + i * 55.f, Y + 146.f, 28.f, 4.f, bOpen ? S.RoadLine : Grey);
		// Güneş / ay
		Circle(X + CW - 60.f, Y + 55.f, 26.f, bOpen ? S.SunColor : Grey);
		// Numara rozeti
		Circle(X + 50.f, Y + 50.f, 34.f, bOpen ? S.UIColor : Grey);
		Text(FString::Printf(TEXT("%d"), W + 1), X + 50.f, Y + 26.f, 1.5f, FLinearColor::White, 1);

		Text(S.Name.ToString(), X + CW * 0.5f, Y + 190.f, 1.1f, bOpen ? S.UIColor : Dim, 1);
		// Alt yazıyı iki satıra böl
		FString Sub = S.Subtitle.ToString();
		FString L1 = Sub, L2;
		if (Sub.Len() > 26)
		{
			int32 Cut = Sub.Len() / 2;
			while (Cut < Sub.Len() && Sub[Cut] != ' ') ++Cut;
			L1 = Sub.Left(Cut);
			L2 = Sub.Mid(Cut + 1);
		}
		Text(L1, X + CW * 0.5f, Y + 240.f, 0.62f, Dim, 1);
		Text(L2, X + CW * 0.5f, Y + 266.f, 0.62f, Dim, 1);

		if (bOpen)
		{
			const int32 St = Save->WorldStars(W);
			Star(X + CW * 0.5f - 50.f, Y + 345.f, 24.f, Gold);
			Text(FString::Printf(TEXT("%d / 30"), St), X + CW * 0.5f - 18.f, Y + 328.f, 1.f, Gold);
			int32 Done = 0;
			for (int32 L = 0; L < KulusturLevelsPerWorld; ++L) if (Save->GetBestTime(W, L) > 0.f) ++Done;
			// 10 bölmeli ilerleme
			for (int32 L = 0; L < 10; ++L)
			{
				Rect(X + 30.f + L * 26.f, Y + 395.f, 20.f, 12.f, L < Done ? S.UIColor : FLinearColor(1.f, 1.f, 1.f, 0.15f));
			}
			Button(*FString::Printf(TEXT("World%d"), W), Done >= 10 ? TEXT("TEKRAR OYNA") : TEXT("GİR"), X + 30.f, Y + CH - 90.f, CW - 60.f, 64.f, S.UIColor, true, 1.1f);
		}
		else
		{
			Lock(X + CW * 0.5f, Y + 360.f, 60.f, Grey);
			Text(TEXT("KİLİTLİ"), X + CW * 0.5f, Y + 420.f, 0.9f, Dim, 1);
			Text(FString::Printf(TEXT("Dünya %d'i bitir"), W), X + CW * 0.5f, Y + 455.f, 0.65f, Dim, 1);
		}
		if (bSel) Frame(X - 4.f, Y - 4.f, CW + 8.f, CH + 8.f, 4.f, bOpen ? S.UIColor : Grey);
	}

	Button(TEXT("Back"), TEXT("< GERİ"), 60.f, 960.f, 240.f, 64.f, FLinearColor(0.3f, 0.32f, 0.4f));
	Text(FString::Printf(TEXT("Toplam yıldız: %d / 150"), Save->TotalStars()), 1860.f, 975.f, 1.f, Gold, 2);
}

void AKulusturHUD::DrawLevels(AKulusturDirector* D)
{
	UKulusturSave* Save = UKulusturSave::Get();
	const int32 W = D->GetSelectedWorld();
	const FKulusturBiomeStyle& S = KulusturGetBiomeStyle((EKulusturBiome)W);
	Rect(-GOX / U, 0.f, Canvas->ClipX / U, 1080.f, FLinearColor(0.01f, 0.02f, 0.05f, 0.68f));

	Text(FString::Printf(TEXT("DÜNYA %d"), W + 1), 80.f, 40.f, 1.f, S.UIColor);
	Text(S.Name.ToString(), 80.f, 72.f, 2.2f, TextC);
	Text(S.Subtitle.ToString(), 80.f, 150.f, 0.8f, Dim);
	Star(1660.f, 85.f, 26.f, Gold);
	Text(FString::Printf(TEXT("%d / 30"), Save->WorldStars(W)), 1700.f, 64.f, 1.3f, Gold);

	// Kıvrımlı yol üzerinde 10 durak
	auto NodePos = [](int32 L) -> FVector2D
	{
		const float X = 150.f + L * 128.f;
		const float Y = 470.f + FMath::Sin(L * 1.1f) * 150.f;
		return FVector2D(X, Y);
	};
	for (int32 L = 0; L < 9; ++L)
	{
		const FVector2D A = NodePos(L), B = NodePos(L + 1);
		Line(A.X, A.Y, B.X, B.Y, 34.f, FLinearColor(0.04f, 0.04f, 0.05f, 0.95f));
	}
	for (int32 L = 0; L < 9; ++L)
	{
		const FVector2D A = NodePos(L), B = NodePos(L + 1);
		const bool bDone = Save->GetBestTime(W, L) > 0.f;
		for (int32 k = 1; k < 4; k += 2)
		{
			const FVector2D P0 = FMath::Lerp(A, B, k / 4.f - 0.08f), P1 = FMath::Lerp(A, B, k / 4.f + 0.08f);
			Line(P0.X, P0.Y, P1.X, P1.Y, 4.f, bDone ? S.RoadLine : FLinearColor(1.f, 1.f, 1.f, 0.25f));
		}
	}
	for (int32 L = 0; L < KulusturLevelsPerWorld; ++L)
	{
		const FVector2D P = NodePos(L);
		const bool bOpen = Save->IsLevelUnlocked(W, L);
		const bool bDone = Save->GetBestTime(W, L) > 0.f;
		const bool bSel = D->GetSelectedLevel() == L;
		const bool bBoss = L == KulusturLevelsPerWorld - 1;
		const float R = bBoss ? 52.f : 42.f;
		const bool bHover = bOpen && IsHover(P.X - R, P.Y - R, R * 2.f, R * 2.f);
		if (bSel)
		{
			Circle(P.X, P.Y, R + 12.f + 4.f * FMath::Sin(Time * 4.f), WithA(S.UIColor, 0.45f));
		}
		Circle(P.X, P.Y, R + 5.f, bHover ? FLinearColor::White : FLinearColor(0.f, 0.f, 0.f, 0.8f));
		Circle(P.X, P.Y, R, bOpen ? (bDone ? S.UIColor : S.UIColor * 0.55f) : Grey * 0.7f);
		if (bOpen)
		{
			Text(FString::Printf(TEXT("%d"), L + 1), P.X, P.Y - 22.f, 1.4f, FLinearColor::White, 1);
			const int32 St = Save->GetStars(W, L);
			for (int32 s = 0; s < 3; ++s)
			{
				Star(P.X - 30.f + s * 30.f, P.Y + R + 22.f, 13.f, s < St ? Gold : FLinearColor(0.f, 0.f, 0.f, 0.6f));
			}
			FButton B;
			B.Box = FBox2D(FVector2D(GOX + (P.X - R) * U, (P.Y - R) * U), FVector2D(GOX + (P.X + R) * U, (P.Y + R) * U));
			B.Id = *FString::Printf(TEXT("Level%d"), L);
			Buttons.Add(B);
		}
		else
		{
			Lock(P.X, P.Y + 4.f, 30.f, FLinearColor(0.15f, 0.15f, 0.18f));
		}
		if (bBoss)
		{
			Text(TEXT("FİNAL"), P.X, P.Y - R - 38.f, 0.7f, Gold, 1);
		}
	}
	// Seçili durağın üstünde küçük tır
	{
		const FVector2D P = NodePos(D->GetSelectedLevel());
		const float Bob = FMath::Sin(Time * 6.f) * 3.f;
		const float TY = P.Y - 95.f + Bob;
		Rect(P.X - 34.f, TY, 46.f, 30.f, Accent);
		Rect(P.X + 12.f, TY + 8.f, 22.f, 22.f, FLinearColor::White);
		Rect(P.X + 26.f, TY + 11.f, 6.f, 9.f, FLinearColor(0.3f, 0.6f, 0.9f));
		Circle(P.X - 22.f, TY + 32.f, 7.f, FLinearColor::Black, 12);
		Circle(P.X + 22.f, TY + 32.f, 7.f, FLinearColor::Black, 12);
	}

	// Bilgi paneli
	const int32 L = D->GetSelectedLevel();
	const FKulusturLevelConfig C = KulusturMakeLevelConfig(W, L);
	const float PX = 1420.f, PY = 230.f, PW = 440.f, PH = 640.f;
	Rect(PX, PY, PW, PH, Panel);
	Rect(PX, PY, PW, 6.f, S.UIColor);
	Text(FString::Printf(TEXT("BÖLÜM %d-%d"), W + 1, L + 1), PX + 24.f, PY + 20.f, 0.9f, S.UIColor);
	Text(C.Title.ToString(), PX + 24.f, PY + 52.f, 1.3f, TextC);
	// İpucu: kelimelerle satırlara böl
	{
		TArray<FString> Words;
		C.Hint.ToString().ParseIntoArray(Words, TEXT(" "));
		FString Cur;
		float Y = PY + 115.f;
		for (const FString& Wd : Words)
		{
			const FString Try = Cur.IsEmpty() ? Wd : Cur + TEXT(" ") + Wd;
			if (Measure(Try, 0.7f).X > PW - 48.f)
			{
				Text(Cur, PX + 24.f, Y, 0.7f, Dim);
				Y += 30.f;
				Cur = Wd;
			}
			else Cur = Try;
		}
		if (!Cur.IsEmpty()) Text(Cur, PX + 24.f, Y, 0.7f, Dim);
	}
	const FString Ch = KulusturChallengeText(C.Challenges).ToString();
	Text(TEXT("Zorluklar:"), PX + 24.f, PY + 250.f, 0.75f, Accent);
	Text(Ch.IsEmpty() ? TEXT("Yok - ısınma turu") : Ch, PX + 24.f, PY + 282.f, 0.65f, TextC);
	Text(FString::Printf(TEXT("Yol: %d m"), FMath::RoundToInt(C.RoadLength / 100.f)), PX + 24.f, PY + 330.f, 0.75f, TextC);
	Text(FString::Printf(TEXT("Yıldız süresi: %s"), *AKulusturDirector::FormatTime(C.ParTime * 0.95f)), PX + 24.f, PY + 365.f, 0.75f, TextC);
	const float Best = Save->GetBestTime(W, L);
	Text(Best > 0.f ? FString::Printf(TEXT("En iyi: %s"), *AKulusturDirector::FormatTime(Best)) : FString(TEXT("En iyi: -")), PX + 24.f, PY + 400.f, 0.75f, Gold);
	for (int32 s = 0; s < 3; ++s) Star(PX + 60.f + s * 60.f, PY + 470.f, 24.f, s < Save->GetStars(W, L) ? Gold : FLinearColor(1.f, 1.f, 1.f, 0.12f));
	Text(TEXT("Yıldız: 1 bitir, 2 süre, 3 altınların yarısı"), PX + 24.f, PY + 505.f, 0.55f, Dim);
	Button(TEXT("Start"), TEXT("OYNA >"), PX + 24.f, PY + PH - 96.f, PW - 48.f, 72.f, Accent, Save->IsLevelUnlocked(W, L), 1.4f);

	Button(TEXT("Back"), TEXT("< DÜNYALAR"), 60.f, 960.f, 300.f, 64.f, FLinearColor(0.3f, 0.32f, 0.4f));
	Text(TEXT("< > seç   Enter: oyna   Esc: geri"), 960.f, 985.f, 0.7f, Dim, 1);
}

void AKulusturHUD::DrawLeaderboard(AKulusturDirector* D)
{
	UKulusturSave* Save = UKulusturSave::Get();
	Rect(-GOX / U, 0.f, Canvas->ClipX / U, 1080.f, FLinearColor(0.01f, 0.02f, 0.05f, 0.7f));
	Title(TEXT("LİDERLİK TABLOSU"), 50.f, Gold);
	Text(TEXT("Sonsuz modda en uzağa gidenler"), 960.f, 150.f, 0.85f, Dim, 1);

	const float X = 460.f, W = 1000.f;
	float Y = 220.f;
	Rect(X, Y, W, 50.f, PanelLight);
	Text(TEXT("#"), X + 30.f, Y + 10.f, 0.85f, Dim);
	Text(TEXT("SÜRÜCÜ"), X + 100.f, Y + 10.f, 0.85f, Dim);
	Text(TEXT("MESAFE"), X + 600.f, Y + 10.f, 0.85f, Dim, 2);
	Text(TEXT("ALTIN"), X + 760.f, Y + 10.f, 0.85f, Dim, 2);
	Text(TEXT("TARİH"), X + W - 30.f, Y + 10.f, 0.85f, Dim, 2);
	Y += 60.f;
	if (Save->Leaderboard.Num() == 0)
	{
		Text(TEXT("Henüz skor yok. Sonsuz modu dene!"), 960.f, Y + 100.f, 1.1f, TextC, 1);
	}
	for (int32 i = 0; i < Save->Leaderboard.Num(); ++i)
	{
		const FKulusturScore& Sc = Save->Leaderboard[i];
		const FLinearColor RC = i == 0 ? Gold : (i == 1 ? FLinearColor(0.8f, 0.82f, 0.88f) : (i == 2 ? FLinearColor(0.85f, 0.5f, 0.25f) : TextC));
		const bool bLast = D->GetMode() == EKulusturMode::Endless && i == D->GetEndlessRank();
		Rect(X, Y, W, 54.f, bLast ? WithA(Accent, 0.4f) : (i % 2 ? Panel : PanelLight));
		if (i < 3) Star(X + 38.f, Y + 27.f, 16.f, RC);
		else Text(FString::Printf(TEXT("%d"), i + 1), X + 38.f, Y + 10.f, 0.95f, RC, 1);
		Text(Sc.Name, X + 100.f, Y + 10.f, 0.95f, RC);
		Text(FString::Printf(TEXT("%d m"), Sc.Meters), X + 600.f, Y + 10.f, 0.95f, RC, 2);
		Text(FString::Printf(TEXT("%d"), Sc.Coins), X + 760.f, Y + 10.f, 0.95f, Gold, 2);
		Text(Sc.Date, X + W - 30.f, Y + 12.f, 0.75f, Dim, 2);
		Y += 58.f;
	}
	Button(TEXT("Back"), TEXT("< GERİ"), 60.f, 960.f, 240.f, 64.f, FLinearColor(0.3f, 0.32f, 0.4f));
	Button(TEXT("Endless"), TEXT("SONSUZ MODA GİR >"), 1520.f, 960.f, 340.f, 64.f, FLinearColor(0.75f, 0.15f, 0.45f));
}

void AKulusturHUD::DrawVolumeRow(const FString& Label, float Value, FName Down, FName Up, float X, float Y)
{
	Text(Label, X, Y + 12.f, 1.f, TextC);
	Button(Down, TEXT("-"), X + 300.f, Y, 64.f, 56.f, FLinearColor(0.3f, 0.32f, 0.4f), true, 1.3f);
	for (int32 i = 0; i < 10; ++i)
	{
		Rect(X + 380.f + i * 30.f, Y + 8.f, 24.f, 40.f, i < FMath::RoundToInt(Value * 10.f) ? Accent : FLinearColor(1.f, 1.f, 1.f, 0.12f));
	}
	Button(Up, TEXT("+"), X + 690.f, Y, 64.f, 56.f, FLinearColor(0.3f, 0.32f, 0.4f), true, 1.3f);
	Text(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Value * 100.f)), X + 780.f, Y + 12.f, 1.f, Dim);
}

void AKulusturHUD::DrawSettings(AKulusturDirector* D)
{
	UKulusturSave* Save = UKulusturSave::Get();
	Rect(-GOX / U, 0.f, Canvas->ClipX / U, 1080.f, FLinearColor(0.01f, 0.02f, 0.05f, 0.7f));
	Title(TEXT("AYARLAR"), 80.f, TextC);
	Rect(500.f, 250.f, 920.f, 420.f, Panel);
	DrawVolumeRow(TEXT("Ana ses"), Save->MasterVolume, TEXT("VolDown"), TEXT("VolUp"), 540.f, 290.f);
	DrawVolumeRow(TEXT("Müzik"), Save->MusicVolume, TEXT("MusDown"), TEXT("MusUp"), 540.f, 380.f);
	Text(FString::Printf(TEXT("Sürücü adı: %s"), *Save->PlayerName), 540.f, 500.f, 1.f, TextC);
	Button(TEXT("Name"), TEXT("DEĞİŞTİR"), 1100.f, 488.f, 260.f, 56.f, FLinearColor(0.2f, 0.45f, 0.7f));
	Button(TEXT("Back"), TEXT("< GERİ"), 60.f, 960.f, 240.f, 64.f, FLinearColor(0.3f, 0.32f, 0.4f));
}

void AKulusturHUD::DrawHelp(AKulusturDirector* D)
{
	Rect(-GOX / U, 0.f, Canvas->ClipX / U, 1080.f, FLinearColor(0.01f, 0.02f, 0.05f, 0.78f));
	Title(TEXT("NASIL OYNANIR"), 50.f, TextC);
	struct FL { const TCHAR* K; const TCHAR* V; };
	const FL Lines[] = {
		{ TEXT("W A S D"), TEXT("Koş / yön değiştir") },
		{ TEXT("Fare"), TEXT("Kamera") },
		{ TEXT("Boşluk"), TEXT("Zıpla - havada tekrar bas: çift zıplama") },
		{ TEXT("Shift"), TEXT("Depar") },
		{ TEXT("E"), TEXT("Atılma: gittiğin yöne fırla (enerji)") },
		{ TEXT("Q (basılı)"), TEXT("Ağır çekim (enerji harcar)") },
		{ TEXT("Ctrl / C"), TEXT("Havadan aşağı çakıl") },
		{ TEXT("R"), TEXT("Bölümü yeniden başlat") },
		{ TEXT("Esc / P"), TEXT("Duraklat") } };
	float Y = 190.f;
	for (const FL& L : Lines)
	{
		Rect(380.f, Y, 220.f, 48.f, PanelLight);
		Text(L.K, 490.f, Y + 8.f, 0.9f, Gold, 1);
		Text(L.V, 630.f, Y + 8.f, 0.9f, TextC);
		Y += 56.f;
	}
	Y += 20.f;
	const TCHAR* Tips[] = {
		TEXT("| Yere değersen ölürsün. Araçların üstünde kal, BİTİŞ kapısından geç."),
		TEXT("| Yeşil rampalı FIRLATICI araçlar seni ileri fırlatır; YAYLI tırlar yukarı."),
		TEXT("| Turuncu raylara ve halatlara tutun, sarı kulelerdeki makaralar seni yukarı çeker."),
		TEXT("| Mavi hız halkalarından geç, altınları topla. TANKERLER çarpınca patlar!"),
		TEXT("| Kırmızı lazerlerin üstünden atla. Şerit duvarında açık şeride geç."),
		TEXT("| Havada takla, atılma, çakılma yapıp tıra in: AKROBASİ enerjini doldurur.") };
	for (const TCHAR* T : Tips)
	{
		Text(T, 380.f, Y, 0.85f, Dim);
		Y += 44.f;
	}
	Button(TEXT("Back"), TEXT("< GERİ"), 60.f, 960.f, 240.f, 64.f, FLinearColor(0.3f, 0.32f, 0.4f));
}

void AKulusturHUD::DrawNameEntry(AKulusturDirector* D)
{
	Rect(-GOX / U, 0.f, Canvas->ClipX / U, 1080.f, FLinearColor(0.01f, 0.02f, 0.05f, 0.75f));
	Title(TEXT("SÜRÜCÜ ADIN NE?"), 260.f, Accent);
	Rect(610.f, 440.f, 700.f, 100.f, PanelLight);
	Frame(610.f, 440.f, 700.f, 100.f, 4.f, Accent);
	const bool bCaret = FMath::Fmod(Time, 1.f) < 0.5f;
	Text(D->GetNameBuffer() + (bCaret ? TEXT("_") : TEXT(" ")), 960.f, 462.f, 1.8f, TextC, 1);
	Text(TEXT("Klavyeyle yaz (A-Z, 0-9). Liderlik tablosunda bu isim görünür."), 960.f, 570.f, 0.8f, Dim, 1);
	Button(TEXT("NameOK"), TEXT("TAMAM"), 810.f, 650.f, 300.f, 72.f, Accent, !D->GetNameBuffer().TrimStartAndEnd().IsEmpty(), 1.3f);
}

void AKulusturHUD::DrawPause(AKulusturDirector* D)
{
	UKulusturSave* Save = UKulusturSave::Get();
	Rect(-GOX / U, 0.f, Canvas->ClipX / U, 1080.f, FLinearColor(0.f, 0.f, 0.02f, 0.6f));
	Title(TEXT("DURAKLATILDI"), 160.f, TextC);
	const float X = 760.f, W = 400.f, H = 70.f;
	float Y = 330.f;
	Button(TEXT("Resume"), TEXT("DEVAM ET"), X, Y, W, H, Accent, true, 1.2f); Y += H + 20.f;
	Button(TEXT("Retry"), TEXT("YENİDEN BAŞLA"), X, Y, W, H, FLinearColor(0.2f, 0.45f, 0.7f), true, 1.1f); Y += H + 20.f;
	if (D->GetMode() == EKulusturMode::Level)
	{
		Button(TEXT("Map"), TEXT("BÖLÜM HARİTASI"), X, Y, W, H, FLinearColor(0.2f, 0.5f, 0.35f), true, 1.1f); Y += H + 20.f;
	}
	Button(TEXT("Menu"), TEXT("ANA MENÜ"), X, Y, W, H, FLinearColor(0.3f, 0.32f, 0.4f), true, 1.1f); Y += H + 50.f;
	DrawVolumeRow(TEXT("Ana ses"), Save->MasterVolume, TEXT("VolDown"), TEXT("VolUp"), 520.f, Y);
	DrawVolumeRow(TEXT("Müzik"), Save->MusicVolume, TEXT("MusDown"), TEXT("MusUp"), 520.f, Y + 76.f);
}

void AKulusturHUD::DrawResults(AKulusturDirector* D)
{
	UKulusturSave* Save = UKulusturSave::Get();
	const FKulusturBiomeStyle& S = KulusturGetBiomeStyle(D->GetCurrentBiome());
	Rect(-GOX / U, 0.f, Canvas->ClipX / U, 1080.f, FLinearColor(0.f, 0.f, 0.02f, 0.55f));
	const float PX = 560.f, PY = 110.f, PW = 800.f, PH = 820.f;
	Rect(PX + 10.f, PY + 10.f, PW, PH, FLinearColor(0.f, 0.f, 0.f, 0.5f));
	Rect(PX, PY, PW, PH, Panel);
	Rect(PX, PY, PW, 8.f, S.UIColor);
	Text(FString::Printf(TEXT("BÖLÜM %d-%d"), D->GetWorldIndex() + 1, D->GetLevelIndex() + 1), 960.f, PY + 30.f, 0.9f, S.UIColor, 1);
	Text(TEXT("TAMAMLANDI!"), 960.f, PY + 60.f, 2.4f, TextC, 1);

	// Yıldızlar sırayla belirir
	const float T = D->GetStateTime() - 1.6f;
	for (int32 s = 0; s < 3; ++s)
	{
		const bool bGot = s < D->GetResultStars();
		const float Appear = FMath::Clamp((T - s * 0.35f) / 0.25f, 0.f, 1.f);
		const float R = (s == 1 ? 62.f : 50.f) * (bGot ? (1.f + (1.f - Appear) * 0.6f) : 1.f);
		const float SX = 960.f + (s - 1) * 150.f;
		const float SY = PY + 240.f - (s == 1 ? 20.f : 0.f);
		Star(SX, SY, R + 6.f, FLinearColor(0.f, 0.f, 0.f, 0.6f));
		Star(SX, SY, R, bGot ? WithA(Gold, Appear) : FLinearColor(1.f, 1.f, 1.f, 0.12f));
	}
	const FKulusturLevelConfig& C = D->GetConfig();
	float Y = PY + 340.f;
	auto Row = [&](const FString& L, const FString& V, const FLinearColor& Col, bool bOk)
	{
		Rect(PX + 60.f, Y, PW - 120.f, 50.f, PanelLight);
		Text(L, PX + 80.f, Y + 10.f, 0.9f, TextC);
		Text(V, PX + PW - 80.f, Y + 10.f, 0.9f, Col, 2);
		if (bOk) Star(PX + PW - 40.f, Y + 25.f, 14.f, Gold);
		Y += 58.f;
	};
	Row(TEXT("Süre"), AKulusturDirector::FormatTime(D->GetElapsed()) + (D->WasNewRecord() ? TEXT("  YENİ REKOR!") : TEXT("")), D->WasNewRecord() ? Gold : TextC,
		D->GetElapsed() <= C.ParTime * 0.95f);
	Row(TEXT("Yıldız süresi"), AKulusturDirector::FormatTime(C.ParTime * 0.95f), Dim, false);
	Row(TEXT("Altın"), FString::Printf(TEXT("%d / %d"), D->GetCoins(), D->GetTotalCoins()), Gold, D->GetTotalCoins() == 0 || D->GetCoins() * 2 >= D->GetTotalCoins());
	Row(TEXT("Ölüm"), FString::Printf(TEXT("%d"), D->GetDeaths()), D->GetDeaths() == 0 ? Good : TextC, false);

	if (D->WasNewWorldUnlocked())
	{
		const int32 NW = FMath::Min(D->GetWorldIndex() + 1, KulusturNumWorlds - 1);
		const FKulusturBiomeStyle& N = KulusturGetBiomeStyle((EKulusturBiome)NW);
		const float A = 0.7f + 0.3f * FMath::Sin(Time * 5.f);
		Rect(PX + 60.f, Y + 6.f, PW - 120.f, 64.f, WithA(N.UIColor, 0.35f));
		Text(FString::Printf(TEXT("YENİ DÜNYA AÇILDI: %s"), *N.Name.ToString()), 960.f, Y + 20.f, 1.1f, WithA(N.UIColor * 1.3f, A), 1);
		Y += 76.f;
	}

	const float BY = PY + PH - 110.f;
	Button(TEXT("Retry"), TEXT("TEKRAR"), PX + 50.f, BY, 210.f, 72.f, FLinearColor(0.2f, 0.45f, 0.7f), true, 1.1f);
	Button(TEXT("Map"), TEXT("HARİTA"), PX + 290.f, BY, 210.f, 72.f, FLinearColor(0.3f, 0.32f, 0.4f), true, 1.1f);
	Button(TEXT("Next"), TEXT("SONRAKİ >"), PX + 530.f, BY, 220.f, 72.f, Accent, true, 1.2f);
}

void AKulusturHUD::DrawGameComplete(AKulusturDirector* D)
{
	UKulusturSave* Save = UKulusturSave::Get();
	Rect(-GOX / U, 0.f, Canvas->ClipX / U, 1080.f, FLinearColor(0.f, 0.f, 0.02f, 0.65f));
	// Konfeti
	for (int32 i = 0; i < 60; ++i)
	{
		const float X = FMath::Fmod(i * 137.f + Time * (40.f + (i % 7) * 10.f), 1920.f);
		const float Y = FMath::Fmod(i * 71.f + Time * (120.f + (i % 5) * 30.f), 1080.f);
		const FLinearColor C = KulusturGetBiomeStyle((EKulusturBiome)(i % 5)).UIColor;
		Rect(X, Y, 12.f, 18.f, C);
	}
	Title(TEXT("TEBRİKLER!"), 140.f, Gold);
	Text(TEXT("5 dünyanın 50 bölümünü de bitirdin. Gerçek bir KULÜSTÜR sürücüsüsün!"), 960.f, 260.f, 1.f, TextC, 1);
	for (int32 W = 0; W < KulusturNumWorlds; ++W)
	{
		const FKulusturBiomeStyle& S = KulusturGetBiomeStyle((EKulusturBiome)W);
		const float X = 360.f + W * 250.f;
		Circle(X + 100.f, 400.f, 60.f, S.UIColor);
		Text(FString::Printf(TEXT("%d"), W + 1), X + 100.f, 370.f, 1.6f, FLinearColor::White, 1);
		Star(X + 70.f, 495.f, 16.f, Gold);
		Text(FString::Printf(TEXT("%d/30"), Save->WorldStars(W)), X + 92.f, 480.f, 0.8f, Gold);
	}
	Star(860.f, 600.f, 30.f, Gold);
	Text(FString::Printf(TEXT("%d / 150"), Save->TotalStars()), 900.f, 578.f, 1.5f, Gold);
	Text(TEXT("Şimdi ne yapmak istersin?"), 960.f, 690.f, 1.f, Dim, 1);
	Button(TEXT("Replay"), TEXT("BAŞTAN OYNA"), 470.f, 760.f, 300.f, 80.f, FLinearColor(0.2f, 0.45f, 0.7f), true, 1.1f);
	Button(TEXT("Endless"), TEXT("SONSUZ MOD"), 810.f, 760.f, 300.f, 80.f, FLinearColor(0.75f, 0.15f, 0.45f), true, 1.2f);
	Button(TEXT("Menu"), TEXT("ANA MENÜ"), 1150.f, 760.f, 300.f, 80.f, FLinearColor(0.3f, 0.32f, 0.4f), true, 1.1f);
}

void AKulusturHUD::DrawEndlessOver(AKulusturDirector* D)
{
	UKulusturSave* Save = UKulusturSave::Get();
	const float A = FMath::Clamp(D->GetStateTime() / 0.6f, 0.f, 1.f);
	Rect(-GOX / U, 0.f, Canvas->ClipX / U, 1080.f, FLinearColor(0.1f, 0.f, 0.02f, 0.6f * A));
	Text(TEXT("OYUN BİTTİ"), 960.f, 150.f, 2.6f, WithA(Bad, A), 1);
	Text(FString::Printf(TEXT("%d m"), D->GetMeters()), 960.f, 280.f, 4.f, WithA(TextC, A), 1);
	Circle(900.f, 470.f, 16.f, Gold);
	Text(FString::Printf(TEXT("%d altın"), D->GetCoins()), 925.f, 452.f, 1.f, Gold);
	const int32 Rank = D->GetEndlessRank();
	FString Msg;
	if (D->WasNewRecord()) Msg = TEXT("YENİ REKOR!");
	else if (Rank >= 0) Msg = FString::Printf(TEXT("Liderlik tablosunda %d. sıra!"), Rank + 1);
	else Msg = FString::Printf(TEXT("Rekor: %d m - biraz daha!"), Save->BestEndless());
	Text(Msg, 960.f, 530.f, 1.4f, WithA(D->WasNewRecord() ? Gold : TextC, A), 1);
	Text(FString::Printf(TEXT("Ulaşılan bölge: %s"), *KulusturGetBiomeStyle(D->GetCurrentBiome()).Name.ToString()), 960.f, 600.f, 0.9f, WithA(Dim, A), 1);
	if (D->GetStateTime() > 0.8f)
	{
		Button(TEXT("Retry"), TEXT("TEKRAR >"), 560.f, 720.f, 260.f, 76.f, Accent, true, 1.2f);
		Button(TEXT("Scores"), TEXT("LİDERLİK"), 830.f, 720.f, 260.f, 76.f, FLinearColor(0.15f, 0.4f, 0.75f), true, 1.1f);
		Button(TEXT("Menu"), TEXT("MENÜ"), 1100.f, 720.f, 260.f, 76.f, FLinearColor(0.3f, 0.32f, 0.4f), true, 1.1f);
	}
}
