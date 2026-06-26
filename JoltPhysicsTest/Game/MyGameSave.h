// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "JoltPhysicsTest/Data/GameSettingsStruct.h"
#include "MyGameSave.generated.h"

UCLASS()
class JOLTPHYSICSTEST_API UMyGameSave : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	uint32 PlayedGamesNum = 0;

	UPROPERTY()
	FGameSettings GameSettings;

	UPROPERTY()
	int CurrentTutorialLevel = 0;

	UPROPERTY()
	bool bTutorialCompleted = false;
};
