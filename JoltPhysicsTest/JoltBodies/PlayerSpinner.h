// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JoltPhysicsTest/Data/LootItems.h"
#include "JoltPhysicsTest/Jolt/JoltBodyActor.h"
#include "PlayerSpinner.generated.h"

UCLASS()
class JOLTPHYSICSTEST_API APlayerSpinner : public AJoltBodyActor
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent, Category = "Lootbox")
	void OnStartSpinning(const TArray<FLootItem>& Items, float SpinDuration);

	virtual void InitializeBody(const FJoltBodyData& JoltBodyData) override;
	virtual void StepSimulation() override;
	virtual void Save(StateRecorderImpl& InStream) override;
	virtual void Load(StateRecorderImpl& InStream) override;

	void SetPossibleOptions(const TArray<ELootItem>& InPossibleOptions);
private:
	void ConfirmedOpenLootbox();
	void GenerateSpinnerItemsList();

	TArray<ELootItem> SpinnerItemsList;

	TArray<ELootItem> PossibleOptions = {
		ELootItem::MachineGun,
		ELootItem::ProjectileBounce,
		ELootItem::MineSweeper,
		ELootItem::Boomerang,
		ELootItem::HomingMissile,
		ELootItem::Sword,
		ELootItem::OrbitalSpheres,
		//ELootItem::StickyBomb,
	};
	
	ELootItem SelectedRewardItem = ELootItem::None;
	bool bConfirmedOpen = false;
	bool bRewardGrantedToPlayer = false;
	int StepCountSinceOpened = 0;
	int StepsToCompleteSpinner = -1;
	int SpinnerStopsOnItemIndex = -1;

	float GetSpinDurationInSeconds() const;
};
