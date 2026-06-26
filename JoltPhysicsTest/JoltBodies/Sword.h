// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JoltPhysicsTest/Jolt/JoltBodyActor.h"
#include "Sword.generated.h"

class UNiagaraSystem;

UCLASS()
class JOLTPHYSICSTEST_API ASword : public AJoltBodyActor
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent)
	void BP_OnUpdatePlayerTeam(EPlayerTeam PlayerTeam);
	
	virtual void InitializeBody(const FJoltBodyData& JoltBodyData) override;

	UPROPERTY(EditDefaultsOnly, Category = "Config|Ability")
	int AbilityDurationFramesNum = 180;

	UPROPERTY(EditDefaultsOnly, Category = "Config|Ability")
	float DistanceFromPlayer = 100.f;

	UPROPERTY(EditDefaultsOnly, Category = "Config|Ability")
	float PlayerKnockbackOnTwoSwordsCollide = 100000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Config|Ability")
	TObjectPtr<UNiagaraSystem> AppearVFX = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Config|Ability")
	TObjectPtr<UNiagaraSystem> DisappearVFX = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Config|Ability")
	TObjectPtr<UNiagaraSystem> SwordsCollideVFX = nullptr;
};
