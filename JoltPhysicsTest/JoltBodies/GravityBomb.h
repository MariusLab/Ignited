// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JoltPhysicsTest/Jolt/JoltBodyActor.h"
#include "JoltPhysicsTest/Jolt/JoltEnumsAndStructs.h"
#include "GravityBomb.generated.h"

enum class EObjectLayer : uint8;

UCLASS()
class JOLTPHYSICSTEST_API AGravityBomb : public AJoltBodyActor
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintImplementableEvent)
	void BP_OnUpdatePlayerTeam(EPlayerTeam PlayerTeam);

	UFUNCTION(BlueprintImplementableEvent)
	void BP_OnGravityActivated();

	UFUNCTION(BlueprintImplementableEvent)
	void BP_OnGravityDeactivated();
	
	virtual void InitializeBody(const FJoltBodyData& JoltBodyData) override;

	virtual void StepSimulation() override;
	virtual void Save(StateRecorderImpl& InStream) override;
	virtual void Load(StateRecorderImpl& InStream) override;

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float ProjectileImpulse = 30000.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float ProjectileKnockbackOnPlayer = 30000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float ProjectileSpawnForwardDistance = 15.f;

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float AffectArea = 300.f;

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float ForceToApplyToAffectedBodies = 100.f;

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	int ActivateOnFrame = 60;

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	int FramesToLive = 240;
	
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	TObjectPtr<class UNiagaraSystem> ThrowVFX = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Config", Meta = (DevelopmentOnly))
	bool bDebugDrawAffectArea = false;

private:
	int StepsSinceFired = 0;
	EObjectLayer CurrentObjectLayer = EObjectLayer::Moving;

	void SetObjectLayer(EObjectLayer NewObjectLayer);
};
