// Fill out your copyright notice in the Description page of Project Settings.


#include "MyReplaySubsystem.h"

#include "JoltPhysicsTest/FunctionLibrary/Helper.h"
#include "JoltPhysicsTest/Game/GameLoopSubsystem.h"
#include "JoltPhysicsTest/Game/MyGameInstance.h"
#include "JoltPhysicsTest/GameInputs/InputsSubsystem.h"
#include "JoltPhysicsTest/Player/MyPlayerController.h"
#include "Kismet/GameplayStatics.h"

bool UMyReplaySubsystem::ShouldCreateSubsystem(UObject* Outer) const
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

void UMyReplaySubsystem::PostInitialize()
{
	Super::PostInitialize();

	LOG_REPLAY(TEXT("InitMyReplay"));

	auto InputsSubsystem = GetWorld()->GetSubsystem<UInputsSubsystem>();
	check(InputsSubsystem);
	check(InputsSubsystem->GameInputs);

	InputsSubsystem->GameInputs->OnConfirmedCharacterInputs.AddUniqueDynamic(this, &UMyReplaySubsystem::OnConfirmedCharacterInputs);

	auto MyGameInstance = UHelper::GetMyGameInstance(GetWorld());
	checkf(MyGameInstance, TEXT("My game instance is null"));
	if (MyGameInstance->bReplayMode)
	{
		LoadedReplay = Cast<USavedReplay>(UGameplayStatics::LoadGameFromSlot(FString::Printf(TEXT("%s"), *MyGameInstance->ReplayName), 0));
		check(LoadedReplay);
		LoadedReplay->UnpackDataForProcessing();

		bProcessReplayMode = MyGameInstance->bProcessReplayMode;
	}
}

void UMyReplaySubsystem::Deinitialize()
{
	Super::Deinitialize();

	SaveReplayToFile();
}

void UMyReplaySubsystem::RecordReplay(const uint32& PlayedGamesNum, FString LevelName)
{
	SavedReplaysNum = PlayedGamesNum;
	LOG_REPLAY(TEXT("Init with players num: %d"), PlayedGamesNum);

	CachedNetMode = GetWorld()->GetNetMode();
	LOG_REPLAY(TEXT("Net mode: %d"), static_cast<int>(CachedNetMode));

	SaveInProgressReplay = Cast<USavedReplay>(UGameplayStatics::CreateSaveGameObject(USavedReplay::StaticClass()));

	SaveInProgressReplay->LevelName = LevelName;
}

void UMyReplaySubsystem::IncreaseFramesSkippedDueToNoInputsForRollbackWindow(const int& InSimulationFrameId)
{
	if (IsReplayMode() || !IsRecording())
	{
		return;
	}

	SaveInProgressReplay->IncreaseFramesSkippedDueToNoInputsForRollbackWindow(InSimulationFrameId);
}

void UMyReplaySubsystem::SavePredictedAndConfirmedFrames(const int& InSimulationFrameId,
	const int& InPredictedFramesNum, const int& InConfirmedFramesNum)
{
	if (IsReplayMode() || !IsRecording())
	{
		return;
	}
	
	SaveInProgressReplay->SavePredictedAndConfirmedFramesNum(InSimulationFrameId, InPredictedFramesNum, InConfirmedFramesNum);
}

void UMyReplaySubsystem::SaveStepTime(const int& InSimulationFrameId, const float& InStepTime)
{
	if (IsReplayMode() || !IsRecording())
	{
		return;
	}
	
	SaveInProgressReplay->SaveStepTime(InSimulationFrameId, InStepTime);
}

void UMyReplaySubsystem::SaveRoundTripTime(const int& InSimulationFrameId, const float& InRoundTripTime)
{
	if (IsReplayMode() || !IsRecording())
	{
		return;
	}
	
	SaveInProgressReplay->SaveRoundTripTime(InSimulationFrameId, InRoundTripTime);
}

bool UMyReplaySubsystem::IsServer() const
{
	return LoadedReplay->bIsServer;
}

float UMyReplaySubsystem::GetRoundTripTime(int InSimulationFrameId)
{
	return LoadedReplay->GetRoundTripTime(InSimulationFrameId);
}

int UMyReplaySubsystem::GetPredictedFramesNum(int InSimulationFrameId)
{
	return LoadedReplay->GetPredictedFramesNum(InSimulationFrameId);
}

int UMyReplaySubsystem::GetConfirmedFramesNum(int InSimulationFrameId)
{
	return LoadedReplay->GetConfirmedFramesNum(InSimulationFrameId);
}

float UMyReplaySubsystem::GetCurrentStepTime(int InSimulationFrameId)
{
	return LoadedReplay->GetStepTime(InSimulationFrameId);
}

int UMyReplaySubsystem::GetTotalFramesSkippedDueToNoInputsForRollbackWindow()
{
	return LoadedReplay->GetTotalFramesSkippedDueToNoInputsForRollbackWindow();
}

int UMyReplaySubsystem::GetFramesSkippedDueToNoInputsForRollbackWindow(int InSimulationFrameId)
{
	return LoadedReplay->GetFramesSkippedDueToNoInputsForRollbackWindow(InSimulationFrameId);
}

int UMyReplaySubsystem::GetInitialRNGSkip()
{
	return LoadedReplay->RandomNumberGenerationsSkip;
}

int UMyReplaySubsystem::GetInputDelayFramesNum()
{
	return LoadedReplay->InputDelayFramesNum;
}

int UMyReplaySubsystem::GetMaxRollbackFramesNum()
{
	return LoadedReplay->MaxRollbackFramesNum;
}

bool UMyReplaySubsystem::IsServer()
{
	return LoadedReplay->bIsServer;
}

void UMyReplaySubsystem::SaveIsServer(bool bIsServer)
{
	if (!IsRecording())
	{
		return;
	}
	
	SaveInProgressReplay->bIsServer = bIsServer;
}

void UMyReplaySubsystem::SavePlayersNum(const int& InPlayersNum)
{
	if (IsReplayMode() || !IsRecording())
	{
		return;
	}

	LOG_REPLAY(TEXT("Save players num: %d"), InPlayersNum);
	
	SaveInProgressReplay->PlayersNum = InPlayersNum;
}

void UMyReplaySubsystem::SavePlayerSettings(const int& InPlayerId, const FPlayerSettings& InPlayerSettings)
{
	if (IsReplayMode() || !IsRecording())
	{
		return;
	}

	SaveInProgressReplay->SavePlayerSettings(InPlayerId, InPlayerSettings);
}

void UMyReplaySubsystem::SaveInputDelayFramesNum(const int& InputDelayFramesNum)
{
	if (IsReplayMode() || !IsRecording())
	{
		return;
	}

	SaveInProgressReplay->InputDelayFramesNum = InputDelayFramesNum;
}

void UMyReplaySubsystem::SaveMaxRollbackFramesNum(const int& MaxRollbackFramesNum)
{
	if (IsReplayMode() || !IsRecording())
	{
		return;
	}

	SaveInProgressReplay->MaxRollbackFramesNum = MaxRollbackFramesNum;
}

void UMyReplaySubsystem::SaveGameSeed(const FString& InGameSeedString)
{
	if (IsReplayMode() || !IsRecording())
	{
		return;
	}

	SaveInProgressReplay->GameSeed = InGameSeedString;
}

void UMyReplaySubsystem::SaveLevelsSequence(const TArray<FLevelData>& InLevelsSequence)
{
	if (IsReplayMode() || !IsRecording())
	{
		return;
	}

	SaveInProgressReplay->LevelsSequence = InLevelsSequence;
}

void UMyReplaySubsystem::SaveInitialRNGSkip(int RandomNumberGenerationsNum)
{
	if (IsReplayMode() || !IsRecording())
	{
		return;
	}

	SaveInProgressReplay->RandomNumberGenerationsSkip = RandomNumberGenerationsNum;
}

bool UMyReplaySubsystem::BP_IsReplayMode()
{
	return IsReplayMode();
}

bool UMyReplaySubsystem::IsReplayMode() const
{
	return IsValid(LoadedReplay);
}

bool UMyReplaySubsystem::IsProcessReplayMode() const
{
	return bProcessReplayMode && IsReplayMode();
}

int UMyReplaySubsystem::GetPlayersNum() const
{
	check(LoadedReplay);

	return LoadedReplay->PlayersNum;
}

const FPlayerSettings& UMyReplaySubsystem::GetPlayerSettings(const int& InPlayerId) const
{
	check(LoadedReplay);
	
	return LoadedReplay->GetPlayerSettings(InPlayerId);
}

bool UMyReplaySubsystem::IsRecording() const
{
	return SaveInProgressReplay != nullptr;
}

FString UMyReplaySubsystem::GetGameSeed() const
{
	check(LoadedReplay);

	return LoadedReplay->GameSeed;
}

void UMyReplaySubsystem::LoadReplayFromFile(const FString& ReplayName)
{
	LoadedReplay = Cast<USavedReplay>(UGameplayStatics::LoadGameFromSlot(FString::Printf(TEXT("%s"), *ReplayName), 0));
	check(LoadedReplay);
	LoadedReplay->UnpackDataForProcessing();

	auto MyGameInstance = UHelper::GetMyGameInstance(GetWorld());
	MyGameInstance->bReplayMode = true;
	MyGameInstance->ReplayName = ReplayName;
	MyGameInstance->bProcessReplayMode = false;
	
	UGameplayStatics::OpenLevel(GetWorld(), FName(LoadedReplay->LevelName));
}

void UMyReplaySubsystem::ProcessReplayFromFile(const FString& ReplayName)
{
	DELETE_LOG_FILE_IF_EXISTS("TemporaryClient.txt");
	
	LoadedReplay = Cast<USavedReplay>(UGameplayStatics::LoadGameFromSlot(FString::Printf(TEXT("%s"), *ReplayName), 0));
	check(LoadedReplay);
	LoadedReplay->UnpackDataForProcessing();

	auto MyGameInstance = UHelper::GetMyGameInstance(GetWorld());
	MyGameInstance->bReplayMode = true;
	MyGameInstance->ReplayName = ReplayName;
	MyGameInstance->bProcessReplayMode = true;
	
	UGameplayStatics::OpenLevel(GetWorld(), FName(LoadedReplay->LevelName));
}

bool UMyReplaySubsystem::ShouldForwardOneFrame()
{
	const bool bForward = bShouldForwardOneFrame;
	bShouldForwardOneFrame = false;

	return bForward;
}

bool UMyReplaySubsystem::ShouldRewindOneFrame()
{
	const bool bRewind = bShouldRewindOneFrame;
	bShouldRewindOneFrame = false;

	return bRewind;
}

bool UMyReplaySubsystem::IsReplayPaused() const
{
	return bReplayPaused;
}

bool UMyReplaySubsystem::IsReplayFinished(const int& InCurrentFrameId) const
{
	check(LoadedReplay);

	return LoadedReplay->GetReplayFramesNum() <= InCurrentFrameId;
}

int UMyReplaySubsystem::GetPlayerIndexToFollow() const
{
	return PlayerIndexToFollow;
}

void UMyReplaySubsystem::ToggleReplayPause()
{
	bReplayPaused = !bReplayPaused;
}

void UMyReplaySubsystem::ForwardOneFrame()
{
	bShouldForwardOneFrame = true;
}

void UMyReplaySubsystem::RewindOneFrame()
{
	bShouldRewindOneFrame = true;
}

void UMyReplaySubsystem::SetPlayerIndexToFollow(int InPlayerIndex)
{
	PlayerIndexToFollow = InPlayerIndex;
	Cast<AMyPlayerController>(GetWorld()->GetFirstPlayerController())->LocalPlayerBody = GetWorld()->GetSubsystem<UJoltSubsystem>()->GetPhysicsSystem()->GetBodyLockInterfaceNoLock().TryGetBody(BodyID(InPlayerIndex));
}

TArray<FInputFrame> UMyReplaySubsystem::GetCharactersInputs(const int& SimulationFrameId) const
{
	check(LoadedReplay);

	return LoadedReplay->GetInputFrames(SimulationFrameId);
}

TArray<FLevelData> UMyReplaySubsystem::GetLevelsSequence() const
{
	check(LoadedReplay);

	return LoadedReplay->LevelsSequence;
}

void UMyReplaySubsystem::OnConfirmedCharacterInputs(const int& InFrameId, const FCharacterInput& InCharacterInput)
{
	if (!IsRecording())
	{
		return;
	}
	
	SaveCharacterInput(InFrameId, InCharacterInput);
}

void UMyReplaySubsystem::SaveCharacterInput(const int& InputFrameId, const FCharacterInput& InCharacterInput)
{
	if (IsReplayMode() || !IsRecording())
	{
		return;
	}
	
	LOG_REPLAY(TEXT("Save character input frame %d"), InputFrameId);

	// There are places like UGameInputs that confirm inputs for a frame that the inputs were meant for.
	// For example if client 1 on frame 10, receives client 2 inputs for frame 5. Then UGameInputs will know nothing about frame 10, only frame 5
	// But for the replay system we need to know the actual local simulation current frame id AND also the frame id on which the inputs were actually generated on a remote client
	// We want to simulate the exact same conditions and receive client 2 for frame 5 only on frame 10. That's why we need to save both.
	const int SimulationFrameId = GetWorld()->GetSubsystem<UGameLoopSubsystem>()->GetCurrentFrameId();

	// We don't save PREDICTED inputs, because as long as we deliver the confirmed inputs on the same frame that they actually
	// arrived under real network conditions, then the predicted inputs will get generated automatically in the same way
	// Since playing the replay we will only get P2 inputs for FR5 on FR10, that means all the frames in between will be forced to use predicted inputs until FR10 arrives
	SaveInProgressReplay->AddCharacterInput(SimulationFrameId, InputFrameId, InCharacterInput);
}


void UMyReplaySubsystem::SaveReplayToFile()
{
	if (IsReplayMode() || !IsRecording())
	{
		return;
	}
	
	FString Prefix = "C_";
	if (CachedNetMode == NM_ListenServer)
	{
		Prefix = "H_";
	}

	LOG_REPLAY(TEXT("Save replay %s"), *FString::Printf(TEXT("%sReplay%d"), *Prefix, SavedReplaysNum));

	SaveInProgressReplay->PackageDataForSaving();
	const bool bSaveReplay = UGameplayStatics::SaveGameToSlot(SaveInProgressReplay, FString::Printf(TEXT("%sReplay%d"), *Prefix, SavedReplaysNum), 0);
	check(bSaveReplay);
}