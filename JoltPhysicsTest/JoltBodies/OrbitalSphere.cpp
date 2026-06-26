// Fill out your copyright notice in the Description page of Project Settings.

#include "OrbitalSphere.h"

#include "JoltPhysicsTest/FunctionLibrary/Helper.h"
#include "JoltPhysicsTest/Jolt/JoltSubsystem.h"
#include "JoltPhysicsTest/Player/MyPlayerController.h"

void AOrbitalSphere::InitializeBody(const FJoltBodyData& JoltBodyData)
{
	Super::InitializeBody(JoltBodyData);

	if (BelongsToPlayerId >= 0)
	{
		auto* JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();
		FPlayerCharacter& PlayerCharacterOwner = UHelper::GetLocalPlayerController(GetWorld())->GetPlayerCharacter(BelongsToPlayerId);
		JoltSubsystem->GetBodyInterface()->SetCollisionGroup(
			BodyID(JoltBodyData.BodyIndex),
			JoltSubsystem->GetBodyInterface()->GetCollisionGroup(BodyID(PlayerCharacterOwner.TakeDamageSensorBodyIndex))
		);
	}
}
