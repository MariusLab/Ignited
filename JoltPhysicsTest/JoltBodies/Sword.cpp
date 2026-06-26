// Fill out your copyright notice in the Description page of Project Settings.


#include "Sword.h"

#include "JoltPhysicsTest/FunctionLibrary/Helper.h"
#include "JoltPhysicsTest/Player/MyPlayerController.h"

void ASword::InitializeBody(const FJoltBodyData& JoltBodyData)
{
	Super::InitializeBody(JoltBodyData);

	if (BelongsToPlayerId > -1)
	{
		auto LocalPlayerController = UHelper::GetLocalPlayerController(GetWorld());
		FPlayerCharacter& OwnerPlayerCharacter = LocalPlayerController->GetPlayerCharacter(BelongsToPlayerId);

		BP_OnUpdatePlayerTeam(OwnerPlayerCharacter.PlayerTeam);
	}
}
