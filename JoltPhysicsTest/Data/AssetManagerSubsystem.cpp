// Fill out your copyright notice in the Description page of Project Settings.


#include "AssetManagerSubsystem.h"

#include "GeneralData.h"
#include "JoltPhysicsTest/Jolt/JoltBodyActor.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "JoltPhysicsTest/Components/Constraints/ConstraintStructs.h"

void UAssetManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

	TArray<FAssetData> GeneralDataAssets;
	AssetRegistryModule.Get().GetAssetsByClass(UGeneralData::StaticClass()->GetClassPathName(), GeneralDataAssets);
	checkf(GeneralDataAssets.Num() > 0, TEXT("General data asset not found."));
	
	GeneralData = Cast<UGeneralData>(GeneralDataAssets[0].GetAsset());
	LOG("General data asset loaded from path %s.", *GeneralDataAssets[0].PackageName.ToString());
}

UGeneralData* UAssetManagerSubsystem::GetGeneralData(const UWorld* World)
{
	check(World);

	auto GameInstance = World->GetGameInstance();
	check(GameInstance);

	auto AssetManagerSubsystem = GameInstance->GetSubsystem<UAssetManagerSubsystem>();
	check(AssetManagerSubsystem);

	return AssetManagerSubsystem->GeneralData;
}

FLootItem UAssetManagerSubsystem::GetLootItemRow(const UWorld* World, ELootItem LootItem)
{
	const FString ContextString = TEXT("UAssetManagerSubsystem::GetLootItemRow");
	
	FString LootItemType;
	if (LootItem == ELootItem::None)
	{
		LootItemType = "Unknown";
	}
	else
	{
		UEnum::GetValueAsString(LootItem).Split(TEXT("::"), nullptr, &LootItemType);
	}

	const FString RowName = FString::Printf(TEXT("%s"), *LootItemType);
	const auto Row = GetGeneralData(World)->LootItemsDataTable->FindRow<FLootItem>(*RowName, ContextString);
	checkf(Row, TEXT("Row not found in loot items data table"));

	return *Row;
}

void UAssetManagerSubsystem::LoadLootItemsData(const UWorld* World)
{
	GetGeneralData(World)->LootItemsDataTable->ForeachRow<FLootItem>(
		TEXT("UAssetManagerSubsystem::LoadLootItemsDataAsync"),
		[](const FName& RowKey, const FLootItem& LootItem)
		{
			LootItem.ItemJoltBodyClass.LoadSynchronous();
		}
	);
}
