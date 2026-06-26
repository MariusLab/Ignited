// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JoltPhysicsTest/Jolt/JoltBodyActor.h"
#include "PlayerSensor.generated.h"

UCLASS()
class JOLTPHYSICSTEST_API APlayerSensor : public AJoltBodyActor
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent)
	void BP_OnPlayerEnter();

	UFUNCTION(BlueprintImplementableEvent)
	void BP_OnPlayerLeave();

	UFUNCTION(BlueprintImplementableEvent)
	void BP_OnStepSimulation();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerSensor")
	bool bActivatedByHandBody = false;

	virtual void PlayerEnter();
	virtual void PlayerLeave();

	virtual void StepSimulation() override;

private:
	UPROPERTY(BlueprintReadOnly, Category = "PlayerSensor", Meta = (AllowPrivateAccess = "true"))
	bool bPlayerInsideSensor = false;

	UPROPERTY(BlueprintReadOnly, Category = "PlayerSensor", Meta = (AllowPrivateAccess = "true"))
	int StepsInsideSensor = 0;
};
