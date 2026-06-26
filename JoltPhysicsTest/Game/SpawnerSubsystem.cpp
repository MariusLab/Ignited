// Fill out your copyright notice in the Description page of Project Settings.


#include "SpawnerSubsystem.h"

#include <iostream>

#include "GameLoopSubsystem.h"
#include "JoltPhysicsTest/Debug/CustomLogger.h"
#include "JoltPhysicsTest/Jolt/JoltSubsystem.h"

bool USpawnerSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (!Super::ShouldCreateSubsystem(Outer))
	{
		return false;
	}

	const TArray AllowedNetModes{NM_ListenServer, NM_Client, NM_Standalone};

	const UWorld* World = Cast<UWorld>(Outer);
	if (World == nullptr)
	{
		return false;
	}

	// For some reason in standalone builds when you launch the game, it first creates this subsystem on some "Untitled" world
	// Which doesn't even have a game instance, so everything goes to hell.
	// So we just skip creating it if that's the case
	if (!World->GetGameInstance())
	{
		return false;
	}

	if (World->WorldType != EWorldType::Game && World->WorldType != EWorldType::PIE)
	{
		return false;
	}

	return AllowedNetModes.Contains(World->GetNetMode());
}

void USpawnerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void USpawnerSubsystem::StepSimulation()
{
	for (const int& BodyIndex : CurrentlySpawnedBodyActors.GetIdsInStackOrder())
	{
		if (BodyIndexesInSimulation.Contains(BodyIndex))
		{
			auto JoltBodyActor = CurrentlySpawnedBodyActors.Get(BodyIndex);
			JoltBodyActor->StepSimulation();
			JoltBodyActor->StaticClass();
		}
	}

	for (const int& ConstraintId : CurrentlySpawnedConstraintActors.GetIdsInStackOrder())
	{
		if (ConstraintIdsInSimulation.Contains(ConstraintId))
		{
			auto JoltConstraintActor = CurrentlySpawnedConstraintActors.Get(ConstraintId);
			JoltConstraintActor->StepSimulation();
		}
	}
}

TArray<FVector2D> USpawnerSubsystem::GetAttachedMeshesVertices()
{
	TArray<FVector2D> MeshesVertices;
	for (const int& BodyIndex : CurrentlySpawnedBodyActors.GetIdsInStackOrder())
	{
		if (BodyIndexesInSimulation.Contains(BodyIndex))
		{
			auto JoltBodyActor = CurrentlySpawnedBodyActors.Get(BodyIndex);

			MeshesVertices += JoltBodyActor->GetAttachedMeshVertices();
		}
	}

	return MeshesVertices;
}

int USpawnerSubsystem::SpawnJoltBody(
	TSubclassOf<AJoltBodyActor> BodyClass,
	const FVector& Location,
	const FRotator& Rotation,
	int BelongsToPlayerId
)
{
	return SpawnJoltBody(BodyClass, Location, Rotation, FVector(1.f, 1.f, 1.f), BelongsToPlayerId);
}

int USpawnerSubsystem::SpawnJoltBody(TSubclassOf<AJoltBodyActor> BodyClass, const FTransform& InTransform,
	int BelongsToPlayerId)
{
	return SpawnJoltBody(BodyClass, InTransform.GetLocation(), InTransform.Rotator(), InTransform.GetScale3D(), BelongsToPlayerId);
}

int USpawnerSubsystem::SpawnJoltBody(TSubclassOf<AJoltBodyActor> BodyClass, const FVector& Location,
	const FRotator& Rotation, const FVector& Scale, int BelongsToPlayerId)
{
	const FTransform SpawnTransform{
		Rotation,
		Location,
		Scale
	};

	const int SteppingFrameId = GetWorld()->GetSubsystem<UGameLoopSubsystem>()->GetSteppingFrameId();

	auto SpawnedBody = Cast<AJoltBodyActor>(GetWorld()->SpawnActor(BodyClass, &SpawnTransform));
	SpawnedBody->BelongsToPlayerId = BelongsToPlayerId;
	
	const int BodyIndex = CurrentlySpawnedBodyActors.Add(SpawnedBody);

	SpawnedBody->InitializeBody({BodyIndex});

	check(!BodyIndexesInSimulation.Contains(BodyIndex));
	BodyIndexesInSimulation.Add(BodyIndex);

	BodySpawnedOnFrame.Add(BodyIndex, SteppingFrameId);

	LOG(TEXT("[SpawnerSubsystem] Spawned jolt body id %d"), BodyIndex);

	return BodyIndex;
}

void USpawnerSubsystem::DespawnJoltBody(const BodyID& InBodyID)
{
#ifdef MY_DEBUG_VALIDATE_CONSTRAINTS_GET_REMOVED_BEFORE_BODY
	for (const int& ConstraintId : CurrentlySpawnedConstraintActors.GetIdsInStackOrder())
	{
		auto ConstraintActor = CurrentlySpawnedConstraintActors.Get(ConstraintId);
		if (ConstraintActor->GetConstraintData().BodyID1 == InBodyID || ConstraintActor->GetConstraintData().BodyID2 == InBodyID)
		{
			checkf(TombstonedConstraintIds.Contains(ConstraintId), TEXT("Trying to despawn body that still has active constraints attached."));
		}
	}
#endif

	const int SteppingFrameId = GetWorld()->GetSubsystem<UGameLoopSubsystem>()->GetSteppingFrameId();
	
	const auto BodyIndex = InBodyID.GetIndex();
	
	check(BodyIndexesInSimulation.Contains(BodyIndex));
	BodyIndexesInSimulation.Remove(BodyIndex);

	// If this body was spawned and destroyed in the same frame, meaning same physics step
	// Then we treat it as a special case where we want to restore everything as if it never happened
	// No tombstoning, just restoring
	// The array might not contain this body index at all if AddJoltBody() method was used instead. Which is mainly used to load bodies in the level.
	if (BodySpawnedOnFrame.Contains(BodyIndex) && BodySpawnedOnFrame[BodyIndex] == SteppingFrameId)
	{
		BodySpawnedOnFrame.Remove(BodyIndex);

		auto JoltBodyActor = CurrentlySpawnedBodyActors.Get(BodyIndex);
		CurrentlySpawnedBodyActors.Remove(BodyIndex);

		JoltBodyActor->RemoveAndDestroyBody();
		JoltBodyActor->DestroyActor();
		
		return;
	}
	
	// We don't destroy the Actor itself here. Instead we "tombstone" it.
	// It means it's non functional and marked for destruction as soon as it goes out the rollback window.
	// This allows to rollback before this actor was destroyed without fully recreating it, all we need to do is re-add the bodies to physics simulation and it works as good as new
	TombstoneBody(BodyIndex);
	LOG(TEXT("[SpawnerSubsystem] Tombstoned jolt body id %d"), BodyIndex);
}

void USpawnerSubsystem::DespawnJoltBody(const int& InBodyIndex)
{
	DespawnJoltBody(BodyID(InBodyIndex));
}

void USpawnerSubsystem::DespawnJoltBodyWithRelatedConstraints(const BodyID& InBodyID)
{
	for (const int& ConstraintId : CurrentlySpawnedConstraintActors.GetIdsInStackOrder())
	{
		// Constraints often attach to 2 bodies. If we try to despawn both on same frame, we'll run into issues, so we need to check
		if (TombstonedConstraintIds.Contains(ConstraintId))
		{
			continue;
		}
		
		auto ConstraintActor = CurrentlySpawnedConstraintActors.Get(ConstraintId);
		if (ConstraintActor->GetConstraintData().BodyID1 == InBodyID || ConstraintActor->GetConstraintData().BodyID2 == InBodyID)
		{
			DespawnJoltConstraint(ConstraintId);
		}
	}

	DespawnJoltBody(InBodyID);
}

void USpawnerSubsystem::DespawnJoltBodyWithRelatedConstraints(const int& InBodyIndex)
{
	DespawnJoltBodyWithRelatedConstraints(BodyID(InBodyIndex));
}

bool USpawnerSubsystem::DoesBodyExistInSimulation(const BodyID& InBodyID) const
{
	return BodyIndexesInSimulation.Contains(InBodyID.GetIndex());
}

bool USpawnerSubsystem::DoesBodyExistInSimulation(const int& InBodyIndex)
{
	return DoesBodyExistInSimulation(BodyID(InBodyIndex));
}

bool USpawnerSubsystem::DoesConstraintExistInSimulation(const int& InConstraintId) const
{
	return ConstraintIdsInSimulation.Contains(InConstraintId);
}

int USpawnerSubsystem::AddJoltBody(AJoltBodyActor* JoltBodyActor)
{
	const int BodyIndex = CurrentlySpawnedBodyActors.Add(JoltBodyActor);
	
	JoltBodyActor->InitializeBody({BodyIndex});

	check(!BodyIndexesInSimulation.Contains(BodyIndex));
	BodyIndexesInSimulation.Add(BodyIndex);

	LOG(TEXT("[SpawnerSubsystem] Added jolt body id %d"), BodyIndex);

	return BodyIndex;
}

int USpawnerSubsystem::SpawnJoltConstraint(TSubclassOf<AJoltConstraintActor> ConstraintClass, const FVector& Location, FJoltConstraintData JoltConstraintData)
{
	const FTransform SpawnTransform{
		FRotator::ZeroRotator,
		Location,
		FVector(1, 1, 1)
	};

	auto SpawnedConstraint = Cast<AJoltConstraintActor>(GetWorld()->SpawnActor(ConstraintClass, &SpawnTransform));
	const int ConstraintId = CurrentlySpawnedConstraintActors.Add(SpawnedConstraint);

	// ConstraintPriority is Jolts way of allowing us to specify deterministic ordering for all constraints
	JoltConstraintData.ConstraintPriority = ConstraintId;
	SpawnedConstraint->InitializeConstraint(JoltConstraintData);
	
	check(!ConstraintIdsInSimulation.Contains(ConstraintId));
	ConstraintIdsInSimulation.Add(ConstraintId);

	LOG(TEXT("[SpawnerSubsystem] Spawned jolt constraint %d"), ConstraintId);

	return ConstraintId;
}

void USpawnerSubsystem::DespawnJoltConstraint(const int& ConstraintId)
{
	check(ConstraintIdsInSimulation.Contains(ConstraintId));
	ConstraintIdsInSimulation.Remove(ConstraintId);
	
	// We don't destroy the Actor itself here. Instead we "tombstone" it.
	// It means it's non functional and will get destroyed as soon as it goes out the rollback window.
	// This allows to rollback before this actor was destroyed without fully recreating it, all we need to do is re-add the constraint to physics simulation and it works as good as new
	TombstoneConstraint(ConstraintId);
	LOG(TEXT("[SpawnerSubsystem] Tombstoned constraint %d"), ConstraintId);
}

int USpawnerSubsystem::AddJoltConstraint(AJoltConstraintActor* JoltConstraintActor)
{
	const int ConstraintId = CurrentlySpawnedConstraintActors.Add(JoltConstraintActor);

	// ConstraintPriority is Jolts way of allowing us to specify deterministic ordering for all constraints
	// We fetch a copy of existing constraint data, because it has BodyIDs already set on it and we don't want to overwrite them
	FJoltConstraintData JoltConstraintData = JoltConstraintActor->GetConstraintData();
	JoltConstraintData.ConstraintPriority = ConstraintId;
	JoltConstraintActor->InitializeConstraint(JoltConstraintData);
	
	check(!ConstraintIdsInSimulation.Contains(ConstraintId));
	ConstraintIdsInSimulation.Add(ConstraintId);

	LOG(TEXT("[SpawnerSubsystem] Added jolt constraint %d"), ConstraintId);

	return ConstraintId;
}

void USpawnerSubsystem::SaveObjects(const int& InFrameId)
{
	//LOG(TEXT("[SpawnerSubsystem] Save frame %d"), InFrameId);
	if (SavedFrames.Contains(InFrameId))
	{
		//LOG(TEXT("[SpawnerSubsystem] Overwrite frame %d"), InFrameId);
	}
	else
	{
		SavedFrames.Add(InFrameId, {});
	}

	SavedFrames[InFrameId].SpawnedBodyIndexes = BodyIndexesInSimulation;
	SavedFrames[InFrameId].SpawnedConstraintIds = ConstraintIdsInSimulation;

	LOG(TEXT("[SpawnerSubsystem] Active Bodies(%d) and Constraints(%d)"), BodyIndexesInSimulation.Num(), ConstraintIdsInSimulation.Num());
	LOG(TEXT("[SpawnerSubsystem] Tombstoned Bodies(%d) and Constraints(%d)"), TombstonedBodyIds.Num(), TombstonedConstraintIds.Num());
	LOG(TEXT("[SpawnerSubsystem] Jolt constraints manager num(%d)"), GetWorld()->GetSubsystem<UJoltSubsystem>()->GetPhysicsSystem()->GetConstraints().size());
}

void USpawnerSubsystem::LoadObjects(const int& InFrameId)
{
	//LOG(TEXT("[SpawnerSubsystem] Load frame %d"), InFrameId);
	check(SavedFrames.Contains(InFrameId));

	/**
	 * The order of these calls is very important.
	 * For destruction we must destroy constraints first.
	 * But for recreation, we must create the bodies first.
	 **/
	DestroyConstraints_Internal(InFrameId);
	DestroyBodies_Internal(InFrameId);

	RecreateBodies_Internal(InFrameId);
	RecreateConstraints_Internal(InFrameId);

	//LOG(TEXT("[SpawnerSubsystem] Active Bodies(%d) and Constraints(%d)"), BodyIndexesInSimulation.Num(), ConstraintIdsInSimulation.Num());
	//LOG(TEXT("[SpawnerSubsystem] Tombstoned Bodies(%d) and Constraints(%d)"), TombstonedBodyIds.Num(), TombstonedConstraintIds.Num());
	//LOG(TEXT("[SpawnerSubsystem] Jolt constraints manager num(%d)"), GetWorld()->GetSubsystem<UJoltSubsystem>()->GetPhysicsSystem()->GetConstraints().size());
}

void USpawnerSubsystem::Remove(const int& InFrameId)
{
	LOG(TEXT("[SpawnerSubsystem] Remove frame %d"), InFrameId);
	
	SavedFrames.Remove(InFrameId);
	
	if (!TombstonedBodyIds.IsEmpty())
	{
		// Destruction order doesn't matter here. These IDs will never get reused during this simulation. They are thrown away for good.
		auto TempTombstonedBodyIds = TombstonedBodyIds;
		for (const auto& pair : TempTombstonedBodyIds)
		{
			const int BodyIndex = pair.Key;
			const int TombstonedOnFrameId = pair.Value;
			if (TombstonedOnFrameId <= InFrameId)
			{
				auto BodyActor = CurrentlySpawnedBodyActors.Get(BodyIndex);
				BodyActor->DestroyBody();
				BodyActor->DestroyActor();
				CurrentlySpawnedBodyActors.RemovePermanently(BodyIndex);
				
				TombstonedBodyIds.Remove(BodyIndex);
				
				LOG(TEXT("Destroyed tombstoned body %d (permanently)"), BodyIndex);
			}
		}
	}

	if (!TombstonedConstraintIds.IsEmpty())
	{
		// Destruction order doesn't matter here. These IDs will never get reused during this simulation. They are thrown away for good.
		auto TempTombstonedConstraintIds = TombstonedConstraintIds;
		for (const auto& pair : TempTombstonedConstraintIds)
		{
			const int ConstraintId = pair.Key;
			const int TombstonedOnFrameId = pair.Value;
			if (TombstonedOnFrameId <= InFrameId)
			{
				CurrentlySpawnedConstraintActors.Get(ConstraintId)->DestroyActor();
				CurrentlySpawnedConstraintActors.RemovePermanently(ConstraintId);
				TombstonedConstraintIds.Remove(ConstraintId);
				
				LOG(TEXT("Destroyed tombstoned constraint %d (permanently)"), ConstraintId);
			}
		}
	}
}

void USpawnerSubsystem::SaveToStream(StateRecorderImpl& InStream)
{
	// We call this for bodies only for saving custom data
	// For example a treasure chest might want to save if it's been opened already or not
	// An enemy might want to save its health and so on
	for (const int& BodyIndex : CurrentlySpawnedBodyActors.GetIdsInAssignedOrder())
	{
		if (BodyIndexesInSimulation.Contains(BodyIndex))
		{
			CurrentlySpawnedBodyActors.Get(BodyIndex)->Save(InStream);
		}
	}
	
	// We use jolts constraint priority field, which is kind of like the constraint id
	// Since constraints don't really have ids, like bodies.
	// But in order to use priority, we must save and load constraints manually.
	// That's why we don't save/load bodies here, only constraints.
	ConstraintIdsInSimulation.Sort();
	for (const int& ConstraintId : ConstraintIdsInSimulation)
	{
		CurrentlySpawnedConstraintActors.Get(ConstraintId)->Save(InStream);
	}
}

void USpawnerSubsystem::LoadFromStream(StateRecorderImpl& InStream)
{
	// We call this for bodies only for saving custom data
	// For example a treasure chest might want to save if it's been opened already or not
	// An enemy might want to save its health and so on
	for (const int& BodyIndex : CurrentlySpawnedBodyActors.GetIdsInAssignedOrder())
	{
		if (BodyIndexesInSimulation.Contains(BodyIndex))
		{
			CurrentlySpawnedBodyActors.Get(BodyIndex)->Load(InStream);
		}
	}
	
	// We use jolts constraint priority field, which is kind of like the constraint id
	// Since constraints don't really have ids, like bodies.
	// But in order to use priority, we must save and load constraints manually.
	// That's why we don't save/load bodies here, only constraints.
	ConstraintIdsInSimulation.Sort();
	for (const int& ConstraintId : ConstraintIdsInSimulation)
	{
		CurrentlySpawnedConstraintActors.Get(ConstraintId)->Load(InStream);
	}
}

AJoltBodyActor* USpawnerSubsystem::GetBodyActor(const BodyID& InBodyID)
{
	check(CurrentlySpawnedBodyActors.ContainsId(InBodyID.GetIndex()));

	return CurrentlySpawnedBodyActors.Get(InBodyID.GetIndex());
}

AJoltConstraintActor* USpawnerSubsystem::GetConstraintActor(const int& ConstraintId)
{
	check(CurrentlySpawnedConstraintActors.ContainsId(ConstraintId));

	return CurrentlySpawnedConstraintActors.Get(ConstraintId);
}

int USpawnerSubsystem::GetCurrentlySpawnedShapesNum() const
{
	return CurrentlySpawnedBodyActors.Num();
}

int USpawnerSubsystem::GetSavedFramesNum() const
{
	return SavedFrames.Num();
}

FSpawnerSubsystemState USpawnerSubsystem::GetSpawnerState() const
{
	FSpawnerSubsystemState SpawnerSubsystemState;
	SavedFrames.GenerateKeyArray(SpawnerSubsystemState.SavedFramesKeys);
	SavedFrames.GenerateValueArray(SpawnerSubsystemState.SavedFramesValues);
	
	BodySpawnedOnFrame.GenerateKeyArray(SpawnerSubsystemState.BodySpawnedOnFrameKeys);
	BodySpawnedOnFrame.GenerateValueArray(SpawnerSubsystemState.BodySpawnedOnFrameValues);
	
	TombstonedBodyIds.GenerateKeyArray(SpawnerSubsystemState.TombstonedBodyIdsKeys);
	TombstonedBodyIds.GenerateValueArray(SpawnerSubsystemState.TombstonedBodyIdsValues);
	
	TombstonedConstraintIds.GenerateKeyArray(SpawnerSubsystemState.TombstonedConstraintIdsKeys);
	TombstonedConstraintIds.GenerateValueArray(SpawnerSubsystemState.TombstonedConstraintIdsValues);

	SpawnerSubsystemState.BodyIndexesInSimulation = BodyIndexesInSimulation;
	SpawnerSubsystemState.ConstraintIdsInSimulation = ConstraintIdsInSimulation;

	SpawnerSubsystemState.CurrentlySpawnedBodyActors = CurrentlySpawnedBodyActors.GetEntityState();
	SpawnerSubsystemState.CurrentlySpawnedConstraintActors = CurrentlySpawnedConstraintActors.GetEntityState();

	return SpawnerSubsystemState;
}

void USpawnerSubsystem::RebuildSpawnerState(const FSpawnerSubsystemState& SpawnerSubsystemState)
{
	SavedFrames.Empty();
	for (int i = 0; i < SpawnerSubsystemState.SavedFramesKeys.Num(); i++)
	{
		SavedFrames.Add(SpawnerSubsystemState.SavedFramesKeys[i], SpawnerSubsystemState.SavedFramesValues[i]);
	}

	BodySpawnedOnFrame.Empty();
	for (int i = 0; i < SpawnerSubsystemState.BodySpawnedOnFrameKeys.Num(); i++)
	{
		BodySpawnedOnFrame.Add(SpawnerSubsystemState.BodySpawnedOnFrameKeys[i], SpawnerSubsystemState.BodySpawnedOnFrameValues[i]);
	}

	TombstonedBodyIds.Empty();
	for (int i = 0; i < SpawnerSubsystemState.TombstonedBodyIdsKeys.Num(); i++)
	{
		TombstonedBodyIds.Add(SpawnerSubsystemState.TombstonedBodyIdsKeys[i], SpawnerSubsystemState.TombstonedBodyIdsValues[i]);
	}

	TombstonedConstraintIds.Empty();
	for (int i = 0; i < SpawnerSubsystemState.TombstonedConstraintIdsKeys.Num(); i++)
	{
		TombstonedConstraintIds.Add(SpawnerSubsystemState.TombstonedConstraintIdsKeys[i], SpawnerSubsystemState.TombstonedConstraintIdsValues[i]);
	}

	BodyIndexesInSimulation = SpawnerSubsystemState.BodyIndexesInSimulation;
	ConstraintIdsInSimulation = SpawnerSubsystemState.ConstraintIdsInSimulation;

	/*const FTransform SpawnTransform = FTransform::Identity;
	TMap<int, AJoltBodyActor*> BodyActorsMap;
	for (int i = 0; i < SpawnerSubsystemState.CurrentlySpawnedBodyActors.ItemsKeys.Num(); i++)
	{
		BodyActorsMap.Add(
			SpawnerSubsystemState.CurrentlySpawnedBodyActors.ItemsKeys[i],
			Cast<AJoltBodyActor>(GetWorld()->SpawnActor( SpawnerSubsystemState.CurrentlySpawnedBodyActors.ItemsClasses[i], &SpawnTransform))
		);
	}

	TMap<int, AJoltConstraintActor*> ConstraintActorsMap;
	for (int i = 0; i < SpawnerSubsystemState.CurrentlySpawnedConstraintActors.ItemsKeys.Num(); i++)
	{
		ConstraintActorsMap.Add(
			SpawnerSubsystemState.CurrentlySpawnedConstraintActors.ItemsKeys[i],
			Cast<AJoltConstraintActor>(GetWorld()->SpawnActor( SpawnerSubsystemState.CurrentlySpawnedConstraintActors.ItemsClasses[i], &SpawnTransform))
		);
	}*/

	CurrentlySpawnedBodyActors.RebuildEntityState(SpawnerSubsystemState.CurrentlySpawnedBodyActors, GetWorld());
	CurrentlySpawnedConstraintActors.RebuildEntityState(SpawnerSubsystemState.CurrentlySpawnedConstraintActors, GetWorld());
}

void USpawnerSubsystem::DestroyBodies_Internal(const int& InFrameId)
{
	/** Destroy bodies that didn't exist in the frame we are rolling back to */
	// We destroy in the opposite order than we created. So if creation was A -> B -> C, we destroy C -> B -> A
	for (const int& BodyIndex : CurrentlySpawnedBodyActors.GetIdsInStackOrder())
	{
		if (!SavedFrames[InFrameId].SpawnedBodyIndexes.Contains(BodyIndex))
		{
			if (BodyIndexesInSimulation.Contains(BodyIndex))
			{
				auto JoltBodyActor = CurrentlySpawnedBodyActors.Get(BodyIndex);
				BodyIndexesInSimulation.Remove(BodyIndex);
				CurrentlySpawnedBodyActors.RemoveLast(BodyIndex);

				JoltBodyActor->RemoveAndDestroyBody();
				JoltBodyActor->DestroyActor();

				LOG(TEXT("[Load frame %d] Destroyed body that was in simulation %d"), InFrameId, BodyIndex);
			}
			else
			{
#ifdef MY_DEBUG_VALIDATE_DETERMINISM
				// A bit of a hack for making our determinism validation not crash tombstone logic
				// The problem is that in a real game scenario once you rollback to frame N, you are guaranteed to never need to rollback to any frame that is < N
				// Because we always rollback to the oldest frame that was misspredicted and fix it, so the next rollbacks can only be > N
				// But when validating determinism what we do is make each step of the simulation step twice and compare it. Which means after 1st step we rollback to the same frame again.
				// Then later normal rollback does its job and might rollback < N in this case. Which means we just destroyed tombstones that under normal game circumstances
				// We are guaranteed to never need again, but with this validation on we might need them, but they're gone. So the game will crash.
				if (GetWorld()->GetSubsystem<UGameLoopSubsystem>()->GetSteppingFrameId() == InFrameId)
				{
					continue;
				}
#endif

				// If the frame we are rolling back to does not contain this body
				// Then only two possibilities exist:
				// 1) We are rolling back BEFORE the body was created. Means it will get created AND tombstoned again during rollback. So we need to destroy it.
				// 2) We are rolling back AFTER the body was tombstoned. In this case we HAVE TO leave it. Same frame can get stepped through many times.
				// As many as our rollback supports + 1, which is the initial step before any rollbacks.
				// So if we change any state here that we cannot restore - we just made the simulation non-deterministic.
				// Say we destroy this tombstone on 2nd run of this frame. On 3rd run it will no longer be there and its ID will be freed up.
				if (InFrameId < TombstonedBodyIds[BodyIndex])
				{
					DestroyTombstonedBody(BodyIndex);
					TombstonedBodyIds.Remove(BodyIndex);
				}
			}
		}
	}
}

void USpawnerSubsystem::RecreateBodies_Internal(const int& InFrameId)
{
	/** Recreate bodies that existed in the frame we are rolling back to */
	for (const int& BodyIndex : SavedFrames[InFrameId].SpawnedBodyIndexes)
	{
		if (!BodyIndexesInSimulation.Contains(BodyIndex))
		{
			auto* JoltBodyActor = CurrentlySpawnedBodyActors.Get(BodyIndex);
			check(JoltBodyActor);

			check(TombstonedBodyIds.Contains(BodyIndex));
			check(JoltBodyActor->IsInitialized());
			JoltBodyActor->AddBody();
			
			// We must recreate it with the same exact id
			BodyIndexesInSimulation.Add(BodyIndex);
			LOG(TEXT("[SpawnerSubsystem] Re-created tombstoned Body %d"), BodyIndex);

			TombstonedBodyIds.Remove(BodyIndex);
		}
	}
}

void USpawnerSubsystem::DestroyConstraints_Internal(const int& InFrameId)
{
	/** Destroy constraints that didn't exist in the frame we are rolling back to */
	// We destroy in the opposite order than we created. So if creation was A -> B -> C, we destroy C -> B -> A
	for (const int& ConstraintId : CurrentlySpawnedConstraintActors.GetIdsInStackOrder())
	{
		if (!SavedFrames[InFrameId].SpawnedConstraintIds.Contains(ConstraintId))
		{
			if (ConstraintIdsInSimulation.Contains(ConstraintId))
			{
				auto JoltConstraintActor = CurrentlySpawnedConstraintActors.Get(ConstraintId);
				ConstraintIdsInSimulation.Remove(ConstraintId);
				CurrentlySpawnedConstraintActors.RemoveLast(ConstraintId);

				JoltConstraintActor->DestroyConstraint();
				JoltConstraintActor->DestroyActor();

				LOG(TEXT("[Load frame %d] Destroyed constraint that was in simulation %d"), InFrameId, ConstraintId);
			}
			else
			{
#ifdef MY_DEBUG_VALIDATE_DETERMINISM
				// A bit of a hack for making our determinism validation not crash tombstone logic
				// The problem is that in a real game scenario once you rollback to frame N, you are guaranteed to never need to rollback to any frame that is < N
				// Because we always rollback to the oldest frame that was misspredicted and fix it, so the next rollbacks can only be > N
				// But when validating determinism what we do is make each step of the simulation step twice and compare it. Which means after 1st step we rollback to the same frame again.
				// Then later normal rollback does its job and might rollback < N in this case. Which means we just destroyed tombstones that under normal game circumstances
				// We are guaranteed to never need again, but with this validation on we might need them, but they're gone. So the game will crash.
				if (GetWorld()->GetSubsystem<UGameLoopSubsystem>()->GetSteppingFrameId() == InFrameId)
				{
					continue;
				}
#endif

				// If the frame we are rolling back to does not contain this body
				// Then only two possibilities exist:
				// 1) We are rolling back BEFORE the body was created. Means it will get created AND tombstoned again during rollback. So we need to destroy it.
				// 2) We are rolling back AFTER the body was tombstoned. In this case we HAVE TO leave it. Same frame can get stepped through many times.
				// As many as our rollback supports + 1, which is the initial step before any rollbacks.
				// So if we change any state here that we cannot restore - we just made the simulation non-deterministic.
				// Say we destroy this tombstone on 2nd run of this frame. On 3rd run it will no longer be there and its ID will be freed up.
				if (InFrameId < TombstonedConstraintIds[ConstraintId])
				{
					DestroyTombstonedConstraint(ConstraintId);
					TombstonedConstraintIds.Remove(ConstraintId);
				}
			}
		}
	}
}

void USpawnerSubsystem::RecreateConstraints_Internal(const int& InFrameId)
{
	/** Recreate Constraints that existed in the frame we are rolling back to */
	for (const int& ConstraintId : SavedFrames[InFrameId].SpawnedConstraintIds)
	{
		if (!ConstraintIdsInSimulation.Contains(ConstraintId))
		{
			auto* JoltConstraintActor = CurrentlySpawnedConstraintActors.Get(ConstraintId);
			check(JoltConstraintActor);

			check(TombstonedConstraintIds.Contains(ConstraintId));
			check(JoltConstraintActor->IsInitialized());
			JoltConstraintActor->RecreateConstraint();
			
			// We must recreate it with the same exact id
			ConstraintIdsInSimulation.Add(ConstraintId);
			LOG(TEXT("[SpawnerSubsystem] Re-created tombstoned constraint %d"), ConstraintId);

			TombstonedConstraintIds.Remove(ConstraintId);
		}
	}
}

void USpawnerSubsystem::TombstoneBody(const int& BodyIndex)
{
	CurrentlySpawnedBodyActors.Get(BodyIndex)->RemoveBody();
	
	check(!TombstonedBodyIds.Contains(BodyIndex));

	const int SteppingFrameId = GetWorld()->GetSubsystem<UGameLoopSubsystem>()->GetSteppingFrameId();
	TombstonedBodyIds.Add(BodyIndex, SteppingFrameId);
}

void USpawnerSubsystem::TombstoneConstraint(const int& ConstraintId)
{
	CurrentlySpawnedConstraintActors.Get(ConstraintId)->DestroyConstraint();
	
	check(!TombstonedConstraintIds.Contains(ConstraintId));
	TombstonedConstraintIds.Add(ConstraintId, GetWorld()->GetSubsystem<UGameLoopSubsystem>()->GetSteppingFrameId());
}

void USpawnerSubsystem::DestroyTombstonedBody(const int& BodyIndex)
{
	auto BodyActor = CurrentlySpawnedBodyActors.Get(BodyIndex);
	BodyActor->DestroyBody();
	BodyActor->DestroyActor();
	CurrentlySpawnedBodyActors.Remove(BodyIndex);

	LOG(TEXT("Destroyed tombstoned body %d"), BodyIndex);
}

void USpawnerSubsystem::DestroyTombstonedConstraint(const int& ConstraintId)
{
	// The constraint is already destroyed, because unlike with Bodies, we don't keep them around. We destroy -> re-create.
	// So the only thing left to destroy is the actor and components that are essentially a glorified data structure
	// That knows all the info needed to spawn the constraint again
	CurrentlySpawnedConstraintActors.Get(ConstraintId)->DestroyActor();
	CurrentlySpawnedConstraintActors.Remove(ConstraintId);

	LOG(TEXT("Destroyed tombstoned constraint %d"), ConstraintId);
}

