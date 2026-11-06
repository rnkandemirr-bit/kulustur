#pragma once

#include "CoreMinimal.h"

class AActor;
class UWorld;

/**
 * 3D Platformer Game Bundle paketindeki Blueprint aktörlerini koddan kurmak için yardımcılar.
 * Paketin aktörleri ayarlarını Blueprint değişkenlerinde tutar; bunları isimle yazarız.
 */
namespace KulusturPackage
{
	// Paket sınıf yolları
	extern const TCHAR* JumpPadA;
	extern const TCHAR* JumpPadB;
	extern const TCHAR* BoostRingA;
	extern const TCHAR* BoostRingB;
	extern const TCHAR* ArcSpring;
	extern const TCHAR* UpSpringA;
	extern const TCHAR* UpSpringB;
	extern const TCHAR* PulleyZipline;
	extern const TCHAR* Rail;
	extern const TCHAR* Coin;
	extern const TCHAR* SpeedUp;
	extern const TCHAR* HiddenTarget;
	extern const TCHAR* Pulley;

	UClass* GetClass(const TCHAR* Path);

	/** Blueprint değişkenini metin olarak yazar, ör. "(X=1,Y=2,Z=3)". Başarısızsa false. */
	bool SetProp(UObject* Obj, FName Name, const FString& Value);

	/** Ayarları kurulumdan (construction script) önce yazarak aktörü doğurur. */
	AActor* SpawnConfigured(UWorld* World, const TCHAR* ClassPath, const FTransform& Transform, const TMap<FName, FString>& Props);

	/**
	 * Spline üzerinde grind ray (bZipline=false) ya da kayan halat (bZipline=true) kurar.
	 * Noktalar dünya koordinatında.
	 */
	AActor* SpawnRail(UWorld* World, const TArray<FVector>& Points, bool bZipline, const TMap<FName, FString>& Props);

	/** Fırlatıcı (jump pad / halka / yay): aktör iniş noktasına, fırlatıcı kendisi LaunchFrom'a konur. */
	AActor* SpawnLauncher(UWorld* World, const TCHAR* ClassPath, const FVector& LaunchFrom, const FVector& LandAt, const TMap<FName, FString>& ExtraProps = {});

	FString Vec(const FVector& V);
}
