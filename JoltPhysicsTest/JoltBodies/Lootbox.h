// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JoltPhysicsTest/Data/LootItems.h"
#include "JoltPhysicsTest/Jolt/JoltBodyActor.h"
#include "Lootbox.generated.h"

UCLASS()
class JOLTPHYSICSTEST_API ALootbox : public AJoltBodyActor
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, Category = "Config")
	int LootRespawnEveryXFrames = 600;

	UPROPERTY(EditAnywhere, Category = "Config")
	TArray<ELootItem> OverwritePossibleLoot;
	
	virtual void StepSimulation() override;
	virtual void Save(StateRecorderImpl& InStream) override;
	virtual void Load(StateRecorderImpl& InStream) override;

	void Pickup();

	bool IsAvailableForPickup() const;
private:
	int FramesSincePickedUp = -1;
	
};
