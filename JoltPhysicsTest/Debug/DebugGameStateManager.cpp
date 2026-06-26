// Fill out your copyright notice in the Description page of Project Settings.


#include "DebugGameStateManager.h"

#include "JoltPhysicsTest/GameInputs/InputsSubsystem.h"

#include "JoltPhysicsTest/Debug/CustomLogger.h"
#include "JoltPhysicsTest/FunctionLibrary/Helper.h"
#include "JoltPhysicsTest/Game/GameStateManager.h"
#include "JoltPhysicsTest/Game/SpawnerSubsystem.h"
#include "JoltPhysicsTest/Jolt/JoltSubsystem.h"
#include "JoltPhysicsTest/Player/MyPlayerController.h"
#include "JoltPhysicsTest/Replay/MyReplaySubsystem.h"

void UDebugGameStateManager::Init(UGameStateManager* InGameStateManager)
{
	GetWorld()->GetSubsystem<UInputsSubsystem>()->GameInputs->OnConfirmedAllInputsForFrame.AddUniqueDynamic(this, &UDebugGameStateManager::OnConfirmedAllInputsForFrame);
	InGameStateManager->OnSavedGameState.AddUniqueDynamic(this, &UDebugGameStateManager::OnSavedGameState);
}

void UDebugGameStateManager::Save(const int& InFrameId)
{
	//LOG(TEXT("[DebugGameStateManager] Save frame %d"), InFrameId);

	auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();
	check(JoltSubsystem);

	if (SavedStates.Contains(InFrameId))
	{
		SavedStates[InFrameId] = GetPackagedGameState();
		//LOG(TEXT("[DebugGameStateManager] Overwrite frame %d"), InFrameId);
	}
	else
	{
		SavedStates.Add(InFrameId, GetPackagedGameState());
	}
}

bool UDebugGameStateManager::IsCurrentStateEqualToFrame(const int& InFrameId)
{
	return SavedStates[InFrameId] == GetPackagedGameState();
}

void UDebugGameStateManager::DumpToLogs(bool bIsServer)
{
	// A simple hack to avoid dumping logs from lobby level after exiting the game. Cause that overwrites the logs of the game that we actually want
	auto MyReplaySubsystem = GetWorld()->GetSubsystem<UMyReplaySubsystem>();
	bool bProcessReplayMode = MyReplaySubsystem->IsProcessReplayMode();
	
	if (!MyReplaySubsystem->IsRecording() && !bProcessReplayMode)
	{
		return;
	}

	if (bProcessReplayMode)
	{
		bIsServer = MyReplaySubsystem->IsServer();
	}

	if (bProcessReplayMode)
	{
		if (bIsServer)
		{
			DELETE_LOG_FILE_IF_EXISTS("ServerReplay.txt");
		}
		else
		{
			DELETE_LOG_FILE_IF_EXISTS("ClientReplay.txt");
		}
	}
	else
	{
		if (bIsServer)
		{
			DELETE_LOG_FILE_IF_EXISTS("Server.txt");
		}
		else
		{
			DELETE_LOG_FILE_IF_EXISTS("Client.txt");
		}
	}
	
	for (auto& pair : SavedStates)
	{
		const int& DebugFrameId = pair.Key;
		const auto& DebugGameState = pair.Value;

		FString ConfirmedInputsString = TEXT("\n(no confirmed inputs)");
		for (auto ConfirmedFrame : ConfirmedInputs)
		{
			if (ConfirmedFrame.FrameId == DebugFrameId)
			{
				ConfirmedFrame.SortCharactersInputs();
				ConfirmedInputsString = FString::Printf(TEXT("\n(\nINPUTS %s)\n"), *ConfirmedFrame.ToString());
			}
		}

		if (bProcessReplayMode)
		{
			if (bIsServer)
			{
				LOG_TO_FILE("ServerReplay.txt", TEXT("Frame %d%s%s"), DebugFrameId, *ConfirmedInputsString, *DebugGameState.ToString());
			}
			else
			{
				LOG_TO_FILE("ClientReplay.txt", TEXT("Frame %d%s%s"), DebugFrameId, *ConfirmedInputsString, *DebugGameState.ToString());
			}
		}
		else
		{
			if (bIsServer)
			{
				LOG_TO_FILE("Server.txt", TEXT("Frame %d%s%s"), DebugFrameId, *ConfirmedInputsString, *DebugGameState.ToString());
			}
			else
			{
				LOG_TO_FILE("Client.txt", TEXT("Frame %d%s%s"), DebugFrameId, *ConfirmedInputsString, *DebugGameState.ToString());
			}
		}
	}
}

FDebugGameState UDebugGameStateManager::GetPackagedGameState() const
{
#ifdef MY_DEBUG_GAME_STATE
	auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();
	auto SpawnerSubsystem = GetWorld()->GetSubsystem<USpawnerSubsystem>();
	FDebugGameState DebugGameState;
	
	BodyIDVector AllBodies;
	JoltSubsystem->GetPhysicsSystem()->GetBodies(AllBodies);
	for (auto Body : AllBodies)
	{
		// If body is not part of the simulation, then we don't really care about it
		// This can happen due to tombstoned bodies that removed from the simulation but wait to go outside
		// of rollback window, in case we would need to rollback and restore them
		if (!SpawnerSubsystem->DoesBodyExistInSimulation(Body.GetIndex()))
		{
			continue;
		}
		
		FDebugPhysicsBody PhysicsBody;
		PhysicsBody.BodyId = Body.GetIndex();
		PhysicsBody.Position = JoltSubsystem->GetBodyInterface()->GetPosition(Body);
		PhysicsBody.Rotation = JoltSubsystem->GetBodyInterface()->GetRotation(Body);
		PhysicsBody.LinearVelocity = JoltSubsystem->GetBodyInterface()->GetLinearVelocity(Body);
		PhysicsBody.AngularVelocity = JoltSubsystem->GetBodyInterface()->GetAngularVelocity(Body);

		DebugGameState.PhysicsBodies.Add(PhysicsBody);
	}

	for (const auto& PlayerCharacter : UHelper::GetLocalPlayerController(GetWorld())->PlayerCharacters)
	{
		FDebugCharacter DebugCharacter;
		DebugCharacter.PlayerId = PlayerCharacter.PlayerId;
		DebugCharacter.ShootingCooldownFramesLeft = PlayerCharacter.ShootingCooldownFramesLeft;
		DebugCharacter.BulletsLeft = PlayerCharacter.BulletsLeft;
		DebugCharacter.HandContactBodyIndex = PlayerCharacter.HandContactBodyIndex;
		DebugCharacter.GrabbedBodyIndex = PlayerCharacter.GrabbedBodyIndex;
		DebugCharacter.bDead = PlayerCharacter.bDead;
		DebugCharacter.DeadFramesNum = PlayerCharacter.DeadFramesNum;
		DebugCharacter.InvulnerabilityFramesNum = PlayerCharacter.InvulnerabilityFramesNum;
		DebugCharacter.CrownHeldFramesNum = PlayerCharacter.CrownHeldFramesNum;
		DebugCharacter.CarryingFlagBodyIndex = PlayerCharacter.CarryingFlagBodyIndex;
		DebugCharacter.ActiveItem = PlayerCharacter.ActiveItem;
		DebugCharacter.ItemActiveForFramesNum = PlayerCharacter.ItemActiveForFramesNum;
		DebugCharacter.CursorPosition = PlayerCharacter.CursorPosition;
		DebugCharacter.CreatedBodyIndexes = PlayerCharacter.CreatedBodyIndexes;
		DebugCharacter.CreatedConstraintsIds = PlayerCharacter.CreatedConstraintsIds;
		DebugGameState.Characters.Add(DebugCharacter);
	}

	DebugGameState.CurrentlySpawnedActorsNum = SpawnerSubsystem->GetCurrentlySpawnedShapesNum();
	DebugGameState.SavedSpawnFramesNum = SpawnerSubsystem->GetSavedFramesNum();
	
	DebugGameState.BodyIndexesInSimulation = SpawnerSubsystem->GetBodyIndexesInSimulation();
	// Sorting is for the log diffing. Otherwise same ids can appear in a different order and is a false positive diff. Makes it harder to debug
	DebugGameState.BodyIndexesInSimulation.Sort();
	DebugGameState.ConstraintIdsInSimulation = SpawnerSubsystem->GetConstraintIdsInSimulation();
	DebugGameState.ConstraintIdsInSimulation.Sort();

	DebugGameState.TombstonedBodyIds = SpawnerSubsystem->GetTombstonedBodyIds();
	DebugGameState.TombstonedConstraintIds = SpawnerSubsystem->GetTombstonedConstraintIds();

	DebugGameState.BodyNextId = SpawnerSubsystem->GetBodyNextId();
	DebugGameState.BodyFreeIds = SpawnerSubsystem->GetBodyFreeIds();
	DebugGameState.BodyAssignedIdsInStackOrder = SpawnerSubsystem->GetBodyAssignedIdsInStackOrder();

	DebugGameState.ConstraintNextId = SpawnerSubsystem->GetConstraintNextId();
	DebugGameState.ConstraintFreeIds = SpawnerSubsystem->GetConstraintFreeIds();
	DebugGameState.ConstraintAssignedIdsInStackOrder = SpawnerSubsystem->GetConstraintAssignedIdsInStackOrder();

	return DebugGameState;

#endif

	return {};
}

void UDebugGameStateManager::OnConfirmedAllInputsForFrame(const FCommandFrame& InCommandFrame)
{
	ConfirmedInputs.Add(InCommandFrame);
}

void UDebugGameStateManager::OnSavedGameState(int FrameId)
{
	Save(FrameId);
}
