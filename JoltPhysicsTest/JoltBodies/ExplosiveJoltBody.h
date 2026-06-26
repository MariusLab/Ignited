// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JoltPhysicsTest/Jolt/JoltBodyActor.h"
#include "JoltPhysicsTest/Player/MyPlayerController.h"
#include "ExplosiveJoltBody.generated.h"

UCLASS()
class JOLTPHYSICSTEST_API AExplosiveJoltBody : public AJoltBodyActor
{
	GENERATED_BODY()

public:
	virtual void Save(StateRecorderImpl& InStream) override;
	virtual void Load(StateRecorderImpl& InStream) override;
	void ThrowByPlayer(const FPlayerCharacter& PlayerCharacter);

	int ThrownByPlayerId = 0;
};
