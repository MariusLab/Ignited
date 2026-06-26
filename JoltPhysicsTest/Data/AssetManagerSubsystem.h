// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GeneralData.h"
#include "LootItems.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AssetManagerSubsystem.generated.h"

UCLASS()
class JOLTPHYSICSTEST_API UAssetManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	static UGeneralData* GetGeneralData(const UWorld* World);

	static FLootItem GetLootItemRow(const UWorld* World, ELootItem LootItem);

	static void LoadLootItemsData(const UWorld* World);

private:
	UPROPERTY()
	TObjectPtr<UGeneralData> GeneralData = nullptr;
};
