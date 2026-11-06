#pragma once

#include "CoreMinimal.h"
#include "KulusturTypes.generated.h"

UENUM()
enum class EKulusturMode : uint8
{
	Menu,
	Level,
	Endless
};

UENUM()
enum class EKulusturBiome : uint8
{
	Gunesli,
	Col,
	Kar,
	Yagmur,
	Sehir
};

constexpr int32 KulusturNumWorlds = 5;
constexpr int32 KulusturLevelsPerWorld = 10;

UENUM()
enum class EKulusturWeather : uint8
{
	None,
	Rain,
	Snow,
	Sand
};

UENUM()
enum class EVehicleType : uint8
{
	Tir,        // standart tır
	Otobus,     // uzun otobüs
	Kamyonet,   // kısa ve alçak
	Tanker,     // silindir tanklı; çarpınca patlar
	YayliTir,   // tavanında paketin yayı var: çok yükseğe fırlatır
	Firlatici,  // tavanında fırlatma rampası: ileri fırlatır
	Hoplayan,   // hidrolik süspansiyon: arada bir zıplar
	Zigzag,     // sürekli şerit değiştirir
	DevTir      // çift römorklu uzun tır
};

/** Bölüm zorlukları (bit bayrakları). */
namespace KChallenge
{
	enum Type : uint32
	{
		None = 0,
		Lasers = 1 << 0,         // lazer kapıları: üstünden atla
		InvertControls = 1 << 1, // kontroller ara ara ters döner
		Wind = 1 << 2,           // havadayken yana iten rüzgar
		Ice = 1 << 3,            // kaygan tavanlar
		FallingTrucks = 1 << 4,  // gökten tır yağar
		Fog = 1 << 5,            // yoğun sis
		GapWalls = 1 << 6,       // yolu kapatan duvar, tek açık şerit: şerit değiştirmek zorunlu
		LaneSwerve = 1 << 7,     // tırlar ani şerit değiştirir
		SpeedBurst = 1 << 8,     // trafik aniden hızlanır
		Darkness = 1 << 9        // gece/karanlık
	};
}

struct FKulusturBiomeStyle
{
	FText Name;
	FText Subtitle;

	FLinearColor Ground;
	FLinearColor GroundAlt;   // dekor/çim lekeleri
	FLinearColor Road;
	FLinearColor RoadLine;
	FLinearColor UIColor;     // menüde dünya rengi

	// Işık
	float SunPitch = -45.f;
	float SunYaw = 30.f;
	float SunIntensity = 8.f;
	FLinearColor SunColor = FLinearColor::White;
	float SkyIntensity = 1.f;

	// Sis
	float FogDensity = 0.01f;
	float FogFalloff = 0.2f;
	FLinearColor FogColor;

	// Hava
	EKulusturWeather Weather = EKulusturWeather::None;
	float WeatherAmount = 0.f;
	bool bLightning = false;

	// Renk ayarı (post process)
	float Saturation = 1.f;
	FLinearColor Tint = FLinearColor::White;
	float ExposureBias = 0.f;

	TArray<FLinearColor> TruckColors;
	/** Bu dünyada görülen araç tipleri ve ağırlıkları. */
	TArray<TPair<EVehicleType, float>> VehicleMix;

	/** Zemin rengine göre zıplanınca kaygan mı (kar). */
	bool bIcyDefault = false;
};

const FKulusturBiomeStyle& KulusturGetBiomeStyle(EKulusturBiome Biome);

/** Bir bölümün (ya da sonsuz moddaki bir anın) ayarları. */
struct FKulusturLevelConfig
{
	EKulusturBiome Biome = EKulusturBiome::Gunesli;
	int32 World = 0; // 0..4
	int32 Level = 0; // 0..9

	float RoadLength = 25000.f;
	float BaseSpeed = 1900.f;
	float SpeedVariation = 120.f;
	float BrakeInterval = 6.f;
	int32 Seed = 1;

	int32 NumRamps = 2;
	int32 NumWalls = 1;
	int32 NumGapWalls = 0;
	int32 NumLasers = 0;
	int32 NumRails = 0;
	int32 NumZiplines = 0;
	int32 NumPulleys = 0;
	int32 NumRings = 2;
	int32 NumCoinLines = 4;

	uint32 Challenges = KChallenge::None;
	float WindStrength = 0.f;

	float ParTime = 15.f;
	FText Title;
	FText Hint;
};

FKulusturLevelConfig KulusturMakeLevelConfig(int32 World, int32 Level);

/** Sonsuz modda X mesafesindeki (cm) ayarlar. */
FKulusturLevelConfig KulusturMakeEndlessConfig(float DistanceCm);

/** Sonsuz modda X'teki dünya. */
EKulusturBiome KulusturEndlessBiomeAt(float X);

/** Sonsuz modda bir dünya bölümünün uzunluğu (cm). */
constexpr float KulusturEndlessBiomeLength = 150000.f;

FText KulusturChallengeText(uint32 Challenges);

inline int32 KulusturLevelIndex(int32 World, int32 Level) { return World * KulusturLevelsPerWorld + Level; }
