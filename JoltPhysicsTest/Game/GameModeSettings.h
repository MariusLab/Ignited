#pragma once

#include "GameModesEnum.h"
#include "GameModeSettings.generated.h"

USTRUCT(BlueprintType)
struct FGameModeSettings
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	EGameMode Mode = EGameMode::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	bool bSpawnPlayerTombstones = true;

	/** Players will start with all bullets used up and will have to wait to reload */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	bool bStartWithNoBullets = false;

	/** Players will start with dash on cooldown */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	bool bStartWithNoDash = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	bool bShootingEnabled = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	bool bAbilitiesEnabled = true;
};

inline FArchive& operator<<(FArchive& Ar, FGameModeSettings& Data)
{
	Ar << Data.Mode;
	Ar << Data.bSpawnPlayerTombstones;
	Ar << Data.bStartWithNoBullets;
	Ar << Data.bStartWithNoDash;
	Ar << Data.bShootingEnabled;
	Ar << Data.bAbilitiesEnabled;
		
	return Ar;
}