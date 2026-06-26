// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "JoltPhysicsTest/Debug/CustomLogger.h"
#include "CoreMinimal.h"
#include "JoltPhysicsTest/Jolt/JoltConstraintActor.h"
#include "JoltPhysicsTest/Jolt/JoltBodyActor.h"
#include "Subsystems/WorldSubsystem.h"
#include "SpawnerSubsystem.generated.h"

USTRUCT()
struct FEntityState
{
	GENERATED_BODY()
public:
	UPROPERTY()
	TArray<int> ItemsKeys;

	UPROPERTY()
	TArray<UClass*> ItemsClasses;

	UPROPERTY()
	int NextId = 0;

	UPROPERTY()
	TArray<int> FreeIds;

	UPROPERTY()
	TArray<int> AssignedIdsOrder;
};

template<typename T>
struct TEntity
{
	int Add(T* Item)
	{
		const int AssignedId = GetNextId();
		AssignedIdsOrder.Add(AssignedId);
		
		check(!Items.Contains(AssignedId));
		Items.Add(AssignedId, Item);

		return AssignedId;
	}

	T* Get(const int& Id)
	{
		check(Items.Contains(Id));
		
		return Items[Id];
	}

	/** Stack order meaning that the first element in the array will be the last ID that was assigned and then the second last, etc. */
	TArray<int> GetIdsInStackOrder() const
	{
		TArray<int> ReversedAssignedIdsOrder;
		ReversedAssignedIdsOrder.Reserve(AssignedIdsOrder.Num());
		for (int i = AssignedIdsOrder.Num() - 1; i >= 0; i--)
		{
			ReversedAssignedIdsOrder.Add(AssignedIdsOrder[i]);
		}
		
		return ReversedAssignedIdsOrder;
	}

	TArray<int> GetIdsInAssignedOrder() const
	{
		return AssignedIdsOrder;
	}

	bool ContainsId(const int& Id) const
	{
		return Items.Contains(Id);
	}

	/** This can only be called for despawns due to player input, since player can despawn in any order he likes */
	void Remove(const int& Id)
	{
		check(Items.Contains(Id));
		AssignedIdsOrder.Remove(Id);
		Items.Remove(Id);
		FreeIds.Push(Id);
		LOG(TEXT("[Remove item] Added id to FreeIds, %d"), Id);
	}

	/** This one should always be used in rollbacks. We ALWAYS have to rollback in a reverse order of creation */
	void RemoveLast(const int& Id)
	{
		const int LastAssignedId = AssignedIdsOrder.Pop();
		checkf(Id == LastAssignedId, TEXT("Attempting to remove entity out of order"));
		
		check(Items.Contains(Id));
		Items.Remove(Id);
		FreeIds.Push(Id);
		LOG(TEXT("[Remove item] Added id to FreeIds, %d"), Id);
	}

	/**
	 * IMPORTANT! This is only to be used when cleaning up an entity that is fully outside the rollback window and will never be used again
	 * We can't actually re-use these IDs, because during the game there really is no safe time to add IDs back to the pool.
	 * Because this action is not compatible with rollback and I have no intention of making it so.
	 * So if we mispredict something on FR 10 and then cleanup FR 0. When we will rollback to FR 10 to fix our mistake, we now have more IDs in the pool,
	 * that weren't there before. And for clients that DIDN'T mispredict - it won't be the same.
	 **/
	void RemovePermanently(const int& Id)
	{
		check(Items.Contains(Id));
		Items.Remove(Id);
		AssignedIdsOrder.Remove(Id);
		LOG(TEXT("[Remove item] Removed permanently %d"), Id);
	}

	int Num() const
	{
		return Items.Num();
	}

	TArray<int> GetFreeIds() const
	{
		return FreeIds;
	}

	int PeekAtNextId() const
	{
		return NextId;
	}

	FEntityState GetEntityState() const
	{
		FEntityState EntityState;
		EntityState.FreeIds = FreeIds;
		EntityState.AssignedIdsOrder = AssignedIdsOrder;
		EntityState.NextId = NextId;

		// We don't include base classes in state, because these are always loaded together with the level
		// We're only interested in entities spawned at runtime
		for (const auto& Pair : Items)
		{
			if (Pair.Value->GetClass() == AJoltBodyActor::StaticClass() || Pair.Value->GetClass() == AJoltConstraintActor::StaticClass())
			{
				continue;
			}
			
			EntityState.ItemsKeys.Add(Pair.Key);
			EntityState.ItemsClasses.Add(Pair.Value->GetClass());
		}

		return EntityState;
	}

	void RebuildEntityState(const FEntityState& InEntityState, UWorld* InWorld)
	{
		FreeIds = InEntityState.FreeIds;
		AssignedIdsOrder = InEntityState.AssignedIdsOrder;
		NextId = InEntityState.NextId;

		for (int i = 0; i < InEntityState.ItemsKeys.Num(); i++)
		{
			if (!Items.Contains(InEntityState.ItemsKeys[i]))
			{
				const FTransform SpawnTransform = FTransform::Identity;
				AActor* SpawnedActor = InWorld->SpawnActor(InEntityState.ItemsClasses[i], &SpawnTransform);
				Items.Add(
					InEntityState.ItemsKeys[i],
					Cast<T>(SpawnedActor)
				);
			}
		}
	}

private:
	int GetNextId()
	{
		if (!FreeIds.IsEmpty())
		{
			int FreeId = FreeIds.Pop();
			
			LOG(TEXT("[GetNextId] Taking id from FreeIds, got %d"), FreeId);

			return FreeId;
		}

		LOG(TEXT("[GetNextId] FreeIds was empty, taking NextId %d"), NextId);

		NextId++;

		return NextId - 1;
	}

	UPROPERTY()
	TMap<int, T*> Items;
	
	int NextId = 0;
	TArray<int> FreeIds;

	TArray<int> AssignedIdsOrder;
};

USTRUCT()
struct FJoltActors
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<int> SpawnedBodyIndexes;
	
	UPROPERTY()
	TArray<int> SpawnedConstraintIds;
};

USTRUCT()
struct FSpawnerSubsystemState
{
	GENERATED_BODY()
public:
	UPROPERTY()
	TArray<int> SavedFramesKeys;
	UPROPERTY()
	TArray<FJoltActors> SavedFramesValues;

	UPROPERTY()
	FEntityState CurrentlySpawnedBodyActors;

	UPROPERTY()
	TArray<int> BodyIndexesInSimulation;

	UPROPERTY()
	TArray<int> BodySpawnedOnFrameKeys;
	UPROPERTY()
	TArray<int> BodySpawnedOnFrameValues;

	UPROPERTY()
	TArray<int> TombstonedBodyIdsKeys;
	UPROPERTY()
	TArray<int> TombstonedBodyIdsValues;
	
	UPROPERTY()
	FEntityState CurrentlySpawnedConstraintActors;

	UPROPERTY()
	TArray<int> ConstraintIdsInSimulation;

	UPROPERTY()
	TArray<int> TombstonedConstraintIdsKeys;
	UPROPERTY()
	TArray<int> TombstonedConstraintIdsValues;
};

UCLASS()
class JOLTPHYSICSTEST_API USpawnerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** Allow each spawned body or constraint to run their own physics in a deterministic order */
	void StepSimulation();
	
	TArray<FVector2D> GetAttachedMeshesVertices();

	int SpawnJoltBody(TSubclassOf<AJoltBodyActor> BodyClass, const FVector& Location, const FRotator& Rotation, int BelongsToPlayerId = -1);
	int SpawnJoltBody(TSubclassOf<AJoltBodyActor> BodyClass, const FTransform& InTransform, int BelongsToPlayerId = -1);
	int SpawnJoltBody(TSubclassOf<AJoltBodyActor> BodyClass, const FVector& Location, const FRotator& Rotation, const FVector& Scale, int BelongsToPlayerId = -1);
	void DespawnJoltBody(const BodyID& InBodyID);
	void DespawnJoltBody(const int& InBodyIndex);
	void DespawnJoltBodyWithRelatedConstraints(const BodyID& InBodyID);
	void DespawnJoltBodyWithRelatedConstraints(const int& InBodyIndex);

	bool DoesBodyExistInSimulation(const BodyID& InBodyID) const;
	bool DoesBodyExistInSimulation(const int& InBodyIndex);
	
	bool DoesConstraintExistInSimulation(const int& InConstraintId) const;

	/** Unlike SpawnJoltShape(), this method allows to add an already spawned actor and assigns it a unique id */
	int AddJoltBody(AJoltBodyActor* JoltBodyActor);

	// Returns the index of the newly spawned constraint. It can be used to reference it through this subsystem.
	int SpawnJoltConstraint(TSubclassOf<AJoltConstraintActor> ConstraintClass, const FVector& Location, FJoltConstraintData JoltConstraintData);
	void DespawnJoltConstraint(const int& ConstraintId);

	/** Unlike SpawnJoltConstraint(), this method allows to add an already spawned actor and assigns it a unique id */
	int AddJoltConstraint(AJoltConstraintActor* JoltConstraintActor);

	// These are about destroying/re-creating physics bodies/constraints
	void SaveObjects(const int& InFrameId);
	void LoadObjects(const int& InFrameId);
	void Remove(const int& InFrameId);

	// These are about restoring state. All the bodies and constraints already have to exist.
	void SaveToStream(StateRecorderImpl& InStream);
	void LoadFromStream(StateRecorderImpl& InStream);

	AJoltBodyActor* GetBodyActor(const BodyID& InBodyID);
	AJoltConstraintActor* GetConstraintActor(const int& ConstraintId);

	template <typename T = AJoltBodyActor>
	T* GetBodyActor(const uint32& InBodyIndex)
	{
		check(CurrentlySpawnedBodyActors.ContainsId(InBodyIndex));

		return Cast<T>(CurrentlySpawnedBodyActors.Get(InBodyIndex));
	}

	int GetCurrentlySpawnedShapesNum() const;

	int GetSavedFramesNum() const;

	FSpawnerSubsystemState GetSpawnerState() const;
	void RebuildSpawnerState(const FSpawnerSubsystemState& SpawnerSubsystemState);

#ifdef MY_DEBUG_GAME_STATE
	FORCEINLINE TArray<int> GetBodyIndexesInSimulation() const { return BodyIndexesInSimulation; }
	FORCEINLINE TArray<int> GetConstraintIdsInSimulation() const { return ConstraintIdsInSimulation; }
	
	FORCEINLINE TMap<int, int> GetTombstonedBodyIds() const { return TombstonedBodyIds; }
	FORCEINLINE TMap<int, int> GetTombstonedConstraintIds() const { return TombstonedConstraintIds; }

	FORCEINLINE int GetBodyNextId() const { return CurrentlySpawnedBodyActors.PeekAtNextId(); }
	FORCEINLINE TArray<int> GetBodyFreeIds() const { return CurrentlySpawnedBodyActors.GetFreeIds(); }
	FORCEINLINE TArray<int> GetBodyAssignedIdsInStackOrder() const { return CurrentlySpawnedBodyActors.GetIdsInStackOrder(); }

	FORCEINLINE int GetConstraintNextId() const { return CurrentlySpawnedConstraintActors.PeekAtNextId(); }
	FORCEINLINE TArray<int> GetConstraintFreeIds() const { return CurrentlySpawnedConstraintActors.GetFreeIds(); }
	FORCEINLINE TArray<int> GetConstraintAssignedIdsInStackOrder() const { return CurrentlySpawnedConstraintActors.GetIdsInStackOrder(); }
#endif
private:
	UPROPERTY()
	TMap<int, FJoltActors> SavedFrames;

	TEntity<AJoltBodyActor> CurrentlySpawnedBodyActors;

	/** Bodies that are currently added to Jolt physics simulation */
	TArray<int> BodyIndexesInSimulation;

	/** <BodyIndex, FrameId> */
	TMap<int, int> BodySpawnedOnFrame;

	/** <BodyId, FrameId> */
	TMap<int, int> TombstonedBodyIds;

	
	/** AJoltConstraintActor is basically a data structure that knows how to create the constraints it owns */
	TEntity<AJoltConstraintActor> CurrentlySpawnedConstraintActors;

	/** Constraints that are currently added to Jolt physics simulation */
	TArray<int> ConstraintIdsInSimulation;

	/** <ConstraintId, FrameId> */
	TMap<int, int> TombstonedConstraintIds;

	void DestroyBodies_Internal(const int& InFrameId);
	void RecreateBodies_Internal(const int& InFrameId);
	
	void DestroyConstraints_Internal(const int& InFrameId);
	void RecreateConstraints_Internal(const int& InFrameId);
	
	void TombstoneBody(const int& BodyIndex);
	void TombstoneConstraint(const int& ConstraintId);
	
	/** Once tombstone goes out of rollback window OR we rollback before the tombstone was created. */
	void DestroyTombstonedBody(const int& BodyIndex);
	
	/** Once tombstone goes out of rollback window OR we rollback before the tombstone was created. */
	void DestroyTombstonedConstraint(const int& ConstraintId);
};
