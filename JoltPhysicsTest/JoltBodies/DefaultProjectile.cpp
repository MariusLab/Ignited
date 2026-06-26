// Fill out your copyright notice in the Description page of Project Settings.


#include "DefaultProjectile.h"

#include "JoltPhysicsTest/Components/JoltCapsuleComponent.h"
#include "JoltPhysicsTest/FunctionLibrary/Helper.h"
#include "JoltPhysicsTest/Game/SpawnerSubsystem.h"
#include "JoltPhysicsTest/Player/MyPlayerController.h"

void ADefaultProjectile::InitializeBody(const FJoltBodyData& JoltBodyData)
{
	if (BelongsToPlayerId > -1)
	{
		FPlayerCharacter& OwnerPlayerCharacter = UHelper::GetLocalPlayerController(GetWorld())->GetPlayerCharacter(BelongsToPlayerId);

		BP_OnUpdatePlayerTeam(OwnerPlayerCharacter.PlayerTeam);
	}
	
	// We only want to init with Jolt AFTER changing the properties like size, speed etc
	Super::InitializeBody(JoltBodyData);

	if (BelongsToPlayerId > -1)
	{
		FPlayerCharacter& OwnerPlayerCharacter = UHelper::GetLocalPlayerController(GetWorld())->GetPlayerCharacter(BelongsToPlayerId);
		auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();

		// This only works AFTER adding the body to Jolt. Otherwise it won't recognize this body index.
		auto& PlayerCollisionGroup = JoltSubsystem->GetBodyInterface()->GetCollisionGroup(BodyID(OwnerPlayerCharacter.TakeDamageSensorBodyIndex));
		JoltSubsystem->GetBodyInterface()->SetCollisionGroup(BodyID(JoltBodyData.BodyIndex), PlayerCollisionGroup);
		
		//JoltSubsystem->SetObjectLayer(JoltBodyData.BodyIndex, EObjectLayer::Projectile);
	}
}

void ADefaultProjectile::StepSimulation()
{
	Super::StepSimulation();

	FramesAlive++;

	if (FramesAlive >= TTLFrames)
	{
		auto* PlayerController = UHelper::GetLocalPlayerController(GetWorld());
		if (BelongsToPlayerId >= 0)
		{
			PlayerController->GetPlayerCharacter(BelongsToPlayerId).CreatedBodyIndexes.Remove(GetBodyID().GetIndex());
		}
		GetWorld()->GetSubsystem<USpawnerSubsystem>()->DespawnJoltBody(GetBodyID());
	}
}

void ADefaultProjectile::Save(StateRecorderImpl& InStream)
{
	Super::Save(InStream);
	InStream.Write(FramesAlive);
}

void ADefaultProjectile::Load(StateRecorderImpl& InStream)
{
	Super::Load(InStream);
	InStream.Read(FramesAlive);
}

