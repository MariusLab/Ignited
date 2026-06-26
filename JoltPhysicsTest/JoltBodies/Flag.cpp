// Fill out your copyright notice in the Description page of Project Settings.


#include "Flag.h"

#include "FlagBase.h"
#include "JoltPhysicsTest/Data/AssetManagerSubsystem.h"
#include "JoltPhysicsTest/FunctionLibrary/Helper.h"
#include "JoltPhysicsTest/Game/MyGameState.h"
#include "JoltPhysicsTest/Game/SpawnerSubsystem.h"
#include "JoltPhysicsTest/Player/MyPlayerController.h"

AFlag::AFlag()
{
	RootSceneComp = CreateDefaultSubobject<USceneComponent>(TEXT("RootSceneComp"));
	SetRootComponent(RootSceneComp);

	ConstraintAttachSceneComp = CreateDefaultSubobject<USceneComponent>(TEXT("ConstraintAttachSceneComp"));
	ConstraintAttachSceneComp->SetupAttachment(RootSceneComp);
}

void AFlag::StepSimulation()
{
	Super::StepSimulation();

	if (!bPlacedInBasePending)
	{
		return;
	}

	StepsSincePlacedInBase++;

	auto LocalPlayerController = UHelper::GetLocalPlayerController(GetWorld());

	if (StepsSincePlacedInBase > LocalPlayerController->GetRollbackWindow())
	{
		if (!bPlacedInBaseConfirmed)
		{
			bPlacedInBaseConfirmed = true;
			LocalPlayerController->IncreaseCurrentGameModeOppositeTeamsScore(BelongsToTeam);
		}

		if (LocalPlayerController->GetCurrentGameModeTeamScores().GetOppositeTeamScore(BelongsToTeam) == UAssetManagerSubsystem::GetGeneralData(GetWorld())->CaptureTheFlagMaxPoints)
		{
			UHelper::GetMyGameState(GetWorld())->EndRound();
		}
		
		auto SpawnerSubsystem = GetWorld()->GetSubsystem<USpawnerSubsystem>();
		
		auto TakenFromFlagBase = Cast<AFlagBase>(SpawnerSubsystem->GetBodyActor(TakenFromFlagBaseBodyIndex));
		check(TakenFromFlagBase);

		TakenFromFlagBase->ReturnFlag();

		SpawnerSubsystem->DespawnJoltBodyWithRelatedConstraints(GetBodyID());
	}
}

void AFlag::Save(StateRecorderImpl& InStream)
{
	Super::Save(InStream);

	InStream.Write(StepsSincePlacedInBase);
	InStream.Write(bPlacedInBasePending);
}

void AFlag::Load(StateRecorderImpl& InStream)
{
	Super::Load(InStream);

	InStream.Read(StepsSincePlacedInBase);
	InStream.Read(bPlacedInBasePending);
}

void AFlag::SetBelongsToTeam(EPlayerTeam PlayerTeam)
{
	BelongsToTeam = PlayerTeam;
	OnTeamUpdated(BelongsToTeam);
}

void AFlag::PlaceInBase()
{
	bPlacedInBasePending = true;
}

void AFlag::ReturnHome()
{
	auto SpawnerSubsystem = GetWorld()->GetSubsystem<USpawnerSubsystem>();
		
	auto TakenFromFlagBase = Cast<AFlagBase>(SpawnerSubsystem->GetBodyActor(TakenFromFlagBaseBodyIndex));
	check(TakenFromFlagBase);

	TakenFromFlagBase->ReturnFlag();

	SpawnerSubsystem->DespawnJoltBodyWithRelatedConstraints(GetBodyID());
}

bool AFlag::IsPlacedInBase() const
{
	return bPlacedInBasePending || bPlacedInBaseConfirmed;
}
