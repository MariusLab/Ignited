// Fill out your copyright notice in the Description page of Project Settings.


#include "Crown.h"

#include "JoltPhysicsTest/Components/JoltCapsuleComponent.h"
#include "JoltPhysicsTest/FunctionLibrary/Helper.h"
#include "JoltPhysicsTest/Game/MyGameState.h"
#include "JoltPhysicsTest/Player/MyPlayerController.h"


void ACrown::InitializeBody(const FJoltBodyData& JoltBodyData)
{
	Super::InitializeBody(JoltBodyData);
}

void ACrown::Save(StateRecorderImpl& InStream)
{
	Super::Save(InStream);
}

void ACrown::Load(StateRecorderImpl& InStream)
{
	Super::Load(InStream);
}

void ACrown::TakeCrown(const int& PlayerId)
{
	UHelper::GetMyGameState(GetWorld())->SetPlayerIdThatHasCrown(PlayerId);

	BP_OnCrownTaken();
	
	auto SpawnerSubsystem = GetWorld()->GetSubsystem<USpawnerSubsystem>();
	SpawnerSubsystem->DespawnJoltBody(GetBodyID());
}
