// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JoltPhysicsTest/Jolt/JoltBodyActor.h"
#include "ExplosiveBarrel.generated.h"

UCLASS()
class JOLTPHYSICSTEST_API AExplosiveBarrel : public AJoltBodyActor
{
	GENERATED_BODY()

public:
	// After the barrel is hit, how many frames to wait until the actual shape cast is triggered
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	int FramesNumAfterExplosionToDealDamage = 6;
	
	virtual void Save(StateRecorderImpl& InStream) override;
	virtual void Load(StateRecorderImpl& InStream) override;
	virtual void StepSimulation() override;

	void Explode(const int& InFrameId);
private:
	bool bExploded = false;
	int StepsSinceMarkedAsExploded = 0;
	int ExplodedOnFrameId = -1;

	void ExplodeInternal();
};
