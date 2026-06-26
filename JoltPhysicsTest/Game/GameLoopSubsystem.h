// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameStateManager.h"
#include "JoltPhysicsTest/Debug/DebugGameStateManager.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameLoopSubsystem.generated.h"

/** ! Be careful with events outside of pure rendering. The simulation has to be deterministic and delegate call order is not guaranteed. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFinishedCommandFrame, int, FinishedFrameId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRenderFrame);

UCLASS()
class JOLTPHYSICSTEST_API UGameLoopSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnFinishedCommandFrame OnFinishedCommandFrame;

	FOnRenderFrame OnRenderFrame;

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void PostInitialize() override;
	virtual void Deinitialize() override;
	virtual bool IsTickable() const override;
	virtual bool IsTickableInEditor() const override;
	virtual TStatId GetStatId() const override;

	// Used for stuff like level transitions, which do not support rollback
	// And in theory rollback shouldn't even be requested, this disabling is just to make it robust and ensure it doesn't rollback over those boundaries
	void DisableRollback();
	void EnableRollback();

	UFUNCTION(BlueprintCallable)
	void BP_PauseGame();

	UFUNCTION(BlueprintCallable)
	void BP_Unpausegame();
	
	void StartLoop();
	void Pause();
	void Unpause();

	virtual void Tick(float DeltaTime) override;

	void StepSimulation(const int& InFrameId);

	int GetSteppingFrameId() const;
	
	int GetCurrentFrameId() const;

	bool ValidateGameStatesMatch(const int& InFrameId, const int64 InGameStateHash);
	TMap<int, int64> GetGameStateHashes() const;
	
	void RebuildGameState(const int& InFrameId, const TArray<uint8>& BinaryGameState);
	TArray<uint8> GetBinaryGameState(const int& InFrameId);
private:
#ifdef MY_DEBUG_GAME_STATE
	void ProcessReplay();
#endif

	bool bRollbackEnabled = true;
	bool DidAtLeastOneStep = false;
	bool bReplayMode = false;
	bool bProcessReplayMode = false;
	UPROPERTY()
	TObjectPtr<UGameStateManager> GameStateManager = nullptr;

	UPROPERTY()
	TObjectPtr<UDebugGameStateManager> DebugGameStateManager = nullptr;
	bool bIsServer = true;

	bool bGameLoopStarted = false;
	bool bGamePaused = false;

	float AccumulatedDelta = 0.f;
	// This acts as a safeguard to not accumulate too much delta and then explode with too many steps per tick
	// Currently it's at max 10 steps per tick, which already is insane. We might want to lower it even further.
	const float MaxAccumulatedDelta = 0.16f;
	int CurrentFrameId = 0;
	// This differs from CurrentFrameId in that it always shows the frame id of the current StepSimulation().
	// CurrentFrameId shows the latest frame id, which if we are stepping the simulation in a rollback will not be the frame we are stepping.
	int SteppingFrameId = 0;
	const float FixedStepTime = 0.016f;
	float CurrentStepTime = 0.016f;
};
