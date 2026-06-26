// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyGameMode.h"

#include "FunctionLibrary/Helper.h"
#include "Player/MyPlayerController.h"


bool ALobbyGameMode::ReadyToStartMatch_Implementation()
{
	if (!Super::ReadyToStartMatch_Implementation())
	{
		return false;
	}
	
	auto LocalPlayerController = UHelper::GetLocalPlayerController(GetWorld());
	LocalPlayerController->LocalControllerLobbyInit();
	LocalPlayerController->OnMatchHasStarted();
	
	return true;
}

