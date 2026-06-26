// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SavedReplay.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/EngineBaseTypes.h"
#include "JoltPhysicsTest/Player/MyPlayerController.h"
#include "MyReplaySubsystem.generated.h"

UCLASS()
class JOLTPHYSICSTEST_API UMyReplaySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void PostInitialize() override;
	
	virtual void Deinitialize() override;

	void RecordReplay(const uint32& PlayedGamesNum, FString LevelName);

	void IncreaseFramesSkippedDueToNoInputsForRollbackWindow(const int& InSimulationFrameId);

	void SavePredictedAndConfirmedFrames(const int& InSimulationFrameId, const int& InPredictedFramesNum, const int& InConfirmedFramesNum);
	
	void SaveStepTime(const int& InSimulationFrameId, const float& InStepTime);
	void SaveRoundTripTime(const int& InSimulationFrameId, const float& InRoundTripTime);

	bool IsServer() const;
	
	UFUNCTION(BlueprintPure, Category = "Replay")
	float GetRoundTripTime(int InSimulationFrameId);
	
	UFUNCTION(BlueprintPure, Category = "Replay")
	int GetPredictedFramesNum(int InSimulationFrameId);

	UFUNCTION(BlueprintPure, Category = "Replay")
	int GetConfirmedFramesNum(int InSimulationFrameId);
	
	UFUNCTION(BlueprintPure, Category = "Replay")
	float GetCurrentStepTime(int InSimulationFrameId);

	UFUNCTION(BlueprintPure, Category = "Replay")
	int GetTotalFramesSkippedDueToNoInputsForRollbackWindow();

	UFUNCTION(BlueprintPure, Category = "Replay")
	int GetFramesSkippedDueToNoInputsForRollbackWindow(int InSimulationFrameId);

	int GetInitialRNGSkip();

	UFUNCTION(BlueprintPure, Category = "Replay")
	int GetInputDelayFramesNum();
	
	UFUNCTION(BlueprintPure, Category = "Replay")
	int GetMaxRollbackFramesNum();

	UFUNCTION(BlueprintPure, Category = "Replay")
	bool IsServer();

	void SaveIsServer(bool bIsServer);
	void SavePlayersNum(const int& InPlayersNum);
	
	void SavePlayerSettings(const int& InPlayerId, const FPlayerSettings& InPlayerSettings);

	void SaveInputDelayFramesNum(const int& InputDelayFramesNum);
	void SaveMaxRollbackFramesNum(const int& MaxRollbackFramesNum);

	void SaveGameSeed(const FString& InSeed);

	void SaveLevelsSequence(const TArray<FLevelData>& InLevelsSequence);

	void SaveInitialRNGSkip(int RandomNumberGenerationsNum);

	UFUNCTION(BlueprintPure, Category = "Replay", Meta = (DisplayName = "Is Replay Mode"))
	bool BP_IsReplayMode();
	
	// Checks if there's a replay loaded
	bool IsReplayMode() const;
	bool IsProcessReplayMode() const;
	int GetPlayersNum() const;
	const FPlayerSettings& GetPlayerSettings(const int& InPlayerId) const;

	bool IsRecording() const;

	FString GetGameSeed() const;

	UFUNCTION(BlueprintCallable, Category = "Replay")
	void LoadReplayFromFile(const FString& ReplayName);

	UFUNCTION(BlueprintCallable, Category = "Replay")
	void ProcessReplayFromFile(const FString& ReplayName);

	bool ShouldForwardOneFrame();
	bool ShouldRewindOneFrame();

	bool IsReplayPaused() const;
	bool IsReplayFinished(const int& InCurrentFrameId) const;

	int GetPlayerIndexToFollow() const;

	UFUNCTION(BlueprintCallable)
	void ToggleReplayPause();

	UFUNCTION(BlueprintCallable)
	void ForwardOneFrame();

	UFUNCTION(BlueprintCallable)
	void RewindOneFrame();

	UFUNCTION(BlueprintCallable)
	void SetPlayerIndexToFollow(int InPlayerIndex);

	TArray<FInputFrame> GetCharactersInputs(const int& SimulationFrameId) const;
	
	TArray<FLevelData> GetLevelsSequence() const;

private:
	bool bProcessReplayMode = false;
	ENetMode CachedNetMode = NM_Standalone;
	uint32 SavedReplaysNum = 0;
	bool bReplayPaused = true;
	bool bShouldForwardOneFrame = false;
	bool bShouldRewindOneFrame = false;
	int PlayerIndexToFollow = 0;
	
	UFUNCTION()
	void OnConfirmedCharacterInputs(const int& InFrameId, const FCharacterInput& InCharacterInput);

	void SaveCharacterInput(const int& InputFrameId, const FCharacterInput& InCharacterInput);
	void SaveReplayToFile();
	
	UPROPERTY()
	USavedReplay* LoadedReplay = nullptr;

	UPROPERTY()
	USavedReplay* SaveInProgressReplay = nullptr;
};
