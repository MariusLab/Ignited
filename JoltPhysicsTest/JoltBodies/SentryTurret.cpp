// Fill out your copyright notice in the Description page of Project Settings.


#include "SentryTurret.h"

#include "JoltPhysicsTest/FunctionLibrary/Helper.h"
#include "JoltPhysicsTest/Jolt/JoltConverter.h"
#include "JoltPhysicsTest/Player/MyPlayerController.h"


void ASentryTurret::InitializeBody(const FJoltBodyData& JoltBodyData)
{
	Super::InitializeBody(JoltBodyData);
	
	if (BelongsToPlayerId > -1)
	{
		auto LocalPlayerController = UHelper::GetLocalPlayerController(GetWorld());
		
		FPlayerCharacter& OwnerPlayerCharacter = LocalPlayerController->GetPlayerCharacter(BelongsToPlayerId);
		OwningPlayerTakeDamageSensorIndex = OwnerPlayerCharacter.TakeDamageSensorBodyIndex;
		BP_OnUpdatePlayerTeam(OwnerPlayerCharacter.PlayerTeam);

		auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();
		Ref<GroupFilterTable> DisableCollisionFilter = new GroupFilterTable();
		JoltSubsystem->GetBodyInterface()->SetCollisionGroup(GetBodyID(), CollisionGroup(DisableCollisionFilter, GetBodyID().GetIndex(), 0));
	}
}

void ASentryTurret::StepSimulation()
{
	Super::StepSimulation();
	
	if (RemainingShootCooldown > 0)
	{
		RemainingShootCooldown--;
	}

	const BodyID TurretBodyID =  GetBodyID();
	
	if (StepsSincePlaced > 10 && RemainingShootCooldown == 0)
	{
		auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();

		const Vec3 TurretPos = JoltSubsystem->GetBodyInterface()->GetPosition(GetBodyID());
	
		SphereShape Sphere(MaxAttackDistance);
		Sphere.SetEmbedded();
		const Vec3 CastPosition = TurretPos;
	
		RayObjectLayerFilter ObjectLayerFilter({EObjectLayer::Projectile, EObjectLayer::PlayerTakeDamageSensor});
		auto CollectedHits = JoltSubsystem->CollideShape(Sphere, CastPosition, &ObjectLayerFilter);
		CollectedHits.Sort();

#if WITH_EDITOR
		if (bDebugDrawAffectArea)
		{
			DrawDebugSphere(GetWorld(), Units::FromJoltPos(CastPosition), Units::FromJoltUnits(Sphere.GetRadius()), 10.f, FColor::Red, false, 1.f, SDPG_World, 30.f);
		}
#endif

		auto SpawnerSubsystem = GetWorld()->GetSubsystem<USpawnerSubsystem>();
		for (const CollideShapeResult& mHit : CollectedHits.mHits)
		{
			if (SpawnerSubsystem->DoesBodyExistInSimulation(mHit.mBodyID2) && SpawnerSubsystem->GetBodyActor(mHit.mBodyID2)->BelongsToPlayerId == BelongsToPlayerId)
			{
				continue;
			}
			
			//auto BodyActor = SpawnerSubsystem->GetBodyActor(mHit.mBodyID2.GetIndex());
			const Vec3 TargetBodyPos = JoltSubsystem->GetBodyInterface()->GetPosition(mHit.mBodyID2);
			const Vec3 AttackDirection = (TargetBodyPos - TurretPos).NormalizedOr(Vec3::sAxisX());

			const FVector ProjectileSpawnLocation = Units::FromJoltPos(TurretPos + AttackDirection * ProjectileSpawnForwardDistance);
			int SpawnedBodyIndex = GetWorld()->GetSubsystem<USpawnerSubsystem>()->SpawnJoltBody(ProjectileActorClass, UHelper::GetLocalPlayerController(GetWorld())->GetTransformWithMirroredAxis(ProjectileSpawnLocation, VecDirectionToFRotator(AttackDirection)), BelongsToPlayerId);

			auto TurretCollisionGroup = JoltSubsystem->GetBodyInterface()->GetCollisionGroup(TurretBodyID);
			JoltSubsystem->GetBodyInterface()->SetCollisionGroup(BodyID(SpawnedBodyIndex), TurretCollisionGroup);
			
			// TODO: we need to cleanup projectiles when new level loads. Currently I think it will keep flying in the next level
			JoltSubsystem->GetBodyInterface()->AddImpulse(BodyID(SpawnedBodyIndex), AttackDirection * ProjectileImpulse);

#if WITH_EDITOR
			if (bDebugDrawCurrentTarget)
			{
				DrawDebugSphere(GetWorld(), Units::FromJoltPos(TargetBodyPos), 2000.f, 10.f, FColor::Orange, false, 1.f, SDPG_World, 30.f);
			}
#endif

			RemainingShootCooldown = CooldownFramesBetweenShots;
			break;
		}
	}

	StepsSincePlaced++;
		
	if (StepsSincePlaced >= FramesToLive)
	{
		GetWorld()->GetSubsystem<USpawnerSubsystem>()->DespawnJoltBody(TurretBodyID);
	}
}

void ASentryTurret::Save(StateRecorderImpl& InStream)
{
	Super::Save(InStream);

	InStream.Write(StepsSincePlaced);
	InStream.Write(RemainingShootCooldown);
}

void ASentryTurret::Load(StateRecorderImpl& InStream)
{
	Super::Load(InStream);

	InStream.Read(StepsSincePlaced);
	InStream.Read(RemainingShootCooldown);
}

