// Fill out your copyright notice in the Description page of Project Settings.


#include "HomingMissile.h"

#include "JoltPhysicsTest/FunctionLibrary/Helper.h"
#include "JoltPhysicsTest/Player/MyPlayerController.h"


void AHomingMissile::InitializeBody(const FJoltBodyData& JoltBodyData)
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

		TargetPlayerId = LocalPlayerController->GetEnemyPlayerCharacter(OwnerPlayerCharacter.PlayerTeam).PlayerId;
	}
}

void AHomingMissile::StepSimulation()
{
	Super::StepSimulation();

	StepsSinceFired++;

	if (StepsSinceFired > StepsToWaitBeforeHoming)
	{
		auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();
		FPlayerCharacter& TargetPlayerCharacter = UHelper::GetLocalPlayerController(GetWorld())->GetPlayerCharacter(TargetPlayerId);

		Vec3 TargetPlayerPos = JoltSubsystem->GetBodyInterface()->GetPosition(BodyID(TargetPlayerCharacter.PlayerId));
		Vec3 DirectionTowardsTarget = (TargetPlayerPos - JoltSubsystem->GetBodyInterface()->GetPosition(GetBodyID())).NormalizedOr(Vec3::sZero());
		
		JoltSubsystem->GetBodyInterface()->AddForce(GetBodyID(), DirectionTowardsTarget * ForcePerStepWhenHoming);
	}
}

void AHomingMissile::Save(StateRecorderImpl& InStream)
{
	Super::Save(InStream);

	InStream.Write(StepsSinceFired);
}

void AHomingMissile::Load(StateRecorderImpl& InStream)
{
	Super::Load(InStream);

	InStream.Read(StepsSinceFired);
}

