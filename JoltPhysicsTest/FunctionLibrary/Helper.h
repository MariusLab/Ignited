// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JoltPhysicsTest/Data/LootItems.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Helper.generated.h"

UCLASS()
class JOLTPHYSICSTEST_API UHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category="Game", meta=(WorldContext="WorldContextObject", UnsafeDuringActorConstruction="true"))
	static class AMyPlayerController* GetLocalPlayerController(UObject* WorldContextObject);
	
	UFUNCTION(BlueprintPure, Category="Game", meta=(WorldContext="WorldContextObject", UnsafeDuringActorConstruction="true"))
	static class AMyGameState* GetMyGameState(UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category="Game", meta=(WorldContext="WorldContextObject", UnsafeDuringActorConstruction="true"))
	static class UMyGameInstance* GetMyGameInstance(UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category="Game", meta=(UnsafeDuringActorConstruction="true"))
	static FString GetGameVersion();
	
	UFUNCTION(BlueprintPure, Category="Game", meta=(WorldContext="WorldContextObject", UnsafeDuringActorConstruction="true", DisplayName = "GetLootItemRow"))
	static FLootItem BP_GetLootItemRow(ELootItem LootItem, UObject* WorldContextObject);
};
