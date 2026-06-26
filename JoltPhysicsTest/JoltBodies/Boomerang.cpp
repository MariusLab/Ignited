// Fill out your copyright notice in the Description page of Project Settings.


#include "Boomerang.h"

#include "JoltPhysicsTest/Components/JoltCapsuleComponent.h"
#include "JoltPhysicsTest/FunctionLibrary/Helper.h"
#include "JoltPhysicsTest/Player/MyPlayerController.h"

void ABoomerangProjectile::InitializeBody(const FJoltBodyData& JoltBodyData)
{
	if (BelongsToPlayerId > -1)
	{
		FPlayerCharacter& OwnerPlayerCharacter = UHelper::GetLocalPlayerController(GetWorld())->GetPlayerCharacter(BelongsToPlayerId);
		BP_OnUpdatePlayerTeam(OwnerPlayerCharacter.PlayerTeam);
	}
	
	Super::InitializeBody(JoltBodyData);

}

void ABoomerangProjectile::StepSimulation()
{
	Super::StepSimulation();

	StepsSinceFired++;

	if (StepsSinceFired > StepsToWaitBeforeAttractingBack)
	{
		auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();
		FPlayerCharacter& OwnerPlayerCharacter = UHelper::GetLocalPlayerController(GetWorld())->GetPlayerCharacter(BelongsToPlayerId);

		Vec3 PlayerPos = JoltSubsystem->GetBodyInterface()->GetPosition(BodyID(OwnerPlayerCharacter.PlayerId));

		Vec3 DirectionTowardsPlayer = (PlayerPos - JoltSubsystem->GetBodyInterface()->GetPosition(GetBodyID())).NormalizedOr(Vec3::sZero());
		
		JoltSubsystem->GetBodyInterface()->AddForce(GetBodyID(), DirectionTowardsPlayer * ForcePerStepWhenReturning);
	}
}

void ABoomerangProjectile::Save(StateRecorderImpl& InStream)
{
	Super::Save(InStream);

	InStream.Write(StepsSinceFired);
}

void ABoomerangProjectile::Load(StateRecorderImpl& InStream)
{
	Super::Load(InStream);

	InStream.Read(StepsSinceFired);
}
