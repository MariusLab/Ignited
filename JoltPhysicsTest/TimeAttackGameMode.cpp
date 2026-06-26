// Fill out your copyright notice in the Description page of Project Settings.

#include "TimeAttackGameMode.h"
#include "FunctionLibrary/Helper.h"
#include "Player/MyPlayerController.h"

bool ATimeAttackGameMode::ReadyToStartMatch_Implementation()
{
	if (!Super::ReadyToStartMatch_Implementation())
	{
		return false;
	}

	auto LocalPlayerController = UHelper::GetLocalPlayerController(GetWorld());
	LocalPlayerController->LocalControllerTimeAttackInit();
	LocalPlayerController->OnMatchHasStarted();

	return true;
}
