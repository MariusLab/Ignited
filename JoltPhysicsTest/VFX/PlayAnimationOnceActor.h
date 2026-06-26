// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "JoltPhysicsTest/Components/AnimationComponent.h"
#include "JoltPhysicsTest/Player/PlayerTeamsEnum.h"
#include "PlayAnimationOnceActor.generated.h"

UCLASS()
class JOLTPHYSICSTEST_API APlayAnimationOnceActor : public AActor
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent)
	void BP_OnTeamUpdated(EPlayerTeam PlayerTeam);
	
	// Randomly vary the scale of actor by this amount to introduce more variety to VFX
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	float ScaleVariation = 0.1f;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	bool bDestroyAfterAnimationFinishes = true;
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<USceneComponent> RootSceneComponent = nullptr;
	
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly)
	TObjectPtr<UAnimationComponent> AnimationComponent = nullptr;
	
	APlayAnimationOnceActor();

	virtual void BeginPlay() override;
	
	UFUNCTION()
	void OnAnimationFinished();

	void SetPlayerTeam(EPlayerTeam PlayerTeam);
private:
	UPROPERTY(BlueprintReadOnly, Meta = (AllowPrivateAccess = "true"))
	EPlayerTeam Team = EPlayerTeam::None;
};
