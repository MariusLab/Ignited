// Fill out your copyright notice in the Description page of Project Settings.


#include "Helper.h"

#include "JoltPhysicsTest/Data/AssetManagerSubsystem.h"
#include "JoltPhysicsTest/Game/GameVersion.h"
#include "JoltPhysicsTest/Game/MyGameState.h"
#include "JoltPhysicsTest/Game/MyGameInstance.h"
#include "JoltPhysicsTest/Player/MyPlayerController.h"
#include "Kismet/GameplayStatics.h"

AMyPlayerController* UHelper::GetLocalPlayerController(UObject* WorldContextObject)
{
	checkf(WorldContextObject, TEXT("WorldContextObject obj passed is invalid"));
	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::Assert);

	// For clients they will only have 1 controller and for the listen server their local controller will also always be the 1st one
	return Cast<AMyPlayerController>(World->GetFirstPlayerController());
}

AMyGameState* UHelper::GetMyGameState(UObject* WorldContextObject)
{
	checkf(WorldContextObject, TEXT("WorldContextObject obj passed is invalid"));
	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::Assert);

	return Cast<AMyGameState>(World->GetGameState());
}

UMyGameInstance* UHelper::GetMyGameInstance(UObject* WorldContextObject)
{
	auto GameInstance = UGameplayStatics::GetGameInstance(WorldContextObject);
	checkf(GameInstance, TEXT("Game instance not found"));
	checkf(Cast<UMyGameInstance>(GameInstance), TEXT("Failed to cast game instance"));
	
	return Cast<UMyGameInstance>(GameInstance);
}

FString UHelper::GetGameVersion()
{
	const FString VersionStr(FApp::GetBuildVersion());
	const FString UniqueDigits = VersionStr.Right(8);

	return FString::Printf(TEXT("%s.%s"), *FString(GAME_VERSION), *UniqueDigits);
}

FLootItem UHelper::BP_GetLootItemRow(ELootItem LootItem, UObject* WorldContextObject)
{
	checkf(WorldContextObject, TEXT("WorldContextObject obj passed is invalid"));
	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::Assert);

	return UAssetManagerSubsystem::GetLootItemRow(World, LootItem);
}
