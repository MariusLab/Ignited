// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JoltPhysicsTest/Jolt/JoltBodyActor.h"
#include "ShootingCannon.generated.h"

UCLASS()
class JOLTPHYSICSTEST_API AShootingCannon : public AJoltBodyActor
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float ProjectileSpawnForwardDistance = 15.f;

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	int CooldownFramesBetweenShots = 180;

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float ProjectileImpulse = 600.f;

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	TSubclassOf<AJoltBodyActor> ProjectileActorClass;

	virtual void InitializeBody(const FJoltBodyData& JoltBodyData) override;
	virtual void StepSimulation() override;
	virtual void Save(StateRecorderImpl& InStream) override;
	virtual void Load(StateRecorderImpl& InStream) override;

	virtual void DestroyBody() override;
private:
	int RemainingShootCooldown = 180;
	int LastSpawnedProjectileIndex = -1;
};
