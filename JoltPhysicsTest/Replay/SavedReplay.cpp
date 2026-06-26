// Fill out your copyright notice in the Description page of Project Settings.


#include "SavedReplay.h"

void USavedReplay::AddCharacterInput(const int& SimulationFrameId, const int& InputsFrameId, const FCharacterInput& InCharacterInput)
{
	if (!SavedFrames.Contains(SimulationFrameId))
	{
		SavedFrames.Add(SimulationFrameId, FSavedFrame());
	}

	checkf(!SavedFrames[SimulationFrameId].DoesCharacterHaveInputs(InputsFrameId, InCharacterInput.CharacterId), TEXT("Replay already contains inputs for this character and frame"));

	SavedFrames[SimulationFrameId].AddCharacterInputs(InputsFrameId, InCharacterInput);
}

TArray<FInputFrame> USavedReplay::GetInputFrames(const int& SimulationFrameId)
{
	check(SavedFrames.Contains(SimulationFrameId));
	
	SavedFrames[SimulationFrameId].SortInputFrames();

	return SavedFrames[SimulationFrameId].InputsFrames;
}

int USavedReplay::GetReplayFramesNum() const
{
	return SavedFrames.Num();
}

void USavedReplay::SaveStepTime(const int& SimulationFrameId, const float& InStepTime)
{
	if (!SavedFrames.Contains(SimulationFrameId))
	{
		SavedFrames.Add(SimulationFrameId, FSavedFrame());
	}

	SavedFrames[SimulationFrameId].StepTime = InStepTime;
}

void USavedReplay::SaveRoundTripTime(const int& SimulationFrameId, const float& InRoundTripTime)
{
	if (!SavedFrames.Contains(SimulationFrameId))
	{
		SavedFrames.Add(SimulationFrameId, FSavedFrame());
	}

	SavedFrames[SimulationFrameId].RoundTripTime = InRoundTripTime;
}

void USavedReplay::SavePredictedAndConfirmedFramesNum(const int& SimulationFrameId, const int& PredictedFramesNum, const int& ConfirmedFramesNum)
{
	if (!SavedFrames.Contains(SimulationFrameId))
	{
		SavedFrames.Add(SimulationFrameId, FSavedFrame());
	}

	SavedFrames[SimulationFrameId].PredictedFramesNum = PredictedFramesNum;
	SavedFrames[SimulationFrameId].ConfirmedFramesNum = ConfirmedFramesNum;
}

float USavedReplay::GetRoundTripTime(const int& SimulationFrameId)
{
	check(SavedFrames.Contains(SimulationFrameId));
	
	return SavedFrames[SimulationFrameId].RoundTripTime;
}

int USavedReplay::GetPredictedFramesNum(const int& SimulationFrameId)
{
	check(SavedFrames.Contains(SimulationFrameId));
	
	return SavedFrames[SimulationFrameId].PredictedFramesNum;
}

int USavedReplay::GetConfirmedFramesNum(const int& SimulationFrameId)
{
	check(SavedFrames.Contains(SimulationFrameId));
	
	return SavedFrames[SimulationFrameId].ConfirmedFramesNum;
}

float USavedReplay::GetStepTime(const int& SimulationFrameId) const
{
	check(SavedFrames.Contains(SimulationFrameId));
	
	return SavedFrames[SimulationFrameId].StepTime;
}

const FPlayerSettings& USavedReplay::GetPlayerSettings(const int& InPlayerId)
{
	checkf(PlayersSettings.Contains(InPlayerId), TEXT("Replay doesn't have settings for player %d"), InPlayerId);

	return PlayersSettings[InPlayerId];
}

void USavedReplay::SavePlayerSettings(const int& InPlayerId, const FPlayerSettings& InPlayerSettings)
{
	if (PlayersSettings.Contains(InPlayerId))
	{
		checkf(false, TEXT("Replay already contains settings for this player id %d"), InPlayerId);
		return;
	}
	
	PlayersSettings.Add(InPlayerId, InPlayerSettings);
}

void USavedReplay::IncreaseFramesSkippedDueToNoInputsForRollbackWindow(const int& SimulationFrameId)
{
	TotalFramesSkippedDueToNoInputsForRollbackWindow++;

	if (!SavedFrames.Contains(SimulationFrameId))
	{
		SavedFrames.Add(SimulationFrameId, FSavedFrame());
	}

	SavedFrames[SimulationFrameId].FramesSkippedDueToNoInputsForRollbackWindow++;
}

int USavedReplay::GetTotalFramesSkippedDueToNoInputsForRollbackWindow() const
{
	return TotalFramesSkippedDueToNoInputsForRollbackWindow;
}

int USavedReplay::GetFramesSkippedDueToNoInputsForRollbackWindow(const int& SimulationFrameId) const
{
	return SavedFrames[SimulationFrameId].FramesSkippedDueToNoInputsForRollbackWindow;
}

void USavedReplay::PackageDataForSaving()
{
	TArray<uint8> Buffer;
	Buffer.Reserve(1024);
	FMemoryWriter Writer(Buffer);

	Writer << bIsServer;
	Writer << PlayersNum;
	Writer << RandomNumberGenerationsSkip;
	Writer << LevelName;
	Writer << MaxRollbackFramesNum;
	Writer << InputDelayFramesNum;
	Writer << GameSeed;

	Writer << LevelsSequence;
	Writer << PlayersSettings;
	Writer << SavedFrames;

	Writer << TotalFramesSkippedDueToNoInputsForRollbackWindow;

	PackagedData = Buffer;
}

void USavedReplay::UnpackDataForProcessing()
{
	FMemoryReader Reader(PackagedData);

	Reader << bIsServer;
	Reader << PlayersNum;
	Reader << RandomNumberGenerationsSkip;
	Reader << LevelName;
	Reader << MaxRollbackFramesNum;
	Reader << InputDelayFramesNum;
	Reader << GameSeed;

	Reader << LevelsSequence;
	Reader << PlayersSettings;
	Reader << SavedFrames;

	Reader << TotalFramesSkippedDueToNoInputsForRollbackWindow;
}

