// Fill out your copyright notice in the Description page of Project Settings.


#include "ExplosiveBarrel.h"

#include "JoltPhysicsTest/FunctionLibrary/Helper.h"
#include "JoltPhysicsTest/Jolt/JoltSubsystem.h"
#include "JoltPhysicsTest/Player/MyPlayerController.h"


void AExplosiveBarrel::Save(StateRecorderImpl& InStream)
{
	Super::Save(InStream);

	InStream.Write(bExploded);
	InStream.Write(StepsSinceMarkedAsExploded);
}

void AExplosiveBarrel::Load(StateRecorderImpl& InStream)
{
	Super::Load(InStream);

	InStream.Read(bExploded);
	InStream.Read(StepsSinceMarkedAsExploded);
}

void AExplosiveBarrel::StepSimulation()
{
	Super::StepSimulation();

	if (bExploded)
	{
		if (StepsSinceMarkedAsExploded == FramesNumAfterExplosionToDealDamage)
		{
			ExplodeInternal();
		}
		
		StepsSinceMarkedAsExploded++;
	}
}

void AExplosiveBarrel::Explode(const int& InFrameId)
{
	if (bExploded)
	{
		return;
	}

	bExploded = true;
	ExplodedOnFrameId = InFrameId;
}

void AExplosiveBarrel::ExplodeInternal()
{
	UHelper::GetLocalPlayerController(GetWorld())->ExplosiveBarrelHit(ExplodedOnFrameId, GetBodyID());
}
