// Fill out your copyright notice in the Description page of Project Settings.

#include "Checkpoint.h"
#include "JoltPhysicsTest/FunctionLibrary/Helper.h"
#include "JoltPhysicsTest/Player/MyPlayerController.h"

void ACheckpoint::PlayerEnter()
{
	auto LocalPlayerController = UHelper::GetLocalPlayerController(GetWorld());
	if (bCheckpointReached || !LocalPlayerController->CanTakeCheckpoint(this))
	{
		return;
	}
	
	Super::PlayerEnter();
	LocalPlayerController->OnCheckpointReached(this);
	bCheckpointReached = true;
	
	GetWorld()->GetSubsystem<USpawnerSubsystem>()->DespawnJoltBody(GetBodyID());
}
