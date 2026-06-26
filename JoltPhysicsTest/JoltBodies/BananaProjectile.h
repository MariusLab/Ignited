// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JoltPhysicsTest/Jolt/JoltBodyActor.h"
#include "JoltPhysicsTest/Player/PlayerTeamsEnum.h"
#include "BananaProjectile.generated.h"

UCLASS()
class JOLTPHYSICSTEST_API ABananaProjectile : public AJoltBodyActor
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent)
	void BP_OnUpdatePlayerTeam(EPlayerTeam PlayerTeam);

	virtual void InitializeBody(const FJoltBodyData& JoltBodyData) override;
	virtual void Save(StateRecorderImpl& InStream) override;
	virtual void Load(StateRecorderImpl& InStream) override;

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float ProjectileImpulse = 30000.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float ProjectileTorque = 550000.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float ProjectileKnockbackOnPlayer = 30000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float ProjectileSpawnForwardDistance = 15.f;

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	int SpawnNumOnBounce = 6;

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	int DegreesOffsetOnBounce = 15;

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	int NumberOfBounces = 2;

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float MinProjectileImpulseOnBounce = 12000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float MaxProjectileImpulseOnBounce = 24000.f;
	
	int BouncesLeft = 0;
};
