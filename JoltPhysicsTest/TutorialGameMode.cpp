// Fill out your copyright notice in the Description page of Project Settings.


#include "TutorialGameMode.h"

#include "FunctionLibrary/Helper.h"
#include "Player/MyPlayerController.h"

bool ATutorialGameMode::ReadyToStartMatch_Implementation()
{
	if (!Super::ReadyToStartMatch_Implementation())
	{
		return false;
	}
	
	auto LocalPlayerController = UHelper::GetLocalPlayerController(GetWorld());
	LocalPlayerController->LocalControllerTutorialInit();
	LocalPlayerController->OnMatchHasStarted();
	
	return true;
}
