// Fill out your copyright notice in the Description page of Project Settings.


#include "GameLoopSubsystem.h"

#include "SpawnerSubsystem.h"
#include "JoltPhysicsTest/FunctionLibrary/Helper.h"
#include "JoltPhysicsTest/GameInputs/GameInputs.h"
#include "JoltPhysicsTest/GameInputs/InputsSubsystem.h"
#include "JoltPhysicsTest/Jolt/JoltSubsystem.h"
#include "JoltPhysicsTest/Player/MyPlayerController.h"
#include "JoltPhysicsTest/Replay/MyReplaySubsystem.h"
#include "Kismet/GameplayStatics.h"

bool UGameLoopSubsystem::ShouldCreateSubsystem(UObject* Outer) const
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

void UGameLoopSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	GameStateManager = NewObject<UGameStateManager>(GetWorld());
	
#ifdef MY_DEBUG_GAME_STATE
	DebugGameStateManager = NewObject<UDebugGameStateManager>(GetWorld());
#endif

	if (GetWorld()->GetNetMode() == NM_Client)
	{
		bIsServer = false;
	}
}

void UGameLoopSubsystem::PostInitialize()
{
	Super::PostInitialize();

#ifdef MY_DEBUG_GAME_STATE
	// Here we are guaranteed that all subsystems have initialized.
	// It's important because this managers init relies on another subsystem having already initialized
	DebugGameStateManager->Init(GameStateManager);
#endif
}

void UGameLoopSubsystem::Deinitialize()
{
	Super::Deinitialize();

#ifdef MY_DEBUG_GAME_STATE
	if (DebugGameStateManager)
	{
		DebugGameStateManager->DumpToLogs(bIsServer);
	}
#endif
}

bool UGameLoopSubsystem::IsTickable() const
{
	return bGameLoopStarted && !bGamePaused;
}

bool UGameLoopSubsystem::IsTickableInEditor() const
{
	return false;
}

TStatId UGameLoopSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UGameLoopSubsystem, STATGROUP_Tickables);
}

void UGameLoopSubsystem::DisableRollback()
{
	GetWorld()->GetSubsystem<UInputsSubsystem>()->GameInputs->MarkDesyncAsFixed();
	bRollbackEnabled = false;
}

void UGameLoopSubsystem::EnableRollback()
{
	GetWorld()->GetSubsystem<UInputsSubsystem>()->GameInputs->MarkDesyncAsFixed();
	bRollbackEnabled = true;
}

void UGameLoopSubsystem::BP_PauseGame()
{
	Pause();
}

void UGameLoopSubsystem::BP_Unpausegame()
{
	Unpause();
}

void UGameLoopSubsystem::StartLoop()
{
	bGameLoopStarted = true;
	auto MyReplaySubsystem = GetWorld()->GetSubsystem<UMyReplaySubsystem>();
	bReplayMode = MyReplaySubsystem->IsReplayMode();
	// For processing replays we want it to work as close as possible to the real thing, so we DON'T want to turn off clean up
	if (bReplayMode && !MyReplaySubsystem->IsProcessReplayMode())
	{
		GetWorld()->GetSubsystem<UInputsSubsystem>()->GameInputs->TurnOffCleanUp();
	}
}

void UGameLoopSubsystem::Pause()
{
	bGamePaused = true;
}

void UGameLoopSubsystem::Unpause()
{
	bGamePaused = false;
}

void UGameLoopSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!IsTickable())
	{
		return;
	}

	auto MyReplaySubsystem = GetWorld()->GetSubsystem<UMyReplaySubsystem>();

#ifdef MY_DEBUG_GAME_STATE
	if (MyReplaySubsystem->IsProcessReplayMode())
	{
		ProcessReplay();
		bGameLoopStarted = false;
		return;
	}
#endif

	const auto GameInputs = GetWorld()->GetSubsystem<UInputsSubsystem>()->GameInputs;
	auto LocalPlayerController = Cast<AMyPlayerController>(UHelper::GetLocalPlayerController(GetWorld()));

	if (bReplayMode)
	{
		const bool bReplayFinished = MyReplaySubsystem->IsReplayFinished(CurrentFrameId);
		if (bReplayFinished && !MyReplaySubsystem->IsReplayPaused())
		{
			MyReplaySubsystem->ToggleReplayPause();
		}
		
		if (MyReplaySubsystem->IsReplayPaused())
		{
			// TODO: This kind of simple rewind will not work with desyncs I don't think
			// TODO: But for now it's good enough, just something to keep in mind if we would want to ship the replay system
			if (MyReplaySubsystem->ShouldRewindOneFrame() && CurrentFrameId > 1)
			{
				GameStateManager->Load(CurrentFrameId - 2);
				CurrentFrameId = CurrentFrameId - 2;
				AccumulatedDelta = FixedStepTime - DeltaTime;
			}
			else if (MyReplaySubsystem->ShouldForwardOneFrame() && !bReplayFinished)
			{
				AccumulatedDelta = FixedStepTime - DeltaTime;
			}
			else
			{
				return;
			}
		}
	}
	
	AccumulatedDelta += DeltaTime;
	if (AccumulatedDelta > MaxAccumulatedDelta)
	{
		AccumulatedDelta = MaxAccumulatedDelta;
	}

	// TODO: Don't cap rendering framerate. Instead save previous frame rendering data and interpolate between previous => current
	if (DidAtLeastOneStep)
	{
		OnRenderFrame.Broadcast();
		DidAtLeastOneStep = false;
	}

	while (AccumulatedDelta >= CurrentStepTime)
	{
		DidAtLeastOneStep = true;
		
		if (!bReplayMode)
		{
			if (GameInputs->HasNoConfirmedInputsForEntireRollbackWindow(CurrentFrameId))
			{
				LOG(TEXT("[GameLoop] SKIPPING ON FRAME %d. No confirmed inputs for entire window."), CurrentFrameId);

				// We keep retrying to send our inputs, just in case some packets got lost. Otherwise we would get stuck permanently.
				if (bIsServer)
				{
					LocalPlayerController->SendInputsToClients(CurrentFrameId);
				}
				else
				{
					LocalPlayerController->SendInputsToServer(CurrentFrameId);
				}

				AccumulatedDelta -= DeltaTime;

				MyReplaySubsystem->IncreaseFramesSkippedDueToNoInputsForRollbackWindow(CurrentFrameId);

				continue;
			}
		}
		
		//LOG_TEMPORARY(TEXT("[GameLoop] Running frame %d"), CurrentFrameId);
		
		LocalPlayerController->HandleLocalInputs(CurrentFrameId);
		LocalPlayerController->SendInputsToServer(CurrentFrameId);
		LocalPlayerController->SendInputsToClients(CurrentFrameId);

#ifdef MY_DEBUG_ROLLBACK
		GameInputs->DebugRollback(CurrentFrameId);
#endif
		
		if (bRollbackEnabled && GameInputs->IsDesyncDetected())
		{
#ifdef MY_DEBUG_VALIDATE_ROLLBACK_DETERMINISM
			// This validation only makes sense in single player
			// Multiplayer is EXPECTED to change game state through rollbacks
			StateRecorderImpl BeforeRollbackState;
			if (GameInputs->GetCharactersNum() == 1)
			{
				GameStateManager->SaveToStream(BeforeRollbackState);
			}
#endif

			int RollbackFrameId = GameInputs->GetFrameIdToRollbackTo();
			//LOG_TEMPORARY(TEXT("[Frame %d][GameLoop] Starting rollback to frame %d"), CurrentFrameId, RollbackFrameId);
			GameStateManager->Load(RollbackFrameId);
			const int LoadedFrameId = RollbackFrameId;
			while (RollbackFrameId < CurrentFrameId)
			{
				// If we literally just loaded this frame, then there's no point in recording it again immediately
				if (RollbackFrameId != LoadedFrameId)
				{
					GameStateManager->Save(RollbackFrameId);
				}
				
				StepSimulation(RollbackFrameId);
				RollbackFrameId++;
			}
			GameInputs->MarkDesyncAsFixed();

#ifdef MY_DEBUG_VALIDATE_ROLLBACK_DETERMINISM
			if (!bReplayMode && GameInputs->GetCharactersNum() == 1)
			{
				GameStateManager->ValidateState(BeforeRollbackState);
			}
#endif
		}
		
		GameStateManager->Save(CurrentFrameId);
		/*if (CurrentFrameId == 10)
		{
			GameStateManager->SaveCheckpoint(CurrentFrameId);
		}*/
		
		MyReplaySubsystem->SaveStepTime(CurrentFrameId, CurrentStepTime);
		MyReplaySubsystem->SavePredictedAndConfirmedFrames(CurrentFrameId, GameInputs->GetPredictedFramesNum(), GameInputs->GetConfirmedFramesNum());
		MyReplaySubsystem->SaveRoundTripTime(CurrentFrameId, LocalPlayerController->GetPing());

		StepSimulation(CurrentFrameId);

#ifdef MY_DEBUG_VALIDATE_DETERMINISM
		StateRecorderImpl AfterStepState;
		GameStateManager->SaveToStream(AfterStepState);

		GameStateManager->Load(CurrentFrameId);
		
		StepSimulation(CurrentFrameId);
		
		GameStateManager->ValidateState(AfterStepState);
#endif

		const int CleanedFrameId = GameInputs->GetFrameIdSafeToCleanUp(CurrentFrameId);
		GameInputs->CleanupFrame(CleanedFrameId);
		GameStateManager->Remove(CleanedFrameId);
		LocalPlayerController->CleanupFrame(CleanedFrameId);
		
		AccumulatedDelta -= CurrentStepTime;

		OnFinishedCommandFrame.Broadcast(CurrentFrameId);
		CurrentFrameId++;

		CurrentStepTime = FixedStepTime;
		if (!bReplayMode && CurrentFrameId > GameInputs->GetRollbackWindow())
		{
			// 0.f = No predicted frames; That is ideal, it means we might as well be playing offline.
			// 1.f = Too many predicted frames; We want to slow down, so we have more time to receive confirmed frames from the other player.
			const float PredictedInputsPercentage = FMath::Clamp(
				static_cast<float>(FMath::Clamp(GameInputs->GetPredictedFramesNum(), 0, GameInputs->GetRollbackWindow()))
					/
				static_cast<float>(GameInputs->GetRollbackWindow()),
				0.f,
				1.f
			);

			// We will adjust each step by a maximum of 1ms
			constexpr float MaxStepAdjustment = 0.001f;
		
			CurrentStepTime = FixedStepTime + PredictedInputsPercentage * MaxStepAdjustment;
		}
	}
}

void UGameLoopSubsystem::StepSimulation(const int& InFrameId)
{
	LOG(TEXT("[GameLoop] Step simulation for frame %d"), InFrameId);
	SteppingFrameId = InFrameId;
	auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();
	
#ifdef MY_DEBUG_LOG_JOLT_CALLS
	JoltSubsystem->PrepareToLogFrame(InFrameId);
#endif

	auto LocalPlayerController = UHelper::GetLocalPlayerController(GetWorld());
	
	LocalPlayerController->StepCharactersSimulation(InFrameId, FixedStepTime);

	if (LocalPlayerController->bPhysicsSimulationPaused)
	{
		return;
	}
	
	GetWorld()->GetSubsystem<USpawnerSubsystem>()->StepSimulation();
	JoltSubsystem->StepSimulation(FixedStepTime);

	LocalPlayerController->ProcessCollisions(InFrameId);
}

int UGameLoopSubsystem::GetSteppingFrameId() const
{
	return SteppingFrameId;
}

int UGameLoopSubsystem::GetCurrentFrameId() const
{
	return CurrentFrameId;
}

bool UGameLoopSubsystem::ValidateGameStatesMatch(const int& InFrameId, const int64 InGameStateHash)
{
	return GameStateManager->ValidateGameStatesMatch(InFrameId, InGameStateHash);
}

TMap<int, int64> UGameLoopSubsystem::GetGameStateHashes() const
{
	return GameStateManager->GetGameStateHashes();
}

void UGameLoopSubsystem::RebuildGameState(const int& InFrameId, const TArray<uint8>& BinaryGameState)
{
	GameStateManager->RebuildGameState(InFrameId, BinaryGameState);
	auto InputsSubsystem = GetWorld()->GetSubsystem<UInputsSubsystem>();
	InputsSubsystem->GameInputs->OnRebuiltGameStateManually(InFrameId);
	InputsSubsystem->GameInputs->MarkDesyncAsFixed();
	
	// The frame we want to start simulating on is the same frame we just rebuilt
	// It goes: save game state -> simulate physics -> increase current frame id
	// So it will save the same thing we just rebuilt again, then simulate next step
	CurrentFrameId = InFrameId;
}

TArray<uint8> UGameLoopSubsystem::GetBinaryGameState(const int& InFrameId)
{
	return GameStateManager->GetBinaryGameState(InFrameId);
}

#ifdef MY_DEBUG_GAME_STATE
void UGameLoopSubsystem::ProcessReplay()
{
	auto MyReplaySubsystem = GetWorld()->GetSubsystem<UMyReplaySubsystem>();
	const auto GameInputs = GetWorld()->GetSubsystem<UInputsSubsystem>()->GameInputs;
	auto LocalPlayerController = Cast<AMyPlayerController>(UHelper::GetLocalPlayerController(GetWorld()));

	while (!MyReplaySubsystem->IsReplayFinished(CurrentFrameId))
	{
		//LOG_TEMPORARY(TEXT("[GameLoop] Running frame %d"), CurrentFrameId);
		
		LocalPlayerController->HandleLocalInputs(CurrentFrameId);
		//LocalPlayerController->SendInputsToServer(CurrentFrameId);
		//LocalPlayerController->SendInputsToClients(CurrentFrameId);

#ifdef MY_DEBUG_ROLLBACK
		GameInputs->DebugRollback(CurrentFrameId);
#endif
		
		if (bRollbackEnabled && GameInputs->IsDesyncDetected())
		{
#ifdef MY_DEBUG_VALIDATE_ROLLBACK_DETERMINISM
			// This validation only makes sense in single player
			// Multiplayer is EXPECTED to change game state through rollbacks
			StateRecorderImpl BeforeRollbackState;
			if (GameInputs->GetCharactersNum() == 1)
			{
				GameStateManager->SaveToStream(BeforeRollbackState);
			}
#endif

			int RollbackFrameId = GameInputs->GetFrameIdToRollbackTo();
			//LOG_TEMPORARY(TEXT("[Frame %d][GameLoop] Starting rollback to frame %d"), CurrentFrameId, RollbackFrameId);
			GameStateManager->Load(RollbackFrameId);
			const int LoadedFrameId = RollbackFrameId;
			while (RollbackFrameId < CurrentFrameId)
			{
				// If we literally just loaded this frame, then there's no point in recording it again immediately
				if (RollbackFrameId != LoadedFrameId)
				{
					GameStateManager->Save(RollbackFrameId);
				}
				
				StepSimulation(RollbackFrameId);
				RollbackFrameId++;
			}
			GameInputs->MarkDesyncAsFixed();

#ifdef MY_DEBUG_VALIDATE_ROLLBACK_DETERMINISM
			if (!bReplayMode && GameInputs->GetCharactersNum() == 1)
			{
				GameStateManager->ValidateState(BeforeRollbackState);
			}
#endif
		}
		
		GameStateManager->Save(CurrentFrameId);
		
		StepSimulation(CurrentFrameId);

#ifdef MY_DEBUG_VALIDATE_DETERMINISM
		StateRecorderImpl AfterStepState;
		GameStateManager->SaveToStream(AfterStepState);

		GameStateManager->Load(CurrentFrameId);
		
		StepSimulation(CurrentFrameId);
		
		GameStateManager->ValidateState(AfterStepState);
#endif

		const int CleanedFrameId = GameInputs->GetFrameIdSafeToCleanUp(CurrentFrameId);
		GameInputs->CleanupFrame(CleanedFrameId);
		GameStateManager->Remove(CleanedFrameId);
		LocalPlayerController->CleanupFrame(CleanedFrameId);
		
		//OnFinishedCommandFrame.Broadcast(CurrentFrameId);
		CurrentFrameId++;
	}

	UKismetSystemLibrary::QuitGame(GetWorld(), nullptr, EQuitPreference::Quit, true);
}
#endif
