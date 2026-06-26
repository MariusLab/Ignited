// Fill out your copyright notice in the Description page of Project Settings.


#include "BananaProjectile.h"

#include "JoltPhysicsTest/Components/JoltCapsuleComponent.h"
#include "JoltPhysicsTest/Data/AssetManagerSubsystem.h"
#include "JoltPhysicsTest/FunctionLibrary/Helper.h"
#include "JoltPhysicsTest/Player/MyPlayerController.h"

void ABananaProjectile::InitializeBody(const FJoltBodyData& JoltBodyData)
{
	FPlayerCharacter& OwnerPlayerCharacter = UHelper::GetLocalPlayerController(GetWorld())->GetPlayerCharacter(BelongsToPlayerId);
	BP_OnUpdatePlayerTeam(OwnerPlayerCharacter.PlayerTeam);
	
	Super::InitializeBody(JoltBodyData);

	auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();

	BouncesLeft = NumberOfBounces;

	// This only works AFTER adding the body to Jolt. Otherwise it won't recognize this body index.
	auto& PlayerCollisionGroup = JoltSubsystem->GetBodyInterface()->GetCollisionGroup(BodyID(OwnerPlayerCharacter.TakeDamageSensorBodyIndex));
	JoltSubsystem->GetBodyInterface()->SetCollisionGroup(BodyID(JoltBodyData.BodyIndex), PlayerCollisionGroup);
}

void ABananaProjectile::Save(StateRecorderImpl& InStream)
{
	Super::Save(InStream);

	InStream.Write(BouncesLeft);
}

void ABananaProjectile::Load(StateRecorderImpl& InStream)
{
	Super::Load(InStream);

	InStream.Read(BouncesLeft);
}
