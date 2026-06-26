// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JoltPhysicsTest/Jolt/JoltBodyActor.h"
#include "JoltPhysicsTest/Player/PlayerTeamsEnum.h"
#include "DefaultProjectile.generated.h"

UCLASS()
class JOLTPHYSICSTEST_API ADefaultProjectile : public AJoltBodyActor
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent)
	void BP_OnUpdatePlayerTeam(EPlayerTeam PlayerTeam);

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	int TTLFrames = 300;

	virtual void InitializeBody(const FJoltBodyData& JoltBodyData) override;
	virtual void StepSimulation() override;
	virtual void Save(StateRecorderImpl& InStream) override;
	virtual void Load(StateRecorderImpl& InStream) override;

private:
	int FramesAlive = 0;
};
