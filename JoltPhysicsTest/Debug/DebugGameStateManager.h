// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "JoltPhysicsTest/Debug/CustomLogger.h"
#include "CoreMinimal.h"
#include "JoltPhysicsTest/Data/LootItems.h"
#include "JoltPhysicsTest/GameInputs/GameInputs.h"
#include "UObject/Object.h"
#include "DebugGameStateManager.generated.h"

class UGameStateManager;

USTRUCT()
struct FDebugPhysicsBody
{
	GENERATED_BODY()

	FDebugPhysicsBody()
	{
	}

	uint32 BodyId = 0;
	Vec3 Position;
	Quat Rotation;
	Vec3 LinearVelocity;
	Vec3 AngularVelocity;

	bool operator==(const FDebugPhysicsBody& rhs) const
	{
		return
			BodyId == rhs.BodyId
			&& Position == rhs.Position
			&& Rotation == rhs.Rotation
			&& LinearVelocity == rhs.LinearVelocity
			&& AngularVelocity == rhs.AngularVelocity;
	}

	bool operator!=(const FDebugPhysicsBody& rhs) const
	{
		return
			BodyId != rhs.BodyId
			|| Position != rhs.Position
			|| Rotation != rhs.Rotation
			|| LinearVelocity != rhs.LinearVelocity
			|| AngularVelocity != rhs.AngularVelocity;
	}

	FString ToString() const
	{
		return FString::Printf(
			TEXT(
				//"BodyId: %d\nPosition: %s\nRotation: %s\nLinearVelocity: %s\nAngularVelocity: %s\nPosition(hex): %s\nRotation(hex): %s\nLinearVelocity(hex): %s\nAngularVelocity(hex): %s"),
				"BodyId: %d\nPosition(hex): %s\nRotation(hex): %s\nLinearVelocity(hex): %s\nAngularVelocity(hex): %s"),
			BodyId,
			/**VecToString(Position),
			*QuatToString(Rotation),
			*VecToString(LinearVelocity),
			*VecToString(AngularVelocity),*/
			*VecToHexString(Position),
			*QuatToHexString(Rotation),
			*VecToHexString(LinearVelocity),
			*VecToHexString(AngularVelocity)
		);
	}
};

USTRUCT()
struct FDebugCharacter
{
	GENERATED_BODY()

	FDebugCharacter()
	{
	}

	uint32 PlayerId = 0;
	int ShootingCooldownFramesLeft = 0;
	int BulletsLeft;
	int HandContactBodyIndex;
	int GrabbedBodyIndex;
	bool bDead;
	int DeadFramesNum;
	int InvulnerabilityFramesNum;
	int CrownHeldFramesNum;
	int CarryingFlagBodyIndex;
	ELootItem ActiveItem;
	int ItemActiveForFramesNum;
	FVector2D CursorPosition;
	TArray<int> CreatedBodyIndexes;
	TArray<int> CreatedConstraintsIds;

	bool operator==(const FDebugCharacter& rhs) const
	{
		return
			PlayerId == rhs.PlayerId
			&& ShootingCooldownFramesLeft == rhs.ShootingCooldownFramesLeft
			&& BulletsLeft == rhs.BulletsLeft
			&& HandContactBodyIndex == rhs.HandContactBodyIndex
			&& GrabbedBodyIndex == rhs.GrabbedBodyIndex
			&& bDead == rhs.bDead
			&& DeadFramesNum == rhs.DeadFramesNum
			&& InvulnerabilityFramesNum == rhs.InvulnerabilityFramesNum
			&& CrownHeldFramesNum == rhs.CrownHeldFramesNum
			&& CarryingFlagBodyIndex == rhs.CarryingFlagBodyIndex
			&& ActiveItem == rhs.ActiveItem
			&& ItemActiveForFramesNum == rhs.ItemActiveForFramesNum
			&& CursorPosition == rhs.CursorPosition
			&& CreatedBodyIndexes == rhs.CreatedBodyIndexes
			&& CreatedConstraintsIds == rhs.CreatedConstraintsIds;
	}

	bool operator!=(const FDebugCharacter& rhs) const
	{
		return
			PlayerId != rhs.PlayerId
			|| ShootingCooldownFramesLeft != rhs.ShootingCooldownFramesLeft
			|| BulletsLeft != rhs.BulletsLeft
			|| HandContactBodyIndex != rhs.HandContactBodyIndex
			|| GrabbedBodyIndex != rhs.GrabbedBodyIndex
			|| bDead != rhs.bDead
			|| DeadFramesNum != rhs.DeadFramesNum
			|| InvulnerabilityFramesNum != rhs.InvulnerabilityFramesNum
			|| CrownHeldFramesNum != rhs.CrownHeldFramesNum
			|| CarryingFlagBodyIndex != rhs.CarryingFlagBodyIndex
			|| ActiveItem != rhs.ActiveItem
			|| ItemActiveForFramesNum != rhs.ItemActiveForFramesNum
			|| CursorPosition != rhs.CursorPosition
			|| CreatedBodyIndexes != rhs.CreatedBodyIndexes
			|| CreatedConstraintsIds != rhs.CreatedConstraintsIds;
	}

	FString ToString() const
	{
		return FString(
			FString::Printf(TEXT("PlayerId: %d\n"), PlayerId) +
			FString::Printf(TEXT("ShootingCooldown(hex): %s\n"), *IntToHexString(ShootingCooldownFramesLeft)) +
			FString::Printf(TEXT("BulletsLeft(hex): %s\n"), *IntToHexString(BulletsLeft)) +
			FString::Printf(TEXT("HandContactBodyIndex(hex): %s\n"), *IntToHexString(HandContactBodyIndex)) +
			FString::Printf(TEXT("GrabbedBodyIndex(hex): %s\n"), *IntToHexString(GrabbedBodyIndex)) +
			FString::Printf(TEXT("bDead(hex): %s\n"), *IntToHexString(bDead)) +
			FString::Printf(TEXT("DeadFramesNum(hex): %s\n"), *IntToHexString(DeadFramesNum)) +
			FString::Printf(TEXT("InvulnerabilityFramesNum(hex): %s\n"), *IntToHexString(InvulnerabilityFramesNum)) +
			FString::Printf(TEXT("CrownHeldFramesNum(hex): %s\n"), *IntToHexString(CrownHeldFramesNum)) +
			FString::Printf(TEXT("CarryingFlagBodyIndex(hex): %s\n"), *IntToHexString(CarryingFlagBodyIndex)) +
			FString::Printf(TEXT("ActiveItem(hex): %s\n"), *IntToHexString(static_cast<int>(ActiveItem))) +
			FString::Printf(TEXT("CursorPos(hex): %s\n"), *Vector2DToHexString(CursorPosition)) +
			FString::Printf(TEXT("BulletsLeft(hex): %s\n"), *IntToHexString(ItemActiveForFramesNum)) +
			FString::Printf(TEXT("BodiesCreated(hex): %s\n"), *IntArrayToHexString(CreatedBodyIndexes)) +
			FString::Printf(TEXT("ConstraintsCreated(hex): %s"), *IntArrayToHexString(CreatedConstraintsIds))
		);
	}
};

USTRUCT()
struct FDebugGameState
{
	GENERATED_BODY()

	FDebugGameState()
	{
	}

	UPROPERTY()
	TArray<FDebugPhysicsBody> PhysicsBodies;

	UPROPERTY()
	TArray<FDebugCharacter> Characters;

	int CurrentlySpawnedActorsNum = 0;;
	int SavedSpawnFramesNum = 0;

	TArray<int> BodyIndexesInSimulation;
	TArray<int> ConstraintIdsInSimulation;

	// <BodyIndex, SteppingFrameId>
	TMap<int, int> TombstonedBodyIds;
	TMap<int, int> TombstonedConstraintIds;

	int BodyNextId = 0;
	TArray<int> BodyFreeIds;
	TArray<int> BodyAssignedIdsInStackOrder;

	int ConstraintNextId = 0;
	TArray<int> ConstraintFreeIds;
	TArray<int> ConstraintAssignedIdsInStackOrder;

	bool operator==(const FDebugGameState& rhs) const
	{
		return
			PhysicsBodies == rhs.PhysicsBodies
			&& Characters == rhs.Characters
			&& CurrentlySpawnedActorsNum == rhs.CurrentlySpawnedActorsNum
			&& SavedSpawnFramesNum == rhs.SavedSpawnFramesNum;
	}

	bool operator!=(const FDebugGameState& rhs) const
	{
		return
			PhysicsBodies != rhs.PhysicsBodies
			|| Characters != rhs.Characters
			|| CurrentlySpawnedActorsNum != rhs.CurrentlySpawnedActorsNum
			|| SavedSpawnFramesNum != rhs.SavedSpawnFramesNum;
	}

	FString ToString() const
	{
		FString GameStateString;
		for (const auto& CharacterBody : Characters)
		{
			GameStateString += "\n" + CharacterBody.ToString();
		}

		for (const auto& PhysicsBody : PhysicsBodies)
		{
			GameStateString += "\n" + PhysicsBody.ToString();
		}

		//GameStateString += "\n" + FString::Printf(TEXT("CurrentlySpawnedActorsNum: %d"), CurrentlySpawnedActorsNum);
		//GameStateString += "\n" + FString::Printf(TEXT("SpawnerSubsystemSavedFramesNum: %d"), SavedSpawnFramesNum);

		GameStateString += "\n" + FString::Printf(
			TEXT("BodyIndexesInSimulation: %s"), *IntArrayToString(BodyIndexesInSimulation));
		GameStateString += "\n" + FString::Printf(
			TEXT("ConstraintIdsInSimulation: %s"), *IntArrayToString(ConstraintIdsInSimulation));

		/*GameStateString += "\n" + FString::Printf(TEXT("TombstonedBodyIds: %s"), *IntsMapToString(TombstonedBodyIds));
		GameStateString += "\n" + FString::Printf(
			TEXT("TombstonedConstraintIds: %s"), *IntsMapToString(TombstonedConstraintIds));*/

		// BodyFreeIds can actually differ between clients, that's fine. The main point is that the next body id would always match
		// So that's what we will provide here
		int BodyNextIdAvailable = BodyNextId;
		if (!BodyFreeIds.IsEmpty())
		{
			BodyNextIdAvailable = BodyFreeIds[0];
		}
		GameStateString += "\n" + FString::Printf(TEXT("BodyNextAvailableId: %d"), BodyNextIdAvailable);
		//GameStateString += "\n" + FString::Printf(TEXT("BodyNextId: %d"), BodyNextId);
		//GameStateString += "\n" + FString::Printf(TEXT("BodyFreeIds: %s"), *IntArrayToString(BodyFreeIds));

		/*GameStateString += "\n" + FString::Printf(
			TEXT("BodyAssignedIdsInStackOrder: %s"), *IntArrayToString(BodyAssignedIdsInStackOrder));*/

		GameStateString += "\n" + FString::Printf(TEXT("ConstraintNextId: %d"), ConstraintNextId);
		GameStateString += "\n" + FString::Printf(TEXT("ConstraintFreeIds: %s"), *IntArrayToString(ConstraintFreeIds));
		/*GameStateString += "\n" + FString::Printf(
			TEXT("ConstraintAssignedIdsInStackOrder: %s"), *IntArrayToString(ConstraintAssignedIdsInStackOrder));*/

		return GameStateString;
	}
};

UCLASS()
class JOLTPHYSICSTEST_API UDebugGameStateManager : public UObject
{
	GENERATED_BODY()

public:
	void Init(UGameStateManager* InGameStateManager);
	bool IsCurrentStateEqualToFrame(const int& InFrameId);

	void DumpToLogs(bool bIsServer);
private:
	void Save(const int& InFrameId);
	
	FDebugGameState GetPackagedGameState() const;
	
	TMap<int, FDebugGameState> SavedStates;

	// This doesn't really belong with game state. BUT, it's very useful for debugging
	TArray<FCommandFrame> ConfirmedInputs;

	UFUNCTION()
	void OnConfirmedAllInputsForFrame(const FCommandFrame& InCommandFrame);

	UFUNCTION()
	void OnSavedGameState(int FrameId);
};
