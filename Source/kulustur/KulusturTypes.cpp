#include "KulusturTypes.h"

#define LOCTEXT_NAMESPACE "Kulustur"

namespace
{
	TArray<FKulusturBiomeStyle> BuildStyles()
	{
		TArray<FKulusturBiomeStyle> S;
		S.SetNum(KulusturNumWorlds);

		// 1) Güneşli otoyol
		{
			FKulusturBiomeStyle& B = S[0];
			B.Name = FText::FromString(TEXT("Güneşli Otoyol"));
			B.Subtitle = FText::FromString(TEXT("Mavi gökyüzü, yeşil tepeler. Isinma turu."));
			B.Ground = FLinearColor(0.16f, 0.42f, 0.10f);
			B.GroundAlt = FLinearColor(0.10f, 0.32f, 0.06f);
			B.Road = FLinearColor(0.07f, 0.07f, 0.08f);
			B.RoadLine = FLinearColor(0.95f, 0.95f, 0.95f);
			B.UIColor = FLinearColor(0.20f, 0.70f, 0.25f);
			B.SunPitch = -50.f; B.SunYaw = 35.f; B.SunIntensity = 10.f; B.SunColor = FLinearColor(1.f, 0.97f, 0.92f);
			B.FogDensity = 0.008f; B.FogFalloff = 0.2f; B.FogColor = FLinearColor(0.55f, 0.70f, 0.95f);
			B.Saturation = 1.1f;
			B.TruckColors = { FLinearColor(0.80f, 0.10f, 0.08f), FLinearColor(0.05f, 0.25f, 0.75f), FLinearColor(0.95f, 0.55f, 0.05f),
				FLinearColor(0.10f, 0.55f, 0.20f), FLinearColor(0.55f, 0.10f, 0.60f), FLinearColor(0.95f, 0.85f, 0.10f) };
			B.VehicleMix = { { EVehicleType::Tir, 5.f }, { EVehicleType::Otobus, 2.f }, { EVehicleType::Kamyonet, 2.f }, { EVehicleType::YayliTir, 0.7f },
				{ EVehicleType::Firlatici, 0.7f }, { EVehicleType::DevTir, 1.f } };
		}
		// 2) Çöl
		{
			FKulusturBiomeStyle& B = S[1];
			B.Name = FText::FromString(TEXT("Kavurucu Çöl"));
			B.Subtitle = FText::FromString(TEXT("Kum fırtınası, rüzgar ve patlayan tankerler."));
			B.Ground = FLinearColor(0.80f, 0.58f, 0.32f);
			B.GroundAlt = FLinearColor(0.70f, 0.47f, 0.24f);
			B.Road = FLinearColor(0.16f, 0.13f, 0.11f);
			B.RoadLine = FLinearColor(0.95f, 0.80f, 0.20f);
			B.UIColor = FLinearColor(0.95f, 0.60f, 0.15f);
			B.SunPitch = -60.f; B.SunYaw = 10.f; B.SunIntensity = 12.f; B.SunColor = FLinearColor(1.f, 0.88f, 0.70f);
			B.FogDensity = 0.02f; B.FogFalloff = 0.25f; B.FogColor = FLinearColor(0.90f, 0.70f, 0.45f);
			B.Weather = EKulusturWeather::Sand; B.WeatherAmount = 0.5f;
			B.Saturation = 1.05f; B.Tint = FLinearColor(1.f, 0.95f, 0.85f);
			B.TruckColors = { FLinearColor(0.85f, 0.85f, 0.82f), FLinearColor(0.60f, 0.15f, 0.08f), FLinearColor(0.20f, 0.35f, 0.55f),
				FLinearColor(0.40f, 0.30f, 0.20f), FLinearColor(0.95f, 0.65f, 0.10f) };
			B.VehicleMix = { { EVehicleType::Tir, 4.f }, { EVehicleType::Tanker, 3.f }, { EVehicleType::Kamyonet, 2.f }, { EVehicleType::Hoplayan, 1.f },
				{ EVehicleType::YayliTir, 0.8f }, { EVehicleType::DevTir, 1.f } };
		}
		// 3) Kar
		{
			FKulusturBiomeStyle& B = S[2];
			B.Name = FText::FromString(TEXT("Karlı Dağlar"));
			B.Subtitle = FText::FromString(TEXT("Kaygan tavanlar, kar yağışı ve lazerler."));
			B.Ground = FLinearColor(0.88f, 0.90f, 0.95f);
			B.GroundAlt = FLinearColor(0.75f, 0.80f, 0.88f);
			B.Road = FLinearColor(0.22f, 0.24f, 0.28f);
			B.RoadLine = FLinearColor(0.95f, 0.95f, 1.f);
			B.UIColor = FLinearColor(0.45f, 0.75f, 1.f);
			B.SunPitch = -30.f; B.SunYaw = 60.f; B.SunIntensity = 7.f; B.SunColor = FLinearColor(0.85f, 0.90f, 1.f);
			B.FogDensity = 0.03f; B.FogFalloff = 0.15f; B.FogColor = FLinearColor(0.80f, 0.85f, 0.95f);
			B.Weather = EKulusturWeather::Snow; B.WeatherAmount = 0.8f;
			B.Saturation = 0.9f; B.Tint = FLinearColor(0.92f, 0.96f, 1.05f); B.ExposureBias = -0.3f;
			B.TruckColors = { FLinearColor(0.75f, 0.08f, 0.08f), FLinearColor(0.10f, 0.20f, 0.50f), FLinearColor(0.15f, 0.45f, 0.30f),
				FLinearColor(0.95f, 0.50f, 0.05f), FLinearColor(0.25f, 0.25f, 0.28f) };
			B.VehicleMix = { { EVehicleType::Tir, 5.f }, { EVehicleType::Otobus, 1.5f }, { EVehicleType::DevTir, 2.f }, { EVehicleType::YayliTir, 1.f },
				{ EVehicleType::Firlatici, 1.f }, { EVehicleType::Kamyonet, 1.f } };
			B.bIcyDefault = true;
		}
		// 4) Yağmur
		{
			FKulusturBiomeStyle& B = S[3];
			B.Name = FText::FromString(TEXT("Fırtınalı Vadi"));
			B.Subtitle = FText::FromString(TEXT("Sağanak, şimşek ve ters dönen kontroller."));
			B.Ground = FLinearColor(0.10f, 0.20f, 0.10f);
			B.GroundAlt = FLinearColor(0.07f, 0.14f, 0.08f);
			B.Road = FLinearColor(0.03f, 0.03f, 0.04f);
			B.RoadLine = FLinearColor(0.80f, 0.80f, 0.70f);
			B.UIColor = FLinearColor(0.35f, 0.45f, 0.70f);
			B.SunPitch = -40.f; B.SunYaw = -40.f; B.SunIntensity = 2.5f; B.SunColor = FLinearColor(0.75f, 0.80f, 0.90f);
			B.SkyIntensity = 0.6f;
			B.FogDensity = 0.05f; B.FogFalloff = 0.1f; B.FogColor = FLinearColor(0.30f, 0.33f, 0.38f);
			B.Weather = EKulusturWeather::Rain; B.WeatherAmount = 1.f; B.bLightning = true;
			B.Saturation = 0.75f; B.Tint = FLinearColor(0.90f, 0.95f, 1.05f); B.ExposureBias = 0.3f;
			B.TruckColors = { FLinearColor(0.60f, 0.08f, 0.06f), FLinearColor(0.85f, 0.75f, 0.10f), FLinearColor(0.12f, 0.30f, 0.60f),
				FLinearColor(0.80f, 0.80f, 0.80f), FLinearColor(0.20f, 0.50f, 0.25f) };
			B.VehicleMix = { { EVehicleType::Tir, 4.f }, { EVehicleType::Otobus, 2.f }, { EVehicleType::Zigzag, 1.5f }, { EVehicleType::Hoplayan, 1.f },
				{ EVehicleType::YayliTir, 0.8f }, { EVehicleType::Kamyonet, 1.f } };
		}
		// 5) Şehir
		{
			FKulusturBiomeStyle& B = S[4];
			B.Name = FText::FromString(TEXT("Neon Şehir"));
			B.Subtitle = FText::FromString(TEXT("Gün batımında gökdelenler arasında her şey bir arada."));
			B.Ground = FLinearColor(0.20f, 0.20f, 0.22f);
			B.GroundAlt = FLinearColor(0.35f, 0.35f, 0.38f);
			B.Road = FLinearColor(0.05f, 0.05f, 0.06f);
			B.RoadLine = FLinearColor(1.f, 0.85f, 0.25f);
			B.UIColor = FLinearColor(0.85f, 0.25f, 0.85f);
			B.SunPitch = -7.f; B.SunYaw = -10.f; B.SunIntensity = 5.f; B.SunColor = FLinearColor(1.f, 0.60f, 0.35f);
			B.SkyIntensity = 0.8f;
			B.FogDensity = 0.012f; B.FogFalloff = 0.2f; B.FogColor = FLinearColor(0.50f, 0.35f, 0.42f);
			B.Saturation = 1.15f; B.Tint = FLinearColor(1.05f, 0.95f, 1.05f); B.ExposureBias = 0.4f;
			B.TruckColors = { FLinearColor(0.95f, 0.15f, 0.55f), FLinearColor(0.10f, 0.85f, 0.90f), FLinearColor(0.95f, 0.85f, 0.10f),
				FLinearColor(0.50f, 0.20f, 0.90f), FLinearColor(0.95f, 0.40f, 0.05f), FLinearColor(0.85f, 0.85f, 0.88f) };
			B.VehicleMix = { { EVehicleType::Tir, 3.f }, { EVehicleType::Otobus, 3.f }, { EVehicleType::Kamyonet, 2.f }, { EVehicleType::Zigzag, 1.5f },
				{ EVehicleType::Firlatici, 1.f }, { EVehicleType::YayliTir, 1.f }, { EVehicleType::Hoplayan, 1.f }, { EVehicleType::Tanker, 1.f } };
		}
		return S;
	}
}

const FKulusturBiomeStyle& KulusturGetBiomeStyle(EKulusturBiome Biome)
{
	static const TArray<FKulusturBiomeStyle> Styles = BuildStyles();
	return Styles[FMath::Clamp((int32)Biome, 0, KulusturNumWorlds - 1)];
}

FKulusturLevelConfig KulusturMakeLevelConfig(int32 W, int32 L)
{
	W = FMath::Clamp(W, 0, KulusturNumWorlds - 1);
	L = FMath::Clamp(L, 0, KulusturLevelsPerWorld - 1);

	FKulusturLevelConfig C;
	C.World = W;
	C.Level = L;
	C.Biome = (EKulusturBiome)W;
	const int32 D = W * KulusturLevelsPerWorld + L; // 0..49 genel zorluk
	C.Seed = 1000 + D * 7919;

	// Bitiş uzakta olsun: 450 m ile 1.3 km arası
	const float ShortLength = 20000.f + L * 2600.f + W * 2500.f;
	C.RoadLength = 45000.f + L * 4500.f + W * 4000.f;
	C.BaseSpeed = 1800.f + D * 22.f;
	C.SpeedVariation = 100.f + D * 4.f;
	C.BrakeInterval = FMath::Max(2.5f, 7.f - D * 0.09f);

	C.NumRamps = 1 + L / 2 + W / 2;
	C.NumWalls = L >= 1 ? 1 + L / 3 : 0;
	C.NumRings = 2 + L / 3;
	C.NumCoinLines = 4 + L / 2;
	C.NumRails = L >= 2 ? 1 + L / 4 : 0;
	C.NumPulleys = L >= 4 ? 1 + L / 5 : 0;
	C.NumZiplines = L >= 5 ? 1 : 0;

	uint32 Ch = KChallenge::None;
	FString Hint;
	switch (C.Biome)
	{
	case EKulusturBiome::Gunesli:
		if (L == 0) Hint = TEXT("Yere değme! Tırdan tıra atla.");
		if (L == 1) Hint = TEXT("Kırmızı duvarlara dikkat: önüne duvar çıkan tır devrilir.");
		if (L == 2) Hint = TEXT("Raylara atla, üstünde kay!");
		if (L >= 3) { Ch |= KChallenge::GapWalls; C.NumGapWalls = 1 + (L - 3) / 3; }
		if (L == 3) Hint = TEXT("Yolu kapatan duvar: tek açık şeride geç!");
		if (L == 4) Hint = TEXT("Sarı makaraya tutun, seni fırlatsın.");
		if (L >= 5) Ch |= KChallenge::LaneSwerve;
		if (L == 5) Hint = TEXT("Tırlar aniden şerit değiştiriyor!");
		if (L >= 7) { Ch |= KChallenge::Lasers; C.NumLasers = 1 + (L - 7); }
		if (L == 7) Hint = TEXT("Lazerlere değme: üstünden atla.");
		if (L >= 8) Ch |= KChallenge::SpeedBurst;
		break;
	case EKulusturBiome::Col:
		Ch |= KChallenge::Wind;
		C.WindStrength = 350.f + L * 60.f;
		if (L == 0) Hint = TEXT("Rüzgar seni havada yana itiyor. Okun yönüne dikkat!");
		if (L >= 2) Ch |= KChallenge::FallingTrucks;
		if (L == 2) Hint = TEXT("Gökten tır yağıyor! Yerdeki gölgelere dikkat.");
		if (L >= 4) { Ch |= KChallenge::GapWalls; C.NumGapWalls = 1 + (L - 4) / 3; }
		if (L >= 6) Ch |= KChallenge::Fog;
		if (L == 6) Hint = TEXT("Kum fırtınası: görüş kısıtlı!");
		if (L >= 8) { Ch |= KChallenge::Lasers; C.NumLasers = L - 6; }
		if (L == 1) Hint = TEXT("Tankerler çarpınca patlar!");
		break;
	case EKulusturBiome::Kar:
		Ch |= KChallenge::Ice;
		if (L == 0) Hint = TEXT("Tavanlar buz tuttu: kayarsın, dikkatli ol!");
		if (L >= 1) { Ch |= KChallenge::Lasers; C.NumLasers = 1 + L / 3; }
		if (L == 1) Hint = TEXT("Lazerlere değme: üstünden atla.");
		if (L >= 4) { Ch |= KChallenge::GapWalls; C.NumGapWalls = 1 + (L - 4) / 3; }
		if (L >= 6) { Ch |= KChallenge::Wind; C.WindStrength = 300.f + L * 40.f; }
		if (L == 6) Hint = TEXT("Tipi! Rüzgar ve kar aynı anda.");
		if (L >= 8) Ch |= KChallenge::InvertControls;
		if (L == 8) Hint = TEXT("Dondurucu soğuk: kontroller ara ara ters dönüyor!");
		break;
	case EKulusturBiome::Yagmur:
		if (L >= 1) Ch |= KChallenge::InvertControls;
		if (L == 0) Hint = TEXT("Sağanak yağmur: görüş az, dikkatli ol.");
		if (L == 1) Hint = TEXT("Şimşek çakınca kontroller ters dönebilir!");
		if (L >= 3) { Ch |= KChallenge::Lasers; C.NumLasers = 1 + (L - 3) / 2; }
		if (L >= 5) Ch |= KChallenge::LaneSwerve;
		if (L >= 7) Ch |= KChallenge::FallingTrucks;
		if (L >= 9) Ch |= KChallenge::Darkness | KChallenge::Fog;
		if (L == 9) Hint = TEXT("Zifiri karanlık fırtına: son sınav!");
		break;
	case EKulusturBiome::Sehir:
		Ch |= KChallenge::LaneSwerve;
		if (L == 0) Hint = TEXT("Şehir trafiği: herkes şerit değiştiriyor!");
		if (L >= 2) Ch |= KChallenge::InvertControls;
		if (L >= 3) { Ch |= KChallenge::Lasers; C.NumLasers = 1 + (L - 3) / 2; }
		if (L >= 4) { Ch |= KChallenge::GapWalls; C.NumGapWalls = 1 + (L - 4) / 3; }
		if (L >= 6) Ch |= KChallenge::SpeedBurst;
		if (L >= 7) Ch |= KChallenge::FallingTrucks;
		if (L >= 9) { Ch |= KChallenge::Wind; C.WindStrength = 500.f; }
		if (L == 9) Hint = TEXT("Final! Her şey bir arada.");
		break;
	}
	C.Challenges = Ch;

	// Son bölümler daha uzun ve kalabalık
	if (L == KulusturLevelsPerWorld - 1)
	{
		C.RoadLength += 15000.f;
		C.NumRails += 1;
		C.NumPulleys += 1;
	}

	// Yol uzadığı için engel ve parça sayılarını da orantılı artır
	{
		const float S = C.RoadLength / ShortLength;
		auto Sc = [S](int32 N) { return FMath::RoundToInt(N * S); };
		C.NumRamps = Sc(C.NumRamps); C.NumWalls = Sc(C.NumWalls); C.NumRings = Sc(C.NumRings); C.NumCoinLines = Sc(C.NumCoinLines);
		C.NumRails = Sc(C.NumRails); C.NumPulleys = Sc(C.NumPulleys); C.NumZiplines = Sc(C.NumZiplines);
		C.NumLasers = Sc(C.NumLasers); C.NumGapWalls = FMath::Min(Sc(C.NumGapWalls), 4);
	}
	C.ParTime = C.RoadLength / C.BaseSpeed;
	C.Title = FText::FromString(FString::Printf(TEXT("%s  %d-%d"), *KulusturGetBiomeStyle(C.Biome).Name.ToString(), W + 1, L + 1));
	C.Hint = FText::FromString(Hint);
	return C;
}

EKulusturBiome KulusturEndlessBiomeAt(float X)
{
	const int32 Seg = FMath::Max(0, FMath::FloorToInt(X / KulusturEndlessBiomeLength));
	return (EKulusturBiome)(Seg % KulusturNumWorlds);
}

FKulusturLevelConfig KulusturMakeEndlessConfig(float DistanceCm)
{
	const float M = FMath::Max(0.f, DistanceCm / 100.f);
	const EKulusturBiome B = KulusturEndlessBiomeAt(DistanceCm);
	const int32 PseudoLevel = FMath::Clamp(FMath::FloorToInt(M / 250.f), 0, KulusturLevelsPerWorld - 1);
	FKulusturLevelConfig C = KulusturMakeLevelConfig((int32)B, PseudoLevel);
	C.BaseSpeed = FMath::Min(1900.f + M * 0.35f, 3200.f);
	C.BrakeInterval = FMath::Max(2.2f, 6.5f - M / 600.f);
	C.Title = FText::FromString(TEXT("Sonsuz Yol"));
	return C;
}

FText KulusturChallengeText(uint32 Ch)
{
	TArray<FString> Parts;
	if (Ch & KChallenge::Lasers) Parts.Add(TEXT("Lazer"));
	if (Ch & KChallenge::InvertControls) Parts.Add(TEXT("Ters kontrol"));
	if (Ch & KChallenge::Wind) Parts.Add(TEXT("Rüzgar"));
	if (Ch & KChallenge::Ice) Parts.Add(TEXT("Buz"));
	if (Ch & KChallenge::FallingTrucks) Parts.Add(TEXT("Gökten tır"));
	if (Ch & KChallenge::Fog) Parts.Add(TEXT("Sis"));
	if (Ch & KChallenge::GapWalls) Parts.Add(TEXT("Zorunlu şerit"));
	if (Ch & KChallenge::LaneSwerve) Parts.Add(TEXT("Şerit değiştiren tırlar"));
	if (Ch & KChallenge::SpeedBurst) Parts.Add(TEXT("Ani hızlanma"));
	if (Ch & KChallenge::Darkness) Parts.Add(TEXT("Karanlık"));
	return FText::FromString(FString::Join(Parts, TEXT("  |  ")));
}

#undef LOCTEXT_NAMESPACE
