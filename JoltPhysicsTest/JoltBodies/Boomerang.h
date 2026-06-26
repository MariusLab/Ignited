// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JoltPhysicsTest/Jolt/JoltBodyActor.h"
#include "JoltPhysicsTest/Player/PlayerTeamsEnum.h"
#include "Boomerang.generated.h"


UCLASS()
class JOLTPHYSICSTEST_API ABoomerangProjectile : public AJoltBodyActor
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent)
	void BP_OnUpdatePlayerTeam(EPlayerTeam PlayerTeam);

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float DefaultProjectileImpulse = 30000.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float TorqueImpulse = 20000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float ProjectileKnockbackOnPlayer = 30000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float ProjectileSpawnForwardDistance = 2.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float ForcePerStepWhenReturning = 200.f;

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	int StepsToWaitBeforeAttractingBack = 1;

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	TObjectPtr<class UNiagaraSystem> ShootVFX = nullptr;

	virtual void InitializeBody(const FJoltBodyData& JoltBodyData) override;

	virtual void StepSimulation() override;
	virtual void Save(StateRecorderImpl& InStream) override;
	virtual void Load(StateRecorderImpl& InStream) override;

private:
	int StepsSinceFired = 0;
};
