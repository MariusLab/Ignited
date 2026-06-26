// Fill out your copyright notice in the Description page of Project Settings.


#include "GravityBomb.h"

#include "JoltPhysicsTest/FunctionLibrary/Helper.h"
#include "JoltPhysicsTest/Jolt/JoltConverter.h"
#include "JoltPhysicsTest/Player/MyPlayerController.h"

void AGravityBomb::InitializeBody(const FJoltBodyData& JoltBodyData)
{
	if (BelongsToPlayerId > -1)
	{
		FPlayerCharacter& OwnerPlayerCharacter = UHelper::GetLocalPlayerController(GetWorld())->GetPlayerCharacter(BelongsToPlayerId);
		BP_OnUpdatePlayerTeam(OwnerPlayerCharacter.PlayerTeam);
	}
	
	Super::InitializeBody(JoltBodyData);
}

void AGravityBomb::StepSimulation()
{
	Super::StepSimulation();

	if (StepsSinceFired >= ActivateOnFrame)
	{
		SetObjectLayer(EObjectLayer::NoCollision);
	}
	
	if (StepsSinceFired >= ActivateOnFrame)
	{
		auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();

		const Vec3 GravityBombPos = JoltSubsystem->GetBodyInterface()->GetPosition(GetBodyID());
	
		SphereShape Sphere(AffectArea);
		Sphere.SetEmbedded();
		const Vec3 CastPosition = GravityBombPos;
	
		RayObjectLayerFilter ObjectLayerFilter({EObjectLayer::Moving, EObjectLayer::Projectile, EObjectLayer::Player2, EObjectLayer::Boomerang, EObjectLayer::Blade});
		auto CollectedHits = JoltSubsystem->CollideShape(Sphere, CastPosition, &ObjectLayerFilter);

#if WITH_EDITOR
		if (bDebugDrawAffectArea)
		{
			DrawDebugSphere(GetWorld(), Units::FromJoltPos(CastPosition), Units::FromJoltUnits(Sphere.GetRadius()), 10.f, FColor::Red, false, 0.016f, SDPG_World, 30.f);
		}
#endif

		const BodyID GravityBombBodyID =  GetBodyID();
		for (const CollideShapeResult& mHit : CollectedHits.mHits)
		{
			if (mHit.mBodyID2 == GravityBombBodyID)
			{
				continue;
			}
			
			//auto BodyActor = SpawnerSubsystem->GetBodyActor(mHit.mBodyID2.GetIndex());
			const Vec3 HitBodyPos = JoltSubsystem->GetBodyInterface()->GetPosition(mHit.mBodyID2);
			// InverseMass = 1 / kg
			const float HitBodyInverseMass = JoltSubsystem->GetPhysicsSystem()->GetBodyLockInterfaceNoLock().TryGetBody(mHit.mBodyID2)->GetMotionProperties()->GetInverseMass();
			const float ForceToApply = ForceToApplyToAffectedBodies / HitBodyInverseMass;
			
			const Vec3 PullDirection = (GravityBombPos - HitBodyPos).NormalizedOr(Vec3::sAxisX());
			JoltSubsystem->GetBodyInterface()->AddForce(mHit.mBodyID2, PullDirection * ForceToApply, EActivation::DontActivate);
		}
	}

	StepsSinceFired++;

	if (StepsSinceFired >= FramesToLive)
	{
		GetWorld()->GetSubsystem<USpawnerSubsystem>()->DespawnJoltBody(GetBodyID());
	}
}

void AGravityBomb::Save(StateRecorderImpl& InStream)
{
	Super::Save(InStream);

	InStream.Write(StepsSinceFired);
}

void AGravityBomb::Load(StateRecorderImpl& InStream)
{
	Super::Load(InStream);

	InStream.Read(StepsSinceFired);
	if (StepsSinceFired >= ActivateOnFrame)
	{
		SetObjectLayer(EObjectLayer::NoCollision);
	}
	else
	{
		SetObjectLayer(EObjectLayer::Moving);
	}
}

void AGravityBomb::SetObjectLayer(EObjectLayer NewObjectLayer)
{
	if (CurrentObjectLayer == NewObjectLayer)
	{
		return;
	}
	
	GetWorld()->GetSubsystem<UJoltSubsystem>()->SetObjectLayer(GetBodyID().GetIndex(), NewObjectLayer);
	CurrentObjectLayer = NewObjectLayer;
	if (NewObjectLayer == EObjectLayer::NoCollision)
	{
		BP_OnGravityActivated();
	}
	else
	{
		BP_OnGravityDeactivated();
	}
}
