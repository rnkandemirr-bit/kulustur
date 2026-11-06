#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "KulusturGameMode.generated.h"

UCLASS()
class KULUSTUR_API AKulusturGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AKulusturGameMode();
	virtual void StartPlay() override;
	/** Menüde karakter yok: kamera trafiği izler. */
	virtual APawn* SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot) override;
};
