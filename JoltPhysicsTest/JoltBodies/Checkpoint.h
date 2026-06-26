// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PlayerSensor.h"
#include "Checkpoint.generated.h"

UCLASS()
class JOLTPHYSICSTEST_API ACheckpoint : public APlayerSensor
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TimeAttack")
	int CheckpointIndex = 0;

	virtual void PlayerEnter() override;

private:
	bool bCheckpointReached = false;
};
