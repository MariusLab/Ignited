// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JoltPhysicsTest/Jolt/JoltBodyActor.h"
#include "JoltPhysicsTest/Player/PlayerTeamsEnum.h"
#include "FlagBase.generated.h"

struct FPlayerCharacter;

UCLASS()
class JOLTPHYSICSTEST_API AFlagBase : public AJoltBodyActor
{
	GENERATED_BODY()

public:
	virtual void InitializeBody(const FJoltBodyData& JoltBodyData) override;

	void UpdateTeam(EPlayerTeam NewPlayerTeam);

	UFUNCTION(BlueprintImplementableEvent, Category = "FlagBase")
	void OnUpdateTeam();
	
	UFUNCTION(BlueprintImplementableEvent, Category = "FlagBase")
	void OnFlagTaken();

	UFUNCTION(BlueprintImplementableEvent, Category = "FlagBase")
	void OnFlagReturned();
	
	virtual void Save(StateRecorderImpl& InStream) override;
	virtual void Load(StateRecorderImpl& InStream) override;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Team")
	EPlayerTeam BelongsToTeam = EPlayerTeam::None;

	int TakeFlag();
	void ReturnFlag();

	bool IsFlagTaken() const;

	virtual void DestroyBody() override;
private:
	UFUNCTION()
	void OnRender();

	// TODO: we should probably have separate events called to blueprints, one for pending, one for confirmed actions.
	// Such as TakenFlagConfirmed, TakenFlagPending.
	bool bFlagTaken = false;
	bool bFlagTakenRender = false;

	int SpawnedFlagBodyIndex = -1;
};
