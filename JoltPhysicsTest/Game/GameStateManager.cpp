// Fill out your copyright notice in the Description page of Project Settings.


#include "GameStateManager.h"

#include "MyGameState.h"
#include "SpawnerSubsystem.h"
#include "JoltPhysicsTest/Debug/CustomLogger.h"
#include "JoltPhysicsTest/FunctionLibrary/Helper.h"
#include "JoltPhysicsTest/Jolt/JoltSubsystem.h"
#include "JoltPhysicsTest/Player/MyPlayerController.h"

void UGameStateManager::Save(const int& InFrameId)
{
	LOG(TEXT("[GameStateManager] Save frame %d"), InFrameId);

	if (SavedStates.Contains(InFrameId))
	{
		LOG(TEXT("[GameStateManager] Overwrite frame %d"), InFrameId);
	}
	else
	{
		SavedStates.Add(InFrameId, MakeUnique<StateRecorderImpl>());
	}

	// We need to save/load SpawnerSubsystem data before we restore physics state, because it needs to destroy/re-create physics actors
	auto SpawnerSubsystem = GetWorld()->GetSubsystem<USpawnerSubsystem>();
	SpawnerSubsystem->SaveObjects(InFrameId);
	
	SaveToStream(*SavedStates[InFrameId].Get());
	SaveGameStateHash(InFrameId);

	// Check bytes size for saved frame
	//auto OneFrameMemorySize = SavedStates[InFrameId]->GetDataSize();

	OnSavedGameState.Broadcast(InFrameId);
}

void UGameStateManager::Load(const int& InFrameId)
{
	LOG(TEXT("[GameStateManager] Load frame %d"), InFrameId);
	checkf(SavedStates.Contains(InFrameId), TEXT("Saved states does not contain frame %d"), InFrameId);

	auto SpawnerSubsystem = GetWorld()->GetSubsystem<USpawnerSubsystem>();
	SpawnerSubsystem->LoadObjects(InFrameId);

	LoadFromStream(*SavedStates[InFrameId].Get());
}

void UGameStateManager::Remove(const int& InFrameId)
{
	if (InFrameId >= 0)
	{
		SavedStates.Remove(InFrameId);
		GetWorld()->GetSubsystem<USpawnerSubsystem>()->Remove(InFrameId);
		LOG(TEXT("[GameStateManager] Removed frame %d"), InFrameId);

		// We want to keep game state hashes around for a bit longer.
		// Otherwise they might always end up cleared before we have a chance to compare them.
		GameStateHashes.Remove(JPH::max(0, InFrameId - 10));
	}
}

void UGameStateManager::SaveToStream(StateRecorderImpl& InStream)
{
	LOG(TEXT("Save to stream"));
	InStream.Clear();
	
	auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();
	check(JoltSubsystem);

	auto SpawnerSubsystem = GetWorld()->GetSubsystem<USpawnerSubsystem>();
	SpawnerSubsystem->SaveToStream(InStream);

	// TODO: We should probably pass this filter to our own SpawnerSubsystem::SaveToStream() and use it. Just to keep things consistent.
	CustomStateRecorderFilter RecorderFilter{};
	// We exclude saving constraints, because we use Jolts mConstraintPriority to ensure deterministic ordering
	// But that means we have to handle calling save/restore on each constraint ourselves
	JoltSubsystem->SaveState(InStream, &RecorderFilter);

	UHelper::GetMyGameState(GetWorld())->SaveState(InStream);
	
	for (const auto& PlayerCharacter : UHelper::GetLocalPlayerController(GetWorld())->PlayerCharacters)
	{
		InStream.Write(PlayerCharacter.BulletReloadCooldownFramesLeft);
		InStream.Write(PlayerCharacter.ShootingCooldownFramesLeft);
		InStream.Write(PlayerCharacter.DashCooldownFramesLeft);
		InStream.Write(PlayerCharacter.BulletsLeft);
		InStream.Write(PlayerCharacter.ShootButtonHeldFramesNum);
		InStream.Write(PlayerCharacter.HandContactBodyIndex);
		InStream.Write(PlayerCharacter.GrabbedBodyIndex);
		InStream.Write(PlayerCharacter.GrappleRopeConstraintId);
		InStream.Write(PlayerCharacter.GrappleCurrentMaxDistance);
		InStream.Write(PlayerCharacter.bDead);
		InStream.Write(PlayerCharacter.DeadFramesNum);
		InStream.Write(PlayerCharacter.InvulnerabilityFramesNum);
		InStream.Write(PlayerCharacter.CrownHeldFramesNum);
		InStream.Write(PlayerCharacter.CarryingFlagBodyIndex);
		
		InStream.Write(static_cast<int>(PlayerCharacter.ActiveItem));
		InStream.Write(PlayerCharacter.ItemActiveForFramesNum);
		
		InStream.Write(PlayerCharacter.CursorPosition.X);
		InStream.Write(PlayerCharacter.CursorPosition.Y);
		
		InStream.Write(PlayerCharacter.CreatedBodyIndexes.Num());
		InStream.Write(PlayerCharacter.CreatedConstraintsIds.Num());
		InStream.Write(PlayerCharacter.PlayerItems.Num());
		LOG(TEXT("[Save frame %d][Player %d] Bodies created num %d"), SteppingFrameId, PlayerCharacter.PlayerId, PlayerCharacter.CreatedBodyIndexes.Num());
		LOG(TEXT("[Save frame %d][Player %d] Constraints created %d"), SteppingFrameId, PlayerCharacter.PlayerId, PlayerCharacter.CreatedConstraintsIds.Num());
		for (const int& BodyIndex : PlayerCharacter.CreatedBodyIndexes)
		{
			InStream.Write(BodyIndex);
			LOG(TEXT("[Save frame %d][Player %d] Save body %d"), SteppingFrameId, PlayerCharacter.PlayerId, BodyIndex);
		}
		for (const int& ConstraintId : PlayerCharacter.CreatedConstraintsIds)
		{
			InStream.Write(ConstraintId);
			LOG(TEXT("[Save frame %d][Player %d] Save constraint %d"), SteppingFrameId, PlayerCharacter.PlayerId, ConstraintId);
		}
		for (ELootItem Item : PlayerCharacter.PlayerItems)
		{
			InStream.Write(static_cast<int>(Item));
			LOG(TEXT("[Save frame %d][Player %d] Save player item %s"), SteppingFrameId, PlayerCharacter.PlayerId, *UEnum::GetValueAsString(Item));
		}
	}
}

void UGameStateManager::LoadFromStream(StateRecorderImpl& InStream)
{
	LOG(TEXT("Load from stream"));
	InStream.Rewind();
	
	auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();
	check(JoltSubsystem);
	
	auto SpawnerSubsystem = GetWorld()->GetSubsystem<USpawnerSubsystem>();
	SpawnerSubsystem->LoadFromStream(InStream);
	
	const bool bRestoreState = JoltSubsystem->RestoreState(InStream);
	checkf(bRestoreState, TEXT("Failed to restore physics state"));

	UHelper::GetMyGameState(GetWorld())->RestoreState(InStream);
	
	for (auto& PlayerCharacter : UHelper::GetLocalPlayerController(GetWorld())->PlayerCharacters)
	{
		InStream.Read(PlayerCharacter.BulletReloadCooldownFramesLeft);
		InStream.Read(PlayerCharacter.ShootingCooldownFramesLeft);
		InStream.Read(PlayerCharacter.DashCooldownFramesLeft);
		InStream.Read(PlayerCharacter.BulletsLeft);
		InStream.Read(PlayerCharacter.ShootButtonHeldFramesNum);
		InStream.Read(PlayerCharacter.HandContactBodyIndex);
		InStream.Read(PlayerCharacter.GrabbedBodyIndex);
		InStream.Read(PlayerCharacter.GrappleRopeConstraintId);
		InStream.Read(PlayerCharacter.GrappleCurrentMaxDistance);
		InStream.Read(PlayerCharacter.bDead);
		InStream.Read(PlayerCharacter.DeadFramesNum);
		InStream.Read(PlayerCharacter.InvulnerabilityFramesNum);
		InStream.Read(PlayerCharacter.CrownHeldFramesNum);
		InStream.Read(PlayerCharacter.CarryingFlagBodyIndex);

		int CurrentlyActiveItem = 0;
		InStream.Read(CurrentlyActiveItem);
		PlayerCharacter.ActiveItem = static_cast<ELootItem>(CurrentlyActiveItem);
		
		InStream.Read(PlayerCharacter.ItemActiveForFramesNum);
		
		InStream.Read(PlayerCharacter.CursorPosition.X);
		InStream.Read(PlayerCharacter.CursorPosition.Y);

		PlayerCharacter.CreatedBodyIndexes.Empty();
		int BodiesNum = 0;
		InStream.Read(BodiesNum);
		LOG_CONSTRAINT(TEXT("[Load frame %d][Player %d] Bodies created num %d"), SteppingFrameId, PlayerCharacter.CharacterVirtual->GetID().GetValue(), BodiesNum);


		PlayerCharacter.CreatedConstraintsIds.Empty();
		int ConstraintsNum = 0;
		InStream.Read(ConstraintsNum);
		LOG_CONSTRAINT(TEXT("[Load frame %d][Player %d] Constraints created num %d"), SteppingFrameId, PlayerCharacter.CharacterVirtual->GetID().GetValue(), ConstraintsNum);
		
		PlayerCharacter.PlayerItems.Empty();
		int PlayerItemsNum = 0;
		InStream.Read(PlayerItemsNum);
		LOG_CONSTRAINT(TEXT("[Load frame %d][Player %d] Player items created num %d"), SteppingFrameId, PlayerCharacter.CharacterVirtual->GetID().GetValue(), PlayerItemsNum);

		for (int i = 0; i < BodiesNum; i++)
		{
			int BodyIndex = 0;
			InStream.Read(BodyIndex);
			PlayerCharacter.CreatedBodyIndexes.Add(BodyIndex);
		}

		for (int i = 0; i < ConstraintsNum; i++)
		{
			int ConstraintId = 0;
			InStream.Read(ConstraintId);
			PlayerCharacter.CreatedConstraintsIds.Add(ConstraintId);
		}

		for (int i = 0; i < PlayerItemsNum; i++)
		{
			int PlayerItemId = 0;
			InStream.Read(PlayerItemId);
			PlayerCharacter.AddItem(static_cast<ELootItem>(PlayerItemId));
		}
	}
}

void UGameStateManager::ValidateState(StateRecorderImpl& InExpectedState)
{
	// Save state
	StateRecorderImpl current_state;
	SaveToStream(current_state);

	check(current_state.IsEqual(InExpectedState));

	// Compare state with expected state
	if (!current_state.IsEqual(InExpectedState))
	{
		// Mark this stream to break whenever it detects a memory change during reading
		InExpectedState.SetValidating(true);

		// Restore state. Anything that changes indicates a problem with the deterministic simulation.
		LoadFromStream(InExpectedState);
		

		// Turn change detection off again
		InExpectedState.SetValidating(false);
	}
}

bool UGameStateManager::ValidateGameStatesMatch(const int& InFrameId, const int64 InGameStateHash)
{
	if (!GameStateHashes.Contains(InFrameId))
	{
		return true;
	}

	//LOG_NETWORKING(TEXT("[Frame %d] Compare game state hashes: %lld and %lld"), InFrameId, GameStateHashes[InFrameId], InGameStateHash);
	
	return GameStateHashes[InFrameId] == InGameStateHash;
}

void UGameStateManager::SaveGameStateHash(const int& InFrameId)
{
	if (GameStateHashes.Contains(InFrameId))
	{
		GameStateHashes[InFrameId] = GenerateGameStateHash();
	}

	GameStateHashes.Add(InFrameId, GenerateGameStateHash());
}

TMap<int, int64> UGameStateManager::GetGameStateHashes() const
{
	return GameStateHashes;
}

TArray<uint8> UGameStateManager::GetBinaryGameState(const int& InFrameId) const
{
	checkf(SavedStates.Contains(InFrameId), TEXT("Requested copy of game state for non-existing frame"));

	std::string GameData = SavedStates[InFrameId]->GetData();
	TArray<uint8> Bytes;
	Bytes.Append(reinterpret_cast<const uint8*>(GameData.data()), GameData.size());

	//auto BytesNum = Bytes.Num();

	return Bytes;
}

void UGameStateManager::RebuildGameState(const int& InFrameId, const TArray<uint8>& BinaryGameState)
{
	GameStateHashes.Empty();
	SavedStates.Empty();
	SavedStates.Add(InFrameId, MakeUnique<StateRecorderImpl>());

	SavedStates[InFrameId]->Clear();
	SavedStates[InFrameId]->WriteBytes(BinaryGameState.GetData(), BinaryGameState.GetAllocatedSize());

	Load(InFrameId);

	GameStateHashes.Add(InFrameId, GenerateGameStateHash());
}

int64 UGameStateManager::GenerateGameStateHash() const
{
	FString StringToHash;
	
	auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();
	for (const auto& PlayerCharacter : UHelper::GetLocalPlayerController(GetWorld())->PlayerCharacters)
	{
		const Vec3 PlayerPos = JoltSubsystem->GetBodyInterface()->GetCenterOfMassPosition(BodyID(PlayerCharacter.PlayerId));
		StringToHash += *VecToHexString(PlayerPos);
	}
	
	return FHashedName(StringToHash).GetHash();
}
