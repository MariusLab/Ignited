// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JoltPhysicsTest/Jolt/JoltBodyActor.h"
#include "SentryTurret.generated.h"

UCLASS()
class JOLTPHYSICSTEST_API ASentryTurret : public AJoltBodyActor
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent)
	void BP_OnUpdatePlayerTeam(EPlayerTeam PlayerTeam);

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float ProjectileSpawnForwardDistance = 15.f;

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float MaxAttackDistance = 100.f;

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	int CooldownFramesBetweenShots = 60;

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float ProjectileImpulse = 600.f;

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	TSubclassOf<AJoltBodyActor> ProjectileActorClass;

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	int FramesToLive = 240;
	
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	TObjectPtr<class UNiagaraSystem> SpawnVFX = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Config", Meta = (DevelopmentOnly))
	bool bDebugDrawAffectArea = false;

	UPROPERTY(EditDefaultsOnly, Category = "Config", Meta = (DevelopmentOnly))
	bool bDebugDrawCurrentTarget = false;
	

	virtual void InitializeBody(const FJoltBodyData& JoltBodyData) override;

	virtual void StepSimulation() override;
	virtual void Save(StateRecorderImpl& InStream) override;
	virtual void Load(StateRecorderImpl& InStream) override;

private:
	int StepsSincePlaced = 0;
	int RemainingShootCooldown = 0;
	int OwningPlayerTakeDamageSensorIndex = -1;
};