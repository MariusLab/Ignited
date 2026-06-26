// Fill out your copyright notice in the Description page of Project Settings.


#include "MineSweeper.h"

#include "JoltPhysicsTest/FunctionLibrary/Helper.h"
#include "JoltPhysicsTest/Player/MyPlayerController.h"


void AMineSweeper::InitializeBody(const FJoltBodyData& JoltBodyData)
{
	Super::InitializeBody(JoltBodyData);

	if (BelongsToPlayerId > -1)
	{
		auto LocalPlayerController = UHelper::GetLocalPlayerController(GetWorld());
		FPlayerCharacter& OwnerPlayerCharacter = LocalPlayerController->GetPlayerCharacter(BelongsToPlayerId);
		auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();

		// This only works AFTER adding the body to Jolt. Otherwise it won't recognize this body index.
		auto& PlayerCollisionGroup = JoltSubsystem->GetBodyInterface()->GetCollisionGroup(BodyID(OwnerPlayerCharacter.TakeDamageSensorBodyIndex));
		JoltSubsystem->GetBodyInterface()->SetCollisionGroup(BodyID(JoltBodyData.BodyIndex), PlayerCollisionGroup);

		BP_OnUpdatePlayerTeam(OwnerPlayerCharacter.PlayerTeam);

		FramesToLive = JoltSubsystem->RandomNumberGenerator.NextIntInRange(MinFramesToLive, MaxFramesToLive);
	}
}

void AMineSweeper::StepSimulation()
{
	Super::StepSimulation();

	StepsAlive++;

	if (StepsAlive >= FramesToLive)
	{
		const BodyID MineBodyID = GetBodyID();
		
		UHelper::GetLocalPlayerController(GetWorld())->MineExpire(MineBodyID);
		GetWorld()->GetSubsystem<USpawnerSubsystem>()->DespawnJoltBody(MineBodyID);
	}
}

void AMineSweeper::Save(StateRecorderImpl& InStream)
{
	Super::Save(InStream);

	InStream.Write(StepsAlive);
}

void AMineSweeper::Load(StateRecorderImpl& InStream)
{
	Super::Load(InStream);

	InStream.Read(StepsAlive);
}
