// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "JoltPhysicsTest/Debug/CustomLogger.h"
#include "GameStateManager.generated.h"

/** ! Be careful with events outside of pure rendering. The simulation has to be deterministic and delegate call order is not guaranteed. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSavedGameState, int, FrameId);

class CustomStateRecorderFilter : public StateRecorderFilter
{
public:
	virtual bool ShouldSaveBody(const Body& inBody) const override
	{
		if (inBody.IsStatic())
		{
			return false;
		}

		return true;
	}

	virtual bool ShouldSaveConstraint(const Constraint& inConstraint) const override
	{
		return true;
	}
};

UCLASS()
class JOLTPHYSICSTEST_API UGameStateManager : public UObject
{
	GENERATED_BODY()
public:
	FOnSavedGameState OnSavedGameState;
	
	void Save(const int& InFrameId);
	void Load(const int& InFrameId);
	void Remove(const int& InFrameId);

	void SaveToStream(StateRecorderImpl& InStream);
	void LoadFromStream(StateRecorderImpl& InStream);
	void ValidateState(StateRecorderImpl& InExpectedState);

	bool ValidateGameStatesMatch(const int& InFrameId, const int64 InGameStateHash);

	void SaveGameStateHash(const int& InFrameId);
	TMap<int, int64> GetGameStateHashes() const;

	TArray<uint8> GetBinaryGameState(const int& InFrameId) const;
	void RebuildGameState(const int& InFrameId, const TArray<uint8>& BinaryGameState);
private:
	int64 GenerateGameStateHash() const;
	TMap<int, TUniquePtr<StateRecorderImpl>> SavedStates;
	TMap<int, int64> GameStateHashes;
};
