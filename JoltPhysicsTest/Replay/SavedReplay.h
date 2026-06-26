// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "JoltPhysicsTest/GameInputs/GameInputs.h"
#include "JoltPhysicsTest/Player/MyPlayerController.h"
#include "SavedReplay.generated.h"

USTRUCT()
struct FInputFrame
{
	GENERATED_BODY()

	FInputFrame() {}

	FInputFrame(const int& InInputFrameId, const FCharacterInput& InCharacterInput)
	{
		FrameId = InInputFrameId;
		CharacterInput = InCharacterInput;
	}

	int FrameId = 0;

	FCharacterInput CharacterInput;
};

inline FArchive& operator<<(FArchive& Ar, FInputFrame& Data)
{
	Ar << Data.FrameId;
	Ar << Data.CharacterInput;

	return Ar;
}

USTRUCT()
struct FSavedFrame
{
	GENERATED_BODY()

	FSavedFrame() {}

	// Very important to keep in mind that one character can have multiple entries here
	// Because in a real scenario we might add multiple frames worth of inputs from a remote client in a single go
	// One FInputFrame represents one frames inputs from one character
	TArray<FInputFrame> InputsFrames;

	int FramesSkippedDueToNoInputsForRollbackWindow = 0;

	float StepTime = 0.f;

	int PredictedFramesNum = 0;
	
	int ConfirmedFramesNum = 0;

	float RoundTripTime = 0.f;

	void AddCharacterInputs(const int& InputsFrameId, const FCharacterInput& InCharacterInput)
	{
		InputsFrames.Add({InputsFrameId, InCharacterInput});
	}
	
	bool DoesCharacterHaveInputs(const int& InputsFrameId, const int& InCharacterId) const
	{
		for (const auto& InputsFrame : InputsFrames)
		{
			if (InputsFrame.FrameId == InputsFrameId && InputsFrame.CharacterInput.CharacterId == InCharacterId)
			{
				return true;
			}
		}

		return false;
	}

	void SortInputFrames()
	{
		InputsFrames.Sort([](const FInputFrame& A, const FInputFrame& B) {
			return A.FrameId < B.FrameId;
		});
	}
};

inline FArchive& operator<<(FArchive& Ar, FSavedFrame& Data)
{
	Ar << Data.InputsFrames;
	Ar << Data.FramesSkippedDueToNoInputsForRollbackWindow;
	Ar << Data.StepTime;
	Ar << Data.PredictedFramesNum;
	Ar << Data.ConfirmedFramesNum;
	Ar << Data.RoundTripTime;

	return Ar;
}

UCLASS()
class JOLTPHYSICSTEST_API USavedReplay : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<uint8> PackagedData;
	
	bool bIsServer = false;
	
	int PlayersNum = 0;

	int RandomNumberGenerationsSkip = 0;

	FString LevelName;

	int MaxRollbackFramesNum = 0;

	int InputDelayFramesNum = 0;
	
	FString GameSeed;

	TArray<FLevelData> LevelsSequence;
	
	TMap<int, FPlayerSettings> PlayersSettings;

	void AddCharacterInput(const int& SimulationFrameId, const int& InputsFrameId, const FCharacterInput& InCharacterInput);
	TArray<FInputFrame> GetInputFrames(const int& SimulationFrameId);

	int GetReplayFramesNum() const;

	void SaveStepTime(const int& SimulationFrameId, const float& InStepTime);
	void SaveRoundTripTime(const int& SimulationFrameId, const float& InRoundTripTime);

	float GetStepTime(const int& SimulationFrameId) const;

	const FPlayerSettings& GetPlayerSettings(const int& InPlayerId);
	void SavePlayerSettings(const int& InPlayerId, const FPlayerSettings& InPlayerSettings);

	void SavePredictedAndConfirmedFramesNum(const int& SimulationFrameId, const int& PredictedFramesNum, const int& ConfirmedFramesNum);
	float GetRoundTripTime(const int& SimulationFrameId);

	int GetPredictedFramesNum(const int& SimulationFrameId);
	int GetConfirmedFramesNum(const int& SimulationFrameId);

	void IncreaseFramesSkippedDueToNoInputsForRollbackWindow(const int& SimulationFrameId);

	int GetTotalFramesSkippedDueToNoInputsForRollbackWindow() const;
	int GetFramesSkippedDueToNoInputsForRollbackWindow(const int& SimulationFrameId) const;

	void PackageDataForSaving();
	void UnpackDataForProcessing();
private:
	// <SimulationFrameId, FSavedFrame>
	// We have frame id for when the input was recorded in the current simulation, which is SimulationFrameId
	// And then we have the frame id on which the inputs were generated, which won't be the same when we receive info from a remote client
	// These are saved inside FSavedFrame as they are unique for each character inputs
	TMap<int, FSavedFrame> SavedFrames;

	int TotalFramesSkippedDueToNoInputsForRollbackWindow = 0;
};
