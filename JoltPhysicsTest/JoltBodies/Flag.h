// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JoltPhysicsTest/Jolt/JoltBodyActor.h"
#include "JoltPhysicsTest/Player/PlayerTeamsEnum.h"
#include "Flag.generated.h"

UCLASS()
class JOLTPHYSICSTEST_API AFlag : public AJoltBodyActor
{
	GENERATED_BODY()

public:
	AFlag();
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<USceneComponent> RootSceneComp = nullptr;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<USceneComponent> ConstraintAttachSceneComp = nullptr;
	
	virtual void StepSimulation() override;

	virtual void Save(StateRecorderImpl& InStream) override;
	virtual void Load(StateRecorderImpl& InStream) override;
	
	UFUNCTION(BlueprintImplementableEvent, Category = "Flag")
	void OnTeamUpdated(EPlayerTeam PlayerTeam);
	
	void SetBelongsToTeam(EPlayerTeam PlayerTeam);
	
	UPROPERTY(EditAnywhere, Category = "Flag")
	EPlayerTeam BelongsToTeam = EPlayerTeam::None;

	int TakenFromFlagBaseBodyIndex = -1;

	void PlaceInBase();
	void ReturnHome();

	bool IsPlacedInBase() const;
private:
	int StepsSincePlacedInBase = 0;
	bool bPlacedInBasePending = false;
	bool bPlacedInBaseConfirmed = false;
};
