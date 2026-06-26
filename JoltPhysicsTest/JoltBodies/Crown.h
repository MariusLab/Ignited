// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JoltPhysicsTest/Jolt/JoltBodyActor.h"
#include "Crown.generated.h"

struct FPlayerCharacter;

UCLASS()
class JOLTPHYSICSTEST_API ACrown : public AJoltBodyActor
{
	GENERATED_BODY()

public:
	virtual void InitializeBody(const FJoltBodyData& JoltBodyData) override;
	
	virtual void Save(StateRecorderImpl& InStream) override;
	virtual void Load(StateRecorderImpl& InStream) override;

	void TakeCrown(const int& PlayerId);

	UFUNCTION(BlueprintImplementableEvent)
	void BP_OnCrownTaken();
};
