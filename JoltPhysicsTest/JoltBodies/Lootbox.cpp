// Fill out your copyright notice in the Description page of Project Settings.


#include "Lootbox.h"

#include "JoltPhysicsTest/Player/MyPlayerController.h"

void ALootbox::StepSimulation()
{
	Super::StepSimulation();

	if (FramesSincePickedUp >= 0)
	{
		FramesSincePickedUp++;

		if (FramesSincePickedUp > LootRespawnEveryXFrames)
		{
			FramesSincePickedUp = -1;
			SetActorHiddenInGame(false);
		}
	}
}

void ALootbox::Save(StateRecorderImpl& InStream)
{
	Super::Save(InStream);
	
	InStream.Write(FramesSincePickedUp);
}

void ALootbox::Load(StateRecorderImpl& InStream)
{
	Super::Load(InStream);

	int PreviousFramesSincePickedUp = FramesSincePickedUp;
	InStream.Read(FramesSincePickedUp);

	if (PreviousFramesSincePickedUp == -1 && FramesSincePickedUp >= 0)
	{
		SetActorHiddenInGame(true);
	}

	if (PreviousFramesSincePickedUp >= 0 && FramesSincePickedUp == -1)
	{
		SetActorHiddenInGame(false);
	}
}

void ALootbox::Pickup()
{
	FramesSincePickedUp = 0;
	SetActorHiddenInGame(true);
}

bool ALootbox::IsAvailableForPickup() const
{
	return FramesSincePickedUp == -1;
}
