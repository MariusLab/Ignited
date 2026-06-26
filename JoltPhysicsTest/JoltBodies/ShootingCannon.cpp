// Fill out your copyright notice in the Description page of Project Settings.


#include "ShootingCannon.h"

#include "JoltPhysicsTest/FunctionLibrary/Helper.h"
#include "JoltPhysicsTest/Jolt/JoltConverter.h"
#include "JoltPhysicsTest/Jolt/JoltSubsystem.h"
#include "JoltPhysicsTest/Player/MyPlayerController.h"
#include "JoltPhysicsTest/Sound/SoundSubsystem.h"


void AShootingCannon::InitializeBody(const FJoltBodyData& JoltBodyData)
{
	Super::InitializeBody(JoltBodyData);

	// We must set collision group manually so that it would exist when we fetch it later shooting projectiles
	//Ref<GroupFilterTable> DisableCollisionFilter = new GroupFilterTable();
	//GetWorld()->GetSubsystem<UJoltSubsystem>()->GetBodyInterface()->SetCollisionGroup(GetBodyID(), CollisionGroup(DisableCollisionFilter, JoltBodyData.BodyIndex, 0));
}

void AShootingCannon::StepSimulation()
{
	Super::StepSimulation();

	if (RemainingShootCooldown > 0)
	{
		RemainingShootCooldown--;
	}
	
	if (RemainingShootCooldown == 0)
	{
		auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();
		auto LocalPlayerController = UHelper::GetLocalPlayerController(GetWorld());

		const Vec3 TurretPos = JoltSubsystem->GetBodyInterface()->GetPosition(GetBodyID());
		const Quat TurretRotation = JoltSubsystem->GetBodyInterface()->GetRotation(GetBodyID());

		// The issue is that when a body gets mirrored its rotation doesn't change, so attack direction stays the same
		// For a mirrored body vs non mirrored one. Which is obviously not correct. This is a hack for now to solve the issue.
		/*float DirectionMultiplier = -1.f;
		constexpr float XAxisMiddle = 100000.f;
		if (GetActorLocation().X > XAxisMiddle)
		{
			DirectionMultiplier = 1.f;
		}*/
		
		const Vec3 AttackDirection = TurretRotation * JPH::Vec3::sAxisX();

		const FVector ProjectileSpawnLocation = Units::FromJoltPos(TurretPos + AttackDirection * ProjectileSpawnForwardDistance);
		int SpawnedBodyIndex = GetWorld()->GetSubsystem<USpawnerSubsystem>()->SpawnJoltBody(ProjectileActorClass, LocalPlayerController->GetTransformWithMirroredAxis(ProjectileSpawnLocation, VecDirectionToFRotator(AttackDirection)));
		LastSpawnedProjectileIndex = SpawnedBodyIndex;
		
		const BodyID ProjectileBodyID = BodyID(SpawnedBodyIndex);
		
		const CollisionGroup& CannonCollisionGroup = JoltSubsystem->GetBodyInterface()->GetCollisionGroup(GetBodyID());
		JoltSubsystem->GetBodyInterface()->SetCollisionGroup(ProjectileBodyID, CannonCollisionGroup);
		
		JoltSubsystem->GetBodyInterface()->AddImpulse(ProjectileBodyID, AttackDirection * ProjectileImpulse);
			
		RemainingShootCooldown = CooldownFramesBetweenShots;

		UHelper::GetLocalPlayerController(GetWorld())->SpawnShootCannonVFX(FVector::ZeroVector);
	}
}

void AShootingCannon::Save(StateRecorderImpl& InStream)
{
	Super::Save(InStream);

	InStream.Write(RemainingShootCooldown);
}

void AShootingCannon::Load(StateRecorderImpl& InStream)
{
	Super::Load(InStream);

	InStream.Read(RemainingShootCooldown);
}

void AShootingCannon::DestroyBody()
{
	Super::DestroyBody();

	if (LastSpawnedProjectileIndex >= 0)
	{
		auto SpawnerSubsystem = GetWorld()->GetSubsystem<USpawnerSubsystem>();
		if (SpawnerSubsystem->DoesBodyExistInSimulation(LastSpawnedProjectileIndex))
		{
			SpawnerSubsystem->DespawnJoltBody(LastSpawnedProjectileIndex);
		}
	}
}