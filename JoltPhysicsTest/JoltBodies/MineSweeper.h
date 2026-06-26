// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JoltPhysicsTest/Jolt/JoltBodyActor.h"
#include "MineSweeper.generated.h"

UCLASS()
class JOLTPHYSICSTEST_API AMineSweeper : public AJoltBodyActor
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent)
	void BP_OnUpdatePlayerTeam(EPlayerTeam PlayerTeam);
	
	UPROPERTY(EditDefaultsOnly, Category = "Config|Ability")
	int FrameDurationToReleaseAllMines = 30;

	UPROPERTY(EditDefaultsOnly, Category = "Config|Ability")
	int ReleaseMineEveryXFrames = 3;

	UPROPERTY(EditDefaultsOnly, Category = "Config|Ability")
	int MinFramesToLive = 1800;

	UPROPERTY(EditDefaultsOnly, Category = "Config|Ability")
	int MaxFramesToLive = 2400;

	UPROPERTY(EditDefaultsOnly, Category = "Config|IndividualMissile")
	int MinDefaultProjectileImpulse = 5000;

	UPROPERTY(EditDefaultsOnly, Category = "Config|IndividualMissile")
	int MaxDefaultProjectileImpulse = 10000;

	UPROPERTY(EditDefaultsOnly, Category = "Config|IndividualMissile")
	float ProjectileKnockbackOnPlayer = 30000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Config|IndividualMissile")
	float ProjectileSpawnForwardDistance = 2.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Config|IndividualMissile")
	int RandomDegreesOffset = 30;

	UPROPERTY(EditDefaultsOnly, Category = "Config|IndividualMissile")
	float TorqueImpulse = 20000.f;
	

	virtual void InitializeBody(const FJoltBodyData& JoltBodyData) override;
	virtual void StepSimulation() override;
	virtual void Save(StateRecorderImpl& InStream) override;
	virtual void Load(StateRecorderImpl& InStream) override;

private:
	int StepsAlive = 0;
	int FramesToLive = 0;
};
