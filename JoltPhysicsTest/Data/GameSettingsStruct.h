#pragma once

#include "GameSettingsStruct.generated.h"

USTRUCT(BlueprintType)
struct FGameSettings
{
	GENERATED_BODY()
public:
	UPROPERTY()
	float MouseSensitivity = 0.5f;
	
	UPROPERTY(BlueprintReadOnly)
	int DisplayResolutionX = 0;

	UPROPERTY(BlueprintReadOnly)
	int DisplayResolutionY = 0;

	UPROPERTY(BlueprintReadOnly)
	float SoundsVolume = 0.7f;

	UPROPERTY(BlueprintReadOnly)
	float MusicVolume = 0.5f;

	UPROPERTY(BlueprintReadOnly)
	bool bFullscreen = false;

	UPROPERTY(BlueprintReadOnly)
	bool bVsync = false;

	UPROPERTY(BlueprintReadOnly)
	FString Language = "English";
};