// Fill out your copyright notice in the Description page of Project Settings.


#include "ExplosiveJoltBody.h"

#include "JoltPhysicsTest/Jolt/JoltSubsystem.h"

void AExplosiveJoltBody::Save(StateRecorderImpl& InStream)
{
	Super::Save(InStream);

	auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();
	
	auto BodyID = GetBodyComponent()->GetBodyID();
	InStream.Write(JoltSubsystem->GetBodyInterface()->GetObjectLayer(BodyID));
	InStream.Write(ThrownByPlayerId);

	JoltSubsystem->GetBodyInterface()->GetCollisionGroup(BodyID).SaveBinaryState(InStream);
}

void AExplosiveJoltBody::Load(StateRecorderImpl& InStream)
{
	Super::Load(InStream);

	auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();
	
	auto BodyID = GetBodyComponent()->GetBodyID();
	
	ObjectLayer BodyObjectLayer;
	InStream.Read(BodyObjectLayer);
	JoltSubsystem->GetBodyInterface()->SetObjectLayer(BodyID, BodyObjectLayer);

	InStream.Read(ThrownByPlayerId);

	Ref<GroupFilterTable> DisableCollisionFilter = new GroupFilterTable();
	CollisionGroup CollGroup;
	CollGroup.RestoreBinaryState(InStream);
	CollGroup.SetGroupFilter(DisableCollisionFilter);

	JoltSubsystem->GetBodyInterface()->SetCollisionGroup(BodyID, CollGroup);
}

void AExplosiveJoltBody::ThrowByPlayer(const FPlayerCharacter& PlayerCharacter)
{
	ThrownByPlayerId = PlayerCharacter.PlayerId;
	
	auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();
	
	//auto& PlayerCollisionGroup = JoltSubsystem->GetBodyInterface()->GetCollisionGroup(BodyID(PlayerCharacter.TakeDamageSensorBodyIndex));

	Ref<GroupFilterTable> DisableCollisionFilter = new GroupFilterTable();
	JoltSubsystem->GetBodyInterface()->SetCollisionGroup(GetBodyComponent()->GetBodyID(), CollisionGroup(DisableCollisionFilter, PlayerCharacter.PlayerId, 0));
}
