// Fill out your copyright notice in the Description page of Project Settings.


#include "FlagBase.h"

#include "Flag.h"
#include "JoltPhysicsTest/Components/JoltCapsuleComponent.h"
#include "JoltPhysicsTest/Data/AssetManagerSubsystem.h"
#include "JoltPhysicsTest/FunctionLibrary/Helper.h"
#include "JoltPhysicsTest/Game/GameLoopSubsystem.h"
#include "JoltPhysicsTest/Player/MyPlayerController.h"


void AFlagBase::InitializeBody(const FJoltBodyData& JoltBodyData)
{
	Super::InitializeBody(JoltBodyData);

	GetWorld()->GetSubsystem<UGameLoopSubsystem>()->OnRenderFrame.AddUniqueDynamic(this, &AFlagBase::OnRender);
}

void AFlagBase::UpdateTeam(EPlayerTeam NewPlayerTeam)
{
	BelongsToTeam = NewPlayerTeam;
	OnUpdateTeam();
}

void AFlagBase::Save(StateRecorderImpl& InStream)
{
	Super::Save(InStream);

	InStream.Write(bFlagTaken);
}

void AFlagBase::Load(StateRecorderImpl& InStream)
{
	Super::Load(InStream);

	InStream.Read(bFlagTaken);
}

int AFlagBase::TakeFlag()
{
	if (bFlagTaken)
	{
		return SpawnedFlagBodyIndex;
	}
	
	bFlagTaken = true;

	auto MyPlayerController = UHelper::GetLocalPlayerController(GetWorld());
	auto SpawnerSubsystem = GetWorld()->GetSubsystem<USpawnerSubsystem>();
	SpawnedFlagBodyIndex = SpawnerSubsystem->SpawnJoltBody(MyPlayerController->FlagClass, GetActorLocation(), FRotator::ZeroRotator);
	AFlag* Flag = Cast<AFlag>(SpawnerSubsystem->GetBodyActor(SpawnedFlagBodyIndex));
	Flag->SetBelongsToTeam(BelongsToTeam);
	Flag->TakenFromFlagBaseBodyIndex = GetBodyID().GetIndex();

	return SpawnedFlagBodyIndex;
}

void AFlagBase::ReturnFlag()
{
	bFlagTaken = false;
}

bool AFlagBase::IsFlagTaken() const
{
	return bFlagTaken;
}

void AFlagBase::DestroyBody()
{
	Super::DestroyBody();

	if (SpawnedFlagBodyIndex >= 0)
	{
		auto SpawnerSubsystem = GetWorld()->GetSubsystem<USpawnerSubsystem>();
		if (SpawnerSubsystem->DoesBodyExistInSimulation(SpawnedFlagBodyIndex))
		{
			SpawnerSubsystem->DespawnJoltBodyWithRelatedConstraints(SpawnedFlagBodyIndex);
		}
	}
}

void AFlagBase::OnRender()
{
	if (bFlagTakenRender != bFlagTaken)
	{
		bFlagTakenRender = bFlagTaken;
		if (bFlagTakenRender)
		{
			OnFlagTaken();
		}
		else
		{
			OnFlagReturned();
		}
	}
}
