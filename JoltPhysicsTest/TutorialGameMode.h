// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "TutorialGameMode.generated.h"

UCLASS()
class JOLTPHYSICSTEST_API ATutorialGameMode : public AGameMode
{
	GENERATED_BODY()
public:
	virtual bool ReadyToStartMatch_Implementation() override;
};
