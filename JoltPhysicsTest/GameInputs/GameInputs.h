// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "JoltPhysicsTest/Debug/CustomLogger.h"
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameInputs.generated.h"

USTRUCT()
struct FCharacterInput
{
	GENERATED_BODY()

	FCharacterInput() {}

	FCharacterInput(const int& InCharacterId)
	{
		CharacterId = InCharacterId;
		MouseDelta = FVector2D::ZeroVector;
		bShoot = false;
		bSpawnBox = false;
		bDash = false;
		bParry = false;
	}

	FCharacterInput(
		const int& InCharacterId,
		const FVector2D& InMouseDelta,
		const bool& InbShoot,
		const bool& InbSpawnBox,
		const bool& InbDash,
		const bool& InbParry
	)
	{
		CharacterId = InCharacterId;
		MouseDelta = InMouseDelta;
		bShoot = InbShoot;
		bSpawnBox = InbSpawnBox;
		bDash = InbDash;
		bParry = InbParry;
	}

	// Character can be anything that has inputs that affect the simulation.
	// For example this could be used to drive an AI bot.
	// This id is just used to tie related inputs together as coming from the same source.
	// And it MUST be consistent across all clients and server.
	UPROPERTY()
	int CharacterId = -1;

	UPROPERTY()
	FVector2D MouseDelta = FVector2D::ZeroVector;

	UPROPERTY()
	bool bShoot = false;

	UPROPERTY()
	bool bSpawnBox = false;

	UPROPERTY()
	bool bDash = false;

	UPROPERTY()
	bool bParry = false;

	bool operator!=(const FCharacterInput& rhs) const
	{
		return MouseDelta != rhs.MouseDelta
			|| bShoot != rhs.bShoot
			|| bSpawnBox != rhs.bSpawnBox
			|| bDash != rhs.bDash
			|| bParry != rhs.bParry;
	}

	FString ToString() const
	{
		return FString::Printf(
			TEXT("CharacterId: %d\nMouseDelta: %s\nShoot: %d\nSpawnBox: %d\nDash: %d\nParry: %d"),
			CharacterId,
			*VectorToHexString(FVector(MouseDelta.X, MouseDelta.Y, 0)),
			bShoot,
			bSpawnBox,
			bDash,
			bParry
		);
	}
};

inline FArchive& operator<<(FArchive& Ar, FCharacterInput& Data)
{
	Ar << Data.CharacterId;
	Ar << Data.MouseDelta;
	Ar << Data.bShoot;
	Ar << Data.bSpawnBox;
	Ar << Data.bDash;
	Ar << Data.bParry;

	return Ar;
}

USTRUCT()
struct FCommandFrame
{
	GENERATED_BODY()

	FCommandFrame() {}

	FCommandFrame(int InFrameId)
	{
		FrameId = InFrameId;
	}

	// FrameId is the current simulation frame. It must never repeat, until a new simulation is started.
	UPROPERTY()
	int FrameId = -1;

	UPROPERTY()
	TArray<FCharacterInput> CharactersInputs;

	bool DoesCharacterHaveInputs(const int& InCharacterId) const
	{
		for (auto& CharacterInputs : CharactersInputs)
		{
			if (CharacterInputs.CharacterId == InCharacterId)
			{
				return true;
			}
		}

		return false;
	}
	
	const FCharacterInput& GetCharacterInputs(const int& InCharacterId) const
	{
		for (auto& CharacterInputs : CharactersInputs)
		{
			if (CharacterInputs.CharacterId == InCharacterId)
			{
				return CharacterInputs;
			}
		}

		checkNoEntry();

		// Should NEVER reach this part of the code
		return CharactersInputs[0];
	}

	FString ToString() const
	{
		FString FinalString = FString::Printf(TEXT("Frame: %d\n"), FrameId);
		for (auto& CharacterInputs : CharactersInputs)
		{
			FinalString += CharacterInputs.ToString();
			FinalString += "\n";
		}

		return FinalString;
	}

	void SortCharactersInputs()
	{
		CharactersInputs.Sort([](const FCharacterInput& A, const FCharacterInput& B) {
			return A.CharacterId < B.CharacterId;
		});
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FConfirmedAllInputsForFrame, const FCommandFrame&, InCommandFrame);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FConfirmedCharacterInputs, const int&, InFrameId, const FCharacterInput&, InCharacterInput);

UCLASS()
class JOLTPHYSICSTEST_API UGameInputs : public UObject
{
	GENERATED_BODY()

public:
	void Init(const uint8& InCharactersNum);

	// If the entire rollback window (MaxRollbackFrames + InputDelayFrames) still has no fully confirmed frame, then we can't proceed with the simulation
	// Because we could desync between clients permanently with no way to rollback
	bool HasNoConfirmedInputsForEntireRollbackWindow(const int& CurrentFrameId) const;

	bool IsDesyncDetected() const;
	
	FConfirmedAllInputsForFrame OnConfirmedAllInputsForFrame;
	FConfirmedCharacterInputs OnConfirmedCharacterInputs;

#ifdef MY_DEBUG_ROLLBACK
	void DebugRollback(const int& CurrentFrameId);
#endif
	
	/**
	 * We don't rollback to the frame where inputs were recorded
	 * It's important to understand the following concept:
	 * There is the frame id where inputs get recorded.
	 * And then there is frame id where inputs get PROCESSED.
	 * Those two are different concepts.
	 * So if we want to re-simulate the desynced inputs from frame X
	 * We need to rollback the simulation to frame X + InputDelayFrames
	*/
	int GetFrameIdToRollbackTo() const;
	
	void MarkDesyncAsFixed();
	
	// Only confirmed inputs can be added externally. Speculating inputs is the responsibility of this class only.
	// So this method should be used for owning player inputs and for remote player inputs received over the net
	void AddConfirmedCharacterInput(const int& InFrameId, FCharacterInput InCharacterInput);

	// This can return confirmed or predicted input or a mix of both. But it will always return inputs for all characters
	// Also it won't necessarily return the inputs for the passed FrameId, it might inject latency and return inputs from 3 frames back
	// The passed FrameId is just a reference point to know what frame the simulation is currently at
	FCharacterInput GetCharacterInputForFrame(const int& InFrameId, const int& InCharacterId);

	// We only ever exchange CONFIRMED inputs over the net. Predicted ones don't make sense, since each client can handle those on their own machine.
	TArray<FCommandFrame> GetConfirmedInputs();

	uint8 GetMaxRollbackFrames() const;
	uint8 GetInputDelayFrames() const;
	uint8 GetRollbackWindow() const;
	int GetPredictedFramesNum() const;
	int GetConfirmedFramesNum() const;

	int GetOldestFrameIdWeCanRollbackTo(const int& CurrentFrameId) const;
	int GetFrameIdSafeToCleanUp(const int& CurrentFrameId) const;

	void CleanupFrame(const int& InFrameId);

	uint8 GetCharactersNum() const;

	void TurnOffCleanUp();

	int GetLatestConfirmedFrameId() const;

	// Should only be used when whole game state is rebuilt, for example when syncing clients
	// Then you need to set the confirmed frame id manually
	void OnRebuiltGameStateManually(const int& InFrameId);

	void ClearPredictedAndConfirmedFramesAfterFrameId(const int& InFrameId);
private:
	// How many characters inputs are we tracking. Otherwise we can't tell when we have full input info for a specific frame.
	uint8 CharactersNum = 0;

	// The FrameId to which we will need to rollback the simulation. We only need to keep the oldest.
	int OldestDesyncFrameId = -1;
	bool bDesync = false;

	// This is the most up to date FrameId that we have all the characters inputs for. Meaning we need 0 prediction for it.
	// Because we exchange all the inputs for all the frames in the rollback window over the net, we can never be in a situation
	// Where we might have fully confirmed frame 3, but frame 2 is still missing inputs. That's because when the inputs arrive for frame 3
	// They will also contain inputs for all the previous frames in the rollback window. So we are guaranteed that every frame BEFORE
	// LatestConfirmedFrameId is also fully confirmed.
	int LatestConfirmedFrameId = -1;

	// These can also include the inputs of the local player, even though those are obviously correct.
	// But even in that case, they will only move to Confirmed once the server has approved them.
	TMap<int, FCommandFrame> PredictedCommandFrames;

	// Confirmed frames contain data that has been verified by the server to be correct
	TMap<int, FCommandFrame> ConfirmedCommandFrames;

	// We only clean frames once they go outside our rollback window
	// Which means they should NEVER get added back, even if we receive info for these frames in some late packet coming from server or client
	// So we use this to check if frame data we received is still relevant to us
	int LastCleanedFrameId = -1;

	// It will delay inputs for processing by the amount of frames specified here.
	// Obviously this makes the gamefeel laggier and should be kept as small as possible.
	// But it allows to have a buffer time for syncing between networks
	// For example 2 intentional frames delayed buys us around 32ms of time for the packet to travel
	uint8 InputDelayFrames = 3;

	// The maximum amount of frames of rollback we support
	// If the simulations diverge more than that, we have to pause the simulation until the inputs arrive
	uint8 MaxRollbackFrames = 7;

	// This should always be on in a real game. It's turned off for replays, so we can rewind as far back as needed.
	bool bCleanUp = true;
};
