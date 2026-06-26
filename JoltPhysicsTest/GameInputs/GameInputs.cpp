// Fill out your copyright notice in the Description page of Project Settings.

#include "GameInputs.h"

void UGameInputs::Init(const uint8& InCharactersNum)
{
	CharactersNum = InCharactersNum;
	LOG(TEXT("[UGameInputs] Initialized game inputs for %d characters"), InCharactersNum);
}

bool UGameInputs::HasNoConfirmedInputsForEntireRollbackWindow(const int& CurrentFrameId) const
{
	const int OldestFrameIdWeCanRollbackTo = GetOldestFrameIdWeCanRollbackTo(CurrentFrameId);
	LOG(TEXT("[UGameInputs] \nOldest frame id we can rollback to: %d\nLatest confirmed frame id: %d"), OldestFrameIdWeCanRollbackTo, LatestConfirmedFrameId);
	return LatestConfirmedFrameId <= OldestFrameIdWeCanRollbackTo;
}

bool UGameInputs::IsDesyncDetected() const
{
	return bDesync;
}

#ifdef MY_DEBUG_ROLLBACK
void UGameInputs::DebugRollback(const int& CurrentFrameId)
{
	if (CurrentFrameId == 0)
	{
		return;
	}

	const auto OldestFrameIdForRollback = GetOldestFrameIdWeCanRollbackTo(CurrentFrameId);
	if (OldestFrameIdForRollback < 0)
	{
		return;
	}
	
	OldestDesyncFrameId = OldestFrameIdForRollback;
	bDesync = true;

	LOG(TEXT("[Frame %d][UGameInputs] Debug rollback on. Will rollback to frame %d"), CurrentFrameId, OldestDesyncFrameId);
}
#endif

int UGameInputs::GetFrameIdToRollbackTo() const
{
	return OldestDesyncFrameId + InputDelayFrames;
}

void UGameInputs::MarkDesyncAsFixed()
{
	OldestDesyncFrameId = -1;
	bDesync = false;
	LOG(TEXT("Marked desync as fixed."));

	/*
	TArray<int> PredictedFrameIdsToRemove;
	for (auto& pair : PredictedCommandFrames)
	{
		const int& PredictedFrameId = pair.Key;
		if (LatestConfirmedFrameId > PredictedFrameId)
		{
			PredictedFrameIdsToRemove.Add(PredictedFrameId);
		}
	}

	for (auto& FrameIdToRemove : PredictedFrameIdsToRemove)
	{
		PredictedCommandFrames.Remove(FrameIdToRemove);
		LOG(TEXT("Removed PredictedCommandFrame %d"), FrameIdToRemove);
	}*/
}

void UGameInputs::AddConfirmedCharacterInput(const int& InFrameId, FCharacterInput InCharacterInput)
{
	// If we received info about a frame that is older than what is relevant to us, we can just ignore it
	if (InFrameId <= LastCleanedFrameId)
	{
		return;
	}
	
	if (!ConfirmedCommandFrames.Contains(InFrameId))
	{
		ConfirmedCommandFrames.Add(InFrameId, FCommandFrame(InFrameId));
	}

	if (ConfirmedCommandFrames[InFrameId].DoesCharacterHaveInputs(InCharacterInput.CharacterId))
	{
#ifdef MY_DEBUG_VALIDATE_DETERMINISM
		if (ConfirmedCommandFrames[InFrameId].GetCharacterInputs(InCharacterInput.CharacterId) != InCharacterInput)
		{
			LOG(TEXT("[UGameInputs] ERROR, confirmed inputs difference at frame %d\n%s\n%s"), InFrameId, *ConfirmedCommandFrames[InFrameId].GetCharacterInputs(InCharacterInput.CharacterId).ToString(), *InCharacterInput.ToString());
			checkNoEntry();
		}
#endif
		
		//LOG(TEXT("[UGameInputs] Already had confirmed inputs for character %d, frame %d. So skipping.\n%s"), InCharacterInput.CharacterId, InFrameId, *InCharacterInput.ToString());
		return;
	}

	ConfirmedCommandFrames[InFrameId].CharactersInputs.Add(InCharacterInput);
	OnConfirmedCharacterInputs.Broadcast(InFrameId, InCharacterInput);
	LOG(TEXT("[UGameInputs] Confirmed character %d inputs for frame %d\n%s"), InCharacterInput.CharacterId, InFrameId, *InCharacterInput.ToString());

	if (ConfirmedCommandFrames[InFrameId].CharactersInputs.Num() == CharactersNum)
	{
		if (LatestConfirmedFrameId < InFrameId)
		{
			LatestConfirmedFrameId = InFrameId;
			LOG(TEXT("[UGameInputs] Confirmed all characters inputs for frame %d. Recorded as the LatestConfirmedFrameId"), InFrameId);
			OnConfirmedAllInputsForFrame.Broadcast(ConfirmedCommandFrames[InFrameId]);

			// We only check for desyncs once we have confirmed inputs from all players
			// Even tho in theory we could rollback to fix a desync with one of the players before we know someone elses input and hope we predicted theirs correctly
			// Something to think about in the future
			if (PredictedCommandFrames.Contains(InFrameId) && PredictedCommandFrames[InFrameId].DoesCharacterHaveInputs(InCharacterInput.CharacterId))
			{
				auto& ConfirmedCharacterInputs = ConfirmedCommandFrames[InFrameId].GetCharacterInputs(InCharacterInput.CharacterId);
				auto& PredictedCharacterInputs = PredictedCommandFrames[InFrameId].GetCharacterInputs(InCharacterInput.CharacterId);
		
				LOG(TEXT("[UGameInputs] Was our prediction correct: %s"), (ConfirmedCharacterInputs != PredictedCharacterInputs) ? TEXT("no") : TEXT("yes"));
				if (ConfirmedCharacterInputs != PredictedCharacterInputs)
				{
					LOG(TEXT("[UGameInputs] Desync detected on frame %d"), InFrameId);
					/**
					 * We don't rollback to the frame where inputs were recorded
					 * It's important to understand the following concept:
					 * There is the frame id where inputs get recorded.
					 * And then there is a separate frame id where inputs get PROCESSED.
					 * Those two are different concepts.
					 * So if we want to re-simulate the desynced inputs from frame X
					 * We need to rollback the simulation to frame X + InputDelayFrames
					*/
					const uint32 RollbackToFrameId = InFrameId + InputDelayFrames;

					if (OldestDesyncFrameId < 0 || InFrameId <= OldestDesyncFrameId)
					{
						OldestDesyncFrameId = InFrameId;
						bDesync = true;
						//LOG_TEMPORARY(TEXT("[UGameInputs] Desync recorded. Expecting to rollback to frame %d"), RollbackToFrameId);
					}
					else
					{
						LOG(TEXT("[UGameInputs] Older frame %d is already marked as desync. So skipping this one."), OldestDesyncFrameId);
					}
				}
			}
		}
	}
}

FCharacterInput UGameInputs::GetCharacterInputForFrame(const int& InFrameId, const int& InCharacterId)
{
	//LOG_TEMPORARY(TEXT("[UGameInputs] GetNextCharacterInput for simulation frame %d, character %d"), InFrameId, InCharacterId);
	if (InFrameId < InputDelayFrames)
	{
		// This is a special case that can only occur at the very start of the simulation during the first frames.
		// We allow the input buffer to fill up to the specified amount in InputDelayFrames.
		// While that happens we don't have any inputs to work with, so we will just return empty ones, which essentially just stalls the simulation for X frames.
		
		LOG(TEXT("[UGameInputs] Returning fake inputs, because current frame %d less than InputDelayFrames %d"), InFrameId, InputDelayFrames);
		return FCharacterInput(InCharacterId);
	}

	const int FrameIdToReturn = InFrameId - InputDelayFrames;		
	LOG(TEXT("[UGameInputs] Input frame is %d (CurrentFrameId - InputDelayFrames)"), FrameIdToReturn);

	if (ConfirmedCommandFrames.Contains(FrameIdToReturn) && ConfirmedCommandFrames[FrameIdToReturn].DoesCharacterHaveInputs(InCharacterId))
	{
		//LOG_TEMPORARY(TEXT("[UGameInputs] Found confirmed inputs \n%s"), *ConfirmedCommandFrames[FrameIdToReturn].GetCharacterInputs(InCharacterId).ToString());
		return ConfirmedCommandFrames[FrameIdToReturn].GetCharacterInputs(InCharacterId);
	}

	if (PredictedCommandFrames.Contains(FrameIdToReturn) && PredictedCommandFrames[FrameIdToReturn].DoesCharacterHaveInputs(InCharacterId))
	{
		//LOG_TEMPORARY(TEXT("[UGameInputs] Found predicted inputs \n%s"), *PredictedCommandFrames[FrameIdToReturn].GetCharacterInputs(InCharacterId).ToString());
		return PredictedCommandFrames[FrameIdToReturn].GetCharacterInputs(InCharacterId);
	}
	else
	{
		PredictedCommandFrames.Add(FrameIdToReturn, FCommandFrame(FrameIdToReturn));
		
		for (uint8 i = 1; i <= MaxRollbackFrames; i++)
		{
			if (ConfirmedCommandFrames.Contains(FrameIdToReturn - i) && ConfirmedCommandFrames[FrameIdToReturn - i].DoesCharacterHaveInputs(InCharacterId))
			{
				const auto& LatestConfirmedInput = ConfirmedCommandFrames[FrameIdToReturn - i].GetCharacterInputs(InCharacterId);
				PredictedCommandFrames[FrameIdToReturn].CharactersInputs.Add(LatestConfirmedInput);
				//LOG_TEMPORARY(TEXT("[UGameInputs] Predicted character %d inputs for frame %d\n%s"), InCharacterId, FrameIdToReturn, *LatestConfirmedInput.ToString());

				return LatestConfirmedInput;
			}
		}

		// We couldn't find no confirmed input for this player. Should only happen at the very start of the simulation
		const auto PredictedInputs = FCharacterInput(InCharacterId);
		PredictedCommandFrames[FrameIdToReturn].CharactersInputs.Add(PredictedInputs);
		LOG(TEXT("[UGameInputs] Predicted character %d inputs for frame %d\n(No confirmed inputs found to base prediction on. SHOULD ONLY HAPPEN AT THE START)\n%s"), InCharacterId, FrameIdToReturn, *PredictedInputs.ToString());
			
		return PredictedCommandFrames[FrameIdToReturn].GetCharacterInputs(InCharacterId);
	}
}

TArray<FCommandFrame> UGameInputs::GetConfirmedInputs()
{
	TArray<FCommandFrame> ConfirmedFrames;
	ConfirmedCommandFrames.GenerateValueArray(ConfirmedFrames);
	LOG(TEXT("[UGameInputs] Generated confirmed inputs array to send over the net. Total frames: %d"), ConfirmedFrames.Num());

	return ConfirmedFrames;
}

uint8 UGameInputs::GetMaxRollbackFrames() const
{
	return MaxRollbackFrames;
}

uint8 UGameInputs::GetInputDelayFrames() const
{
	return InputDelayFrames;
}

uint8 UGameInputs::GetRollbackWindow() const
{
	return InputDelayFrames + MaxRollbackFrames;
}

int UGameInputs::GetPredictedFramesNum() const
{
	return PredictedCommandFrames.Num();
}

int UGameInputs::GetConfirmedFramesNum() const
{
	return ConfirmedCommandFrames.Num();
}

int UGameInputs::GetOldestFrameIdWeCanRollbackTo(const int& CurrentFrameId) const
{
	return CurrentFrameId - GetRollbackWindow();
}

int UGameInputs::GetFrameIdSafeToCleanUp(const int& CurrentFrameId) const
{
	if (!bCleanUp)
	{
		return -1;
	}
	
	const int RollbackWindow = GetRollbackWindow();
	
	// It's only safe to clean frames that we can't rollback to anymore
	if (CurrentFrameId > static_cast<int>(RollbackWindow))
	{
		return CurrentFrameId - RollbackWindow - 1;
	}

	return -1;
}

void UGameInputs::CleanupFrame(const int& InFrameId)
{
	// For predicted and confirmed frames we only want to clean them when they reach 2x the rollback window
	// Because these get exchanged between players over the net and another player might be a whole rollback window behind or ahead of us
	// So in order to guarantee that no player ever loses an input for any given frame, we must keep 2x the rollback window
	const auto RollbackWindow = GetRollbackWindow();
	PredictedCommandFrames.Remove(InFrameId - RollbackWindow);
	ConfirmedCommandFrames.Remove(InFrameId - RollbackWindow);

	LastCleanedFrameId = InFrameId;
	LOG(TEXT("[UGameInputs] Cleaned old data for frame %d\nPredictedCommandFrames size: %d\nConfirmedCommandFrames size: %d"), InFrameId, PredictedCommandFrames.Num(), ConfirmedCommandFrames.Num());
}

uint8 UGameInputs::GetCharactersNum() const
{
	return CharactersNum;
}

void UGameInputs::TurnOffCleanUp()
{
	bCleanUp = false;
}

int UGameInputs::GetLatestConfirmedFrameId() const
{
	return LatestConfirmedFrameId;
}

void UGameInputs::OnRebuiltGameStateManually(const int& InFrameId)
{
	LatestConfirmedFrameId = InFrameId - InputDelayFrames;
	ClearPredictedAndConfirmedFramesAfterFrameId(LatestConfirmedFrameId);
}

void UGameInputs::ClearPredictedAndConfirmedFramesAfterFrameId(const int& InFrameId)
{
	TArray<int> KeysToRemove;
	for (const auto& Pair : PredictedCommandFrames)
	{
		if (Pair.Key > InFrameId)
		{
			KeysToRemove.Add(Pair.Key);
		}
	}
	for (const auto& KeyToRemove : KeysToRemove)
	{
		PredictedCommandFrames.Remove(KeyToRemove);
	}

	for (const auto& Pair : ConfirmedCommandFrames)
	{
		if (Pair.Key > InFrameId)
		{
			KeysToRemove.Add(Pair.Key);
		}
	}
	for (const auto& KeyToRemove : KeysToRemove)
	{
		ConfirmedCommandFrames.Remove(KeyToRemove);
	}
}
