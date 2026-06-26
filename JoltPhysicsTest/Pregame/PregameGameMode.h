// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "PregameGameMode.generated.h"

UCLASS()
class JOLTPHYSICSTEST_API APregameGameMode : public AGameMode
{
	GENERATED_BODY()
public:
	virtual bool ReadyToStartMatch_Implementation() override;
protected:
	virtual void OnPostLogin(AController* NewPlayer) override;

};
