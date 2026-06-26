#include "StickyBomb.h"

#include "JoltPhysicsTest/FunctionLibrary/Helper.h"
#include "JoltPhysicsTest/Game/SpawnerSubsystem.h"
#include "JoltPhysicsTest/Jolt/JoltConstraintDataStruct.h"
#include "JoltPhysicsTest/Jolt/JoltConverter.h"
#include "JoltPhysicsTest/Jolt/JoltSubsystem.h"
#include "JoltPhysicsTest/Player/MyPlayerController.h"

void AStickyBomb::InitializeBody(const FJoltBodyData& JoltBodyData)
{
	Super::InitializeBody(JoltBodyData);

	if (BelongsToPlayerId > -1)
	{
		auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();
		FPlayerCharacter& PlayerCharacterOwner = UHelper::GetLocalPlayerController(GetWorld())->GetPlayerCharacter(BelongsToPlayerId);

		// Inherit the owner's collision group so we don't collide with our own player's bodies
		auto& PlayerCollisionGroup = JoltSubsystem->GetBodyInterface()->GetCollisionGroup(BodyID(PlayerCharacterOwner.TakeDamageSensorBodyIndex));
		JoltSubsystem->GetBodyInterface()->SetCollisionGroup(BodyID(JoltBodyData.BodyIndex), PlayerCollisionGroup);
	}
}

void AStickyBomb::StepSimulation()
{
	Super::StepSimulation();

	if (!bStuck)
	{
		return;
	}

	StepsSinceStuck++;

	const int RemainingFrames = FramesUntilExplosion - StepsSinceStuck;
	BP_SecondsLeftUntilExplode((RemainingFrames + 30) / 60);

	if (StepsSinceStuck == FramesUntilExplosion)
	{
		auto SpawnerSubsystem = GetWorld()->GetSubsystem<USpawnerSubsystem>();

		UHelper::GetLocalPlayerController(GetWorld())->StickyBombExplode(GetBodyID(), ExplosionRadius);

		if (SpawnerSubsystem->DoesConstraintExistInSimulation(StuckConstraintId))
		{
			SpawnerSubsystem->DespawnJoltConstraint(StuckConstraintId);
		}

		SpawnerSubsystem->DespawnJoltBody(GetBodyID());
	}
}

void AStickyBomb::Stick(const BodyID& InTargetBodyID, const Vec3& InContactWorldPos)
{
	if (bStuck)
	{
		return;
	}

	auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();
	auto SpawnerSubsystem = GetWorld()->GetSubsystem<USpawnerSubsystem>();

	bStuck = true;

	JoltSubsystem->GetBodyInterface()->SetLinearVelocity(GetBodyID(), Vec3::sZero());
	JoltSubsystem->GetBodyInterface()->SetAngularVelocity(GetBodyID(), Vec3::sZero());

	FJoltConstraintData ConstraintData;
	ConstraintData.BodyID1 = InTargetBodyID;
	ConstraintData.BodyID2 = GetBodyID();

	StuckConstraintId = SpawnerSubsystem->SpawnJoltConstraint(
		StickConstraintClass,
		Units::FromJoltPos(InContactWorldPos),
		ConstraintData
	);
}

void AStickyBomb::Save(StateRecorderImpl& InStream)
{
	Super::Save(InStream);

	InStream.Write(bStuck);
	InStream.Write(StepsSinceStuck);
	InStream.Write(StuckConstraintId);
}

void AStickyBomb::Load(StateRecorderImpl& InStream)
{
	Super::Load(InStream);

	InStream.Read(bStuck);
	InStream.Read(StepsSinceStuck);
	InStream.Read(StuckConstraintId);
}
