// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JoltPhysicsTest/Jolt/JoltBodyActor.h"
#include "HomingMissile.generated.h"

UCLASS()
class JOLTPHYSICSTEST_API AHomingMissile : public AJoltBodyActor
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent)
	void BP_OnUpdatePlayerTeam(EPlayerTeam PlayerTeam);
	
	UPROPERTY(EditDefaultsOnly, Category = "Config|Ability")
	int FrameDurationToShootAllRockets = 30;

	UPROPERTY(EditDefaultsOnly, Category = "Config|Ability")
	int ShootRocketEveryXFrames = 3;

	UPROPERTY(EditDefaultsOnly, Category = "Config|Ability")
	TSubclassOf<AActor> ShootVFX;

	UPROPERTY(EditDefaultsOnly, Category = "Config|IndividualMissile")
	float DefaultProjectileImpulse = 30000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Config|IndividualMissile")
	float ProjectileKnockbackOnPlayer = 30000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Config|IndividualMissile")
	float ProjectileSpawnForwardDistance = 2.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Config|IndividualMissile")
	float ForcePerStepWhenHoming = 200.f;

	UPROPERTY(EditDefaultsOnly, Category = "Config|IndividualMissile")
	int StepsToWaitBeforeHoming = 60;

	UPROPERTY(EditDefaultsOnly, Category = "Config|IndividualMissile")
	int RandomDegreesOffset = 30;

	virtual void InitializeBody(const FJoltBodyData& JoltBodyData) override;

	virtual void StepSimulation() override;
	virtual void Save(StateRecorderImpl& InStream) override;
	virtual void Load(StateRecorderImpl& InStream) override;

private:
	int StepsSinceFired = 0;
	int TargetPlayerId = -1;
};
