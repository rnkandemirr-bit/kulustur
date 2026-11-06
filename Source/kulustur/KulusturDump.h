#pragma once

#include "CoreMinimal.h"

class UWorld;

/** Geliştirici aracı: paketteki Blueprint aktörlerinin ayarlarını loga döker (-KulusturDump). */
namespace KulusturDump
{
	void DumpWorld(UWorld* World);
}
