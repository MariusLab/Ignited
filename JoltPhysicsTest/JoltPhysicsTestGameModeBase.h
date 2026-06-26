// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "JoltPhysicsTestGameModeBase.generated.h"

UCLASS()
class JOLTPHYSICSTEST_API AJoltPhysicsTestGameModeBase : public AGameMode
{
	GENERATED_BODY()

public:
	virtual bool ReadyToStartMatch_Implementation() override;

	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

protected:
	void OnPostLogin(AController* NewPlayer) override;
	virtual void GenericPlayerInitialization(AController* C) override;
};
