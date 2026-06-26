// Fill out your copyright notice in the Description page of Project Settings.


#include "MyPlayerController.h"

#include "EnhancedInputComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "PlayerBodyVFX.h"
#include "PlayerRenderer.h"
#include "PlayerSpawn.h"
#include "TwoBoneIK.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/PostProcessVolume.h"
#include "JoltPhysicsTest/Components/JoltBoxComponent.h"
#include "JoltPhysicsTest/Components/Constraints/JoltConstraintComponent.h"
#include "JoltPhysicsTest/Data/GeneralData.h"
#include "JoltPhysicsTest/Debug/CustomLogger.h"
#include "JoltPhysicsTest/FunctionLibrary/Helper.h"
#include "JoltPhysicsTest/Game/GameLoopSubsystem.h"
#include "JoltPhysicsTest/Game/SpawnerSubsystem.h"
#include "JoltPhysicsTest/GameInputs/InputsSubsystem.h"
#include "JoltPhysicsTest/Jolt/JoltContainerActor.h"
#include "JoltPhysicsTest/Jolt/JoltConverter.h"
#include "JoltPhysicsTest/Jolt/JoltSubsystem.h"
#include "JoltPhysicsTest/Rendering/LightSource.h"
#include "Kismet/GameplayStatics.h"
#include "JoltPhysicsTest/Game/MyGameInstance.h"
#include "JoltPhysicsTest/Data/AssetManagerSubsystem.h"
#include "JoltPhysicsTest/Game/MyGameState.h"
#include "JoltPhysicsTest/JoltBodies/ExplosiveJoltBody.h"
#include "JoltPhysicsTest/Replay/MyReplaySubsystem.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/LevelScriptActor.h"
#include "Engine/StaticMeshActor.h"
#include "JoltPhysicsTest/Components/AnimationComponent.h"
#include "JoltPhysicsTest/Components/EyesAnimationComponent.h"
#include "JoltPhysicsTest/Components/JoltDestructibleCompoundComponent.h"
#include "JoltPhysicsTest/Components/JoltStaticCompoundComponent.h"
#include "JoltPhysicsTest/Components/MultipleMaterialStaticMeshComponent.h"
#include "JoltPhysicsTest/Components/Constraints/JoltHingeConstraint.h"
#include "JoltPhysicsTest/Components/Constraints/JoltSliderConstraint.h"
#include "JoltPhysicsTest/Game/GameBackground.h"
#include "JoltPhysicsTest/Game/GameCameraLimits.h"
#include "JoltPhysicsTest/Jolt/DestructibleJoltBodyActor.h"
#include "JoltPhysicsTest/JoltBodies/BananaProjectile.h"
#include "JoltPhysicsTest/JoltBodies/Boomerang.h"
#include "JoltPhysicsTest/JoltBodies/Crown.h"
#include "JoltPhysicsTest/JoltBodies/ExplosiveBarrel.h"
#include "JoltPhysicsTest/JoltBodies/Flag.h"
#include "JoltPhysicsTest/JoltBodies/FlagBase.h"
#include "JoltPhysicsTest/JoltBodies/GravityBomb.h"
#include "JoltPhysicsTest/JoltBodies/StickyBomb.h"
#include "JoltPhysicsTest/JoltBodies/HomingMissile.h"
#include "JoltPhysicsTest/JoltBodies/Lootbox.h"
#include "JoltPhysicsTest/JoltBodies/MineSweeper.h"
#include "JoltPhysicsTest/JoltBodies/PlayerSensor.h"
#include "JoltPhysicsTest/JoltBodies/Checkpoint.h"
#include "JoltPhysicsTest/JoltBodies/Portal.h"
#include "JoltPhysicsTest/JoltBodies/SentryTurret.h"
#include "JoltPhysicsTest/JoltBodies/Sword.h"
#include "JoltPhysicsTest/JoltBodies/OrbitalSphere.h"
#include "JoltPhysicsTest/Sound/SoundSubsystem.h"
#include "JoltPhysicsTest/VFX/PlayAnimationOnceActor.h"
#include "SteamIntegrationKit/Functions/Friends/SIK_FriendsLibrary.h"
#include "SteamIntegrationKit/Functions/User/SIK_UserLibrary.h"

void AMyPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void AMyPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void AMyPlayerController::ServerUpdatePlayerSettings_Implementation(const FPlayerSettings& InPlayerSettings)
{
	auto ClientPlayerState = GetMyPlayerState();
	ClientPlayerState->MulticastSetPlayerSettings(InPlayerSettings);

	GetWorld()->GetSubsystem<UMyReplaySubsystem>()->SavePlayerSettings(ClientPlayerState->GetMyPlayerId(), InPlayerSettings);
	
	auto LocalPlayerController = UHelper::GetLocalPlayerController(GetWorld());
	
	LocalPlayerController->GetPlayerNetworkData().PlayersSettingsReplicated++;
}

void AMyPlayerController::ServerSendInputs_Implementation(const int& CharacterId, const TArray<FCommandFrame>& ConfirmedFrames)
{
	if (bSyncingStateInProgress)
	{
		return;
	}
	
	auto InputsSubsystem = GetWorld()->GetSubsystem<UInputsSubsystem>();

	// It's important for us to process the frames in ascending order.
	// Because both client and server always send ALL confirmed frames they have within the rollback window
	// It means we are guaranteed to have confirmed frames 1 and 2, if we have confirmed frame 3. Because when we got the package for frame 3, we also received frame 1 and 2.
	// But sometimes some frames get skipped because of incorrect order here.
	auto SortedConfirmedFrames = ConfirmedFrames;
	SortedConfirmedFrames.Sort([](const FCommandFrame& A, const FCommandFrame& B)
	{
		return A.FrameId < B.FrameId;
	});
	
	for (auto& ConfirmedFrame : SortedConfirmedFrames)
	{
		for (auto& CharacterInput : ConfirmedFrame.CharactersInputs)
		{
			ConfirmedFrame.SortCharactersInputs();
			
			// On the server we are receiving input here from all remote players, could be 1, could be 10
			// We want to add them all to the same GameInputs instance. Which is why it's a singleton.
			InputsSubsystem->GameInputs->AddConfirmedCharacterInput(ConfirmedFrame.FrameId, CharacterInput);
		}
	}
}

void AMyPlayerController::ClientSendInputs_Implementation(const int& ServerFrameId, const TArray<FCommandFrame>& ConfirmedFrames)
{
	if (bSyncingStateInProgress)
	{
		return;
	}
	
	auto InputsSubsystem = GetWorld()->GetSubsystem<UInputsSubsystem>();
	LOG(TEXT("[MyPlayerController] Received confirmed inputs from server frame %d"), ServerFrameId);

	// It's important for us to process the frames in ascending order.
	// Because both client and server always send ALL confirmed frames they have within the rollback window
	// It means we are guaranteed to have confirmed frames 1 and 2, if we have confirmed frame 3. Because when we got the package for frame 3, we also received frame 1 and 2.
	// But sometimes some frames get skipped because of incorrect order here.
	auto SortedConfirmedFrames = ConfirmedFrames;
	SortedConfirmedFrames.Sort([](const FCommandFrame& A, const FCommandFrame& B)
	{
		return A.FrameId < B.FrameId;
	});
	
	for (auto& ConfirmedFrame : SortedConfirmedFrames)
	{
		ConfirmedFrame.SortCharactersInputs();
		for (auto& CharacterInput : ConfirmedFrame.CharactersInputs)
		{
			InputsSubsystem->GameInputs->AddConfirmedCharacterInput(ConfirmedFrame.FrameId, CharacterInput);
		}
	}
}

void AMyPlayerController::ServerSendGameStateHash_Implementation(const int& FrameId, const int64& GameStateHash)
{
	if (bSyncingStateInProgress)
	{
		return;
	}
	
	//LOG_NETWORKING(TEXT("Received game state hash from client for frame %d, hash: %lld"), FrameId, GameStateHash);
	if (!GetWorld()->GetSubsystem<UGameLoopSubsystem>()->ValidateGameStatesMatch(FrameId, GameStateHash))
	{
		LOG_NETWORKING(TEXT("[Frame %d] Desync detected: %lld"), FrameId, GameStateHash);
		// This message arrives from a client, so it will be called on their player controller
		// And we need to call it on the hosts player controller
		UHelper::GetLocalPlayerController(GetWorld())->DesyncDetected(FrameId);
	}
	else
	{
		auto LocalPlayerController = UHelper::GetLocalPlayerController(GetWorld());
		if (LocalPlayerController->GetDesyncDetectedFrameId() < FrameId)
		{
			LocalPlayerController->DesyncFixed(FrameId);
		}
	}
}

void AMyPlayerController::ServerUpdatePlayerNetworkData_Implementation(const FPlayerNetworkData& InPlayerNetworkData)
{
	PlayerNetworkData = InPlayerNetworkData;
}

void AMyPlayerController::ClientInitializeController_Implementation()
{
	LocalControllerInit();
}

void AMyPlayerController::ClientSyncGameStateWithServer_Implementation(const int& FrameId,
	const TArray<uint8>& BinaryGameState, const FSpawnerSubsystemState& SpawnerSubsystemState)
{
	GetWorld()->GetSubsystem<UGameLoopSubsystem>()->RebuildGameState(FrameId, BinaryGameState);
	GetWorld()->GetSubsystem<USpawnerSubsystem>()->RebuildSpawnerState(SpawnerSubsystemState);
	ServerConfirmStateSynced();
}

void AMyPlayerController::ServerSyncGameStateWithServer_Implementation(const int& FrameId,
	const TArray<uint8>& BinaryGameState, const FSpawnerSubsystemState& SpawnerSubsystemState)
{
	GetWorld()->GetSubsystem<USpawnerSubsystem>()->RebuildSpawnerState(SpawnerSubsystemState);
	GetWorld()->GetSubsystem<UGameLoopSubsystem>()->RebuildGameState(FrameId, BinaryGameState);
	ClientConfirmStateSynced();
}

void AMyPlayerController::ClientConfirmStateSynced_Implementation()
{
	GetWorld()->GetSubsystem<UGameLoopSubsystem>()->Unpause();
	bSyncingStateInProgress = false;
}

void AMyPlayerController::ServerConfirmStateSynced_Implementation()
{
	GetWorld()->GetSubsystem<UGameLoopSubsystem>()->Unpause();
	bSyncingStateInProgress = false;
}

void AMyPlayerController::RequestClientToSyncState()
{
	auto GameLoopSubsystem = GetWorld()->GetSubsystem<UGameLoopSubsystem>();
	auto SpawnerSubsystem = GetWorld()->GetSubsystem<USpawnerSubsystem>();
	const int CurrentFrameId = GameLoopSubsystem->GetCurrentFrameId() - 1;

	GetWorld()->GetSubsystem<UInputsSubsystem>()->GameInputs->ClearPredictedAndConfirmedFramesAfterFrameId(CurrentFrameId);
	GameLoopSubsystem->Pause();
	bSyncingStateInProgress = true;

	//ClientSyncGameStateWithServer(CurrentFrameId, GameLoopSubsystem->GetBinaryGameState(CurrentFrameId), SpawnerSubsystem->GetSpawnerState());
	
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		AMyPlayerController* MyPlayerController = Cast<AMyPlayerController>(Iterator->Get());
		if (MyPlayerController && MyPlayerController->GetRemoteRole() == ROLE_AutonomousProxy)
		{
			//size_t SpawnerStateSize = sizeof(SpawnerSubsystem->GetSpawnerState());
			return MyPlayerController->ClientSyncGameStateWithServer(CurrentFrameId, GameLoopSubsystem->GetBinaryGameState(CurrentFrameId), SpawnerSubsystem->GetSpawnerState());
		}
	}
}

void AMyPlayerController::RequestServerToSyncState()
{
	auto GameLoopSubsystem = GetWorld()->GetSubsystem<UGameLoopSubsystem>();
	auto SpawnerSubsystem = GetWorld()->GetSubsystem<USpawnerSubsystem>();
	const int CurrentFrameId = GameLoopSubsystem->GetCurrentFrameId() - 1;

	GetWorld()->GetSubsystem<UInputsSubsystem>()->GameInputs->ClearPredictedAndConfirmedFramesAfterFrameId(CurrentFrameId);
	GameLoopSubsystem->Pause();
	bSyncingStateInProgress = true;

	ServerSyncGameStateWithServer(CurrentFrameId, GameLoopSubsystem->GetBinaryGameState(CurrentFrameId), SpawnerSubsystem->GetSpawnerState());
}

void AMyPlayerController::DesyncOnPurpose()
{
	auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();
	const int MyPlayerId = GetMyPlayerState()->GetMyPlayerId();

	const Vec3 NewPosition = JoltSubsystem->GetBodyInterface()->GetPosition(BodyID(MyPlayerId)) + Vec3(10, 0, 0);
	JoltSubsystem->GetBodyInterface()->SetPosition(BodyID(MyPlayerId), NewPosition, EActivation::Activate);
}


void AMyPlayerController::IncreaseMouseSensitivity()
{
	auto CurrentGameSave = GetCurrentGameSave();
	
	CurrentGameSave->GameSettings.MouseSensitivity = FMath::Clamp(CurrentGameSave->GameSettings.MouseSensitivity + 0.1f, MinMouseSensitivity, MaxMouseSensitivity);

	OnMouseSensitivityUpdated.Broadcast(CurrentGameSave->GameSettings.MouseSensitivity, GetMouseSensitivityPercentage());
}

void AMyPlayerController::DecreaseMouseSensitivity()
{
	auto CurrentGameSave = GetCurrentGameSave();

	CurrentGameSave->GameSettings.MouseSensitivity = FMath::Clamp(CurrentGameSave->GameSettings.MouseSensitivity - 0.1f, MinMouseSensitivity, MaxMouseSensitivity);

	OnMouseSensitivityUpdated.Broadcast(CurrentGameSave->GameSettings.MouseSensitivity, GetMouseSensitivityPercentage());
}

void AMyPlayerController::SetMouseSensitivty(float InPercentage)
{
	auto CurrentGameSave = GetCurrentGameSave();

	const float Percentage = FMath::Clamp(InPercentage, 0.f, 1.f);

	CurrentGameSave->GameSettings.MouseSensitivity = FMath::Lerp(MinMouseSensitivity, MaxMouseSensitivity, Percentage);

	OnMouseSensitivityUpdated.Broadcast(CurrentGameSave->GameSettings.MouseSensitivity, GetMouseSensitivityPercentage());
}

float AMyPlayerController::GetMouseSensitivity()
{
	return FMath::Clamp(GetCurrentGameSave()->GameSettings.MouseSensitivity, MinMouseSensitivity, MaxMouseSensitivity); 
}

float AMyPlayerController::GetMouseSensitivityPercentage()
{
	const float MouseSensitivity = GetCurrentGameSave()->GameSettings.MouseSensitivity;

	return (MouseSensitivity - MinMouseSensitivity) / (MaxMouseSensitivity - MinMouseSensitivity);
}

EGameMode AMyPlayerController::GetCurrentGameModeType()
{
	return GameModeSettings.Mode;
}

FString AMyPlayerController::GetLeaderboardName()
{
	checkf(CurrentLevelData.MapData, TEXT("CurrentLevelData has no MapData assigned"));
	const FString LevelName = CurrentLevelData.MapData->MapName.Replace(TEXT(" "), TEXT(""));
	const FString GameModeName = UEnum::GetValueAsString(GetCurrentGameModeType()).RightChop(11); // strip "EGameMode::"
	return LevelName + GameModeName;
}

bool AMyPlayerController::IsCurrentGameModeType(EGameMode CheckGameModeType)
{
	return GetCurrentGameModeType() == CheckGameModeType;
}

void AMyPlayerController::DisableLocalPlayerInputs()
{
	bLocalPlayerInputDisabled = true;
}

void AMyPlayerController::EnableLocalPlayerInputs()
{
	bLocalPlayerInputDisabled = false;
}

AMyPlayerState* AMyPlayerController::GetMyPlayerState()
{
	return GetPlayerState<AMyPlayerState>();
}

AMyGameState* AMyPlayerController::GetMyGameState()
{
	return UHelper::GetMyGameState(GetWorld());
}

AMyPlayerState* AMyPlayerController::GetMyPlayerState(const int& InPlayerId)
{
	for (TObjectPtr<APlayerState> CurrentPlayerState : GetMyGameState()->PlayerArray)
	{
		if (auto MyPlayerState = Cast<AMyPlayerState>(CurrentPlayerState))
		{
			if (MyPlayerState->GetMyPlayerId() == InPlayerId)
			{
				return MyPlayerState;
			}
		}
	}

	checkNoEntry();

	return nullptr;
}

float AMyPlayerController::GetPing() const
{
	if (GetLocalRole() == ROLE_Authority)
	{
		// For server we just get one of the clients controllers RTT. Cause servers controller can't have one.
		// It can't have an RTT with itself, because it only exists on the server.
		for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
		{
			AMyPlayerController* MyPlayerController = Cast<AMyPlayerController>(Iterator->Get());
			if (MyPlayerController && MyPlayerController->GetRemoteRole() == ROLE_AutonomousProxy)
			{
				return MyPlayerController->GetRoundTripTime();
			}
		}
	}

	if (GetLocalRole() == ROLE_AutonomousProxy)
	{
		return GetRoundTripTime();
	}

	return 0.f;
}

float AMyPlayerController::BP_GetPing()
{
	return GetPing();
}

TArray<FPlayerCharacterRenderData> AMyPlayerController::GetCharactersRenderData() const
{
	TArray<FPlayerCharacterRenderData> RenderData;
	for (const auto& PlayerCharacter : PlayerCharacters)
	{
		RenderData.Add(PlayerCharacter.RenderData);
	}

	return RenderData;
}

void AMyPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (GameCamera)
	{
		if (!bLevelBoundsInitialized)
		{
			UCameraComponent* CameraComponent = GameCamera->GetCameraComponent();
			const auto CameraLocation = GameCamera->GetActorLocation();
			const auto HalfOrthoWidth = CameraComponent->OrthoWidth * 0.5f;
			const float AspectRatio = CameraComponent->AspectRatio;
		
			LevelBoundsX.Min = CameraLocation.X - HalfOrthoWidth;
			LevelBoundsX.Max = CameraLocation.X + HalfOrthoWidth;
			LevelBoundsZ.Min = CameraLocation.Z - HalfOrthoWidth / AspectRatio;
			LevelBoundsZ.Max = CameraLocation.Z + HalfOrthoWidth / AspectRatio;
			bLevelBoundsInitialized = true;
		}

		UCameraComponent* CameraComponent = GameCamera->GetCameraComponent();
		const float AspectRatio = CameraComponent->AspectRatio;
		FVector PlayersAverageLocation = FVector::ZeroVector;
		if (GameCameraLimits && GameCameraLimits->bStaticCamera)
		{
			PlayersAverageLocation = GameCameraLimits->GetActorLocation();
		}
		else
		{
			for (const auto& PlayerCharacter : PlayerCharacters)
			{
				PlayersAverageLocation += PlayerCharacter.RenderData.BodyLocation;
			}
			PlayersAverageLocation /= PlayerCharacters.Num();
			PlayersAverageLocation.Y = GameCamera->GetCameraLocation().Y;
			PlayersAverageLocation += GeneralData->CameraLocationOffset;
			//PlayersAverageLocation.X = FMath::Clamp(PlayersAverageLocation.X, 27000.f, 37000.f);
			//PlayersAverageLocation.Z = FMath::Clamp(PlayersAverageLocation.Z, 21000.f, 25000.f);
		}

		if (PlayerCharacters.Num() > 1)
		{
			// 1. Compute bounding box (X and Z)
			float MinX = FLT_MAX, MaxX = -FLT_MAX;
			float MinZ = FLT_MAX, MaxZ = -FLT_MAX;
			for (const auto& PlayerCharacter : PlayerCharacters)
			{
				MinX = FMath::Min(MinX, PlayerCharacter.RenderData.BodyLocation.X);
				MaxX = FMath::Max(MaxX, PlayerCharacter.RenderData.BodyLocation.X);
				MinZ = FMath::Min(MinZ, PlayerCharacter.RenderData.BodyLocation.Z);
				MaxZ = FMath::Max(MaxZ, PlayerCharacter.RenderData.BodyLocation.Z);
			}

			// 2. Compute required ortho width to fit both dimensions
			const float PaddingX = GeneralData->CameraPaddingX;
			const float PaddingZ = GeneralData->CameraPaddingZ;
			const float SpreadX = MaxX - MinX + PaddingX;
			const float SpreadZ = MaxZ - MinZ + PaddingZ;

			// Ensure vertical spread fits into height: Height = Width / AspectRatio → Width = Height * AspectRatio
			float MinOrthoWidth = GeneralData->MinOrthoWidth;
			float MaxOrthoWidth = GeneralData->MaxOrthoWidth;
			if (GameCameraLimits && GameCameraLimits->bUseCustomOrthoLimits)
			{
				MinOrthoWidth = GameCameraLimits->MinOrthoWidth;
				MaxOrthoWidth = GameCameraLimits->MaxOrthoWidth;
			}
			const float RequiredOrthoWidth = FMath::Clamp(FMath::Max(SpreadX, SpreadZ * AspectRatio), MinOrthoWidth, MaxOrthoWidth);

			// 3. Smooth ortho width transition
			const float CurrentOrtho = CameraComponent->OrthoWidth;
			const float InterpOrtho = FMath::FInterpTo(CurrentOrtho, RequiredOrthoWidth, DeltaSeconds, 2.f);
			CameraComponent->SetOrthoWidth(InterpOrtho);
		}
		else
		{
			float OrthoWidth = GeneralData->SinglePlayerOrthoWidth;
			if (GameCameraLimits && GameCameraLimits->bUseCustomOrthoLimits)
			{
				OrthoWidth = GameCameraLimits->MaxOrthoWidth;
			}
			CameraComponent->SetOrthoWidth(OrthoWidth);
		}

#if WITH_EDITOR
		if (GeneralData->bKeepCameraAtMaximumZoomOut)
		{
			CameraComponent->SetOrthoWidth(GeneralData->MaxOrthoWidth);
		}
#endif
		
		FVector NewCameraLocation = FMath::VInterpTo(GameCamera->GetCameraLocation(), PlayersAverageLocation, DeltaSeconds, 6.f);
		//DrawDebugSphere(GetWorld(), NewCameraLocation, 800.f, 8.f, FColor::Red, false, 0.016f, SDPG_World, 60.f);
		
		if (GameCameraLimits)
		{
			const FVector BoxCenter = GameCameraLimits->BoxComponent->GetComponentLocation();
			const FVector BoxExtent = GameCameraLimits->BoxComponent->GetScaledBoxExtent();

			const float MinX = BoxCenter.X - BoxExtent.X;
			const float MaxX = BoxCenter.X + BoxExtent.X;

			const float MinZ = BoxCenter.Z - BoxExtent.Z;
			const float MaxZ = BoxCenter.Z + BoxExtent.Z;
		
			const float HalfWidth  = CameraComponent->OrthoWidth * 0.5f;
			const float HalfHeight = HalfWidth / CameraComponent->AspectRatio - 500.f;

			// These two points are usually quite close to one another and they can also swap places, so we check which one is bigger
			const float CameraLocationXClamp1 = MinX + HalfWidth;
			const float CameraLocationXClamp2 = MaxX - HalfWidth;
			if (CameraLocationXClamp2 > CameraLocationXClamp1)
			{
				NewCameraLocation.X = FMath::Clamp(
					NewCameraLocation.X,
					CameraLocationXClamp1,
					CameraLocationXClamp2
				);
			}
			else
			{
				NewCameraLocation.X = FMath::Clamp(
					NewCameraLocation.X,
					CameraLocationXClamp2,
					CameraLocationXClamp1
				);
			}
			
			const float MinCameraLocationZ = MinZ + HalfHeight;
			const float MaxCameraLocationZ = MaxZ - HalfHeight;
			NewCameraLocation.Z = FMath::Clamp(
				NewCameraLocation.Z,
				MinCameraLocationZ,
				MaxCameraLocationZ
			);
		}

		// There seems to be a bug where the camera Y axis spins out and renders black screen for players.
		// That's because we use GameCamera->GetActorLocation().Y as a constant up above, as a base value.
		// But the thing is it gets updated here. So if it somehow changes, it will not get fixed again.
		// Yes it would be nice to figure out why, but due to time constraints I'm just hardcoding this shit here and be done with it.
		// Bottom line is if it works - it works.
		NewCameraLocation.Y = 35000.f;
		//GameCamera->Update(NewCameraLocation, DeltaSeconds);
		GameCamera->Update(NewCameraLocation, DeltaSeconds);
		
		if (!bReplayMode)
		{
			float DeltaX;
			float DeltaY;
			GetInputMouseDelta(DeltaX, DeltaY);

			//GetInputAnalogStickState(EControllerAnalogStick::CAS_LeftStick, DeltaX, DeltaY);
			
			AccumulatedMouseDelta += FVector2D(DeltaX, DeltaY) * GetMouseSensitivity();
		}
	}
}

FPlayerNetworkData& AMyPlayerController::GetPlayerNetworkData()
{
	return PlayerNetworkData;
}

void AMyPlayerController::OnMatchHasStarted()
{
	if (!bReplayMode)
	{
		GetCurrentGameSave()->PlayedGamesNum++;
	}
	
	BP_OnMatchHasStarted();

	auto LocalPlayerState = GetMyPlayerState();
	
	for (auto& UserPlayerState : GetMyGameState()->PlayerArray)
	{
		for (auto& PlayerCharacter : PlayerCharacters)
		{
			if (bReplayMode)
			{
				PlayerCharacter.PlayerRenderer->DefaultPlayerName = GeneralData->PossiblePlayerColorSchemes[PlayerCharacter.PlayerIndex].DefaultPlayerName;
			}
			else if (PlayerCharacter.PlayerId == Cast<AMyPlayerState>(UserPlayerState)->GetMyPlayerId())
			{
				if (!USIK_UserLibrary::LoggedOn())
				{
					PlayerCharacter.PlayerRenderer->DefaultPlayerName = GeneralData->PossiblePlayerColorSchemes[PlayerCharacter.PlayerIndex].DefaultPlayerName;
				}
				else if (LocalPlayerState->GetPlayerId() == UserPlayerState->GetPlayerId())
				{
					PlayerCharacter.PlayerRenderer->DefaultPlayerName = USIK_FriendsLibrary::GetPersonaName();
				}
				else
				{
					const auto SteamId = USIK_SharedFile::GetSteamIdFromUniqueNetId(UserPlayerState->GetUniqueId());
					PlayerCharacter.PlayerRenderer->DefaultPlayerName = USIK_FriendsLibrary::GetFriendPersonaName(SteamId);
				}
				
				break;
			}
		}
	}

	for (auto& PlayerCharacter : PlayerCharacters)
	{
		PlayerCharacter.PlayerRenderer->BP_OnMatchHasStarted(PlayerCharacter.PlayerId);
	}
}

APlayerRenderer* AMyPlayerController::GetLocalPlayerRenderer()
{
	for (auto& PlayerCharacter : PlayerCharacters)
	{
		if (PlayerCharacter.bLocalPlayer)
		{
			return PlayerCharacter.PlayerRenderer;
		}
	}

	checkNoEntry();

	return nullptr;
}

APlayerRenderer* AMyPlayerController::GetPlayerRenderer(int PlayerId)
{
	for (auto& PlayerCharacter : PlayerCharacters)
	{
		if (PlayerCharacter.PlayerId == PlayerId)
		{
			return PlayerCharacter.PlayerRenderer;
		}
	}

	checkNoEntry();

	return nullptr;
}

FLinearColor AMyPlayerController::GetTeamColor(EPlayerTeam Team)
{
	for (FPlayerColorScheme& ColorScheme : GeneralData->PossiblePlayerColorSchemes)
	{
		if (ColorScheme.Team == Team)
		{
			return ColorScheme.MainColor;
		}
	}

	checkNoEntry();

	return FLinearColor::White;
}

EPlayerTeam AMyPlayerController::GetPlayerTeam(int PlayerId)
{
	for (auto PlayerCharacter : PlayerCharacters)
	{
		if (PlayerCharacter.PlayerId == PlayerId)
		{
			return PlayerCharacter.PlayerTeam;
		}
	}

	return EPlayerTeam::None;
}

EPlayerTeam AMyPlayerController::GetLocalPlayerTeam()
{
	for (auto PlayerCharacter : PlayerCharacters)
	{
		if (PlayerCharacter.bLocalPlayer)
		{
			return PlayerCharacter.PlayerTeam;
		}
	}

	return EPlayerTeam::None;
}

int AMyPlayerController::GetLocalPlayerId()
{
	for (auto PlayerCharacter : PlayerCharacters)
	{
		if (PlayerCharacter.bLocalPlayer)
		{
			return PlayerCharacter.PlayerId;
		}
	}

	checkNoEntry();
	
	return -1;
}

FTeamScores AMyPlayerController::GetTeamScores()
{
	return TeamScores;
}

FTeamScores AMyPlayerController::GetCurrentGameModeTeamScores()
{
	return CurrentGameModeTeamScores;
}

int AMyPlayerController::GetCurrentGameModeRespawnFrames() const
{
	if (GameModeSettings.Mode == EGameMode::CaptureTheFlag)
	{
		return GeneralData->CaptureTheFlagPlayerRespawnFrames;
	}
	
	if (GameModeSettings.Mode == EGameMode::KingOfTheCrown)
	{
		return GeneralData->KingOfTheCrownPlayerRespawnFrames;
	}

	if (GameModeSettings.Mode == EGameMode::Deathmatch)
	{
		return GeneralData->DeathmatchPlayerRespawnFrames + 180;
	}
	
	return 300;
}

void AMyPlayerController::OnRender()
{
	auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();
	auto SpawnerSubsystem = GetWorld()->GetSubsystem<USpawnerSubsystem>();

#if WITH_EDITOR
	auto PostProcessVolume = Cast<APostProcessVolume>(UGameplayStatics::GetActorOfClass(GetWorld(), APostProcessVolume::StaticClass()));
	PostProcessVolume->Settings = GeneralData->PostProcessSettings;
#endif

	if (LightSource)
	{
		const float BackgroundSize = LightSource->BackgroundSize;
		FVector LightSourceActorLocation = LightSource->GetActorLocation();
		FVector2D LightSourceLocation2D = FVector2D(LightSourceActorLocation.X, LightSourceActorLocation.Z);
		const float ZoomRatio = Cast<UCameraComponent>(GetViewTarget()->GetComponentByClass(UCameraComponent::StaticClass()))->OrthoWidth / 2000.f;
		const float HalfScreenWidthInWorldUnits = BackgroundSize * ZoomRatio;
		FVector ViewTargetLocation = GetViewTarget()->GetActorLocation();
		
		TArray<FVector2D> MeshesVertices = SpawnerSubsystem->GetAttachedMeshesVertices();
		MeshesVertices.Add(FVector2D(ViewTargetLocation.X - HalfScreenWidthInWorldUnits, ViewTargetLocation.Z - HalfScreenWidthInWorldUnits));
		MeshesVertices.Add(FVector2D(ViewTargetLocation.X + HalfScreenWidthInWorldUnits, ViewTargetLocation.Z - HalfScreenWidthInWorldUnits));
		MeshesVertices.Add(FVector2D(ViewTargetLocation.X - HalfScreenWidthInWorldUnits, ViewTargetLocation.Z + HalfScreenWidthInWorldUnits));
		MeshesVertices.Add(FVector2D(ViewTargetLocation.X + HalfScreenWidthInWorldUnits, ViewTargetLocation.Z + HalfScreenWidthInWorldUnits));

		MeshesVertices.Sort([LightSourceLocation2D](const FVector2D& A, const FVector2D& B)
		{
			// Direction vectors from light to vertex
			FVector2D DirA = A - LightSourceLocation2D;
			FVector2D DirB = B - LightSourceLocation2D;

			// Compute angles in radians
			float AngleA = FMath::Atan2(DirA.Y, DirA.X);
			float AngleB = FMath::Atan2(DirB.Y, DirB.X);

			return AngleA > AngleB; // Sort from highest to lowest angle
		});

		FVector LineTraceStartLocation = FVector(LightSourceActorLocation.X, 0.f, LightSourceActorLocation.Z);

		TArray<FVector> TriangleFanLocations;
		constexpr float OffsetAngle = 0.0001f; // Very small angle in radians
		for (const auto& Vertex : MeshesVertices)
		{
			FVector2D Direction2D = Vertex - LightSourceLocation2D;
			float BaseAngle = FMath::Atan2(Direction2D.Y, Direction2D.X);

			for (float AngleOffset : {OffsetAngle, 0.0f, -OffsetAngle})
			{
				float Angle = BaseAngle + AngleOffset;
				FVector2D RotatedDirection(FMath::Cos(Angle), FMath::Sin(Angle));
				FVector TraceDirection = FVector(RotatedDirection.X, 0.f, RotatedDirection.Y);
				FVector LineTraceEndLocation = LineTraceStartLocation + TraceDirection * BackgroundSize * ZoomRatio;
				FHitResult OutHit;
				FCollisionQueryParams QueryParams;
				GetWorld()->LineTraceSingleByChannel(OutHit, LineTraceStartLocation, LineTraceEndLocation, ECC_GameTraceChannel1, QueryParams);

				if (OutHit.bBlockingHit)
				{
					auto HitObjectName = OutHit.HitObjectHandle.GetName();
					OutHit.Location.Y = 0.f;
					TriangleFanLocations.Add(OutHit.Location);
				
					//DrawDebugBox(GetWorld(), OutHit.Location, FVector(40, 40, 40), FColor::Red, false, 0.016f, SDPG_World, 20.f);
					//DrawDebugLine(GetWorld(), LineTraceStartLocation, OutHit.Location, FColor::Red, false, 0.016f, SDPG_World, 20.f);
				}
				else
				{
					TriangleFanLocations.Add(LineTraceEndLocation);
				
					//DrawDebugLine(GetWorld(), LineTraceStartLocation, LineTraceEndLocation, FColor::Green, false, 0.016f, SDPG_World, 20.f);
				}
			}
		}

		LightSource->UpdateMesh(TriangleFanLocations);
	}

	auto MyGameState = GetMyGameState();
	for (auto PlayerCharacter : PlayerCharacters)
	{
		if (PlayerCharacter.bDead)
		{
			PlayerCharacter.PlayerRenderer->SetActorLocation(FVector(-10000.f, 0.f, -10000.f));

			// We don't want to trigger respawn UI unless it's confirmed that the player is dead
			if (GetCurrentGameModeType() != EGameMode::Deathmatch && PlayerCharacter.DeadFramesNum > GetRollbackWindow())
			{
				const int FramesTillPlayerRespawn = GetCurrentGameModeRespawnFrames() - PlayerCharacter.DeadFramesNum;
				const int SecondsLeftTillRespawn = FMath::RoundHalfToEven(static_cast<float>(FramesTillPlayerRespawn) / 60.f); // 60 steps per second
				PlayerCharacter.PlayerRenderer->OnRespawnCountdown.Broadcast(SecondsLeftTillRespawn);
			}
			
			continue;
		}

		PlayerCharacter.RenderData.HeadAnimationComp->Update(PlayerCharacter.RenderData.SimulationFrameId);
		PlayerCharacter.RenderData.EyesAnimationComp->Update(PlayerCharacter.RenderData.SimulationFrameId);
		if (PlayerCharacter.ShootingCooldownFramesLeft > ShootingCooldownInFrames - 10)
		{
			PlayerCharacter.RenderData.EyesAnimationComp->TriggerMeanEyes(PlayerCharacter.RenderData.SimulationFrameId);
		}

		FPlayerRenderFrameData PlayerRenderFrameData;
		PlayerRenderFrameData.EquippedItem = PlayerCharacter.GetFirstItem();
		PlayerRenderFrameData.BulletsLeft = PlayerCharacter.BulletsLeft;
		PlayerRenderFrameData.MaxBullets = PlayerCharacter.RenderData.MaxBullets;
		//PlayerRenderFrameData.BulletReloadPerc = 1.f - (static_cast<float>(PlayerCharacter.BulletReloadCooldownFramesLeft) / static_cast<float>(GetBulletReloadCooldownInFrames(PlayerCharacter)));
		//PlayerRenderFrameData.DashReloadPerc = 1.f - (static_cast<float>(PlayerCharacter.DashCooldownFramesLeft) / static_cast<float>(DashCooldownInFrames));
		PlayerRenderFrameData.bHasCrown = MyGameState->GetPlayerIdThatHasCrown() == PlayerCharacter.PlayerId;
		PlayerRenderFrameData.CrownHeldFramesNum = PlayerCharacter.CrownHeldFramesNum;
		PlayerRenderFrameData.InvulnerabilityFramesNum = PlayerCharacter.InvulnerabilityFramesNum;
		PlayerRenderFrameData.ShootChargePerc = static_cast<float>(PlayerCharacter.ShootButtonHeldFramesNum) / static_cast<float>(MaxFramesHoldShootButton);
		if (!bUseChargeShooting || bUseHandExtensionToCalculateShootPower)
		{
			PlayerRenderFrameData.ShootChargePerc = 0.f;
		}

		if (PlayerCharacter.CarryingFlagBodyIndex > -1)
		{
			PlayerRenderFrameData.CarryingFlagOfTeam = SpawnerSubsystem->GetBodyActor<AFlag>(PlayerCharacter.CarryingFlagBodyIndex)->BelongsToTeam;
		}
		
		PlayerCharacter.PlayerRenderer->OnPlayerRenderFrame.Broadcast(PlayerRenderFrameData);

		if (PlayerCharacter.bLocalPlayer && PlayerCharacter.PlayerItems != PlayerCharacter.RenderData.PlayerItems)
		{
			PlayerCharacter.RenderData.PlayerItems = PlayerCharacter.PlayerItems;

			TArray<ELootItem> Items;
			TArray<int> Amounts;
			TMap<ELootItem, int> ItemToAmountArrayIndex;
			for (ELootItem Item : PlayerCharacter.PlayerItems)
			{
				if (!Items.Contains(Item))
				{
					Items.Add(Item);
					ItemToAmountArrayIndex.Add(Item, Amounts.Add(1));
					continue;
				}

				Amounts[ItemToAmountArrayIndex[Item]]++;
			}
			OnLocalPlayerItemsChanged.Broadcast(Items, Amounts);
		}
		
		Vec3 PlayerPos = JoltSubsystem->GetBodyInterface()->GetCenterOfMassPosition(BodyID(PlayerCharacter.ShoulderBodyIndex));
		Vec3 CursorPos = PlayerPos + Vec3(Units::ToJoltUnits(PlayerCharacter.CursorPosition.X), Units::ToJoltUnits(PlayerCharacter.CursorPosition.Y), 0);
		Vec3 HandCurrentPos = JoltSubsystem->GetBodyInterface()->GetCenterOfMassPosition(BodyID(PlayerCharacter.HandBodyIndex));

		Vec3 PlayerVel = JoltSubsystem->GetBodyInterface()->GetLinearVelocity(BodyID(PlayerCharacter.ShoulderBodyIndex));
		FVector PlayerDirection = Units::FromJoltCoord(PlayerVel).GetSafeNormal();
	
		const float MaxSpeed = 200.f;
		float Speed = PlayerVel.Length();

		constexpr float MaxStretch = 0.3f;

		float Stretch = FMath::Lerp(0.0f, MaxStretch, FMath::Clamp(Speed / MaxSpeed, 0.0f, 1.0f));
		float Squash = 0.f;
		if (Stretch > 0.f)
		{
			Squash = Stretch * -0.5f;
		}

		
		if ((PlayerCharacter.RenderData.BodyLocation.Z - 300.f) < PlayerCharacter.RenderData.CursorLocation.Z)
		{
			//PlayerCharacter.PlayerRenderer->UpperArmMeshL->SetTranslucentSortPriority(17);
			//PlayerCharacter.PlayerRenderer->UpperArmMeshR->SetTranslucentSortPriority(17);
			
			//Cast<UStaticMeshComponent>(PlayerCharacter.PlayerRenderer->UpperArmMeshL->GetChildComponent(0))->SetTranslucentSortPriority(16);
			//Cast<UStaticMeshComponent>(PlayerCharacter.PlayerRenderer->UpperArmMeshR->GetChildComponent(0))->SetTranslucentSortPriority(16);
		}
		else
		{
			//PlayerCharacter.PlayerRenderer->UpperArmMeshL->SetTranslucentSortPriority(4);
			//PlayerCharacter.PlayerRenderer->UpperArmMeshR->SetTranslucentSortPriority(4);
			
			//Cast<UStaticMeshComponent>(PlayerCharacter.PlayerRenderer->UpperArmMeshL->GetChildComponent(0))->SetTranslucentSortPriority(3);
			//Cast<UStaticMeshComponent>(PlayerCharacter.PlayerRenderer->UpperArmMeshR->GetChildComponent(0))->SetTranslucentSortPriority(3);
		}
		//DrawDebugString(GetWorld(), PlayerCharacter.RenderData.CursorLocation, FString::Printf(TEXT("%s\n%s"), *PlayerCharacter.RenderData.CursorLocation.ToString(), *PlayerCharacter.RenderData.BodyLocation.ToString()), 0, FColor::White, 0.016f);
		
		// DEBUG STRING FOR DIR AND SPEED
		//DrawDebugString(GetWorld(), PlayerDirection, FString::Printf(TEXT("Dir: %s\nSpeed: %f"), *PlayerDirection.ToString(), Speed), 0, FColor::White, 0.016f);
		//PlayerCharacter.RenderData.BodyDynamicMaterial->SetVectorParameterValue(FName("StretchDirection"), PlayerDirection);
		//PlayerCharacter.RenderData.BodyDynamicMaterial->SetScalarParameterValue(FName("StretchAmount"), Stretch);
		//PlayerCharacter.RenderData.BodyDynamicMaterial->SetScalarParameterValue(FName("SquashAmount"), Squash);
		/*PlayerCharacter.RenderData.HeadDynamicMaterial->SetVectorParameterValue(FName("StretchDirection"), PlayerDirection);
		PlayerCharacter.RenderData.HeadDynamicMaterial->SetScalarParameterValue(FName("StretchAmount"), Stretch);
		PlayerCharacter.RenderData.HeadDynamicMaterial->SetScalarParameterValue(FName("SquashAmount"), Squash);*/

		FVector PlayerLocation = Units::FromJoltPos(PlayerPos);
		FVector HandLocation = Units::FromJoltPos(HandCurrentPos);

		auto PlayerToHandDirection = (HandCurrentPos - PlayerPos).NormalizedOr(Vec3::sZero());

		PlayerCharacter.RenderData.EyesAnimationComp->UpdatePupils(Units::FromJoltCoord(PlayerToHandDirection));

		const FVector ArmDepth = FVector(0, 1000.f, 0);
		constexpr float ArmSegmentLength = 450.f;
		constexpr float ArmDistanceFromBodyCenter = 350.f;
		PlayerCharacter.RenderData.LeftHandStartLocation = PlayerLocation + FVector(-ArmDistanceFromBodyCenter, 0, 0.f) + ArmDepth;
		PlayerCharacter.RenderData.RightHandStartLocation = PlayerLocation + FVector(ArmDistanceFromBodyCenter, 0, 0.f) + ArmDepth;

		FVector LeftHandJointTarget = PlayerCharacter.RenderData.LeftHandStartLocation + FVector(-1000.f, 0.f, -500.f);
		FVector RightHandJointTarget = PlayerCharacter.RenderData.RightHandStartLocation + FVector(1000.f, 0.f, -500.f);

		FVector LeftHandEffector = HandLocation + ArmDepth + Units::FromJoltPos(PlayerToHandDirection * -11.f) + FVector(0, 300.f, 0);
		FVector RightHandEffector = HandLocation + ArmDepth + Units::FromJoltPos(PlayerToHandDirection * -13.f) + FVector(0, 300.f, 0);
		
		AnimationCore::SolveTwoBoneIK(
			PlayerCharacter.RenderData.LeftHandStartLocation,
			PlayerCharacter.RenderData.LeftHandStartLocation - FVector(ArmSegmentLength, 0, 0),
			PlayerCharacter.RenderData.LeftHandStartLocation - FVector(ArmSegmentLength * 2.f, 0, 0),
			LeftHandJointTarget,
			LeftHandEffector,
			PlayerCharacter.RenderData.LeftHandJointLocation,
			PlayerCharacter.RenderData.LeftHandEndLocation,
			false,
			1.f,
			1.2f
		);
	
		//DrawDebugLine(GetWorld(), PlayerCharacter.RenderData.LeftHandStartLocation, PlayerCharacter.RenderData.LeftHandJointLocation, FColor::Cyan, false, 0.02f, SDPG_World, 100.f);
		//DrawDebugLine(GetWorld(), PlayerCharacter.RenderData.LeftHandJointLocation, PlayerCharacter.RenderData.LeftHandEndLocation, FColor::Cyan, false, 0.02f, SDPG_World, 100.f);

		AnimationCore::SolveTwoBoneIK(
			PlayerCharacter.RenderData.RightHandStartLocation,
			PlayerCharacter.RenderData.RightHandStartLocation + FVector(ArmSegmentLength, 0, 0),
			PlayerCharacter.RenderData.RightHandStartLocation + FVector(ArmSegmentLength * 2.f, 0, 0),
			RightHandJointTarget,
			RightHandEffector,
			PlayerCharacter.RenderData.RightHandJointLocation,
			PlayerCharacter.RenderData.RightHandEndLocation,
			false,
			1.f,
			1.2f
		);

		//DrawDebugLine(GetWorld(), PlayerCharacter.RenderData.RightHandStartLocation, PlayerCharacter.RenderData.RightHandJointLocation, FColor::Cyan, false, 0.02f, SDPG_World, 100.f);
		//DrawDebugLine(GetWorld(), PlayerCharacter.RenderData.RightHandJointLocation, PlayerCharacter.RenderData.RightHandEndLocation, FColor::Cyan, false, 0.02f, SDPG_World, 100.f);


		if (bPhysicsSimulationPaused)
		{
			
		}
		else
		{
			PlayerCharacter.PlayerRenderer->SetActorLocation(PlayerLocation);
		}

		APlayerRenderer::UpdateMeshLocationAndRotationGivenTwoPoints(
			PlayerCharacter.PlayerRenderer->UpperArmMeshL,
			PlayerCharacter.RenderData.LeftHandStartLocation,
			PlayerCharacter.RenderData.LeftHandJointLocation
		);

		APlayerRenderer::UpdateMeshLocationAndRotationGivenTwoPoints2D(
			PlayerCharacter.PlayerRenderer->LowerArmMeshL,
			PlayerCharacter.RenderData.LeftHandJointLocation,
			PlayerCharacter.RenderData.LeftHandEndLocation
		);

		APlayerRenderer::UpdateMeshLocationAndRotationGivenTwoPoints(
			PlayerCharacter.PlayerRenderer->UpperArmMeshR,
			PlayerCharacter.RenderData.RightHandStartLocation,
			PlayerCharacter.RenderData.RightHandJointLocation
		);

		APlayerRenderer::UpdateMeshLocationAndRotationGivenTwoPoints2D(
			PlayerCharacter.PlayerRenderer->LowerArmMeshR,
			PlayerCharacter.RenderData.RightHandJointLocation,
			PlayerCharacter.RenderData.RightHandEndLocation
		);

		FVector StickStartLocation = HandLocation;
		StickStartLocation.Y = 5000.f;
		FVector StickEndLocation = HandLocation + Units::FromJoltPos(PlayerToHandDirection * -17.f);
		StickEndLocation.Y = 5000.f;

		//PlayerCharacter.RenderData.HandStaticMeshComponent->SetWorldRotation(UKismetMathLibrary::FindLookAtRotation(StickStartLocation, StickEndLocation));
		
		APlayerRenderer::UpdateMeshLocationAndRotationGivenTwoPoints2D(
			PlayerCharacter.PlayerRenderer->StickMesh,
			StickStartLocation,
			StickEndLocation
		);
	}
}

void AMyPlayerController::LocalControllerInit()
{
	if (bInitialized)
	{
		return;
	}

	bInitialized = true;

	LoadAssetsForGame();

	if (bReplayMode && GetNetMode() == NM_ListenServer)
	{
		bReplayMode = false;
	}

	GeneralData = UAssetManagerSubsystem::GetGeneralData(GetWorld());
	check(GeneralData);

	LOG(TEXT("[MyPlayerController] Init called"));

	auto MyReplaySubsystem = GetWorld()->GetSubsystem<UMyReplaySubsystem>();
	bReplayMode = MyReplaySubsystem->IsReplayMode();
	
	auto InputsSubsystem = GetWorld()->GetSubsystem<UInputsSubsystem>();
	checkf(InputsSubsystem, TEXT("InputsSubsystem not found"));
	auto GameInputs = InputsSubsystem->GameInputs;
	checkf(GameInputs, TEXT("GameInputs not found"));

	if (bReplayMode)
	{
		GameInputs->Init(MyReplaySubsystem->GetPlayersNum());
	}
	else
	{
		GameInputs->Init(UGameplayStatics::GetNumPlayerStates(GetWorld()));
	}

	RollbackWindow = InputsSubsystem->GameInputs->GetRollbackWindow();
	
	auto CurrentGameSave = GetCurrentGameSave();

	if (!bReplayMode && bRecordReplays)
	{
		MyReplaySubsystem->RecordReplay(CurrentGameSave->PlayedGamesNum, GetWorld()->GetName());
	}
	
	if (bReplayMode)
	{
		const uint64_t GameSeed = FCString::Strtoui64(*MyReplaySubsystem->GetGameSeed(), nullptr, 10);
		GetWorld()->GetSubsystem<UJoltSubsystem>()->InitRandomNumberGenerator(GameSeed);

		for (int i = 0; i < MyReplaySubsystem->GetInitialRNGSkip(); i++)
		{
			GetWorld()->GetSubsystem<UJoltSubsystem>()->RandomNumberGenerator.NextIntInRange(0, 10);
		}
		
		int CharacterIndex = 0;

		LevelsSequence = MyReplaySubsystem->GetLevelsSequence();
		LoadNextLevel();
		
		const int ServerCharacterId = CreateCharacter(GetPlayerSpawn(CharacterIndex, PossibleSpawns));
		CharacterIndex++;
		auto MyPlayerState = GetMyPlayerState();
		LOG(TEXT("Created physical character %d"), CharacterIndex);
		MyPlayerState->MulticastSetPlayerIdAndGameSeed(ServerCharacterId, MyReplaySubsystem->GetGameSeed());

		// We start from i = 1, because the local player has already been created above
		for (int i = 1; i < MyReplaySubsystem->GetPlayersNum(); i++)
		{
			const int ClientCharacterId = CreateCharacter(GetPlayerSpawn(CharacterIndex, PossibleSpawns));
			LOG(TEXT("Created physical character %d"), ClientCharacterId);
		}
	}
	else if (GetNetMode() == NM_ListenServer || GetNetMode() == NM_Standalone)
	{
		const auto GameSeed = FPlatformTime::Cycles64();
		const auto GameSeedAsString = FString::Printf(TEXT("%llu"), GameSeed);
		GetWorld()->GetSubsystem<UJoltSubsystem>()->InitRandomNumberGenerator(GameSeed);

		LevelsSequence = BuildLevelsSequenceForMatch();
		MyReplaySubsystem->SaveLevelsSequence(LevelsSequence);
		LoadNextLevel();
		
		int CharacterIndex = 0;
	
		const int ServerCharacterId = CreateCharacter(GetPlayerSpawn(CharacterIndex, PossibleSpawns));
		CharacterIndex++;
		auto MyPlayerState = GetMyPlayerState();
		LOG(TEXT("Created physical character %d for authoritative client %d"), ServerCharacterId, MyPlayerState->GetMyPlayerId());
		MyPlayerState->MulticastSetPlayerIdAndGameSeed(ServerCharacterId, GameSeedAsString);
		
		for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
		{
			APlayerController* PlayerController = Iterator->Get();
			if (PlayerController->GetUniqueID() == GetUniqueID())
			{
				continue;
			}

			auto MyPlayerController = Cast<AMyPlayerController>(PlayerController);

			const int ClientCharacterId = CreateCharacter(GetPlayerSpawn(CharacterIndex, PossibleSpawns));
			CharacterIndex++;
			LOG(TEXT("Created physical character %d for client %d"), ClientCharacterId, Cast<AMyPlayerController>(PlayerController)->GetMyPlayerState()->GetMyPlayerId());
			MyPlayerController->GetMyPlayerState()->MulticastSetPlayerIdAndGameSeed(ClientCharacterId, GameSeedAsString);
		}

		const FPlayerSettings ServerPlayerSettings = FPlayerSettings(GetMouseSensitivity());
		MyPlayerState->MulticastSetPlayerSettings(ServerPlayerSettings);

		MyReplaySubsystem->SavePlayersNum(CharacterIndex);
		MyReplaySubsystem->SaveMaxRollbackFramesNum(GameInputs->GetMaxRollbackFrames());
		MyReplaySubsystem->SaveInputDelayFramesNum(GameInputs->GetInputDelayFrames());
		MyReplaySubsystem->SaveGameSeed(GameSeedAsString);
		MyReplaySubsystem->SaveIsServer(true);
		MyReplaySubsystem->SavePlayerSettings(ServerCharacterId, ServerPlayerSettings);
	}
	// The fact that this method was called (Init()) means we must already have all the needed data about all connected players replicated
	// If that's not the case, then all hell will break loose
	else if (GetNetMode() == NM_Client)
	{
		LevelsSequence = BuildLevelsSequenceForMatch();
		MyReplaySubsystem->SaveLevelsSequence(LevelsSequence);
		LoadNextLevel();
		
		int CharacterIndex = 0;
	
		for (int i = 0; i < UGameplayStatics::GetNumPlayerStates(GetWorld()); i++)
		{
			LOG(TEXT("Created character %d"), CharacterIndex);
			const int CreatedPlayerId = CreateCharacter(GetPlayerSpawn(CharacterIndex, PossibleSpawns));
			const FPlayerSettings CurrentPlayerSettings = GetMyPlayerState(CreatedPlayerId)->GetPlayerSettings();
			CharacterIndex++;

			MyReplaySubsystem->SavePlayerSettings(CreatedPlayerId, CurrentPlayerSettings);
		}

		MyReplaySubsystem->SavePlayersNum(CharacterIndex);
		MyReplaySubsystem->SaveMaxRollbackFramesNum(GameInputs->GetMaxRollbackFrames());
		MyReplaySubsystem->SaveInputDelayFramesNum(GameInputs->GetInputDelayFrames());
		MyReplaySubsystem->SaveGameSeed(GetPlayerNetworkData().GameSeed);
		MyReplaySubsystem->SaveIsServer(false);
	}

	auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();
	// Optional step: Before starting the physics simulation you can optimize the broad phase. This improves collision detection performance (it's pointless here because we only have 2 bodies).
	// You should definitely not call this every frame or when e.g. streaming in a new level section as it is an expensive operation.
	// Instead insert all new objects in batches instead of 1 at a time to keep the broad phase efficient.
	JoltSubsystem->GetPhysicsSystem()->OptimizeBroadPhase();

	auto& MyPlayerCharacter = GetPlayerCharacter(GetMyPlayerState()->GetMyPlayerId());
	MyPlayerCharacter.bLocalPlayer = true;
	MyPlayerCharacter.PlayerRenderer->SetBelongsToLocalPlayer(true);
	LocalPlayerBody = JoltSubsystem->GetPhysicsSystem()->GetBodyLockInterfaceNoLock().TryGetBody(BodyID(MyPlayerCharacter.ShoulderBodyIndex));

	checkf(LocalPlayerBody, TEXT("ShoulderBodyIndex %d"), MyPlayerCharacter.ShoulderBodyIndex);

	GetWorld()->GetSubsystem<UGameLoopSubsystem>()->OnRenderFrame.AddUniqueDynamic(this, &AMyPlayerController::OnRender);

	GetPlayerNetworkData().bReadyToStartGameLoop = true;
	if (GetNetMode() == NM_Client)
	{
		ServerUpdatePlayerNetworkData(GetPlayerNetworkData());
	}

	GameCamera = Cast<AGameCamera>(UGameplayStatics::GetActorOfClass(GetWorld(), AGameCamera::StaticClass()));
	checkf(GameCamera, TEXT("Game camera not found"));

	SetViewTarget(GameCamera);

	SetGameModeSettings(LevelsSequence[0].GameModeSettings);

	RespawnPlayerCharacters();

	ReinitGameCamera();

	InitGameMode(GameModeSettings, PreviousGameModeSettings);

	OnTeamScoresChanged.Broadcast(TeamScores);

	OnRender();
	OnFinishLoading.Broadcast();
}

void AMyPlayerController::LocalControllerLobbyInit()
{
	if (bInitialized)
	{
		return;
	}

	checkf(GetNetMode() == NM_Standalone, TEXT("Menu level not in standalone mode"));

	InitGameMode(FGameModeSettings(EGameMode::Lobby), FGameModeSettings());
	SetGameModeSettings(FGameModeSettings(EGameMode::Lobby));

	bInitialized = true;

	LoadAssetsForGame();

	GeneralData = UAssetManagerSubsystem::GetGeneralData(GetWorld());
	check(GeneralData);

	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerSpawn::StaticClass(), PossibleSpawns);

	auto InputsSubsystem = GetWorld()->GetSubsystem<UInputsSubsystem>();
	checkf(InputsSubsystem, TEXT("InputsSubsystem not found"));
	auto GameInputs = InputsSubsystem->GameInputs;
	checkf(GameInputs, TEXT("GameInputs not found"));
	GameInputs->Init(UGameplayStatics::GetNumPlayerStates(GetWorld()));

	const auto GameSeed = FPlatformTime::Cycles64();
	const auto GameSeedAsString = FString::Printf(TEXT("%llu"), GameSeed);
	int CharacterIndex = 0;

	const int ServerCharacterId = CreateCharacter(GetPlayerSpawn(CharacterIndex, PossibleSpawns));
	CharacterIndex++;
	auto MyPlayerState = GetMyPlayerState();
	LOG(TEXT("Created physical character %d for authoritative client %d"), ServerCharacterId, MyPlayerState->GetMyPlayerId());
	MyPlayerState->MulticastSetPlayerIdAndGameSeed(ServerCharacterId, GameSeedAsString);

	auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();
	// Optional step: Before starting the physics simulation you can optimize the broad phase. This improves collision detection performance (it's pointless here because we only have 2 bodies).
	// You should definitely not call this every frame or when e.g. streaming in a new level section as it is an expensive operation.
	// Instead insert all new objects in batches instead of 1 at a time to keep the broad phase efficient.
	JoltSubsystem->GetPhysicsSystem()->OptimizeBroadPhase();
	

	auto& MyPlayerCharacter = GetPlayerCharacter(GetMyPlayerState()->GetMyPlayerId());
	MyPlayerCharacter.bLocalPlayer = true;
	MyPlayerCharacter.PlayerRenderer->SetBelongsToLocalPlayer(true);
	LocalPlayerBody = JoltSubsystem->GetPhysicsSystem()->GetBodyLockInterfaceNoLock().TryGetBody(BodyID(MyPlayerCharacter.ShoulderBodyIndex));

	checkf(LocalPlayerBody, TEXT("ShoulderBodyIndex %d"), MyPlayerCharacter.ShoulderBodyIndex);

	GetWorld()->GetSubsystem<UGameLoopSubsystem>()->OnRenderFrame.AddUniqueDynamic(this, &AMyPlayerController::OnRender);

	GetPlayerNetworkData().bReadyToStartGameLoop = true;

	GameCamera = Cast<AGameCamera>(UGameplayStatics::GetActorOfClass(GetWorld(), AGameCamera::StaticClass()));
	checkf(GameCamera, TEXT("Game camera not found"));

	GameBackground = Cast<AGameBackground>(UGameplayStatics::GetActorOfClass(GetWorld(), AGameBackground::StaticClass()));

	SetViewTarget(GameCamera);

	GameCameraLimits = Cast<AGameCameraLimits>(UGameplayStatics::GetActorOfClass(GetWorld(), AGameCameraLimits::StaticClass()));

	InitializeJoltBodiesAndConstraints();

	RespawnPlayerCharacters();

	ReinitGameCamera();
	
	InitGameMode(GameModeSettings, PreviousGameModeSettings);

	OnTeamScoresChanged.Broadcast(TeamScores);

	OnRender();
	OnFinishLoading.Broadcast();
}

void AMyPlayerController::LocalControllerTutorialInit()
{
	if (bInitialized)
	{
		return;
	}

	checkf(GetNetMode() == NM_Standalone, TEXT("Tutorial level not in standalone mode"));

	bInitialized = true;

	LoadAssetsForGame();

	GeneralData = UAssetManagerSubsystem::GetGeneralData(GetWorld());
	check(GeneralData);

	const auto GameSeed = FPlatformTime::Cycles64();
	const auto GameSeedAsString = FString::Printf(TEXT("%llu"), GameSeed);
	int CharacterIndex = 0;
	GetWorld()->GetSubsystem<UJoltSubsystem>()->InitRandomNumberGenerator(GameSeed);

	LevelsSequence = BuildLevelsSequenceForTutorial();
	LoadNextLevel();

	SetGameModeSettings(CurrentLevelData.GameModeSettings);
	InitGameMode(GameModeSettings, PreviousGameModeSettings);

	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerSpawn::StaticClass(), PossibleSpawns);

	auto InputsSubsystem = GetWorld()->GetSubsystem<UInputsSubsystem>();
	checkf(InputsSubsystem, TEXT("InputsSubsystem not found"));
	auto GameInputs = InputsSubsystem->GameInputs;
	checkf(GameInputs, TEXT("GameInputs not found"));
	GameInputs->Init(UGameplayStatics::GetNumPlayerStates(GetWorld()));

	const int ServerCharacterId = CreateCharacter(GetPlayerSpawn(CharacterIndex, PossibleSpawns));
	CharacterIndex++;
	auto MyPlayerState = GetMyPlayerState();
	LOG(TEXT("Created physical character %d for authoritative client %d"), ServerCharacterId, MyPlayerState->GetMyPlayerId());
	MyPlayerState->MulticastSetPlayerIdAndGameSeed(ServerCharacterId, GameSeedAsString);

	auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();
	// Optional step: Before starting the physics simulation you can optimize the broad phase. This improves collision detection performance (it's pointless here because we only have 2 bodies).
	// You should definitely not call this every frame or when e.g. streaming in a new level section as it is an expensive operation.
	// Instead insert all new objects in batches instead of 1 at a time to keep the broad phase efficient.
	JoltSubsystem->GetPhysicsSystem()->OptimizeBroadPhase();
	
	auto& MyPlayerCharacter = GetPlayerCharacter(GetMyPlayerState()->GetMyPlayerId());
	MyPlayerCharacter.bLocalPlayer = true;
	MyPlayerCharacter.PlayerRenderer->SetBelongsToLocalPlayer(true);
	LocalPlayerBody = JoltSubsystem->GetPhysicsSystem()->GetBodyLockInterfaceNoLock().TryGetBody(BodyID(MyPlayerCharacter.ShoulderBodyIndex));

	checkf(LocalPlayerBody, TEXT("ShoulderBodyIndex %d"), MyPlayerCharacter.ShoulderBodyIndex);

	GetWorld()->GetSubsystem<UGameLoopSubsystem>()->OnRenderFrame.AddUniqueDynamic(this, &AMyPlayerController::OnRender);

	GetPlayerNetworkData().bReadyToStartGameLoop = true;

	GameCamera = Cast<AGameCamera>(UGameplayStatics::GetActorOfClass(GetWorld(), AGameCamera::StaticClass()));
	checkf(GameCamera, TEXT("Game camera not found"));

	GameBackground = Cast<AGameBackground>(UGameplayStatics::GetActorOfClass(GetWorld(), AGameBackground::StaticClass()));

	SetViewTarget(GameCamera);

	GameCameraLimits = Cast<AGameCameraLimits>(UGameplayStatics::GetActorOfClass(GetWorld(), AGameCameraLimits::StaticClass()));

	InitializeJoltBodiesAndConstraints();

	RespawnPlayerCharacters();

	ReinitGameCamera();
	
	InitGameMode(GameModeSettings, PreviousGameModeSettings);

	OnTeamScoresChanged.Broadcast(TeamScores);

	OnRender();
	OnFinishLoading.Broadcast();
}

void AMyPlayerController::LocalControllerTimeAttackInit()
{
	if (bInitialized)
	{
		return;
	}

	checkf(GetNetMode() == NM_Standalone, TEXT("Time attack level not in standalone mode"));

	bInitialized = true;

	LoadAssetsForGame();

	GeneralData = UAssetManagerSubsystem::GetGeneralData(GetWorld());
	check(GeneralData);

	const auto GameSeed = FPlatformTime::Cycles64();
	const auto GameSeedAsString = FString::Printf(TEXT("%llu"), GameSeed);
	int CharacterIndex = 0;
	GetWorld()->GetSubsystem<UJoltSubsystem>()->InitRandomNumberGenerator(GameSeed);

	LevelsSequence = BuildLevelsSequenceForTimeAttack();
	SetGameModeSettings(LevelsSequence[0].GameModeSettings);
	LoadNextLevel();

	InitGameMode(GameModeSettings, PreviousGameModeSettings);

	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerSpawn::StaticClass(), PossibleSpawns);

	auto InputsSubsystem = GetWorld()->GetSubsystem<UInputsSubsystem>();
	checkf(InputsSubsystem, TEXT("InputsSubsystem not found"));
	auto GameInputs = InputsSubsystem->GameInputs;
	checkf(GameInputs, TEXT("GameInputs not found"));
	GameInputs->Init(UGameplayStatics::GetNumPlayerStates(GetWorld()));

	const int ServerCharacterId = CreateCharacter(GetPlayerSpawn(CharacterIndex, PossibleSpawns));
	CharacterIndex++;
	auto MyPlayerState = GetMyPlayerState();
	LOG(TEXT("Created physical character %d for authoritative client %d"), ServerCharacterId, MyPlayerState->GetMyPlayerId());
	MyPlayerState->MulticastSetPlayerIdAndGameSeed(ServerCharacterId, GameSeedAsString);

	auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();
	JoltSubsystem->GetPhysicsSystem()->OptimizeBroadPhase();

	auto& MyPlayerCharacter = GetPlayerCharacter(GetMyPlayerState()->GetMyPlayerId());
	MyPlayerCharacter.bLocalPlayer = true;
	MyPlayerCharacter.PlayerRenderer->SetBelongsToLocalPlayer(true);
	LocalPlayerBody = JoltSubsystem->GetPhysicsSystem()->GetBodyLockInterfaceNoLock().TryGetBody(BodyID(MyPlayerCharacter.ShoulderBodyIndex));

	checkf(LocalPlayerBody, TEXT("ShoulderBodyIndex %d"), MyPlayerCharacter.ShoulderBodyIndex);

	GetWorld()->GetSubsystem<UGameLoopSubsystem>()->OnRenderFrame.AddUniqueDynamic(this, &AMyPlayerController::OnRender);

	GetPlayerNetworkData().bReadyToStartGameLoop = true;

	GameCamera = Cast<AGameCamera>(UGameplayStatics::GetActorOfClass(GetWorld(), AGameCamera::StaticClass()));
	checkf(GameCamera, TEXT("Game camera not found"));

	GameBackground = Cast<AGameBackground>(UGameplayStatics::GetActorOfClass(GetWorld(), AGameBackground::StaticClass()));

	SetViewTarget(GameCamera);

	GameCameraLimits = Cast<AGameCameraLimits>(UGameplayStatics::GetActorOfClass(GetWorld(), AGameCameraLimits::StaticClass()));

	InitializeJoltBodiesAndConstraints();

	RespawnPlayerCharacters();

	ReinitGameCamera();

	InitGameMode(GameModeSettings, PreviousGameModeSettings);

	OnTeamScoresChanged.Broadcast(TeamScores);

	OnRender();
	OnFinishLoading.Broadcast();
}

TArray<FTimeAttackLevelInfo> AMyPlayerController::GetTimeAttackLevels() const
{
	TArray<FTimeAttackLevelInfo> Result;
	if (!GeneralData)
	{
		return Result;
	}

	const FString GameModeSuffix = UEnum::GetValueAsString(EGameMode::TimeAttack).RightChop(11); // strip "EGameMode::"
	for (int32 i = 0; i < GeneralData->TimeAttackLevels.Num(); ++i)
	{
		const FLevelData& LevelData = GeneralData->TimeAttackLevels[i];
		FTimeAttackLevelInfo Info;
		Info.LevelIndex = i;
		checkf(LevelData.MapData, TEXT("FLevelData at index %d has no MapData assigned"), i);
		Info.Level = LevelData.MapData->WorldPtr;
		Info.LevelName = LevelData.MapData->MapName;
		Info.LeaderboardName = Info.LevelName.Replace(TEXT(" "), TEXT("")) + GameModeSuffix;
		Info.Thumbnail = LevelData.MapData->Thumbnail;
		Result.Add(Info);
	}

	return Result;
}

FTimeAttackLevelInfo AMyPlayerController::GetCurrentTimeAttackLevelInfo() const
{
	checkf(GeneralData, TEXT("GeneralData is not initialized"));

	const int32 Index = UHelper::GetMyGameInstance(GetWorld())->CurrentTimeAttackLevelIndex;
	checkf(GeneralData->TimeAttackLevels.IsValidIndex(Index), TEXT("CurrentTimeAttackLevelIndex %d is out of range"), Index);

	const FLevelData& LevelData = GeneralData->TimeAttackLevels[Index];
	checkf(LevelData.MapData, TEXT("FLevelData at index %d has no MapData assigned"), Index);

	const FString GameModeSuffix = UEnum::GetValueAsString(EGameMode::TimeAttack).RightChop(11);
	FTimeAttackLevelInfo Info;
	Info.LevelIndex = Index;
	Info.Level = LevelData.MapData->WorldPtr;
	Info.LevelName = LevelData.MapData->MapName;
	Info.LeaderboardName = Info.LevelName.Replace(TEXT(" "), TEXT("")) + GameModeSuffix;
	Info.Thumbnail = LevelData.MapData->Thumbnail;
	return Info;
}

void AMyPlayerController::SetCurrentTimeAttackLevelIndex(int32 Index)
{
	UMyGameInstance* MyGameInstance = UHelper::GetMyGameInstance(GetWorld());
	if (MyGameInstance->CurrentTimeAttackLevelIndex == Index)
	{
		return;
	}

	checkf(GeneralData && GeneralData->TimeAttackLevels.IsValidIndex(Index), TEXT("SetCurrentTimeAttackLevelIndex: index %d is out of range"), Index);
	MyGameInstance->CurrentTimeAttackLevelIndex = Index;
	OnTimeAttackLevelIndexChanged.Broadcast(Index);
}

float AMyPlayerController::GetTimeAttackElapsedSeconds() const
{
	if (TimeAttackStartFrame < 0)
	{
		return 0.f;
	}

	if (TimeAttackFinishTime >= 0.f)
	{
		return TimeAttackFinishTime;
	}
	
	const int ElapsedFrames = GetWorld()->GetSubsystem<UGameLoopSubsystem>()->GetSteppingFrameId() - TimeAttackStartFrame;
	
	return ElapsedFrames / 60.f;
}

bool AMyPlayerController::CanTakeCheckpoint(const ACheckpoint* InCheckpoint) const
{
	return InCheckpoint->CheckpointIndex == TimeAttackNextCheckpointIndex;
}

void AMyPlayerController::OnCheckpointReached(ACheckpoint* InCheckpoint)
{
	if (TimeAttackFinishTime >= 0.f || InCheckpoint->CheckpointIndex != TimeAttackNextCheckpointIndex)
	{
		return;
	}

	TimeAttackNextCheckpointIndex++;
	OnCheckpointCollected.Broadcast(InCheckpoint->CheckpointIndex);

	if (TimeAttackNextCheckpointIndex < TimeAttackTotalCheckpoints)
	{
		return;
	}

	TimeAttackFinishTime = GetTimeAttackElapsedSeconds();
	checkf(CurrentLevelData.MapData, TEXT("CurrentLevelData has no MapData assigned"));
	BP_OnTimeAttackComplete(TimeAttackFinishTime, GetTimeAttackFinishTimeMilliseconds(), CurrentLevelData.MapData->MapName);

	bLocalPlayerInputDisabled = true;
}

bool AMyPlayerController::IsInitialized() const
{
	return bInitialized;
}

void AMyPlayerController::HandleLocalInputs(const int& InFrameId)
{
	auto MyReplaySubsystem = GetWorld()->GetSubsystem<UMyReplaySubsystem>();
	if (MyReplaySubsystem->IsReplayMode())
	{
		// During replays we add inputs for ALL characters here. There's no longer such thing as a local input
		for (const auto& InputsFrame : MyReplaySubsystem->GetCharactersInputs(InFrameId))
		{
			GetWorld()->GetSubsystem<UInputsSubsystem>()->GameInputs->AddConfirmedCharacterInput(
				InputsFrame.FrameId,
				InputsFrame.CharacterInput
			);
		}

		return;
	}

	if (bLocalPlayerInputDisabled)
	{
		GetWorld()->GetSubsystem<UInputsSubsystem>()->GameInputs->AddConfirmedCharacterInput(InFrameId, {
			GetMyPlayerState()->GetMyPlayerId(),
			FVector2D::ZeroVector,
			false,
			false,
			false,
			false
		});
	}
	else
	{
		GetWorld()->GetSubsystem<UInputsSubsystem>()->GameInputs->AddConfirmedCharacterInput(InFrameId, {
			GetMyPlayerState()->GetMyPlayerId(),
			GetMouseDelta(),
			bShootInputPressed,
			bResetInputPressed,
			bDashInputPressed,
			bParryInputPressed
		});
	}
}

void AMyPlayerController::SendInputsToServer(const int& InFrameId)
{
	if (GetNetMode() == NM_Client)
	{
		const auto CharacterId = GetMyPlayerState()->GetMyPlayerId();
		ServerSendInputs(
			CharacterId,
			GetWorld()->GetSubsystem<UInputsSubsystem>()->GameInputs->GetConfirmedInputs()
		);
		LOG(TEXT("[MyPlayerController] Sent local inputs to server at frame %d"), InFrameId);

		constexpr int SendGameStateHashEveryXFrames = 20;
		if (InFrameId % SendGameStateHashEveryXFrames == 0)
		{
			const int LatestConfirmedFrameId = GetWorld()->GetSubsystem<UInputsSubsystem>()->GameInputs->GetLatestConfirmedFrameId();
			TMap<int, int64> GameStateHashes = GetWorld()->GetSubsystem<UGameLoopSubsystem>()->GetGameStateHashes();
			if (GameStateHashes.Contains(LatestConfirmedFrameId))
			{
				const auto LatestGameStateHash = GameStateHashes[LatestConfirmedFrameId];													

				//LOG_NETWORKING(TEXT("[Frame %d] Send game state hash to server for frame %d, hash: %lld"), InFrameId, LatestConfirmedFrameId, LatestGameStateHash);
				ServerSendGameStateHash(LatestConfirmedFrameId, LatestGameStateHash);
			}
		}
	}
}

void AMyPlayerController::SendInputsToClients(const int& InFrameId)
{
	if (GetNetMode() == NM_ListenServer)
	{
		for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
		{
			AMyPlayerController* MyPlayerController = Cast<AMyPlayerController>(Iterator->Get());
			if (MyPlayerController && MyPlayerController->GetRemoteRole() == ROLE_AutonomousProxy)
			{
				MyPlayerController->ClientSendInputs(InFrameId, GetWorld()->GetSubsystem<UInputsSubsystem>()->GameInputs->GetConfirmedInputs());
				LOG(TEXT("[MyPlayerController] Sent confirmed inputs to client %d at frame %d"), MyPlayerController->GetMyPlayerState()->GetMyPlayerId(), InFrameId);
			}
		}
	}
}

void AMyPlayerController::StepCharactersSimulation(const int& InFrameId, const float& FixedStepTime)
{
	if (bPendingPhysicsSimulationTurnPauseOff)
	{
		bPendingPhysicsSimulationTurnPauseOff = false;
		bPhysicsSimulationPaused = false;
	}
	
	if (bPhysicsSimulationPaused)
	{
		const int FramesLeftTillMatchStart = StartMatchOnFrame - InFrameId;
		const int SecondsLeft = FMath::RoundHalfToEven(static_cast<float>(FramesLeftTillMatchStart) / 60.f); // 60 steps per second
		OnRoundCountdownUpdate.Broadcast(SecondsLeft);
		
		if (StartMatchOnFrame == InFrameId)
		{
			for (auto* JoltBodyActor : CurrentStageJoltBodyActors)
			{
				if (JoltBodyActor->ShouldSkipTweening())
				{
					continue;
				}
				
				JoltBodyActor->SetActorLocation(JoltBodyActor->GetRecordedActorLocation());
				JoltBodyActor->EnableUpdateLocationAndRotationFromPhysics();
			}
			
			OnRoundCountdownFinished.Broadcast();
			StartMatchOnFrame = -1;
			StartMatchAfterStageLoaded();
		}
		else
		{
			const float SecondsLeftFloat = static_cast<float>(FramesLeftTillMatchStart) / 60.f;
			const float CountdownPerc = FMath::Clamp(1.f - (SecondsLeftFloat / SecondsCountdownBeforeRoundStart), 0.f, 1.f);
			
			for (auto* JoltBodyActor : CurrentStageJoltBodyActors)
			{
				if (JoltBodyActor->ShouldSkipTweening())
				{
					continue;
				}
				
				const FVector RecordedActorLocation = JoltBodyActor->GetRecordedActorLocation();
				const float Alpha = PlatformsMoveIntoScreenCurve->GetFloatValue(FMath::Clamp(CountdownPerc + JoltBodyActor->RandomTweeningOffset, 0.f, 1.f));
				const FVector NewLocation = UKismetMathLibrary::VLerp(JoltBodyActor->OffsetLocation, RecordedActorLocation, Alpha);

				JoltBodyActor->SetActorLocation(NewLocation);
			}
		}
		
		return;
	}

	auto MyGameState = GetMyGameState();
	if (MyGameState->HasRoundEnded() && !bGameOver)
	{
		const int RoundEndedFramesNum = MyGameState->GetRoundEndedFramesNum();
		MyGameState->IncrementRoundEndedFramesNum();

		// At this point we want to stop the respawn logic, because it can mess with our logic by turning local inputs back on for example
		for (auto& PlayerCharacter : PlayerCharacters)
		{
			if (PlayerCharacter.DeadFramesNum > 25)
			{
				PlayerCharacter.DeadFramesNum = 25;
			}
		}

		// We disable inputs and then spin for another 30 frames to make sure all input related mispredicts have been rolled back and sorted
		// If we pause the simulation BEFORE we sort out the inputs, then there's issues with rollback, because in reality the frame ids keep going
		// It's just the physics simulation that is paused
		if (RoundEndedFramesNum == 270)
		{
			//OnStartLoading.Broadcast();
			bLocalPlayerInputDisabled = true;
			AccumulatedMouseDelta = FVector2D::ZeroVector;
			LOG_NETWORKING(TEXT("Disable inputs for local player"));
		}

		/*if (RoundEndedFramesNum >= 240)
		{
			const float SecondsLeftFloat = (300.f - RoundEndedFramesNum) / 60.f;
			const float CountdownPerc = FMath::Clamp(1.f - SecondsLeftFloat, 0.f, 1.f);
			
			for (auto* JoltBodyActor : CurrentStageJoltBodyActors)
			{
				if (JoltBodyActor->ShouldSkipTweening())
				{
					continue;
				}

				JoltBodyActor->DisableUpdateLocationAndRotationFromPhysics();
				const FVector RecordedActorLocation = JoltBodyActor->GetRecordedActorLocation();
				const float Alpha = PlatformsMoveOutOfScreenCurve->GetFloatValue(FMath::Clamp(CountdownPerc + JoltBodyActor->RandomTweeningOffset, 0.f, 1.f));
				const FVector NewLocation = UKismetMathLibrary::VLerp(RecordedActorLocation, JoltBodyActor->OffsetLocation, Alpha);

				JoltBodyActor->SetActorLocation(NewLocation);
			}
		}*/
		
		if (RoundEndedFramesNum == 300)
		{
			GetWorld()->GetSubsystem<UGameLoopSubsystem>()->DisableRollback();
			LoadNextLevel();

			RespawnPlayerCharacters();
			
			OnRender();

			return;
		}
	}
	
	if (InFrameId == 0)
	{
		LOG(TEXT("Player characters num: %d"), PlayerCharacters.Num());
	}

	for (auto& PlayerCharacter : PlayerCharacters)
	{
		// If player 1 dies first and pauses the simulation, player 2 will still run its logic, unless we break out here
		if (bPhysicsSimulationPaused)
		{
			return;
		}
		//LOG_TEMPORARY(TEXT("[Frame %d] Step character simulation for player %d"), InFrameId, PlayerCharacter.PlayerId);
		//LOG_TEMPORARY(TEXT("[Frame %d] Player %d pos %s pos hex %s"), InFrameId, PlayerCharacter.PlayerId, *GetWorld()->GetSubsystem<UJoltSubsystem>()->GetCenterOfMassPosition(BodyID(PlayerCharacter.PlayerId)).ToString(), *VecToHexString(GetWorld()->GetSubsystem<UJoltSubsystem>()->GetBodyInterface()->GetCenterOfMassPosition(BodyID(PlayerCharacter.PlayerId))));
		StepCharacterSimulation(InFrameId, PlayerCharacter, FixedStepTime);
	}
}

void AMyPlayerController::ProcessCollisions(const int& InFrameId)
{
	auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();
	auto SpawnerSubsystem = GetWorld()->GetSubsystem<USpawnerSubsystem>();
	auto SoundSubsystem = GetWorld()->GetSubsystem<USoundSubsystem>();

	/*auto& ContactsPersisted = JoltSubsystem->GetContactsPersistedFromLastStep();
	while(ContactsPersisted.Dequeue(Contact))
	{
		if (
			!SpawnerSubsystem->DoesBodyExistInSimulation(Contact.BodyID1)
			|| !SpawnerSubsystem->DoesBodyExistInSimulation(Contact.BodyID2)
		)
		{
			continue;
		}

		auto Body1Actor = SpawnerSubsystem->GetBodyActor(Contact.BodyID1);
		auto Body2Actor = SpawnerSubsystem->GetBodyActor(Contact.BodyID2);
		const auto Body1ObjectLayer = JoltSubsystem->GetObjectLayer(Contact.BodyID1.GetIndex());
		const auto Body2ObjectLayer = JoltSubsystem->GetObjectLayer(Contact.BodyID2.GetIndex());

		if (Body1Actor->BodyName == EBodyName::PlayerBody && Body2Actor->BodyName == EBodyName::ConveyorBelt)
		{
			if (AConveyorBelt* ConveyorBelt = Cast<AConveyorBelt>(Body2Actor))
			{	
				FPlayerCharacter& PlayerCharacter = GetPlayerCharacter(Body1Actor->BelongsToPlayerId);
				JoltSubsystem->GetBodyInterface()->SetLinearVelocity(BodyID(PlayerCharacter.PlayerId), ConveyorBelt->ConveyorVel);
			}
		}

		if (Body2Actor->BodyName == EBodyName::PlayerBody && Body1Actor->BodyName == EBodyName::ConveyorBelt)
		{
			if (AConveyorBelt* ConveyorBelt = Cast<AConveyorBelt>(Body1Actor))
			{
				FPlayerCharacter& PlayerCharacter = GetPlayerCharacter(Body2Actor->BelongsToPlayerId);
				JoltSubsystem->GetBodyInterface()->SetLinearVelocity(BodyID(PlayerCharacter.PlayerId), ConveyorBelt->ConveyorVel);
			}
		}

		if (Body1Actor->BodyName == EBodyName::PlayerHammer && Body2Actor->BodyName == EBodyName::ConveyorBelt)
		{
			if (AConveyorBelt* ConveyorBelt = Cast<AConveyorBelt>(Body2Actor))
			{	
				FPlayerCharacter& PlayerCharacter = GetPlayerCharacter(Body1Actor->BelongsToPlayerId);
				JoltSubsystem->GetBodyInterface()->SetLinearVelocity(BodyID(PlayerCharacter.HandBodyIndex), ConveyorBelt->ConveyorVel);
			}
		}

		if (Body2Actor->BodyName == EBodyName::PlayerHammer && Body1Actor->BodyName == EBodyName::ConveyorBelt)
		{
			if (AConveyorBelt* ConveyorBelt = Cast<AConveyorBelt>(Body1Actor))
			{
				FPlayerCharacter& PlayerCharacter = GetPlayerCharacter(Body2Actor->BelongsToPlayerId);
				JoltSubsystem->GetBodyInterface()->SetLinearVelocity(BodyID(PlayerCharacter.HandBodyIndex), ConveyorBelt->ConveyorVel);
			}
		}
	}*/

	const TArray<EObjectLayer> LayersToIgnoreHandContact = {
		EObjectLayer::Lootbox,
		EObjectLayer::PlayerSensor,
		EObjectLayer::Blackhole,
		EObjectLayer::Portal
	};

	const TArray<EBodyName> BodyTypesToIgnoreHandContact = {
		EBodyName::PracticeTarget,
	};
	
	auto ContactsAdded = JoltSubsystem->GetContactsAddedFromLastStep();
	for (const FContact& Contact : ContactsAdded)
	{
		if (
			!SpawnerSubsystem->DoesBodyExistInSimulation(Contact.BodyID1)
			|| !SpawnerSubsystem->DoesBodyExistInSimulation(Contact.BodyID2)
		)
		{
			continue;
		}
		
		auto Body1Actor = SpawnerSubsystem->GetBodyActor(Contact.BodyID1);
		auto Body2Actor = SpawnerSubsystem->GetBodyActor(Contact.BodyID2);
		const auto Body1ObjectLayer = JoltSubsystem->GetObjectLayer(Contact.BodyID1.GetIndex());
		const auto Body2ObjectLayer = JoltSubsystem->GetObjectLayer(Contact.BodyID2.GetIndex());

		if (Body1ObjectLayer == EObjectLayer::Portal || Body2ObjectLayer == EObjectLayer::Portal)
		{
			APortal* Portal = nullptr;
			BodyID ContactBodyID;
			EObjectLayer ContactLayer;
			AJoltBodyActor* ContactActor = nullptr;

			if (Body1ObjectLayer == EObjectLayer::Portal)
			{
				Portal = Cast<APortal>(Body1Actor);
				ContactBodyID = Contact.BodyID2;
				ContactLayer = Body2ObjectLayer;
				ContactActor = Body2Actor;
			}
			else
			{
				Portal = Cast<APortal>(Body2Actor);
				ContactBodyID = Contact.BodyID1;
				ContactLayer = Body1ObjectLayer;
				ContactActor = Body1Actor;
			}

			if (Portal && Portal->CanTeleportBody(ContactBodyID.GetIndex(), InFrameId))
			{
				if (Portal->DestinationPortal)
				{
					Portal->DestinationPortal->RecordTeleport(ContactBodyID.GetIndex(), InFrameId);
				}

				if (ContactLayer == EObjectLayer::PlayerTakeDamageSensor && ContactActor->BelongsToPlayerId >= 0)
				{
					const FPlayerCharacter& PlayerCharacter = GetPlayerCharacter(ContactActor->BelongsToPlayerId);
					auto BodyInterface = JoltSubsystem->GetBodyInterface();
					const Vec3 NewPos = Units::ToJoltPos(Portal->GetActorLocation() + Portal->TeleportDestination);
					BodyInterface->SetPosition(BodyID(PlayerCharacter.ShoulderBodyIndex), NewPos, EActivation::Activate);
					BodyInterface->SetPosition(BodyID(PlayerCharacter.HandBodyIndex), NewPos + Vec3(0, -10.f, 0.f), EActivation::Activate);
					BodyInterface->SetPosition(BodyID(PlayerCharacter.HiddenHandBodyIndex), NewPos + Vec3(0, -10.f, 0.f), EActivation::Activate);
					BodyInterface->SetPosition(BodyID(PlayerCharacter.PlayerId), NewPos, EActivation::Activate);
					BodyInterface->SetPosition(BodyID(PlayerCharacter.PlayerVisualRootBodyIndex), NewPos, EActivation::Activate);
					BodyInterface->SetPosition(BodyID(PlayerCharacter.TakeDamageSensorBodyIndex), NewPos, EActivation::Activate);
					for (const int& VisualBodyIndex : PlayerCharacter.PlayerVisualBodyIndexes)
					{
						BodyInterface->SetPosition(BodyID(VisualBodyIndex), NewPos, EActivation::Activate);
					}

					if (Portal->bReverseVelocityOnTeleport)
					{
						const BodyID PlayerID(PlayerCharacter.PlayerId);
						const BodyID ShoulderID(PlayerCharacter.ShoulderBodyIndex);
						const BodyID HandID(PlayerCharacter.HandBodyIndex);
						BodyInterface->SetLinearVelocity(PlayerID, -BodyInterface->GetLinearVelocity(PlayerID));
						BodyInterface->SetLinearVelocity(ShoulderID, -BodyInterface->GetLinearVelocity(ShoulderID));
						BodyInterface->SetLinearVelocity(HandID, -BodyInterface->GetLinearVelocity(HandID));
					}
				}
				else
				{
					Portal->BodyEntered(ContactBodyID);
				}
			}

			continue;
		}

		for (auto& PlayerCharacter : PlayerCharacters)
		{
			if (Body1Actor && Body1Actor->BodyName == EBodyName::OrbitalSphere && Body2ObjectLayer == EObjectLayer::PlayerTakeDamageSensor)
			{
				const Vec3 PlayerPos = JoltSubsystem->GetBodyInterface()->GetCenterOfMassPosition(Contact.BodyID2);
				PlayerExplodeDirectional(InFrameId, Body2Actor, Contact.BodyID2, Contact.ContactWorldPosition, (PlayerPos - Contact.ContactWorldPosition).NormalizedOr(Vec3::sZero()), Body1Actor->BelongsToPlayerId);
			}

			if (Body2Actor && Body2Actor->BodyName == EBodyName::OrbitalSphere && Body1ObjectLayer == EObjectLayer::PlayerTakeDamageSensor)
			{
				const Vec3 PlayerPos = JoltSubsystem->GetBodyInterface()->GetCenterOfMassPosition(Contact.BodyID1);
				PlayerExplodeDirectional(InFrameId, Body1Actor, Contact.BodyID1, Contact.ContactWorldPosition, (PlayerPos - Contact.ContactWorldPosition).NormalizedOr(Vec3::sZero()), Body2Actor->BelongsToPlayerId);
			}

			if (Body1ObjectLayer == EObjectLayer::Blade && Body2ObjectLayer == EObjectLayer::PlayerTakeDamageSensor
				&& Body1Actor->BodyName != EBodyName::OrbitalSphere)
			{
				//LOG_TEMPORARY(TEXT("[Frame %d] Player %d collide with blade"), InFrameId, PlayerCharacter.PlayerId);
				const Vec3 PlayerPos = JoltSubsystem->GetBodyInterface()->GetCenterOfMassPosition(Contact.BodyID2);
				PlayerExplodeDirectional(InFrameId, Body2Actor, Contact.BodyID2, Contact.ContactWorldPosition, (PlayerPos - Contact.ContactWorldPosition).NormalizedOr(Vec3::sZero()));
			}

			if (Body2ObjectLayer == EObjectLayer::Blade && Body1ObjectLayer == EObjectLayer::PlayerTakeDamageSensor
				&& Body2Actor->BodyName != EBodyName::OrbitalSphere)
			{
				//LOG_TEMPORARY(TEXT("[Frame %d] Player %d collide with blade"), InFrameId, PlayerCharacter.PlayerId);
				const Vec3 PlayerPos = JoltSubsystem->GetBodyInterface()->GetCenterOfMassPosition(Contact.BodyID1);
				PlayerExplodeDirectional(InFrameId, Body1Actor, Contact.BodyID1, Contact.ContactWorldPosition, (PlayerPos - Contact.ContactWorldPosition).NormalizedOr(Vec3::sZero()));
			}
			
			if (Body1ObjectLayer == EObjectLayer::FlagBase && PlayerCharacter.HandBodyIndex == Contact.BodyID2.GetIndex())
			{
				PlayerHandContactWithFlagBase(InFrameId, Body1Actor, PlayerCharacter, Contact.ContactWorldPosition);
				
				continue;
			}

			if (Body2ObjectLayer == EObjectLayer::FlagBase && PlayerCharacter.HandBodyIndex == Contact.BodyID1.GetIndex())
			{
				PlayerHandContactWithFlagBase(InFrameId, Body2Actor, PlayerCharacter, Contact.ContactWorldPosition);
				
				continue;
			}

			if (Body1ObjectLayer == EObjectLayer::PlayerSensor)
			{
				auto* Sensor = Cast<APlayerSensor>(Body1Actor);
				if (PlayerCharacter.PlayerId == Contact.BodyID2.GetIndex()
					|| (Sensor->bActivatedByHandBody && PlayerCharacter.HandBodyIndex == Contact.BodyID2.GetIndex()))
				{
					Sensor->PlayerEnter();
					continue;
				}
			}

			if (Body2ObjectLayer == EObjectLayer::PlayerSensor)
			{
				auto* Sensor = Cast<APlayerSensor>(Body2Actor);
				if (PlayerCharacter.PlayerId == Contact.BodyID1.GetIndex()
					|| (Sensor->bActivatedByHandBody && PlayerCharacter.HandBodyIndex == Contact.BodyID1.GetIndex()))
				{
					Sensor->PlayerEnter();
					continue;
				}
			}

			if (Body1ObjectLayer == EObjectLayer::PlayerTakeDamageSensor && Body2Actor->BodyName == EBodyName::Boomerang)
			{
				BoomerangHitPlayer(InFrameId, Body2Actor, Body1Actor, Contact.ContactWorldPosition);

				break;
			}
			
			if (Body2ObjectLayer == EObjectLayer::PlayerTakeDamageSensor && Body1Actor->BodyName == EBodyName::Boomerang)
			{
				BoomerangHitPlayer(InFrameId, Body1Actor, Body2Actor, Contact.ContactWorldPosition);

				break;
			}

			// Boomerang cut through projectiles
			if (
				Body1ObjectLayer == EObjectLayer::Boomerang
				&& Body2ObjectLayer == EObjectLayer::Projectile
				&& Body1Actor->BelongsToPlayerId != Body2Actor->BelongsToPlayerId
			)
			{
				BulletHit(InFrameId, Contact.BodyID2, Contact.Body2LinearVelocity, Contact);
				return;
			}

			if (
				Body1ObjectLayer == EObjectLayer::Projectile
				&& Body2ObjectLayer == EObjectLayer::Boomerang
				&& Body1Actor->BelongsToPlayerId != Body2Actor->BelongsToPlayerId
			)
			{
				BulletHit(InFrameId, Contact.BodyID1, Contact.Body1LinearVelocity, Contact);
				return;
			}
			
			if (
				Body1Actor->BodyName == EBodyName::ExplosiveBullet
				&& Body1ObjectLayer == EObjectLayer::Projectile
				&& PlayerCharacter.HandBodyIndex == Contact.BodyID2.GetIndex()
			)
			{
				auto HandVelocity = JoltSubsystem->GetBodyInterface()->GetLinearVelocity(BodyID(PlayerCharacter.HiddenVelocityBodyIndex));
				if (HandVelocity.LengthSq() > JPH::Square(100.f))
				{
					JoltSubsystem->GetBodyInterface()->SetLinearVelocity(Contact.BodyID1, HandVelocity * 4.f);
				}
				else
				{
					BulletHit(InFrameId, Contact.BodyID1, Contact.Body1LinearVelocity, Contact);
				}
			}

			if (
				Body2Actor->BodyName == EBodyName::ExplosiveBullet
				&& Body2ObjectLayer == EObjectLayer::Projectile
				&& PlayerCharacter.HandBodyIndex == Contact.BodyID1.GetIndex()
			)
			{
				auto HandVelocity = JoltSubsystem->GetBodyInterface()->GetLinearVelocity(BodyID(PlayerCharacter.HiddenVelocityBodyIndex));
				if (HandVelocity.LengthSq() > JPH::Square(100.f))
				{
					JoltSubsystem->GetBodyInterface()->SetLinearVelocity(Contact.BodyID2, HandVelocity * 4.f);
				}
				else
				{
					BulletHit(InFrameId, Contact.BodyID2, Contact.Body2LinearVelocity, Contact);
				}
			}
			
			if ((PlayerCharacter.HandBodyIndex == Contact.BodyID1.GetIndex() || PlayerCharacter.HandBodyIndex == Contact.BodyID2.GetIndex())
				&&
				PlayerCharacter.GrabbedBodyIndex != Contact.BodyID1.GetIndex() && PlayerCharacter.GrabbedBodyIndex != Contact.BodyID2.GetIndex()
			)
			{
				if (
					!LayersToIgnoreHandContact.Contains(Body1ObjectLayer) && !LayersToIgnoreHandContact.Contains(Body2ObjectLayer)
					&& !BodyTypesToIgnoreHandContact.Contains(Body1Actor->BodyName) && !BodyTypesToIgnoreHandContact.Contains(Body2Actor->BodyName)
				)
				{
					PlayerCharacter.HandContactBodyIndex = PlayerCharacter.HandBodyIndex == Contact.BodyID1.GetIndex() ? Contact.BodyID2.GetIndex() : Contact.BodyID1.GetIndex();

					FString StringToHash = FString::Printf(TEXT("HandContact%d%d"), InFrameId, PlayerCharacter.HandContactBodyIndex);
					const auto EffectHash = THashContainer::Generate(StringToHash);
					if (!UsedEffects.Contains(EffectHash))
					{
						//UGameplayStatics::PlaySound2D(GetWorld(), GeneralData->HandContactSound);
						GetWorld()->GetSubsystem<USoundSubsystem>()->PlaySound("HandContact");
						UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), GeneralData->HandContactDust, Units::FromJoltPos(Contact.ContactWorldPosition) + FVector(0, 5000.f, 0))->SetTranslucentSortPriority(124);
						/*if (PlayerCharacter.bLocalPlayer)
						{
							GameCamera->Shake();
						}*/

						AddUsedEffect(InFrameId, EffectHash);
					}
				}
			}

			/*if (PlayerCharacter.HandBodyIndex == Contact.BodyID1.GetIndex() && JoltSubsystem->GetPhysicsSystem()->GetBodyLockInterfaceNoLock().TryGetBody(Contact.BodyID2)->IsDynamic())
			{
				JoltSubsystem->GetPhysicsSystem()->GetBodyLockInterfaceNoLock().TryGetBody(Contact.BodyID2)->AddImpulse(PlayerCharacter.HammerApproximateVelocity * 20.f);
			}

			if (PlayerCharacter.HandBodyIndex == Contact.BodyID2.GetIndex() && JoltSubsystem->GetPhysicsSystem()->GetBodyLockInterfaceNoLock().TryGetBody(Contact.BodyID1)->IsDynamic())
			{
				JoltSubsystem->GetPhysicsSystem()->GetBodyLockInterfaceNoLock().TryGetBody(Contact.BodyID1)->AddImpulse(PlayerCharacter.HammerApproximateVelocity * 20.f);
			}*/

			/*if (PlayerCharacter.HandBodyIndex == Contact.BodyID1.GetIndex() && JoltSubsystem->GetPhysicsSystem()->GetBodyLockInterfaceNoLock().TryGetBody(Contact.BodyID2)->IsStatic())
			{
				JoltSubsystem->GetPhysicsSystem()->GetBodyLockInterfaceNoLock().TryGetBody(BodyID(PlayerCharacter.PlayerId))->AddImpulse(PlayerCharacter.HammerApproximateVelocity * -1000.f);
			}

			if (PlayerCharacter.HandBodyIndex == Contact.BodyID2.GetIndex() && JoltSubsystem->GetPhysicsSystem()->GetBodyLockInterfaceNoLock().TryGetBody(Contact.BodyID1)->IsStatic())
			{
				JoltSubsystem->GetPhysicsSystem()->GetBodyLockInterfaceNoLock().TryGetBody(BodyID(PlayerCharacter.PlayerId))->AddImpulse(PlayerCharacter.HammerApproximateVelocity * -1000.f);
			}*/

			if (
				(PlayerCharacter.PlayerId == Contact.BodyID1.GetIndex() || PlayerCharacter.PlayerId == Contact.BodyID2.GetIndex())
				&&
				(JoltSubsystem->GetBodyInterface()->GetMotionType(Contact.BodyID1) == EMotionType::Static || JoltSubsystem->GetBodyInterface()->GetMotionType(Contact.BodyID2) == EMotionType::Static)
			)
			{
				FJoltConstraintData ConstraintData;
				ConstraintData.BodyID1 = PlayerCharacter.PlayerId == Contact.BodyID1.GetIndex() ? Contact.BodyID2 : Contact.BodyID1;
				ConstraintData.BodyID2 = BodyID(PlayerCharacter.PlayerId);
				//const int ConstraintId = GetWorld()->GetSubsystem<USpawnerSubsystem>()->`(JavelinConstraintActorClass, JoltSubsystem->GetCenterOfMassPosition(BodyID(PlayerCharacter.PlayerId)), ConstraintData);
				//PlayerCharacter.CreatedConstraintsIds.Add(ConstraintId);
			}

			/** Player to player collisions. Dealing damage. */
			if (Body1Actor->bIsPlayer && Body2Actor->bIsPlayer && Body1Actor->BelongsToPlayerId != Body2Actor->BelongsToPlayerId)
			{
				// If only hammers collided, nobody takes damage
				if (Body1Actor->BodyName == EBodyName::PlayerHammer && Body2Actor->BodyName == EBodyName::PlayerHammer)
				{
					continue;
				}

				auto& PlayerCharacter1 = GetPlayerCharacter(Body1Actor->BelongsToPlayerId);
				auto& PlayerCharacter2 = GetPlayerCharacter(Body2Actor->BelongsToPlayerId);

				if (PlayerCharacter1.bDead || PlayerCharacter2.bDead)
				{
					continue;
				}

				/*if (PlayerCharacter1.ActiveItem == EAbility::Dash)
				{
					PlayerExplodeDirectional(InFrameId, Body2Actor, Contact.BodyID2, Contact.ContactWorldPosition, JoltSubsystem->GetBodyInterface()->GetLinearVelocity(BodyID(PlayerCharacter1.PlayerId)).Normalized());
					continue;
				}

				if (PlayerCharacter2.ActiveItem == EAbility::Dash)
				{
					PlayerExplodeDirectional(InFrameId, Body1Actor, Contact.BodyID1, Contact.ContactWorldPosition, JoltSubsystem->GetBodyInterface()->GetLinearVelocity(BodyID(PlayerCharacter2.PlayerId)).Normalized());
					continue;
				}*/

				//DrawDebugString(GetWorld(), Units::FromJoltPos(Contact.ContactWorldPosition), FString::Printf(TEXT("Vel1: %s\nVel2: %s"), *VecToString(Contact.Body1LinearVelocity), *VecToString(Contact.Body2LinearVelocity)), 0, FColor::Red, 5.f, true);
				
				/*if (Body1Actor->BodyName == EBodyName::PlayerHammer && Body2Actor->BodyName == EBodyName::PlayerBody)
				{
					const float Magnitude = (PlayerCharacter1.HammerApproximateVelocity - Contact.Body2LinearVelocity).Length();
					if (Magnitude >= 20.f)
					{
						DrawDebugString(GetWorld(), Units::FromJoltPos(Contact.ContactWorldPosition), FString::Printf(TEXT("%f"), Magnitude), 0, FColor::Red, 1.f, true);
					}
				}

				if (Body2Actor->BodyName == EBodyName::PlayerHammer && Body1Actor->BodyName == EBodyName::PlayerBody)
				{
					const float Magnitude = (PlayerCharacter2.HammerApproximateVelocity - Contact.Body1LinearVelocity).Length();
					if (Magnitude >= 20.f)
					{
						DrawDebugString(GetWorld(), Units::FromJoltPos(Contact.ContactWorldPosition), FString::Printf(TEXT("%f"), Magnitude), 0, FColor::Red, 1.f, true);
					}
				}*/
			}

			/** Add impulse to bodies when dashing into them. Currently this is bugged. It seems to apply this impulse even when not dashing */
			/*if (Body1ObjectLayer == EObjectLayer::Moving && Body2Actor->bIsPlayer)
			{
				auto& PlayerCharacter2 = GetPlayerCharacter(Body2Actor->BelongsToPlayerId);
				if (PlayerCharacter2.DashCooldownFramesLeft > 0)
				{
					const auto HitDirection = JoltSubsystem->GetBodyInterface()->GetLinearVelocity(BodyID(PlayerCharacter2.PlayerId)).Normalized();
					JoltSubsystem->GetBodyInterface()->AddImpulse(Contact.BodyID1, HitDirection * 50000.f);
				}
			}

			if (Body2ObjectLayer == EObjectLayer::Moving && Body1Actor->bIsPlayer)
			{
				auto& PlayerCharacter1 = GetPlayerCharacter(Body1Actor->BelongsToPlayerId);
				if (PlayerCharacter1.DashCooldownFramesLeft > 0)
				{
					const auto HitDirection = JoltSubsystem->GetBodyInterface()->GetLinearVelocity(BodyID(PlayerCharacter1.PlayerId)).Normalized();
					JoltSubsystem->GetBodyInterface()->AddImpulse(Contact.BodyID2, HitDirection * 50000.f);
				}
			}*/
		}

		if (Body1Actor->BodyName == EBodyName::Destructible && Body2Actor->BodyName == EBodyName::OrbitalSphere)
		{
			ADestructibleJoltBodyActor* DestructibleBodyActor = Cast<ADestructibleJoltBodyActor>(Body1Actor);
			checkf(DestructibleBodyActor, TEXT("Body of type Destructible is not using the correct class"));

			if (!DestructibleBodyActor->IsDestroyedIntoSeparateParts())
			{
				DestructibleBodyActor->DestroyAndSeparateIntoParts(Vec3::sAxisY() * DefaultProjectileImpulse, Contact.ContactWorldPosition);
			}
		}

		if (Body2Actor->BodyName == EBodyName::Destructible && Body1Actor->BodyName == EBodyName::OrbitalSphere)
		{
			ADestructibleJoltBodyActor* DestructibleBodyActor = Cast<ADestructibleJoltBodyActor>(Body2Actor);
			checkf(DestructibleBodyActor, TEXT("Body of type Destructible is not using the correct class"));

			if (!DestructibleBodyActor->IsDestroyedIntoSeparateParts())
			{
				DestructibleBodyActor->DestroyAndSeparateIntoParts(Vec3::sAxisY() * DefaultProjectileImpulse, Contact.ContactWorldPosition);
			}
		}

		if (auto Crown = Cast<ACrown>(Body1Actor))
		{
			if (Body2Actor->bIsPlayer && Body2Actor->BelongsToPlayerId > -1 && !GetPlayerCharacter(Body2Actor->BelongsToPlayerId).bDead)
			{
				Crown->TakeCrown(Body2Actor->BelongsToPlayerId);
				
				FVector EffectSpawnLocation = Units::FromJoltPos(Contact.ContactWorldPosition) + FVector(0, 10000.f, 0);
				FString StringToHash = FString::Printf(TEXT("TakeCrown%d%s"), InFrameId, *EffectSpawnLocation.ToString());
				const auto EffectHash = THashContainer::Generate(StringToHash);
				FString VoiceLineName = SoundSubsystem->ChooseRandomVoiceLineDeterministic({ "V_ImOnFire" });

				if (!UsedEffects.Contains(EffectHash))
				{
					//UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), GeneralData->TakeFlagVFX, EffectSpawnLocation)->SetTranslucentSortPriority(124);
					SpawnSpriteActor(GeneralData->PickupLootSpinnerVFX, EffectSpawnLocation);

					SoundSubsystem->PlaySound("PickupCrown");

					SoundSubsystem->PlayVoiceLine(VoiceLineName, GetPlayerRenderer(Body2Actor->BelongsToPlayerId));
				
					AddUsedEffect(InFrameId, EffectHash);
				}
				
				continue;
			}
		}

		if (auto Crown = Cast<ACrown>(Body2Actor))
		{
			if (Body1Actor->bIsPlayer && Body1Actor->BelongsToPlayerId > -1 && !GetPlayerCharacter(Body1Actor->BelongsToPlayerId).bDead)
			{
				Crown->TakeCrown(Body1Actor->BelongsToPlayerId);
				
				FVector EffectSpawnLocation = Units::FromJoltPos(Contact.ContactWorldPosition) + FVector(0, 10000.f, 0);
				FString StringToHash = FString::Printf(TEXT("TakeCrown%d%s"), InFrameId, *EffectSpawnLocation.ToString());
				const auto EffectHash = THashContainer::Generate(StringToHash);
				FString VoiceLineName = SoundSubsystem->ChooseRandomVoiceLineDeterministic({ "V_ImOnFire" });

				if (!UsedEffects.Contains(EffectHash))
				{
					SpawnSpriteActor(GeneralData->PickupLootSpinnerVFX, EffectSpawnLocation);
					SoundSubsystem->PlaySound("PickupCrown");

					SoundSubsystem->PlayVoiceLine(VoiceLineName, GetPlayerRenderer(Body1Actor->BelongsToPlayerId));
				
					AddUsedEffect(InFrameId, EffectHash);
				}
				
				continue;
			}
		}

		
		if (auto Lootbox = Cast<ALootbox>(Body1Actor))
		{
			if (Lootbox->IsAvailableForPickup() && Body2Actor->BelongsToPlayerId > -1)
			{
				Lootbox->Pickup();
					
				Vec3 PlayerPos = JoltSubsystem->GetBodyInterface()->GetCenterOfMassPosition(BodyID(Body2Actor->BelongsToPlayerId));
				const int SpawnedBodyIndex = SpawnerSubsystem->SpawnJoltBody(PlayerSpinnerClass, Units::FromJoltPos(PlayerPos), FRotator::ZeroRotator, Body2Actor->BelongsToPlayerId);
				GetPlayerCharacter(Body2Actor->BelongsToPlayerId).CreatedBodyIndexes.Add(SpawnedBodyIndex);

				if (!Lootbox->OverwritePossibleLoot.IsEmpty())
				{
					APlayerSpinner* PlayerSpinner = SpawnerSubsystem->GetBodyActor<APlayerSpinner>(SpawnedBodyIndex);
					PlayerSpinner->SetPossibleOptions(Lootbox->OverwritePossibleLoot);
				}
				
				FVector EffectSpawnLocation = Units::FromJoltPos(Contact.ContactWorldPosition) + FVector(0, 10000.f, 0);
				FString StringToHash = FString::Printf(TEXT("OpenLootbox%d%s"), InFrameId, *EffectSpawnLocation.ToString());
				const auto EffectHash = THashContainer::Generate(StringToHash);

				if (!UsedEffects.Contains(EffectHash))
				{
					SoundSubsystem->PlaySound("OpenLootbox");

					SpawnSpriteActor(GeneralData->PickupLootSpinnerVFX, EffectSpawnLocation);
					
					AddUsedEffect(InFrameId, EffectHash);
				}
			}
				
			continue;
		}

		if (auto Lootbox = Cast<ALootbox>(Body2Actor))
		{
			if (Lootbox->IsAvailableForPickup() && Body1Actor->BelongsToPlayerId > -1)
			{
				Lootbox->Pickup();
					
				Vec3 PlayerPos = JoltSubsystem->GetBodyInterface()->GetCenterOfMassPosition(BodyID(Body1Actor->BelongsToPlayerId));
				const int SpawnedBodyIndex = SpawnerSubsystem->SpawnJoltBody(PlayerSpinnerClass, Units::FromJoltPos(PlayerPos), FRotator::ZeroRotator, Body1Actor->BelongsToPlayerId);
				GetPlayerCharacter(Body1Actor->BelongsToPlayerId).CreatedBodyIndexes.Add(SpawnedBodyIndex);

				if (!Lootbox->OverwritePossibleLoot.IsEmpty())
				{
					APlayerSpinner* PlayerSpinner = SpawnerSubsystem->GetBodyActor<APlayerSpinner>(SpawnedBodyIndex);
					PlayerSpinner->SetPossibleOptions(Lootbox->OverwritePossibleLoot);
				}
				
				FVector EffectSpawnLocation = Units::FromJoltPos(Contact.ContactWorldPosition) + FVector(0, 10000.f, 0);
				FString StringToHash = FString::Printf(TEXT("OpenLootbox%d%s"), InFrameId, *EffectSpawnLocation.ToString());
				const auto EffectHash = THashContainer::Generate(StringToHash);

				if (!UsedEffects.Contains(EffectHash))
				{
					SoundSubsystem->PlaySound("OpenLootbox");

					SpawnSpriteActor(GeneralData->PickupLootSpinnerVFX, EffectSpawnLocation);
					
					AddUsedEffect(InFrameId, EffectHash);
				}
			}
				
			continue;
		}

		if (Body1ObjectLayer == EObjectLayer::Crown && Body2ObjectLayer == EObjectLayer::OutOfBounds)
		{
			//SpawnerSubsystem->DespawnJoltBody(Contact.BodyID1);
			//SpawnerSubsystem->SpawnJoltBody(CrownClass, CrownSpawnLocation, FRotator::ZeroRotator);
			JoltSubsystem->GetBodyInterface()->SetPosition(Contact.BodyID1, Units::ToJoltPos(CrownSpawnLocation), EActivation::Activate);
		}

		if (Body2ObjectLayer == EObjectLayer::Crown && Body1ObjectLayer == EObjectLayer::OutOfBounds)
		{
			//SpawnerSubsystem->DespawnJoltBody(Contact.BodyID2);
			//SpawnerSubsystem->SpawnJoltBody(CrownClass, CrownSpawnLocation, FRotator::ZeroRotator);
			JoltSubsystem->GetBodyInterface()->SetPosition(Contact.BodyID2, Units::ToJoltPos(CrownSpawnLocation), EActivation::Activate);
		}
		
		if (Body1ObjectLayer == EObjectLayer::PlayerTakeDamageSensor && Body2ObjectLayer == EObjectLayer::OutOfBounds)
		{
			//LOG_TEMPORARY(TEXT("[Frame %d] Player %d outofbounds"), InFrameId, Body1Actor->BelongsToPlayerId);
			//PlayerExplodeOmniDirectional(InFrameId, Body1Actor, Contact.BodyID1);
			if (Body2Actor->BodyName == EBodyName::Spikes)
			{
				const Vec3 PlayerPos = JoltSubsystem->GetBodyInterface()->GetCenterOfMassPosition(Contact.BodyID1);
				//PlayerExplodeDirectional(InFrameId, Body1Actor, Contact.BodyID1, Contact.ContactWorldPosition, (PlayerPos - Contact.ContactWorldPosition).NormalizedOr(Vec3::sZero()));
				PlayerExplodeDirectional(InFrameId, Body1Actor, Contact.BodyID1, Contact.ContactWorldPosition, Contact.WorldNormal);
			}
			else
			{
				if (GetPlayerCharacter(Body1Actor->BelongsToPlayerId).InvulnerabilityFramesNum > 0)
				{
					JoltSubsystem->GetBodyInterface()->AddImpulse(BodyID(Body1Actor->BelongsToPlayerId), Vec3(0, 100000.f, 0));
				}
				
				PlayerExplodeDirectional(InFrameId, Body1Actor, Contact.BodyID1, Contact.ContactWorldPosition, -JoltSubsystem->GetBodyInterface()->GetLinearVelocity(BodyID(Body1Actor->BelongsToPlayerId)).NormalizedOr(Vec3::sZero()));
			}
		}

		if (Body2ObjectLayer == EObjectLayer::PlayerTakeDamageSensor && Body1ObjectLayer == EObjectLayer::OutOfBounds)
		{
			//LOG_TEMPORARY(TEXT("[Frame %d] Player %d outofbounds"), InFrameId, Body2Actor->BelongsToPlayerId);
			//PlayerExplodeOmniDirectional(InFrameId, Body2Actor, Contact.BodyID2);

			if (Body1Actor->BodyName == EBodyName::Spikes)
			{
				const Vec3 PlayerPos = JoltSubsystem->GetBodyInterface()->GetCenterOfMassPosition(Contact.BodyID2);
				//PlayerExplodeDirectional(InFrameId, Body2Actor, Contact.BodyID2, Contact.ContactWorldPosition, (PlayerPos - Contact.ContactWorldPosition).NormalizedOr(Vec3::sZero()));
				PlayerExplodeDirectional(InFrameId, Body2Actor, Contact.BodyID2, Contact.ContactWorldPosition, Contact.WorldNormal);
			}
			else
			{
				if (GetPlayerCharacter(Body2Actor->BelongsToPlayerId).InvulnerabilityFramesNum > 0)
				{
					JoltSubsystem->GetBodyInterface()->AddImpulse(BodyID(Body2Actor->BelongsToPlayerId), Vec3(0, 100000.f, 0));
				}
				
				PlayerExplodeDirectional(InFrameId, Body2Actor, Contact.BodyID2, Contact.ContactWorldPosition, -JoltSubsystem->GetBodyInterface()->GetLinearVelocity(BodyID(Body2Actor->BelongsToPlayerId)).NormalizedOr(Vec3::sZero()));
			}
		}

		// StickyBomb sticking — must run before the generic projectile-kill check so bombs
		// attach instead of immediately exploding the player on contact.
		if (Body1Actor->BodyName == EBodyName::StickyBomb && Body2Actor->BelongsToPlayerId != Body1Actor->BelongsToPlayerId)
		{
			AStickyBomb* Bomb = Cast<AStickyBomb>(Body1Actor);
			if (!Bomb->IsStuck())
			{
				Bomb->Stick(Contact.BodyID2, Contact.ContactWorldPosition);
			}
			continue;
		}
		if (Body2Actor->BodyName == EBodyName::StickyBomb && Body1Actor->BelongsToPlayerId != Body2Actor->BelongsToPlayerId)
		{
			AStickyBomb* Bomb = Cast<AStickyBomb>(Body2Actor);
			if (!Bomb->IsStuck())
			{
				Bomb->Stick(Contact.BodyID1, Contact.ContactWorldPosition);
			}
			continue;
		}

		if (!bTurnOffDamageBetweenPlayers)
		{
			if (Body1ObjectLayer == EObjectLayer::PlayerTakeDamageSensor && Body2ObjectLayer == EObjectLayer::Projectile)
			{
				//LOG_TEMPORARY(TEXT("[Frame %d] Player %d collide with projectile %d"), InFrameId, Body1Actor->BelongsToPlayerId, Contact.BodyID2.GetIndex());
				Vec3 HitDirection = Contact.Body2LinearVelocity.NormalizedOr(Vec3::sZero());
				if (Body2Actor->BodyName == EBodyName::Minesweeper)
				{
					HitDirection = -JoltSubsystem->GetBodyInterface()->GetLinearVelocity(BodyID(Body1Actor->BelongsToPlayerId)).NormalizedOr(Vec3::sZero());
					SpawnerSubsystem->DespawnJoltBody(Contact.BodyID2);
				}
				
				PlayerExplodeDirectional(InFrameId, Body1Actor, Contact.BodyID1, Contact.ContactWorldPosition, HitDirection, Body2Actor->BelongsToPlayerId);
			}

			if (Body2ObjectLayer == EObjectLayer::PlayerTakeDamageSensor && Body1ObjectLayer == EObjectLayer::Projectile)
			{
				//LOG_TEMPORARY(TEXT("[Frame %d] Player %d collide with projectile %d"), InFrameId, Body2Actor->BelongsToPlayerId, Contact.BodyID1.GetIndex());

				Vec3 HitDirection = Contact.Body1LinearVelocity.NormalizedOr(Vec3::sZero());
				if (Body1Actor->BodyName == EBodyName::Minesweeper)
				{
					HitDirection = -JoltSubsystem->GetBodyInterface()->GetLinearVelocity(BodyID(Body2Actor->BelongsToPlayerId)).NormalizedOr(Vec3::sZero());
					SpawnerSubsystem->DespawnJoltBody(Contact.BodyID1);
				}
				
				PlayerExplodeDirectional(InFrameId, Body2Actor, Contact.BodyID2, Contact.ContactWorldPosition, HitDirection, Body1Actor->BelongsToPlayerId);
			}
		}

		if (
			Body1ObjectLayer == EObjectLayer::Sword
			&& Body2ObjectLayer == EObjectLayer::Sword
			&& Body1Actor->BelongsToPlayerId != Body2Actor->BelongsToPlayerId
		)
		{
			const BodyID Player1BodyID = BodyID(Body1Actor->BelongsToPlayerId);
			const BodyID Player2BodyID = BodyID(Body2Actor->BelongsToPlayerId);
			
			const Vec3 Player1Pos = JoltSubsystem->GetBodyInterface()->GetPosition(Player1BodyID);
			const Vec3 Player2Pos = JoltSubsystem->GetBodyInterface()->GetPosition(Player2BodyID);
			
			const Vec3 Player1KnockbackDirection = (Player1Pos - Player2Pos).NormalizedOr(Vec3::sZero());
			const Vec3 Player2KnockbackDirection = (Player2Pos - Player1Pos).NormalizedOr(Vec3::sZero());

			ASword* Sword = Cast<ASword>(Body1Actor);
			JoltSubsystem->GetBodyInterface()->AddImpulse(Player1BodyID, Player1KnockbackDirection * Sword->PlayerKnockbackOnTwoSwordsCollide);
			JoltSubsystem->GetBodyInterface()->AddImpulse(Player2BodyID, Player2KnockbackDirection * Sword->PlayerKnockbackOnTwoSwordsCollide);

			FString StringToHash = FString::Printf(TEXT("SwordsCollide%d%d%d"), InFrameId, Body1Actor->BelongsToPlayerId, Body2Actor->BelongsToPlayerId);
			const auto EffectHash = THashContainer::Generate(StringToHash);

			if (!UsedEffects.Contains(EffectHash))
			{
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), Sword->SwordsCollideVFX, Units::FromJoltPos(Contact.ContactWorldPosition));
				GetWorld()->GetSubsystem<USoundSubsystem>()->PlaySound("SwordsCollide");
			
				AddUsedEffect(InFrameId, EffectHash);
			}
		}

		if (
			Body1ObjectLayer == EObjectLayer::PlayerTakeDamageSensor
			&& Body2ObjectLayer == EObjectLayer::Sword
			&& Body1Actor->BelongsToPlayerId != Body2Actor->BelongsToPlayerId
		)
		{
			PlayerExplodeDirectional(InFrameId, Body1Actor, Contact.BodyID1, Contact.ContactWorldPosition, Contact.WorldNormal, Body2Actor->BelongsToPlayerId);
			return;
		}

		if (
			Body1ObjectLayer == EObjectLayer::Sword
			&& Body2ObjectLayer == EObjectLayer::PlayerTakeDamageSensor
			&& Body1Actor->BelongsToPlayerId != Body2Actor->BelongsToPlayerId
		)
		{
			PlayerExplodeDirectional(InFrameId, Body2Actor, Contact.BodyID2, Contact.ContactWorldPosition, Contact.WorldNormal, Body1Actor->BelongsToPlayerId);
			return;
		}

		if (
			Body1ObjectLayer == EObjectLayer::Projectile
			&& Body2ObjectLayer == EObjectLayer::Sword
		)
		{
			if (Body1Actor->BelongsToPlayerId != Body2Actor->BelongsToPlayerId)
			{
				BulletHit(InFrameId, Contact.BodyID1, Contact.Body1LinearVelocity, Contact);
			}
			
			return;
		}

		if (
			Body1ObjectLayer == EObjectLayer::Sword
			&& Body2ObjectLayer == EObjectLayer::Projectile
		)
		{
			if (Body1Actor->BelongsToPlayerId != Body2Actor->BelongsToPlayerId)
			{
				BulletHit(InFrameId, Contact.BodyID2, Contact.Body2LinearVelocity, Contact);
			}
			
			return;
		}
			

		// Projectile vs Projectile collisions
		if (
			Body1ObjectLayer == EObjectLayer::Projectile
			&& Body2ObjectLayer == EObjectLayer::Projectile
		)
		{
			BulletHit(InFrameId, Contact.BodyID1, Contact.Body1LinearVelocity, Contact);
			BulletHit(InFrameId, Contact.BodyID2, Contact.Body2LinearVelocity, Contact);
			return;
		}

		if (
			Body1Actor->BodyName == EBodyName::ExplosiveBullet
			&& Body1ObjectLayer == EObjectLayer::Projectile
			&& Body2Actor->BodyName != EBodyName::PlayerHammer
			&& !Cast<ALootbox>(Body2Actor)
		)
		{
			JoltSubsystem->GetBodyInterface()->AddImpulse(Contact.BodyID2, Contact.Body1LinearVelocity.NormalizedOr(Vec3::sZero()) * ProjectileApplyImpulseOnHit);
			BulletHit(InFrameId, Contact.BodyID1, Contact.Body1LinearVelocity, Contact);
		}

		if (
			Body2Actor->BodyName == EBodyName::ExplosiveBullet
			&& Body2ObjectLayer == EObjectLayer::Projectile
			&& Body1Actor->BodyName != EBodyName::PlayerHammer
			&& !Cast<ALootbox>(Body1Actor)
		)
		{
			JoltSubsystem->GetBodyInterface()->AddImpulse(Contact.BodyID1, Contact.Body2LinearVelocity.NormalizedOr(Vec3::sZero()) * ProjectileApplyImpulseOnHit);
			BulletHit(InFrameId, Contact.BodyID2, Contact.Body2LinearVelocity, Contact);
		}

		if (Body1ObjectLayer == EObjectLayer::Projectile)
		{
			auto ExplosiveJoltBody = Cast<AExplosiveJoltBody>(Body1Actor);
			if (ExplosiveJoltBody && ExplosiveJoltBody->ThrownByPlayerId != Body2Actor->BelongsToPlayerId)
			{
				BombExplode(InFrameId, Contact.BodyID1);
			}
		}

		if (Body2ObjectLayer == EObjectLayer::Projectile)
		{
			auto ExplosiveJoltBody = Cast<AExplosiveJoltBody>(Body2Actor);
			if (ExplosiveJoltBody && ExplosiveJoltBody->ThrownByPlayerId != Body1Actor->BelongsToPlayerId)
			{
				BombExplode(InFrameId, Contact.BodyID2);
			}
		}

		if (Body1Actor->BodyName == EBodyName::ExplosiveBarrel && Body2ObjectLayer == EObjectLayer::Projectile)
		{
			Cast<AExplosiveBarrel>(Body1Actor)->Explode(InFrameId);
		}

		if (Body2Actor->BodyName == EBodyName::ExplosiveBarrel && Body1ObjectLayer == EObjectLayer::Projectile)
		{
			Cast<AExplosiveBarrel>(Body2Actor)->Explode(InFrameId);
		}

		/*if (Body1ObjectLayer == EObjectLayer::Projectile)
		{
			if (Body1Actor->BodyName == EBodyName::Spear)
			{
				JoltSubsystem->SetObjectLayer(Contact.BodyID1.GetIndex(), EObjectLayer::Moving);

				FJoltConstraintData ConstraintData;
				ConstraintData.BodyID1 = Contact.BodyID2;
				ConstraintData.BodyID2 = Contact.BodyID1;
				SpawnerSubsystem->SpawnJoltConstraint(JavelinConstraintActorClass, Units::FromJoltPos(Contact.ContactWorldPosition), ConstraintData);
				SpawnerSubsystem->SpawnJoltConstraint(JavelinConstraintActorClass, JoltSubsystem->GetCenterOfMassPosition(Contact.BodyID1), ConstraintData);
			}
			else
			{
				JoltSubsystem->SetObjectLayer(Contact.BodyID1.GetIndex(), EObjectLayer::Pickup);
			}
			
			JoltSubsystem->GetBodyInterface()->SetCollisionGroup(Contact.BodyID1, CollisionGroup());
		}

		if (Body2ObjectLayer == EObjectLayer::Projectile)
		{
			if (Body2Actor->BodyName == EBodyName::Spear)
			{
				JoltSubsystem->SetObjectLayer(Contact.BodyID2.GetIndex(), EObjectLayer::Moving);

				FJoltConstraintData ConstraintData;
				ConstraintData.BodyID1 = Contact.BodyID1;
				ConstraintData.BodyID2 = Contact.BodyID2;
				SpawnerSubsystem->SpawnJoltConstraint(JavelinConstraintActorClass, Units::FromJoltPos(Contact.ContactWorldPosition), ConstraintData);
				SpawnerSubsystem->SpawnJoltConstraint(JavelinConstraintActorClass, JoltSubsystem->GetCenterOfMassPosition(Contact.BodyID2), ConstraintData);
			}
			else
			{
				JoltSubsystem->SetObjectLayer(Contact.BodyID2.GetIndex(), EObjectLayer::Pickup);
			}
			
			JoltSubsystem->GetBodyInterface()->SetCollisionGroup(Contact.BodyID2, CollisionGroup());		
		}*/
	}

	auto ContactsRemoved = JoltSubsystem->GetContactsRemovedFromLastStep();
	for (const FContact& Contact : ContactsRemoved)
	{
		for (auto& PlayerCharacter : PlayerCharacters)
		{
			if (
				(PlayerCharacter.HandBodyIndex == Contact.BodyID1.GetIndex() && PlayerCharacter.HandContactBodyIndex == Contact.BodyID2.GetIndex())
				|| (PlayerCharacter.HandContactBodyIndex == Contact.BodyID1.GetIndex() && PlayerCharacter.HandBodyIndex == Contact.BodyID2.GetIndex())
			)
			{
				PlayerCharacter.HandContactBodyIndex = -1;
			}

			const auto Body1ObjectLayer = JoltSubsystem->GetObjectLayer(Contact.BodyID1.GetIndex());
			const auto Body2ObjectLayer = JoltSubsystem->GetObjectLayer(Contact.BodyID2.GetIndex());

			if (Body1ObjectLayer == EObjectLayer::PlayerSensor)
			{
				auto Body1Actor = SpawnerSubsystem->GetBodyActor(Contact.BodyID1);
				auto* Sensor = Cast<APlayerSensor>(Body1Actor);
				if (PlayerCharacter.PlayerId == Contact.BodyID2.GetIndex()
					|| (Sensor->bActivatedByHandBody && PlayerCharacter.HandBodyIndex == Contact.BodyID2.GetIndex()))
				{
					Sensor->PlayerLeave();
					continue;
				}
			}

			if (Body2ObjectLayer == EObjectLayer::PlayerSensor)
			{
				auto Body2Actor = SpawnerSubsystem->GetBodyActor(Contact.BodyID2);
				auto* Sensor = Cast<APlayerSensor>(Body2Actor);
				if (PlayerCharacter.PlayerId == Contact.BodyID1.GetIndex()
					|| (Sensor->bActivatedByHandBody && PlayerCharacter.HandBodyIndex == Contact.BodyID1.GetIndex()))
				{
					Sensor->PlayerLeave();
					continue;
				}
			}
		}
	}
}

FPlayerCharacter& AMyPlayerController::GetPlayerCharacter(const int& InPlayerId)
{
	for (FPlayerCharacter& PlayerCharacter : PlayerCharacters)
	{
		if (PlayerCharacter.PlayerId == InPlayerId)
		{
			return PlayerCharacter;
		}
	}

	checkNoEntry();

	return PlayerCharacters[0];
}

FPlayerCharacter& AMyPlayerController::GetEnemyPlayerCharacter(EPlayerTeam FriendlyTeam)
{
	for (FPlayerCharacter& PlayerCharacter : PlayerCharacters)
	{
		if (PlayerCharacter.PlayerTeam != FriendlyTeam)
		{
			return PlayerCharacter;
		}
	}

	checkNoEntry();

	return PlayerCharacters[0];
}

void AMyPlayerController::GrantPlayerItem(const int& InPlayerId, ELootItem LootItem)
{
	auto SoundSubsystem = GetWorld()->GetSubsystem<USoundSubsystem>();
	
	FPlayerCharacter& PlayerCharacter = GetPlayerCharacter(InPlayerId);
	
	PlayerCharacter.AddItem(LootItem);

	const int SteppingFrameId = GetWorld()->GetSubsystem<UGameLoopSubsystem>()->GetSteppingFrameId();
	FString StringToHash = FString::Printf(TEXT("GrantPlayerAbility%d%d%d"), SteppingFrameId, InPlayerId, LootItem);
	const auto EffectHash = THashContainer::Generate(StringToHash);

	FString VoiceLineName = SoundSubsystem->ChooseRandomVoiceLineDeterministic({ "V_YouAboutToBeToast", "V_TurnUpTheHeat", "V_IFeelSoHot", "V_ImHeatingUp", "V_YourePlayingWithFire" });
	
	
	if (!UsedEffects.Contains(EffectHash))
	{
		SoundSubsystem->PlayVoiceLine(VoiceLineName, PlayerCharacter.PlayerRenderer);
		
		const FVector EffectSpawnLocation = GetWorld()->GetSubsystem<UJoltSubsystem>()->GetCenterOfMassPosition(BodyID(PlayerCharacter.PlayerId));
		
		if (!PlayerCharacter.PlayerHasAbilityAnimationActor)
		{
			PlayerCharacter.PlayerHasAbilityAnimationActor = SpawnSpriteActor(GeneralData->PlayerHasAbilityVFX, EffectSpawnLocation);
			PlayerCharacter.PlayerHasAbilityAnimationActor->AttachToActor(PlayerCharacter.PlayerRenderer, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		}
		
		AddUsedEffect(SteppingFrameId, EffectHash);
	}
}

bool AMyPlayerController::IsPlayerDead(const int& InPlayerId)
{
	return GetPlayerCharacter(InPlayerId).bDead;
}

void AMyPlayerController::SpawnGrantAbilityVFX(const FVector& EffectSpawnLocation, const int& BelongsToPlayerId)
{
	const int SteppingFrameId = GetWorld()->GetSubsystem<UGameLoopSubsystem>()->GetSteppingFrameId();
	FString StringToHash = FString::Printf(TEXT("GrantAbilityVFX%d%s"), SteppingFrameId, *EffectSpawnLocation.ToString());
	const auto EffectHash = THashContainer::Generate(StringToHash);

	if (!UsedEffects.Contains(EffectHash))
	{
		GetWorld()->GetSubsystem<USoundSubsystem>()->PlaySound("GrantAbility");
		
		AActor* SpawnedSprite = SpawnSpriteActor(UAssetManagerSubsystem::GetGeneralData(GetWorld())->GrantAbilityVFX, EffectSpawnLocation);
		SpawnedSprite->AttachToActor(GetPlayerCharacter(BelongsToPlayerId).PlayerRenderer, FAttachmentTransformRules::SnapToTargetNotIncludingScale);

		AddUsedEffect(SteppingFrameId, EffectHash);
	}
}

void AMyPlayerController::SpawnShootCannonVFX(const FVector& EffectSpawnLocation)
{
	const int SteppingFrameId = GetWorld()->GetSubsystem<UGameLoopSubsystem>()->GetSteppingFrameId();
	FString StringToHash = FString::Printf(TEXT("ShootCannon%d%s"), SteppingFrameId, *EffectSpawnLocation.ToString());
	const auto EffectHash = THashContainer::Generate(StringToHash);

	if (!UsedEffects.Contains(EffectHash))
	{
		GetWorld()->GetSubsystem<USoundSubsystem>()->PlaySound("CannonShoot");
		
		AddUsedEffect(SteppingFrameId, EffectHash);
	}
}

void AMyPlayerController::CleanupFrame(const int& InFrameId)
{
	UsedEffects.CleanupFrame(InFrameId);
}

void AMyPlayerController::StartMatchAfterStageLoaded()
{
	auto GameLoopSubsystem = GetWorld()->GetSubsystem<UGameLoopSubsystem>();
	//const int SteppingFrame = GameLoopSubsystem->GetSteppingFrameId();
	//LOG_NETWORKING(TEXT("[Frame %d] StartMatchAfterStageLoaded()"), SteppingFrame);

	GameLoopSubsystem->EnableRollback();
	bPendingPhysicsSimulationTurnPauseOff = true;
	bLocalPlayerInputDisabled = false;

	if (IsCurrentGameModeType(EGameMode::TimeAttack))
	{
		GameLoopSubsystem->DisableRollback();
		TimeAttackStartFrame = GameLoopSubsystem->GetSteppingFrameId();
	}

	
}

int AMyPlayerController::GetMaxBullets(const FPlayerCharacter& InPlayerCharacter)
{
	if (
		GetCurrentGameModeType() == EGameMode::KingOfTheCrown
		&& GetMyGameState()->GetPlayerIdThatHasCrown() == InPlayerCharacter.PlayerId
	)
	{
		return GeneralData->KingOfTheCrownHolderMaxBullets;
	}
	
	return StartingMaxBullets + InPlayerCharacter.GetItemAmount(ELootItem::MoreAmmo);
}

float AMyPlayerController::GetDefaultProjectileImpulse(const FPlayerCharacter& InPlayerCharacter) const
{
	constexpr float IncreasePercentagePerStack = 0.5f;
	const float ImpulseIncreasePerStack = DefaultProjectileImpulse * IncreasePercentagePerStack;
	return DefaultProjectileImpulse + (InPlayerCharacter.GetItemAmount(ELootItem::IncreaseProjectileSpeed) * ImpulseIncreasePerStack);
}

void AMyPlayerController::SetGameModeSettings(const FGameModeSettings& NewGameModeSettings)
{
	PreviousGameModeSettings = GameModeSettings;
	GameModeSettings = NewGameModeSettings;
}

void AMyPlayerController::InitGameMode(const FGameModeSettings& NewGameModeSettings, const FGameModeSettings& InPreviousGameModeSettings)
{
	if (NewGameModeSettings.Mode == InPreviousGameModeSettings.Mode)
	{
		return;
	}

	for (FPlayerCharacter& PlayerCharacter : PlayerCharacters)
	{
		if (NewGameModeSettings.bStartWithNoBullets)
		{
			PlayerCharacter.BulletsLeft = 0;
		}

		if (NewGameModeSettings.bStartWithNoDash)
		{
			PlayerCharacter.DashCooldownFramesLeft = DashCooldownInFrames;
		}
	}

	auto SpawnerSubsystem = GetWorld()->GetSubsystem<USpawnerSubsystem>();
	auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();

	if (InPreviousGameModeSettings.Mode == EGameMode::PullingRope || InPreviousGameModeSettings.Mode == EGameMode::RopedTogether)
	{
		if (SpawnerSubsystem->DoesConstraintExistInSimulation(RopeConstraintId))
		{
			SpawnerSubsystem->DespawnJoltConstraint(RopeConstraintId);
		}
	}

	if (NewGameModeSettings.Mode == EGameMode::Deathmatch)
	{
		bTurnOffDamageBetweenPlayers = false;
	}

	if (NewGameModeSettings.Mode == EGameMode::CaptureTheFlag)
	{
		bTurnOffDamageBetweenPlayers = false;
	}

	if (NewGameModeSettings.Mode == EGameMode::KingOfTheCrown)
	{
		bTurnOffDamageBetweenPlayers = false;
	}

	if (NewGameModeSettings.Mode == EGameMode::RopedTogether)
	{
		bTurnOffDamageBetweenPlayers = true;
		
		const FVector ConstraintSpawnLocation = JoltSubsystem->GetCenterOfMassPosition(BodyID(PlayerCharacters[0].PlayerId));
		FJoltConstraintData ConstraintData;
		ConstraintData.BodyID1 = BodyID(PlayerCharacters[0].PlayerId);
		ConstraintData.BodyID2 = BodyID(PlayerCharacters[1].PlayerId);
		RopeConstraintId = SpawnerSubsystem->SpawnJoltConstraint(PlayerToPlayerRopeConstraintClass, ConstraintSpawnLocation, ConstraintData);
	}

	if (NewGameModeSettings.Mode == EGameMode::PullingRope)
	{
		bTurnOffDamageBetweenPlayers = true;
		
		const FVector ConstraintSpawnLocation = JoltSubsystem->GetCenterOfMassPosition(BodyID(PlayerCharacters[0].PlayerId));
		FJoltConstraintData ConstraintData;
		ConstraintData.BodyID1 = BodyID(PlayerCharacters[0].PlayerId);
		ConstraintData.BodyID2 = BodyID(PlayerCharacters[1].PlayerId);
		RopeConstraintId = SpawnerSubsystem->SpawnJoltConstraint(PullingRopeConstraintClass, ConstraintSpawnLocation, ConstraintData);
	}

	if (NewGameModeSettings.Mode == EGameMode::Lobby)
	{
		bTurnOffDamageBetweenPlayers = false;
	}
}

void AMyPlayerController::TearDownCurrentLevel()
{
	auto SpawnerSubsystem = GetWorld()->GetSubsystem<USpawnerSubsystem>();

	for (FPlayerCharacter& PlayerCharacter : PlayerCharacters)
	{
		for (const int& PlayerCreatedBodyIndex : PlayerCharacter.CreatedBodyIndexes)
		{
			BodyID PlayerCreatedBodyID = BodyID(PlayerCreatedBodyIndex);
			if (SpawnerSubsystem->DoesBodyExistInSimulation(PlayerCreatedBodyID))
			{
				SpawnerSubsystem->DespawnJoltBodyWithRelatedConstraints(PlayerCreatedBodyID);
			}
		}

		PlayerCharacter.CreatedBodyIndexes.Empty();
	}

	for (AJoltBodyActor* JoltBodyActor : CurrentStageJoltBodyActors)
	{
		if (!IsValid(JoltBodyActor))
		{
			continue;
		}

		BodyID JoltBodyID = JoltBodyActor->GetBodyID();
		if (SpawnerSubsystem->DoesBodyExistInSimulation(JoltBodyID))
		{
			SpawnerSubsystem->DespawnJoltBodyWithRelatedConstraints(JoltBodyID);
		}
	}

	TArray<AActor*> StaticMeshActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AStaticMeshActor::StaticClass(), StaticMeshActors);
	for (AActor* StaticMeshActor : StaticMeshActors)
	{
		if (StaticMeshActor->ActorHasTag(TEXT("Persistent")))
		{
			continue;
		}

		StaticMeshActor->Destroy();
	}

	if (GameBackground)
	{
		GameBackground->Destroy();
		GameBackground = nullptr;
	}

	if (GameCameraLimits)
	{
		GameCameraLimits->Destroy();
		GameCameraLimits = nullptr;
	}

	TimeAttackFinishTime = -1.f;
	TimeAttackStartFrame = -1;
	TimeAttackNextCheckpointIndex = 0;
	TimeAttackTotalCheckpoints = 0;

	CurrentStageJoltBodyActors.Empty();
	CrownSpawnLocation = FVector::ZeroVector;

	if (IsValid(LightSource))
	{
		LightSource->Destroy();
	}
	DestroySpawnLocationActors();
	GEngine->ForceGarbageCollection(true);
}

void AMyPlayerController::LoadCurrentLevelImpl()
{
	LOG_NETWORKING(TEXT("Before call GetNextLevel()."));
	checkf(CurrentLevelData.MapData, TEXT("LoadCurrentLevelImpl has no MapData assigned"));
	FString PackageName = FPackageName::ObjectPathToPackageName(CurrentLevelData.MapData->WorldPtr.ToString());

	UPackage* LevelPackage = LoadPackage(nullptr, *PackageName, LOAD_None);
	checkf(LevelPackage, TEXT("LevelPackage is null"));

	UWorld* NextWorld = UWorld::FindWorldInPackage(LevelPackage);
	checkf(NextWorld, TEXT("NextWorld is null"));

	LOG_NETWORKING(TEXT("Next level actors list num %d"), NextWorld->PersistentLevel->Actors.Num());

	const bool bIsTimeAttack = GameModeSettings.Mode == EGameMode::TimeAttack;

	for (auto LevelActor : NextWorld->PersistentLevel->Actors)
	{
		if (!LevelActor)
		{
			continue;
		}

		if (Cast<AWorldSettings>(LevelActor))
		{
			continue;
		}

		if (Cast<ALevelScriptActor>(LevelActor))
		{
			continue;
		}

		if (bIsTimeAttack && LevelActor->IsA<ACrown>()) continue;
		if (bIsTimeAttack && LevelActor->IsA<AFlagBase>()) continue;
		if (bIsTimeAttack && LevelActor->IsA<APlayerSpinner>()) continue;
		if (bIsTimeAttack && LevelActor->IsA<ALootbox>()) continue;

		if (!bIsTimeAttack && LevelActor->IsA<ACheckpoint>()) continue;

		LOG_NETWORKING(TEXT("LevelActor: %s"), *LevelActor->GetName());

		FObjectDuplicationParameters DuplicationParameters(LevelActor, GetWorld());
		AActor* Template = Cast<AActor>(
			StaticDuplicateObjectEx(DuplicationParameters)
		);

		LOG_NETWORKING(TEXT("Called StaticDuplicateObjectEx"));

		FActorSpawnParameters Params;
		Params.Template = Template;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		AActor* SpawnedActor = GetWorld()->SpawnActor<AActor>(
			Template->GetClass(),
			Template->GetActorTransform(),
			Params
		);

		LOG_NETWORKING(TEXT("Called SpawnActor"));

		LOG_NETWORKING(TEXT("Check if player spawn"));

		if (AGameBackground* FoundGameBackground = Cast<AGameBackground>(SpawnedActor))
		{
			GameBackground = FoundGameBackground;
		}

		if (APlayerSpawn* PlayerSpawn = Cast<APlayerSpawn>(SpawnedActor))
		{
			PossibleSpawns.Add(PlayerSpawn);
			if (PlayerSpawn->bMirror)
			{
				APlayerSpawn* MirroredPlayerSpawn = SpawnMirroredActor<APlayerSpawn>(PlayerSpawn);
				MirroredPlayerSpawn->SpawnIndex++;
				PossibleSpawns.Add(MirroredPlayerSpawn);
			}
		}

		LOG_NETWORKING(TEXT("Check if light source"));
		if (Cast<ALightSource>(SpawnedActor))
		{
			LightSource = Cast<ALightSource>(SpawnedActor);
		}

		if (ACrown* Crown = Cast<ACrown>(SpawnedActor))
		{
			CrownSpawnLocation = Crown->GetActorLocation();
		}

		if (bIsTimeAttack && Cast<ACheckpoint>(SpawnedActor))
		{
			TimeAttackTotalCheckpoints++;
		}

		Template->Destroy();
		LOG_NETWORKING(TEXT("Called Destroy on Template actor"));
	}

	LOG_NETWORKING(TEXT("PossibleSpawns num %d"), PossibleSpawns.Num());

	checkf(!PossibleSpawns.IsEmpty(), TEXT("PossibleSpawns is empty"));

	GameCameraLimits = Cast<AGameCameraLimits>(UGameplayStatics::GetActorOfClass(GetWorld(), AGameCameraLimits::StaticClass()));

	InitializeJoltBodiesAndConstraints();

	for (auto* JoltBodyActor : CurrentStageJoltBodyActors)
	{
		if (JoltBodyActor->ShouldSkipTweening())
		{
			continue;
		}

		JoltBodyActor->DisableUpdateLocationAndRotationFromPhysics();
		JoltBodyActor->RecordActorLocation();

		FVector DirectionAwayFromLevelCenter = (JoltBodyActor->GetActorLocation() - FVector(100000.f, 0, 80000.f)).GetSafeNormal();
		JoltBodyActor->OffsetLocation = JoltBodyActor->GetActorLocation() + DirectionAwayFromLevelCenter * DistanceAwayFromTargetPositionForTweening;
		JoltBodyActor->SetActorLocation(JoltBodyActor->OffsetLocation);

		JoltBodyActor->RandomTweeningOffset = UKismetMathLibrary::RandomFloat() * TweeningAlphaVariation;
	}

	if (CurrentLevelData.LevelLoadSettings.bPlayLevelIntroSound)
	{
		GetWorld()->GetSubsystem<USoundSubsystem>()->PlaySound("LevelIntro");
	}

	UHelper::GetMyGameState(GetWorld())->ClearRoundData();

	for (FPlayerCharacter& PlayerCharacter : PlayerCharacters)
	{
		PlayerCharacter.CrownHeldFramesNum = 0;
	}
	CrownModeEndStep = 0;

	bPhysicsSimulationPaused = true;
	bLocalPlayerInputDisabled = true;
	StartMatchOnFrame = GetWorld()->GetSubsystem<UGameLoopSubsystem>()->GetSteppingFrameId() + CurrentLevelData.LevelLoadSettings.TweenLevelFramesNum;
	SecondsCountdownBeforeRoundStart = static_cast<float>(CurrentLevelData.LevelLoadSettings.TweenLevelFramesNum) / 60.f;
}

void AMyPlayerController::BP_ReloadCurrentLevel()
{
	ReloadCurrentLevel();
}

void AMyPlayerController::ReloadCurrentLevel()
{
	TearDownCurrentLevel();
	LoadCurrentLevelImpl();
	// When resetting current level in time attack we want it to happen fast, unlike on the first load. Cause otherwise it's annoying
	if (IsCurrentGameModeType(EGameMode::TimeAttack))
	{
		StartMatchOnFrame = GetWorld()->GetSubsystem<UGameLoopSubsystem>()->GetSteppingFrameId() + 60;
		SecondsCountdownBeforeRoundStart = 1.f;
	}
	
	RespawnPlayerCharacters();
	if (GameCamera)
	{
		ReinitGameCamera();
	}
	OnNewLevelLoaded.Broadcast();
}

void AMyPlayerController::LoadNextLevel()
{
	LOG_NETWORKING(TEXT("[LoadNextLevel]"));

	if (CurrentLevelIndex > 0)
	{
		LOG_NETWORKING(TEXT("Already loaded level exists. Going to despawn all actors."));
		TearDownCurrentLevel();
	}

	LOG_NETWORKING(TEXT("Before call GetNextLevel()."));
	checkf(LevelsSequence.IsValidIndex(CurrentLevelIndex), TEXT("Tried to access non-existing level index"));

	CurrentLevelData = LevelsSequence[CurrentLevelIndex];

	LOG_NETWORKING(TEXT("GetNextLevel() = %s"), *CurrentLevelData.MapData->WorldPtr.ToString());

	LoadCurrentLevelImpl();

	CurrentGameModeTeamScores = FTeamScores();
	OnCurrentGameModeTeamScoreChanged.Broadcast(CurrentGameModeTeamScores);

	if (GameCamera)
	{
		SetGameModeSettings(LevelsSequence[CurrentLevelIndex].GameModeSettings);
	}

	RespawnPlayerCharacters();

	if (GameCamera)
	{
		InitGameMode(GameModeSettings, PreviousGameModeSettings);
		ReinitGameCamera();
	}

	CurrentLevelIndex++;

	OnNewLevelLoaded.Broadcast();
}

void AMyPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);

	EnhancedInputComponent->BindAction(InputShoot, ETriggerEvent::Triggered, this, &AMyPlayerController::OnShootInputStarted);
	EnhancedInputComponent->BindAction(InputShoot, ETriggerEvent::Canceled, this, &AMyPlayerController::OnShootInputFinished);
	EnhancedInputComponent->BindAction(InputShoot, ETriggerEvent::Completed, this, &AMyPlayerController::OnShootInputFinished);

	EnhancedInputComponent->BindAction(InputReset, ETriggerEvent::Triggered, this, &AMyPlayerController::OnResetInputStarted);
	EnhancedInputComponent->BindAction(InputReset, ETriggerEvent::Canceled, this, &AMyPlayerController::OnResetInputFinished);
	EnhancedInputComponent->BindAction(InputReset, ETriggerEvent::Completed, this, &AMyPlayerController::OnResetInputFinished);

	EnhancedInputComponent->BindAction(InputDash, ETriggerEvent::Triggered, this, &AMyPlayerController::OnDashInputStarted);
	EnhancedInputComponent->BindAction(InputDash, ETriggerEvent::Canceled, this, &AMyPlayerController::OnDashInputFinished);
	EnhancedInputComponent->BindAction(InputDash, ETriggerEvent::Completed, this, &AMyPlayerController::OnDashInputFinished);

	EnhancedInputComponent->BindAction(InputParry, ETriggerEvent::Triggered, this, &AMyPlayerController::OnParryInputStarted);
	EnhancedInputComponent->BindAction(InputParry, ETriggerEvent::Canceled, this, &AMyPlayerController::OnParryInputFinished);
	EnhancedInputComponent->BindAction(InputParry, ETriggerEvent::Completed, this, &AMyPlayerController::OnParryInputFinished);
}

void AMyPlayerController::OnShootInputStarted(const FInputActionValue& InputActionValue)
{
	bShootInputPressed = true;
}

void AMyPlayerController::OnShootInputFinished(const FInputActionValue& InputActionValue)
{
	bShootInputPressed = false;
}

void AMyPlayerController::OnResetInputStarted(const FInputActionValue& InputActionValue)
{
	bResetInputPressed = true;
}

void AMyPlayerController::OnResetInputFinished(const FInputActionValue& InputActionValue)
{
	bResetInputPressed = false;
}

void AMyPlayerController::OnDashInputStarted(const FInputActionValue& InputActionValue)
{
	bDashInputPressed = true;
}

void AMyPlayerController::OnDashInputFinished(const FInputActionValue& InputActionValue)
{
	bDashInputPressed = false;
}

void AMyPlayerController::OnParryInputStarted(const FInputActionValue& InputActionValue)
{
	bParryInputPressed = true;
}

void AMyPlayerController::OnParryInputFinished(const FInputActionValue& InputActionValue)
{
	bParryInputPressed = false;
}

TArray<FLevelData> AMyPlayerController::BuildLevelsSequenceForMatch() const
{
	bool BuildRandomSequence = true;
#if WITH_EDITOR
	BuildRandomSequence = GeneralData->bBuildRandomLevelSequenceFromPool;
#endif
	
	if (BuildRandomSequence)
	{
		auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();
		
		// The maximum number of levels we might need in the sequence is points needed to win * 2. But it doesn't mean all of them will be used.
		const int AmountOfLevels = GeneralData->PointsNeededToWinTheMatch * 2;
		
		TArray<FLevelData> RandomLevelsSequence;
		TArray<FLevelData> LevelsPool = GeneralData->LevelsPool;
		int RandomNumberGenerationsNum = 0;
		for (int i = 0; i < AmountOfLevels; i++)
		{
			const int RandomIndex = JoltSubsystem->RandomNumberGenerator.NextIntInRange(0, LevelsPool.Num() - 1);
			RandomLevelsSequence.Add(LevelsPool[RandomIndex]);
			RandomNumberGenerationsNum++;

			LevelsPool.RemoveAt(RandomIndex);
			// This should really only happen in testing, because otherwise it means we don't have enough levels to finish the game potentially. Should not be possible.
			if (LevelsPool.IsEmpty())
			{
				break;
			}
		}

		GetWorld()->GetSubsystem<UMyReplaySubsystem>()->SaveInitialRNGSkip(RandomNumberGenerationsNum);
		
		return RandomLevelsSequence;
	}

	return GeneralData->LevelsData;
}

TArray<FLevelData> AMyPlayerController::BuildLevelsSequenceForTutorial()
{
	int CurrentTutorialLevel = GetCurrentGameSave()->CurrentTutorialLevel;
	if (CurrentTutorialLevel > GeneralData->TutorialLevels.Num())
	{
		CurrentTutorialLevel = 0;
		GetCurrentGameSave()->CurrentTutorialLevel = 0;
	}
	
#if WITH_EDITOR
	if (GeneralData->bDebugTutorial)
	{
		CurrentTutorialLevel = GeneralData->DebugTutorialLevelIndex;
	}
#endif

	TArray<FLevelData> RemainingLevels;
	for (int i = 0; i < GeneralData->TutorialLevels.Num(); i++)
	{
		if (i < CurrentTutorialLevel)
		{
			continue;
		}

		RemainingLevels.Add(GeneralData->TutorialLevels[i]);
	}
	
	return RemainingLevels;
}

TArray<FLevelData> AMyPlayerController::BuildLevelsSequenceForTimeAttack()
{
	checkf(!GeneralData->TimeAttackLevels.IsEmpty(), TEXT("No time attack levels configured in GeneralData"));

	const int32 LevelIndex = UHelper::GetMyGameInstance(GetWorld())->CurrentTimeAttackLevelIndex;
	checkf(GeneralData->TimeAttackLevels.IsValidIndex(LevelIndex), TEXT("CurrentTimeAttackLevelIndex %d is out of range"), LevelIndex);

	return { GeneralData->TimeAttackLevels[LevelIndex] };
}

UMyGameSave* AMyPlayerController::GetCurrentGameSave()
{
	return UHelper::GetMyGameInstance(GetWorld())->GetCurrentGameSave();
}

void AMyPlayerController::ReinitGameCamera()
{
	FVector PlayersAverageLocation = FVector::ZeroVector;
	for (const auto& PlayerCharacter : PlayerCharacters)
	{
		PlayersAverageLocation += PlayerCharacter.RenderData.BodyLocation;
	}
	PlayersAverageLocation /= PlayerCharacters.Num();
	PlayersAverageLocation.Y = 35000.f;
	PlayersAverageLocation.Z += 500.f;
	GameCamera->InitGameCamera(PlayersAverageLocation, GameBackground);
}

void AMyPlayerController::RespawnPlayerCharacters()
{
	for (FPlayerCharacter& PlayerCharacter : PlayerCharacters)
	{
		ResetPlayerCharacterToStartingPosition(PlayerCharacter);

#if WITH_EDITOR
		if (PlayerCharacter.PlayerIndex == 0 && GeneralData->StartPlayerOneWithAbility != ELootItem::None)
		{
			PlayerCharacter.AddItem(GeneralData->StartPlayerOneWithAbility);
		}

		if (PlayerCharacter.PlayerIndex == 1 && GeneralData->StartPlayerTwoWithAbility != ELootItem::None)
		{
			PlayerCharacter.AddItem(GeneralData->StartPlayerTwoWithAbility);
		}
#endif
	}
}

void AMyPlayerController::ResetPlayerCharacterToStartingPosition(FPlayerCharacter& PlayerCharacter)
{
	auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();
	
	PlayerCharacter.CursorPosition = FVector2D(0, 0.f);
	PlayerCharacter.BulletReloadCooldownFramesLeft = 0;
	PlayerCharacter.ShootingCooldownFramesLeft = 0;
	PlayerCharacter.DashCooldownFramesLeft = 0;
	PlayerCharacter.BulletsLeft = 3;
	PlayerCharacter.ShootButtonHeldFramesNum = 0;
	PlayerCharacter.HandContactBodyIndex = -1;
	PlayerCharacter.GrabbedBodyIndex = -1;

	if (PlayerCharacter.GrappleRopeConstraintId != -1)
	{
		auto SpawnerSubsystem = GetWorld()->GetSubsystem<USpawnerSubsystem>();
		if (SpawnerSubsystem->DoesConstraintExistInSimulation(PlayerCharacter.GrappleRopeConstraintId))
		{
			SpawnerSubsystem->DespawnJoltConstraint(PlayerCharacter.GrappleRopeConstraintId);
		}
		PlayerCharacter.GrappleRopeConstraintId = -1;
		PlayerCharacter.GrappleCurrentMaxDistance = -1.f;
	}

	PlayerCharacter.bDead = false;
	PlayerCharacter.DeadFramesNum = 0;
	PlayerCharacter.DeadFramesCurrentStep = 0;
	PlayerCharacter.DeadOnFrameId = -1;
	PlayerCharacter.ActiveItem = ELootItem::None;
	PlayerCharacter.ItemActiveForFramesNum = 0;
	PlayerCharacter.CarryingFlagBodyIndex = -1;
	PlayerCharacter.RemoveAllItems();

	CleanupActiveSwordBody(GetWorld()->GetSubsystem<UGameLoopSubsystem>()->GetSteppingFrameId(), PlayerCharacter);

	if (PlayerCharacter.bLocalPlayer)
	{
		AccumulatedMouseDelta = FVector2D::ZeroVector;
	}
	
	APlayerSpawn* PlayerSpawn = GetPlayerSpawn(PlayerCharacter.PlayerIndex, PossibleSpawns);
	PlayerSpawn->Init(PlayerCharacter.PlayerId);
	const FVector CharacterSpawnLocation = PlayerSpawn->GetActorLocation();
	PlayerCharacter.PlayerRenderer->SetActorLocation(CharacterSpawnLocation);

	Vec3 NewPlayerPos = Units::ToJoltPos(CharacterSpawnLocation);
	auto BodyInterface = JoltSubsystem->GetBodyInterface();
	BodyInterface->SetPosition(BodyID(PlayerCharacter.ShoulderBodyIndex), NewPlayerPos, EActivation::DontActivate);
	BodyInterface->SetPosition(BodyID(PlayerCharacter.HandBodyIndex), NewPlayerPos + Vec3(0, -10.f, 0.f), EActivation::DontActivate);
	BodyInterface->SetPosition(BodyID(PlayerCharacter.HiddenHandBodyIndex), NewPlayerPos + Vec3(0, -10.f, 0.f), EActivation::DontActivate);
	BodyInterface->SetPosition(BodyID(PlayerCharacter.PlayerId), NewPlayerPos, EActivation::DontActivate);
	BodyInterface->SetPosition(BodyID(PlayerCharacter.PlayerVisualRootBodyIndex), NewPlayerPos, EActivation::DontActivate);
	BodyInterface->SetPosition(BodyID(PlayerCharacter.TakeDamageSensorBodyIndex), NewPlayerPos, EActivation::DontActivate);


	BodyInterface->SetLinearAndAngularVelocity(BodyID(PlayerCharacter.ShoulderBodyIndex), Vec3::sZero(), Vec3::sZero());
	BodyInterface->SetLinearAndAngularVelocity(BodyID(PlayerCharacter.HandBodyIndex), Vec3::sZero(), Vec3::sZero());
	BodyInterface->SetLinearAndAngularVelocity(BodyID(PlayerCharacter.PlayerId), Vec3::sZero(), Vec3::sZero());
	BodyInterface->SetLinearAndAngularVelocity(BodyID(PlayerCharacter.PlayerVisualRootBodyIndex), Vec3::sZero(), Vec3::sZero());
	BodyInterface->SetLinearAndAngularVelocity(BodyID(PlayerCharacter.TakeDamageSensorBodyIndex), Vec3::sZero(), Vec3::sZero());
	BodyInterface->SetLinearAndAngularVelocity(BodyID(PlayerCharacter.HiddenHandBodyIndex), Vec3::sZero(), Vec3::sZero());
	for (const int& BodyIndex : PlayerCharacter.PlayerVisualBodyIndexes)
	{
		JoltSubsystem->GetBodyInterface()->SetPosition(BodyID(BodyIndex), NewPlayerPos, EActivation::DontActivate);
		JoltSubsystem->GetBodyInterface()->SetLinearAndAngularVelocity(BodyID(BodyIndex), Vec3::sZero(), Vec3::sZero());
	}
	PlayerCharacter.RenderData.BodyLocation = CharacterSpawnLocation;
}

void AMyPlayerController::IncreaseOppositeTeamsScore(EPlayerTeam PlayerTeam)
{
	EPlayerTeam WinningTeam = FTeamScores::GetOppositeTeam(PlayerTeam);

	checkf(WinningTeam != EPlayerTeam::None, TEXT("Winning team is none"));
	
	IncreaseTeamsScore(WinningTeam);
}

void AMyPlayerController::IncreaseCurrentGameModeOppositeTeamsScore(EPlayerTeam PlayerTeam)
{
	EPlayerTeam WinningTeam = FTeamScores::GetOppositeTeam(PlayerTeam);

	checkf(WinningTeam != EPlayerTeam::None, TEXT("Winning team is none"));
	
	CurrentGameModeTeamScores.IncreaseScore(WinningTeam);
	OnCurrentGameModeTeamScoreChanged.Broadcast(CurrentGameModeTeamScores);

	if (GameModeSettings.Mode == EGameMode::Deathmatch)
	{
		if (CurrentGameModeTeamScores.GetTeamScore(WinningTeam) == GeneralData->DeathmatchMaxPoints)
		{
			IncreaseOppositeTeamsScore(PlayerTeam);
		}
	}

	if (GameModeSettings.Mode == EGameMode::CaptureTheFlag)
	{
		if (CurrentGameModeTeamScores.GetTeamScore(WinningTeam) == GeneralData->CaptureTheFlagMaxPoints)
		{
			IncreaseOppositeTeamsScore(PlayerTeam);
		}
	}
}

void AMyPlayerController::IncreaseTeamsScore(EPlayerTeam WinningTeam)
{
	TeamScores.IncreaseScore(WinningTeam);
	OnTeamScoresChanged.Broadcast(TeamScores);

	if (TeamScores.GetTeamScore(WinningTeam) == GeneralData->PointsNeededToWinTheMatch)
	{
		OnTeamWinsWholeGame.Broadcast(WinningTeam);
		bGameOver = true;
	}
	else
	{
		OnTeamWin.Broadcast(WinningTeam);
	}
}

void AMyPlayerController::DevelopmentIncreaseTeamScore(EPlayerTeam InPlayerTeam)
{
	IncreaseTeamsScore(InPlayerTeam);
}

void AMyPlayerController::DevelopmentIncreaseCurrentGameModeScore(EPlayerTeam InPlayerTeam)
{
	IncreaseCurrentGameModeOppositeTeamsScore(FTeamScores::GetOppositeTeam(InPlayerTeam));

	if (GetCurrentGameModeTeamScores().GetTeamScore(InPlayerTeam) == UAssetManagerSubsystem::GetGeneralData(GetWorld())->CaptureTheFlagMaxPoints)
	{
		UHelper::GetMyGameState(GetWorld())->EndRound();
	}
}

void AMyPlayerController::CompleteCurrentTutorialLevel()
{
	GetCurrentGameSave()->CurrentTutorialLevel++;
	UHelper::GetMyGameInstance(GetWorld())->SaveGame();
	
	LoadNextLevel();
	SetGameModeSettings(CurrentLevelData.GameModeSettings);
	InitGameMode(GameModeSettings, PreviousGameModeSettings);
}

void AMyPlayerController::FinishTutorials()
{
	GetCurrentGameSave()->bTutorialCompleted = true;
	UHelper::GetMyGameInstance(GetWorld())->SaveGame();
}

int AMyPlayerController::GetRollbackWindow() const
{
	return RollbackWindow;
}

int AMyPlayerController::GetHoldCrownFramesToWin() const
{
	if (GameModeSettings.Mode == EGameMode::Tutorial)
	{
		return 300;
	}
	
	return GeneralData->HoldCrownFramesToWin;
}

void AMyPlayerController::DestroySpawnLocationActors()
{
	for (AActor* PossibleSpawn : PossibleSpawns)
	{
		LOG_NETWORKING(TEXT("Destroy possible spawn location %s"), *PossibleSpawn->GetName());
		PossibleSpawn->Destroy();
	}

	PossibleSpawns.Empty();
}

void AMyPlayerController::MarkPlayerCharacterAsDead(const int& InFrameId, FPlayerCharacter& PlayerCharacter)
{
	//LOG_TEMPORARY(TEXT("[Frame %d] Mark player dead %d"), InFrameId, PlayerCharacter.PlayerId);
	
	auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();
	auto SpawnerSubsystem = GetWorld()->GetSubsystem<USpawnerSubsystem>();
	
	if (GameModeSettings.Mode == EGameMode::CaptureTheFlag)
	{
		if (PlayerCharacter.CarryingFlagBodyIndex > -1 && SpawnerSubsystem->DoesBodyExistInSimulation(BodyID(PlayerCharacter.CarryingFlagBodyIndex)))
		{
			Cast<AFlag>(SpawnerSubsystem->GetBodyActor(PlayerCharacter.CarryingFlagBodyIndex))->ReturnHome();
		}
	}

	if (GameModeSettings.Mode == EGameMode::PullingRope)
	{
		if (SpawnerSubsystem->DoesConstraintExistInSimulation(RopeConstraintId))
		{
			SpawnerSubsystem->DespawnJoltConstraint(RopeConstraintId);
		}
	}

	if (GameModeSettings.Mode == EGameMode::KingOfTheCrown)
	{
		auto MyGameState = GetMyGameState();
		if (MyGameState->GetPlayerIdThatHasCrown() == PlayerCharacter.PlayerId)
		{
			const int SpawnedBodyIndex = SpawnerSubsystem->SpawnJoltBody(CrownClass, CrownSpawnLocation, FRotator::ZeroRotator);
			CurrentStageJoltBodyActors.Add(SpawnerSubsystem->GetBodyActor(SpawnedBodyIndex));
			MyGameState->SetPlayerIdThatHasCrown(-1);

			// Spawn crown where the player was killed
			/*const int SpawnedBodyIndex = SpawnerSubsystem->SpawnJoltBody(CrownClass, JoltSubsystem->GetBodyLocation(BodyID(PlayerCharacter.PlayerId)), FRotator::ZeroRotator);
			CurrentStageJoltBodyActors.Add(SpawnerSubsystem->GetBodyActor(SpawnedBodyIndex));
			JoltSubsystem->GetBodyInterface()->AddImpulse(BodyID(SpawnedBodyIndex), Vec3(0, CrownImpulseAfterBeingDropped, 0));
			MyGameState->SetPlayerIdThatHasCrown(-1);*/
		}
	}

	if (PlayerCharacter.GrabbedBodyIndex > 0)
	{
		if (SpawnerSubsystem->DoesBodyExistInSimulation(PlayerCharacter.GrabbedBodyIndex))
		{
			SpawnerSubsystem->DespawnJoltBodyWithRelatedConstraints(PlayerCharacter.GrabbedBodyIndex);
		}
		
		PlayerCharacter.GrabbedBodyIndex = -1;
	}

	CleanupActiveSwordBody(InFrameId, PlayerCharacter);

	if (PlayerCharacter.GrappleRopeConstraintId != -1)
	{
		if (SpawnerSubsystem->DoesConstraintExistInSimulation(PlayerCharacter.GrappleRopeConstraintId))
		{
			SpawnerSubsystem->DespawnJoltConstraint(PlayerCharacter.GrappleRopeConstraintId);
		}
		PlayerCharacter.GrappleRopeConstraintId = -1;
		PlayerCharacter.GrappleCurrentMaxDistance = -1.f;
	}

	PlayerCharacter.bDead = true;
	PlayerCharacter.DeadFramesNum = 0;
	PlayerCharacter.DeadFramesCurrentStep = 0;
	PlayerCharacter.DeadLocation = JoltSubsystem->GetCenterOfMassPosition(BodyID(PlayerCharacter.PlayerId));
	PlayerCharacter.DeadOnFrameId = InFrameId;
	PlayerCharacter.CarryingFlagBodyIndex = -1;

	if (GameModeSettings.bSpawnPlayerTombstones)
	{
		const int TombstoneBodyIndex = GetWorld()->GetSubsystem<USpawnerSubsystem>()->SpawnJoltBody(TombstoneClass, PlayerCharacter.DeadLocation  + FVector(0, 0, 500.f), FRotator::ZeroRotator, PlayerCharacter.PlayerId);
		PlayerCharacter.CreatedBodyIndexes.Add(TombstoneBodyIndex);
	}
	
	UHelper::GetMyGameState(GetWorld())->SetFirstPlayerToDieId(PlayerCharacter.PlayerId);
	LOG_NETWORKING(TEXT("[Frame %d] Player %d set as dead"), InFrameId, PlayerCharacter.PlayerId);
	
	JoltSubsystem->GetBodyInterface()->SetPosition(BodyID(PlayerCharacter.PlayerVisualRootBodyIndex), Vec3(-1000, -1000, 0), EActivation::DontActivate);
	JoltSubsystem->GetBodyInterface()->SetPosition(BodyID(PlayerCharacter.TakeDamageSensorBodyIndex), Vec3(-1000, -1000, 0), EActivation::DontActivate);
	JoltSubsystem->GetBodyInterface()->SetPosition(BodyID(PlayerCharacter.HandBodyIndex), Vec3(-1000, -1000, 0), EActivation::DontActivate);
	for (const int& BodyIndex : PlayerCharacter.PlayerVisualBodyIndexes)
	{
		JoltSubsystem->GetBodyInterface()->SetPosition(BodyID(BodyIndex), Vec3(-1000, -1000, 0), EActivation::DontActivate);
	}
}

void AMyPlayerController::StepCharacterSimulationMovementAndAbilities(const int& InFrameId,
                                                                      FPlayerCharacter& PlayerCharacter)
{
	const BodyID PlayerBodyID = BodyID(PlayerCharacter.PlayerId);
	const int PlayerId = PlayerCharacter.PlayerId;
	auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();
	auto SpawnerSubsystem = GetWorld()->GetSubsystem<USpawnerSubsystem>();
	auto SoundSubsystem = GetWorld()->GetSubsystem<USoundSubsystem>();

	if (PlayerCharacter.InvulnerabilityFramesNum > 0)
	{
		PlayerCharacter.InvulnerabilityFramesNum--;
	}

	FCharacterInput CharacterInput = GetWorld()->GetSubsystem<UInputsSubsystem>()->GameInputs->GetCharacterInputForFrame(InFrameId, PlayerId);
	LOG(TEXT("[MyPlayerController] Step character %d simulation for frame %d."), PlayerId, InFrameId);
	LOG(TEXT("[MyPlayerController] ShootingCooldownFramesLeft: %d."), PlayerCharacter.ShootingCooldownFramesLeft);
	

	LOG(TEXT("[Player %d] Pos before step: %s"), PlayerId, *VecToHexString(JoltSubsystem->GetBodyInterface()->GetCenterOfMassPosition(PlayerBodyID)));
		
	bool bMotorsOn = PlayerCharacter.HandContactBodyIndex >= 0;
	// The lower the value, the more sensitive the hammer is to mouse movement. Higher values provide more possible expression and consistency, just like in FPS with mouse sensitivity
	// But we control the sensitivity through MouseSensitivity in GameSettings. This value should stay constant.
	constexpr float MaxCursorValue = 10.f;
	Vec3 PlayerVelocity = JoltSubsystem->GetBodyInterface()->GetLinearVelocity(PlayerBodyID);
	Vec3 PlayerPos = JoltSubsystem->GetBodyInterface()->GetCenterOfMassPosition(BodyID(PlayerCharacter.ShoulderBodyIndex));
	Vec3 HandCurrentPos = JoltSubsystem->GetBodyInterface()->GetCenterOfMassPosition(BodyID(PlayerCharacter.HandBodyIndex));
	Vec3 OutHitPos = Vec3::sZero();
	BodyID OutBodyID = BodyID();

	RayObjectLayerFilter PlayerHandRayFilter({EObjectLayer::NonMoving, EObjectLayer::Moving});

	// "Fake" cursor movement is decoupled from the player hammer movement and does not translate 1 : 1. So we need the ratio to convert back and forth.
	const float CursorToHandUnitsRatio = PlayerCharacter.HandSliderConstraint->GetLimitsMax() / MaxCursorValue;
	
	//0 to MaxCursorValue takes MINIMUM 3 frames. Going from fully extended to the opposite side will be double that. So 6 frames. Which is 50ms and 100ms. 
	const float MaxCursorDelta = MaxCursorValue / 3.f;
	//PlayerCharacter.CursorPosition += CharacterInput.MouseDelta.ClampAxes(-MaxCursorDelta, MaxCursorDelta);
	PlayerCharacter.CursorPosition += CharacterInput.MouseDelta;
	if (PlayerCharacter.CursorPosition.SquaredLength() > FMath::Square(MaxCursorValue))
	{
		PlayerCharacter.CursorPosition = PlayerCharacter.CursorPosition.GetSafeNormal() * MaxCursorValue;
	}
	
	Vec3 CursorPos = PlayerPos + Vec3(PlayerCharacter.CursorPosition.X, PlayerCharacter.CursorPosition.Y, 0);
	if (PlayerPos == CursorPos)
	{
		CursorPos = PlayerPos + Vec3(1.f, 0, 0);
	}
	
	//DrawDebugBox(GetWorld(), Units::FromJoltPos(CursorPos) + FVector(0, 10000.f, 0), FVector(200, 200, 200), FColor::Yellow, false, 0.016f, SDPG_World, 30.f);

	Vec3 CurrentHandPosConvertedToCursorSpace = PlayerPos + (HandCurrentPos - PlayerPos) / CursorToHandUnitsRatio;
	//DrawDebugBox(GetWorld(), Units::FromJoltPos(CurrentHandPosConvertedToCursorSpace) + FVector(0, 10000.f, 0), FVector(200, 200, 200), FColor::Cyan, false, 0.016f, SDPG_World, 30.f);

	const float CursorDistanceFromHand = (CursorPos - CurrentHandPosConvertedToCursorSpace).Length();
	//DrawDebugString(GetWorld(), Units::FromJoltPos(HandCurrentPos), FString::Printf(TEXT("CursorDistance: %f"), CursorDistanceFromHand), 0, FColor::White, 0.016f);

	/**
	 * This handles "hugging". When player hand and body are on opposite sides of a platform.
	 * In those cases we don't want to allow the cursor to get away too far from the hand.
	 * If it does, it will cause an explosion of movement when the hand ties to snap to cursor position.
	 * Resulting in janky movement.
	 **/
	constexpr float MaxCursorDistanceFromHand = 1.5f;
	if (CursorDistanceFromHand > MaxCursorDistanceFromHand)
	{
		if (JoltSubsystem->CastRayFirstHit(PlayerPos, (HandCurrentPos - PlayerPos).NormalizedOr(Vec3::sZero()) * 5.f, OutHitPos, OutBodyID, &PlayerHandRayFilter))
		{
			Vec3 AdjustedCursorPos = CurrentHandPosConvertedToCursorSpace + (CursorPos - CurrentHandPosConvertedToCursorSpace).NormalizedOr(Vec3::sZero()) * MaxCursorDistanceFromHand;
			//DrawDebugBox(GetWorld(), Units::FromJoltPos(AdjustedCursorPos) + FVector(0, 10000.f, 0), FVector(200, 200, 200), FColor::Red, false, 0.016f, SDPG_World, 30.f);

			CursorPos = AdjustedCursorPos;
			Vec3 NewCursorPos = CursorPos - PlayerPos;
			PlayerCharacter.CursorPosition = FVector2D(NewCursorPos.GetX(), NewCursorPos.GetY());
		}
	}
	/** Hugging end */

	const Vec3 HandDesiredPos = PlayerPos + Vec3(PlayerCharacter.CursorPosition.X, PlayerCharacter.CursorPosition.Y, 0) * CursorToHandUnitsRatio;

	/**
	 * This handles cases when the motors are off and the hand moves so fast that it might jump through a narrow platform.
	 * Which would result in the player being trapped on one side, while the hand is stuck on the other one.
	 * So we ray cast the path and turn the motors on preemptively if we see there's a collider in the way.
	 **/
	if (!bMotorsOn)
	{
		//DrawDebugBox(GetWorld(), Units::FromJoltPos(HandCurrentPos) + FVector(0, 12000.f, 0), FVector(100, 100, 100), FColor::White, false, 0.016f, SDPG_World, 50.f);
		//DrawDebugBox(GetWorld(), Units::FromJoltPos(HandDesiredPos) + FVector(0, 12000.f, 0), FVector(100, 100, 100), FColor::Black, false, 0.016f, SDPG_World, 50.f);
		//DrawDebugLine(GetWorld(), Units::FromJoltPos(HandCurrentPos) + FVector(0, 12000.f, 0), Units::FromJoltPos(HandDesiredPos) + FVector(0, 12000.f, 0), FColor::Cyan, false, 0.016f, SDPG_World, 80.f);

		RefConst<Shape> HandShape;
		{
			BodyLockRead Lock(JoltSubsystem->GetPhysicsSystem()->GetBodyLockInterfaceNoLock(), BodyID(PlayerCharacter.HandBodyIndex));
			if (Lock.Succeeded())
			{
				HandShape = Lock.GetBody().GetShape();
			}
		}

		bool bHit = JoltSubsystem->CastShapeFirstHit(HandShape, HandCurrentPos, HandDesiredPos - HandCurrentPos, OutHitPos, OutBodyID, &PlayerHandRayFilter);
		if (!bHit)
		{
			bHit = JoltSubsystem->CastRayFirstHit(HandCurrentPos, HandDesiredPos - HandCurrentPos, OutHitPos, OutBodyID, &PlayerHandRayFilter);
		}
		
		if (bHit && OutBodyID.GetIndex() != PlayerCharacter.GrabbedBodyIndex)
		{
			//DrawDebugBox(GetWorld(), Units::FromJoltPos(OutHitPos), FVector(200, 200, 200), FColor::Orange, false, 0.016f, SDPG_World, 50.f);

			bMotorsOn = true;
			HandCurrentPos = OutHitPos;
			JoltSubsystem->GetBodyInterface()->SetPosition(BodyID(PlayerCharacter.HandBodyIndex), OutHitPos, EActivation::Activate);
			//DrawDebugString(GetWorld(), Units::FromJoltPos(OutHitPos) + FVector(0, 0, -200.f), FString::Printf(TEXT("Body: %d\nTurned motors on\nLength: %f"), OutBodyID.GetIndex(), (OutHitPos - HandCurrentPos).Length()), 0, FColor::Green, 0.016f, false);
		}
	}
	
	//DrawDebugString(GetWorld(), Units::FromJoltPos(PlayerPos), FString::Printf(TEXT("BodyVel: %s\nShoulderVel: %s\nHandVel: %s"), *VecToString(BodyVel), *VecToString(ShoulderVel), *VecToString(HandVel)), 0, FColor::White, 0.5f, true, 2);
	//DrawDebugString(GetWorld(), Units::FromJoltPos(PlayerPos), FString::Printf(TEXT("Motors: %d\nContactBodyIndex: %d"), bMotorsOn, PlayerCharacter.HandContactBodyIndex), 0, FColor::White, 0.016f, false);
	//DrawDebugString(GetWorld(), Units::FromJoltPos(PlayerPos), FString::Printf(TEXT("CameraLoc: %s"), *GetViewTarget()->GetActorLocation().ToString()), 0, FColor::White, 0.016f, false);

	//const float PressureLambda = PlayerCharacter.HeadHingeConstraint->GetTotalLambdaPosition().Length();
	//DrawDebugString(GetWorld(), Units::FromJoltPos(PlayerPos), FString::Printf(TEXT("PressureOnHead: %f"), PressureLambda), 0, FColor::White, 0.016f, false);
	/*if (PressureLambda > 5000.f)
	{
		FVector NiagaraSpawnLocation = Units::FromJoltPos(PlayerPos);
		NiagaraSpawnLocation.Y = 10000.f;
		auto NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), BulletHitVFX, NiagaraSpawnLocation);
		NiagaraComponent->SetVariableVec3(TEXT("Direction"), Units::FromJoltCoord(Vec3::sAxisY()));

		PlayerCharacter.bDead = true;

		JoltSubsystem->GetBodyInterface()->SetMotionType(PlayerBodyID, EMotionType::Static, EActivation::Activate);
		PlayerCharacter.PlayerRenderer->SetActorHiddenInGame(true);
		SpawnerSubsystem->GetBodyActor(BodyID(PlayerCharacter.PlayerVisualRootBodyIndex))->SetActorHiddenInGame(true);
		SpawnerSubsystem->GetBodyActor(BodyID(PlayerCharacter.PlayerId))->SetActorHiddenInGame(true);
		SpawnerSubsystem->GetBodyActor(BodyID(PlayerCharacter.HandBodyIndex))->SetActorHiddenInGame(true);
		for (const int& BodyIndex : PlayerCharacter.PlayerVisualBodyIndexes)
		{
			SpawnerSubsystem->GetBodyActor(BodyID(BodyIndex))->SetActorHiddenInGame(true);
		}

		return;
	}*/

	auto PlayerToCursorDirection = (CursorPos - PlayerPos).NormalizedOr(Vec3(-1, 0, 0));
	
	JoltSubsystem->GetBodyInterface()->SetLinearVelocity(BodyID(PlayerCharacter.PlayerVisualRootBodyIndex), PlayerVelocity * 1.f);
	JoltSubsystem->GetBodyInterface()->SetPosition(BodyID(PlayerCharacter.PlayerVisualRootBodyIndex), PlayerPos, EActivation::Activate);
	JoltSubsystem->GetBodyInterface()->SetPosition(BodyID(PlayerCharacter.TakeDamageSensorBodyIndex), PlayerPos + Vec3(0, 5.f, 0.f), EActivation::Activate);

	/*if (PlayerCharacter.HandSliderConstraint->GetEnabled() != bMotorsOn)
	{
		const int FramesSinceLastMotorsToggle = InFrameId - PlayerCharacter.MotorsToggledOnFrameId;
		if (FramesSinceLastMotorsToggle > 0 && FramesSinceLastMotorsToggle < 30)
		{
			//We prevent the toggling of motors so soon
			bMotorsOn = !bMotorsOn;
		}
		else
		{
			// Motors are about to toggle to the opposite mode
			PlayerCharacter.MotorsToggledOnFrameId = InFrameId;
		}
	}*/

	/*if (bMotorsOn)
	{
		DrawDebugString(GetWorld(), Units::FromJoltPos(PlayerPos), FString::Printf(TEXT("Motors")), 0, FColor::Red, 0.2f, false);
	}
	else
	{
		DrawDebugString(GetWorld(), Units::FromJoltPos(PlayerPos), FString::Printf(TEXT("Free")), 0, FColor::Green, 0.2f, false);
	}

	if (PlayerCharacter.HandContactBodyIndex >= 0)
	{
		DrawDebugString(GetWorld(), Units::FromJoltPos(PlayerPos + Vec3(0, 10.f, 0)), FString::Printf(TEXT("ContactBodyIndex: %d"), PlayerCharacter.HandContactBodyIndex), 0, FColor::Red, 0.2f, true);
	}
	else
	{
		DrawDebugString(GetWorld(), Units::FromJoltPos(PlayerPos + Vec3(0, 10.f, 0)), FString::Printf(TEXT("No contact")), 0, FColor::White, 0.2f, true);
	}*/
	
	//DrawDebugString(GetWorld(), Units::FromJoltPos(CursorPos), FString::Printf(TEXT("%s"), *VecToString(CursorPos)), 0, FColor::White, 0.016f, false);

	if (bMotorsOn)
	{
		PlayerCharacter.HandSliderConstraint->SetEnabled(true);
		
		constexpr float MinCursorValue = 0.f;
		const float PlayerToCursorDistance = JPH::Clamp((CursorPos - PlayerPos).Length(), MinCursorValue, MaxCursorValue);

		const float PosLimitMin = PlayerCharacter.HandSliderConstraint->GetLimitsMin();
		const float PosLimitMax = PlayerCharacter.HandSliderConstraint->GetLimitsMax();
		const float TargetPosition = PosLimitMin + (PlayerToCursorDistance - MinCursorValue) * (PosLimitMax - PosLimitMin) / (MaxCursorValue - MinCursorValue);
		PlayerCharacter.HandSliderConstraint->SetTargetPosition(TargetPosition);
	}
	else
	{
		PlayerCharacter.HandSliderConstraint->SetEnabled(false);
		
		Vec3 NewHandPos = PlayerPos + Vec3(PlayerCharacter.CursorPosition.X, PlayerCharacter.CursorPosition.Y, 0) * CursorToHandUnitsRatio;
		JoltSubsystem->GetBodyInterface()->SetPosition(BodyID(PlayerCharacter.HandBodyIndex), NewHandPos, EActivation::Activate);
		JoltSubsystem->GetBodyInterface()->SetLinearVelocity(BodyID(PlayerCharacter.HandBodyIndex), Vec3::sZero());
		//DrawDebugSphere(GetWorld(), Units::FromJoltPos(NewHandPos), 400.f, 8.f, FColor::Magenta, false, 0.016f, SDPG_World, 30.f);
		//DrawDebugString(GetWorld(), Units::FromJoltPos(NewHandPos), FString::Printf(TEXT("NewHandPos: %s"), *VecToString(NewHandPos)), 0, FColor::Red, 0.016f, false);

		if (PlayerCharacter.ShootingCooldownFramesLeft > 0)
		{
			const int CurrentShootFrame = ShootingCooldownInFrames - PlayerCharacter.ShootingCooldownFramesLeft;
			if (CurrentShootFrame == 1 || CurrentShootFrame == 2)
			{
				JoltSubsystem->GetBodyInterface()->SetPosition(BodyID(PlayerCharacter.HandBodyIndex), NewHandPos + PlayerToCursorDirection * -5.f, EActivation::Activate);
			}
			else if (CurrentShootFrame == 3 || CurrentShootFrame == 4)
			{
				JoltSubsystem->GetBodyInterface()->SetPosition(BodyID(PlayerCharacter.HandBodyIndex), NewHandPos + PlayerToCursorDirection * -5.f, EActivation::Activate);
			}
			else if (CurrentShootFrame == 5 || CurrentShootFrame == 6)
			{
				JoltSubsystem->GetBodyInterface()->SetPosition(BodyID(PlayerCharacter.HandBodyIndex), NewHandPos + PlayerToCursorDirection * -3.f, EActivation::Activate);
			}
			else if (CurrentShootFrame == 12)
			{
				//SpawnerSubsystem->GetBodyActor(PlayerCharacter.HandBodyIndex)->SetActorHiddenInGame(false);
			}
		}
	}
	
	const float TargetAngleInRadians = JPH::ATan2(PlayerToCursorDirection.GetX(), PlayerToCursorDirection.GetY()) * -1.f;
	// Tried rotating hammer head based together with the handle. But it lacks stability.
	//JoltSubsystem->GetBodyInterface()->SetRotation(BodyID(PlayerCharacter.HandBodyIndex), Quat::sRotation(Vec3::sAxisZ(), TargetAngleInRadians), EActivation::Activate);

	//DrawDebugString(GetWorld(), Units::FromJoltPos(CursorPos), FString::Printf(TEXT("Angle: %f"), TargetAngleInRadians), 0, FColor::Red, 0.016f, false);
	//DrawDebugLine(GetWorld(), Units::FromJoltPos(PlayerPos), Units::FromJoltPos(CursorPos + PlayerToCursorDirection * 100.f), FColor::Red, false, 0.016f, SDPG_World, 30.f);

	//DrawDebugSphere(GetWorld(), Units::FromJoltPos(PlayerPos), 400.f, 8.f, FColor::Green, false, 0.016f, SDPG_World, 30.f);
	//DrawDebugSphere(GetWorld(), Units::FromJoltPos(CursorPos), 400.f, 8.f, FColor::Red, false, 0.016f, SDPG_World, 30.f);

	//Quat NewRotation = Quat::sRotation(Vec3::sAxisZ(), TargetAngleInRadians);
	//DrawDebugLine(GetWorld(), Units::FromJoltPos(PlayerPos), Units::FromJoltPos(CursorPos) + Units::FromJoltQuat(NewRotation).GetRightVector() * 10000.f, FColor::Yellow, false, 0.016f, SDPG_World, 30.f);

	
	if (PlayerCharacter.HandHingeConstraint)
	{
		if (bMotorsOn)
		{
			PlayerCharacter.HandHingeConstraint->SetMotorState(EMotorState::Position);
			PlayerCharacter.HandHingeConstraint->SetTargetAngle(TargetAngleInRadians);
		}
		else
		{
			PlayerCharacter.HandHingeConstraint->SetMotorState(EMotorState::Off);
			JoltSubsystem->GetBodyInterface()->SetRotation(BodyID(PlayerCharacter.ShoulderBodyIndex), Quat::sRotation(Vec3::sAxisZ(), TargetAngleInRadians), EActivation::Activate);
		}
	}

	// Update hidden hand body position
	JoltSubsystem->GetBodyInterface()->SetPosition(BodyID(PlayerCharacter.HiddenHandBodyIndex), JoltSubsystem->GetBodyInterface()->GetCenterOfMassPosition(BodyID(PlayerCharacter.HandBodyIndex)), EActivation::Activate);


	const int MaxBullets = GetMaxBullets(PlayerCharacter);
	
	if (PlayerCharacter.ShootingCooldownFramesLeft > 0)
	{
		PlayerCharacter.ShootingCooldownFramesLeft--;
	}
	
	if (PlayerCharacter.BulletReloadCooldownFramesLeft > 0)
	{
		PlayerCharacter.BulletReloadCooldownFramesLeft--;
		if (PlayerCharacter.BulletReloadCooldownFramesLeft == 0)
		{
			PlayerCharacter.BulletsLeft = JPH::min(MaxBullets, PlayerCharacter.BulletsLeft + 1);
			if (PlayerCharacter.BulletsLeft < MaxBullets)
			{
				PlayerCharacter.BulletReloadCooldownFramesLeft = GetBulletReloadCooldownInFrames(PlayerCharacter);
			}
		}
	}

	if (PlayerCharacter.BulletReloadCooldownFramesLeft == 0 && PlayerCharacter.BulletsLeft < MaxBullets)
	{
		PlayerCharacter.BulletReloadCooldownFramesLeft = GetBulletReloadCooldownInFrames(PlayerCharacter);
	}

	if (PlayerCharacter.DashCooldownFramesLeft > 0)
	{
		PlayerCharacter.DashCooldownFramesLeft--;
	}

	if (PlayerCharacter.ActiveItem != ELootItem::None)
	{
		PlayerCharacter.ItemActiveForFramesNum++;

		FLootItem ActiveItemData = UAssetManagerSubsystem::GetLootItemRow(GetWorld(), PlayerCharacter.ActiveItem);

		if (PlayerCharacter.ActiveItem == ELootItem::MachineGun && PlayerCharacter.ItemActiveForFramesNum % ActiveItemData.FrameInterval == 0)
		{
			const FVector ProjectileSpawnLocation = Units::FromJoltPos(HandCurrentPos + PlayerToCursorDirection * ProjectileSpawnForwardDistance);
			int SpawnedBodyIndex = SpawnerSubsystem->SpawnJoltBody(ShootActorClass, ProjectileSpawnLocation, Units::FromJoltCoord(PlayerToCursorDirection).Rotation(), PlayerCharacter.PlayerId);

			const float RandomDegreesOffset = static_cast<float>(JoltSubsystem->RandomNumberGenerator.NextIntInRange(-ActiveItemData.DegreesOffset, ActiveItemData.DegreesOffset));
			Vec3 ProjectileShootDirection = OffsetDirectionAngle(PlayerToCursorDirection, RandomDegreesOffset);
			
			PlayerCharacter.CreatedBodyIndexes.Add(SpawnedBodyIndex);
			JoltSubsystem->GetBodyInterface()->AddImpulse(BodyID(SpawnedBodyIndex), ProjectileShootDirection * GetDefaultProjectileImpulse(PlayerCharacter));
			JoltSubsystem->GetBodyInterface()->AddImpulse(PlayerBodyID, PlayerToCursorDirection * -ProjectileKnockbackOnPlayer);

			FString StringToHash = FString::Printf(TEXT("MachineGunShootProjectile%d"), InFrameId);
			const auto EffectHash = THashContainer::Generate(StringToHash);

			if (!UsedEffects.Contains(EffectHash))
			{
				if (PlayerCharacter.bLocalPlayer)
				{
					GameCamera->DirectionalShake(Units::FromJoltCoord(-ProjectileShootDirection), 0.3f);
				}

				const FVector VFXSpawnLocation = Units::FromJoltPos(HandCurrentPos + ProjectileShootDirection * 2.f);
				const FRotator VFXSpawnRotation = VecDirectionToFRotator(ProjectileShootDirection);
				AActor* SpawnedSprite = SpawnSpriteActor(GeneralData->ShootFireballVFX, VFXSpawnLocation, VFXSpawnRotation);
				Cast<APlayAnimationOnceActor>(SpawnedSprite)->SetPlayerTeam(PlayerCharacter.PlayerTeam);

				SoundSubsystem->PlaySound("ShootFireball");
			
				AddUsedEffect(InFrameId, EffectHash);
			}
		}

		if (PlayerCharacter.ActiveItem == ELootItem::MachineGun && PlayerCharacter.ItemActiveForFramesNum > ActiveItemData.TotalFrameDuration)
		{
			PlayerCharacter.ActiveItem = ELootItem::None;
			PlayerCharacter.ItemActiveForFramesNum = 0;
		}

		if (PlayerCharacter.ActiveItem == ELootItem::Sword)
		{
			ASword* SwordCDO = ActiveItemData.GetItemCDO<ASword>();

			if (PlayerCharacter.ItemActiveForFramesNum > SwordCDO->AbilityDurationFramesNum)
			{
				PlayerCharacter.ActiveItem = ELootItem::None;
				PlayerCharacter.ItemActiveForFramesNum = 0;

				CleanupActiveSwordBody(InFrameId, PlayerCharacter);
			}
			else if (PlayerCharacter.ActiveSwordBody.IsValid())
			{
				JoltSubsystem->GetBodyInterface()->SetPositionAndRotation(
					PlayerCharacter.ActiveSwordBody->GetBodyID(),
					HandCurrentPos + PlayerToCursorDirection * SwordCDO->DistanceFromPlayer,
					Quat::sRotation(Vec3::sAxisZ(), TargetAngleInRadians),
					EActivation::Activate
				);
			}
		}

		if (PlayerCharacter.ActiveItem == ELootItem::HomingMissile)
		{
			AHomingMissile* HomingMissileCDO = ActiveItemData.GetItemCDO<AHomingMissile>();
			
			if (PlayerCharacter.ItemActiveForFramesNum % HomingMissileCDO->ShootRocketEveryXFrames == 0)
			{
				const FVector ProjectileSpawnLocation = Units::FromJoltPos(HandCurrentPos + PlayerToCursorDirection * HomingMissileCDO->ProjectileSpawnForwardDistance);
				int SpawnedBodyIndex = SpawnerSubsystem->SpawnJoltBody(ActiveItemData.GetItemClass(), ProjectileSpawnLocation, Units::FromJoltCoord(PlayerToCursorDirection).Rotation(), PlayerCharacter.PlayerId);

				const float RandomDegreesOffset = static_cast<float>(JoltSubsystem->RandomNumberGenerator.NextIntInRange(-HomingMissileCDO->RandomDegreesOffset, HomingMissileCDO->RandomDegreesOffset));
				Vec3 ProjectileShootDirection = OffsetDirectionAngle(PlayerToCursorDirection, RandomDegreesOffset);
			
				PlayerCharacter.CreatedBodyIndexes.Add(SpawnedBodyIndex);
				JoltSubsystem->GetBodyInterface()->AddImpulse(BodyID(SpawnedBodyIndex), ProjectileShootDirection * HomingMissileCDO->DefaultProjectileImpulse);
				JoltSubsystem->GetBodyInterface()->AddImpulse(PlayerBodyID, PlayerToCursorDirection * -HomingMissileCDO->ProjectileKnockbackOnPlayer);

				FString StringToHash = FString::Printf(TEXT("HomingMissileShoot%d"), InFrameId);
				const auto EffectHash = THashContainer::Generate(StringToHash);

				if (!UsedEffects.Contains(EffectHash))
				{
					if (PlayerCharacter.bLocalPlayer)
					{
						GameCamera->DirectionalShake(Units::FromJoltCoord(-ProjectileShootDirection), 0.3f);
					}

					const FVector VFXSpawnLocation = Units::FromJoltPos(HandCurrentPos + ProjectileShootDirection * 2.f);
					const FRotator VFXSpawnRotation = VecDirectionToFRotator(ProjectileShootDirection);
					SpawnSpriteActor(HomingMissileCDO->ShootVFX, VFXSpawnLocation, VFXSpawnRotation);

					SoundSubsystem->PlaySound("ShootFireball");
			
					AddUsedEffect(InFrameId, EffectHash);
				}
			}

			if (PlayerCharacter.ItemActiveForFramesNum > HomingMissileCDO->FrameDurationToShootAllRockets)
			{
				PlayerCharacter.ActiveItem = ELootItem::None;
				PlayerCharacter.ItemActiveForFramesNum = 0;
			}
		}

		if (PlayerCharacter.ActiveItem == ELootItem::MineSweeper)
		{
			AMineSweeper* MineSweeperCDO = ActiveItemData.GetItemCDO<AMineSweeper>();
			
			if (PlayerCharacter.ItemActiveForFramesNum % MineSweeperCDO->ReleaseMineEveryXFrames == 0)
			{
				const FVector ProjectileSpawnLocation = Units::FromJoltPos(HandCurrentPos + PlayerToCursorDirection * MineSweeperCDO->ProjectileSpawnForwardDistance);
				int SpawnedBodyIndex = SpawnerSubsystem->SpawnJoltBody(ActiveItemData.GetItemClass(), ProjectileSpawnLocation, FRotator::ZeroRotator, PlayerCharacter.PlayerId);

				const float RandomDegreesOffset = static_cast<float>(JoltSubsystem->RandomNumberGenerator.NextIntInRange(-MineSweeperCDO->RandomDegreesOffset, MineSweeperCDO->RandomDegreesOffset));
				Vec3 ProjectileShootDirection = OffsetDirectionAngle(PlayerToCursorDirection, RandomDegreesOffset);

				const float ProjectileImpulse = static_cast<float>(JoltSubsystem->RandomNumberGenerator.NextIntInRange(-MineSweeperCDO->MinDefaultProjectileImpulse, MineSweeperCDO->MaxDefaultProjectileImpulse));
			
				PlayerCharacter.CreatedBodyIndexes.Add(SpawnedBodyIndex);
				JoltSubsystem->GetBodyInterface()->AddImpulse(BodyID(SpawnedBodyIndex), ProjectileShootDirection * ProjectileImpulse);
				JoltSubsystem->GetBodyInterface()->AddImpulse(PlayerBodyID, PlayerToCursorDirection * -MineSweeperCDO->ProjectileKnockbackOnPlayer);
				JoltSubsystem->GetBodyInterface()->AddTorque(BodyID(SpawnedBodyIndex), Vec3(0, 0, MineSweeperCDO->TorqueImpulse));
				
				FString StringToHash = FString::Printf(TEXT("MineSweeperRelease%d"), InFrameId);
				const auto EffectHash = THashContainer::Generate(StringToHash);

				if (!UsedEffects.Contains(EffectHash))
				{
					if (PlayerCharacter.bLocalPlayer)
					{
						GameCamera->DirectionalShake(Units::FromJoltCoord(-ProjectileShootDirection), 0.3f);
					}

					const FVector VFXSpawnLocation = Units::FromJoltPos(HandCurrentPos + ProjectileShootDirection * 2.f);
					const FRotator VFXSpawnRotation = VecDirectionToFRotator(ProjectileShootDirection);
					AActor* SpawnedSprite = SpawnSpriteActor(GeneralData->ShootFireballVFX, VFXSpawnLocation, VFXSpawnRotation);
					Cast<APlayAnimationOnceActor>(SpawnedSprite)->SetPlayerTeam(PlayerCharacter.PlayerTeam);

					SoundSubsystem->PlaySound("ShootFireball");
			
					AddUsedEffect(InFrameId, EffectHash);
				}
			}

			if (PlayerCharacter.ItemActiveForFramesNum > MineSweeperCDO->FrameDurationToReleaseAllMines)
			{
				PlayerCharacter.ActiveItem = ELootItem::None;
				PlayerCharacter.ItemActiveForFramesNum = 0;
			}
		}

		if (PlayerCharacter.ActiveItem == ELootItem::OrbitalSpheres)
		{
			AOrbitalSphere* CDO = ActiveItemData.GetItemCDO<AOrbitalSphere>();

			if (PlayerCharacter.ItemActiveForFramesNum > CDO->AbilityDurationFramesNum)
			{
				for (int i = PlayerCharacter.CreatedBodyIndexes.Num() - 1; i >= 0; i--)
				{
					const int Idx = PlayerCharacter.CreatedBodyIndexes[i];
					if (SpawnerSubsystem->DoesBodyExistInSimulation(BodyID(Idx))
						&& Cast<AOrbitalSphere>(SpawnerSubsystem->GetBodyActor(BodyID(Idx))))
					{
						SpawnerSubsystem->DespawnJoltBody(BodyID(Idx));
						PlayerCharacter.CreatedBodyIndexes.RemoveAt(i);
					}
				}
				PlayerCharacter.ActiveItem = ELootItem::None;
				PlayerCharacter.ItemActiveForFramesNum = 0;
			}
			else
			{
				const float BaseAngle = PlayerCharacter.ItemActiveForFramesNum * CDO->RotationSpeedDegreesPerFrame;
				const int CycleLength = CDO->PulseDurationFrames + CDO->PulsePauseFrames;
				const int FrameInCycle = PlayerCharacter.ItemActiveForFramesNum % CycleLength;
				const float PulseProgress = static_cast<float>(FrameInCycle) / static_cast<float>(CDO->PulseDurationFrames);
				const float PulseT = FrameInCycle < CDO->PulseDurationFrames ? JPH::Sin(JPH_PI * PulseProgress) : 0.f;
				const float Radius = CDO->OrbitRadius + CDO->PulseAmplitude * PulseT;

				const Vec3 Center = JoltSubsystem->GetBodyInterface()->GetCenterOfMassPosition(BodyID(PlayerCharacter.ShoulderBodyIndex));

				for (const int SphereIdx : PlayerCharacter.CreatedBodyIndexes)
				{
					if (!SpawnerSubsystem->DoesBodyExistInSimulation(BodyID(SphereIdx))) continue;
					AOrbitalSphere* Sphere = Cast<AOrbitalSphere>(SpawnerSubsystem->GetBodyActor(BodyID(SphereIdx)));
					if (!Sphere) continue;

					const float AngleRad = (BaseAngle + Sphere->OrbitIndex * (360.f / CDO->SphereCount)) * (JPH_PI / 180.f);
					const Vec3 NewPos = Center + Vec3(JPH::Cos(AngleRad) * Radius, JPH::Sin(AngleRad) * Radius, 0.f);
					const Quat NewRot = Quat::sRotation(Vec3::sAxisZ(), AngleRad);
					JoltSubsystem->GetBodyInterface()->SetPositionAndRotation(BodyID(SphereIdx), NewPos, NewRot, EActivation::Activate);
				}
			}
		}
	}
	
	
	/** Throw picked up object, like a bomb **/
	if (CharacterInput.bShoot && PlayerCharacter.ShootingCooldownFramesLeft == 0 && PlayerCharacter.GrabbedBodyIndex >= 0)
	{
		//LOG_TEMPORARY(TEXT("[Frame %d] Throw object player %d"), InFrameId, PlayerCharacter.PlayerId);
		//const FVector ProjectileSpawnLocation = Units::FromJoltPos(HandCurrentPos + PlayerToCursorDirection * 2.f);
		//int SpawnedBodyIndex = SpawnerSubsystem->SpawnJoltBody(ShootActorClass, ProjectileSpawnLocation, Units::FromJoltPos(PlayerToCursorDirection).Rotation());
		
		JoltSubsystem->GetBodyInterface()->SetPosition(BodyID(PlayerCharacter.GrabbedBodyIndex), HandCurrentPos + PlayerToCursorDirection * 2.f, EActivation::Activate);
		JoltSubsystem->SetObjectLayer(PlayerCharacter.GrabbedBodyIndex, EObjectLayer::Projectile);
		if (auto ExplosiveJoltBody = Cast<AExplosiveJoltBody>(SpawnerSubsystem->GetBodyActor(PlayerCharacter.GrabbedBodyIndex)))
		{
			ExplosiveJoltBody->ThrowByPlayer(PlayerCharacter);
		}
		
		//PlayerCharacter.CreatedBodyIndexes.Add(SpawnedBodyIndex);

		SpawnerSubsystem->DespawnJoltConstraint(PlayerCharacter.CreatedConstraintsIds.Pop());

		auto HandVelocity = JoltSubsystem->GetBodyInterface()->GetLinearVelocity(BodyID(PlayerCharacter.HiddenVelocityBodyIndex));
		JoltSubsystem->GetBodyInterface()->AddImpulse(BodyID(PlayerCharacter.GrabbedBodyIndex), HandVelocity * 30.f);
		JoltSubsystem->GetBodyInterface()->AddImpulse(PlayerBodyID, HandVelocity * -30.f);
	
		PlayerCharacter.GrabbedBodyIndex = -1;

		PlayerCharacter.ShootingCooldownFramesLeft = ShootingCooldownInFrames;
		LOG(TEXT("[MyPlayerController] Shot bullet"));
	}

	/** Grab object by attaching a distance constraint **/
	if (PlayerCharacter.GrabbedBodyIndex == -1 && JoltSubsystem->GetObjectLayer(PlayerCharacter.HandContactBodyIndex) == EObjectLayer::Pickup)
	{
		//LOG_TEMPORARY(TEXT("[Frame %d] Grab object player %d"), InFrameId, PlayerCharacter.PlayerId);
		FJoltConstraintData ConstraintData;
		ConstraintData.BodyID1 = BodyID(PlayerCharacter.HiddenHandBodyIndex);
		ConstraintData.BodyID2 = BodyID(PlayerCharacter.HandContactBodyIndex);
		
		JoltSubsystem->SetObjectLayer(PlayerCharacter.HandContactBodyIndex, EObjectLayer::NoCollision);
		
		const int ConstraintId = SpawnerSubsystem->SpawnJoltConstraint(JavelinConstraintActorClass, Units::FromJoltPos(HandCurrentPos), ConstraintData);
		PlayerCharacter.CreatedConstraintsIds.Add(ConstraintId);
		PlayerCharacter.GrabbedBodyIndex = PlayerCharacter.HandContactBodyIndex;
		PlayerCharacter.HandContactBodyIndex = -1;

		PlayerCharacter.ShootingCooldownFramesLeft = 3;

		FString StringToHash = FString::Printf(TEXT("GrabPickup%d"), InFrameId);
		const auto EffectHash = THashContainer::Generate(StringToHash);

		if (!UsedEffects.Contains(EffectHash))
		{
			FVector EffectSpawnLocation = Units::FromJoltPos(HandCurrentPos);
			SpawnSpriteActor(GeneralData->PickupLootSpinnerVFX, EffectSpawnLocation);
			SoundSubsystem->PlaySound("GrabPickup");

			AddUsedEffect(InFrameId, EffectHash);
		}
	}

	const FRotator PlayerToCursorRotation = VecDirectionToFRotator(-PlayerToCursorDirection);
	//DrawDebugString(GetWorld(), Units::FromJoltPos(PlayerPos) + FVector(0, 0.f, 1000.f), FString::Printf(TEXT("%s"), *PlayerToCursorRotation.ToString()), 0, FColor::Black, 0.016f, false);

	if (GameModeSettings.bShootingEnabled && CharacterInput.bShoot && PlayerCharacter.ShootingCooldownFramesLeft == 0 && PlayerCharacter.BulletsLeft > 0 && PlayerCharacter.GrabbedBodyIndex == -1)
	{
		PlayerCharacter.ShootButtonHeldFramesNum++;
		if (PlayerCharacter.ShootButtonHeldFramesNum > MaxFramesHoldShootButton)
		{
			PlayerCharacter.ShootButtonHeldFramesNum = MaxFramesHoldShootButton;
		}
	}

	bool bShouldShoot = GameModeSettings.bShootingEnabled
		&& CharacterInput.bShoot
		&& PlayerCharacter.ShootingCooldownFramesLeft == 0
		&& PlayerCharacter.BulletsLeft > 0
		&& PlayerCharacter.GrabbedBodyIndex == -1;
	
	if (bUseChargeShooting && !bUseHandExtensionToCalculateShootPower)
	{
		bShouldShoot = GameModeSettings.bShootingEnabled
			&& PlayerCharacter.ShootButtonHeldFramesNum > 0
			&& !CharacterInput.bShoot
			&& PlayerCharacter.ShootingCooldownFramesLeft == 0
			&& PlayerCharacter.BulletsLeft > 0
			&& PlayerCharacter.GrabbedBodyIndex == -1;
	}
	
	if (bShouldShoot)
	{
		//LOG_TEMPORARY(TEXT("[Frame %d] Shoot fireball player %d"), InFrameId, PlayerCharacter.PlayerId);
		const FVector ProjectileSpawnLocation = Units::FromJoltPos(HandCurrentPos + PlayerToCursorDirection * ProjectileSpawnForwardDistance);
		int SpawnedBodyIndex = SpawnerSubsystem->SpawnJoltBody(ShootActorClass, GetTransformWithMirroredAxis(ProjectileSpawnLocation, VecDirectionToFRotator(PlayerToCursorDirection)), PlayerCharacter.PlayerId);
	
		PlayerCharacter.CreatedBodyIndexes.Add(SpawnedBodyIndex);
		//auto HandVelocity = JoltSubsystem->GetBodyInterface()->GetLinearVelocity(BodyID(PlayerCharacter.HiddenVelocityBodyIndex));
		//JoltSubsystem->GetBodyInterface()->AddImpulse(BodyID(SpawnedBodyIndex), HandVelocity * 30.f); 
		//JoltSubsystem->GetBodyInterface()->AddImpulse(PlayerBodyID, HandVelocity * -30.f);

		if (bUseChargeShooting)
		{
			// Hold mouse button down to calculate shooting power
			float ShootPowerPerc = static_cast<float>(PlayerCharacter.ShootButtonHeldFramesNum) / static_cast<float>(MaxFramesHoldShootButton);

			// Use hand extension distance to calculate shooting power
			if (bUseHandExtensionToCalculateShootPower)
			{
				constexpr float MinCursorValue = 4.f;
				ShootPowerPerc = JPH::Clamp(((CursorPos - PlayerPos).Length() - MinCursorValue) / MaxCursorValue, 0.f, 1.f);
			}

			const float CalculatedFireballImpulse = FMath::Lerp(FireballImpulseMin, FireballImpulseMax, ShootPowerPerc);
			JoltSubsystem->GetBodyInterface()->AddImpulse(BodyID(SpawnedBodyIndex), PlayerToCursorDirection * CalculatedFireballImpulse);

			const float ShootKnockback = FMath::Lerp(ShootKnockbackMin, ShootKnockbackMax, ShootPowerPerc);
			JoltSubsystem->GetBodyInterface()->AddImpulse(PlayerBodyID, PlayerToCursorDirection * -ShootKnockback);
		}
		else
		{
			JoltSubsystem->GetBodyInterface()->AddImpulse(BodyID(SpawnedBodyIndex), PlayerToCursorDirection * FireballImpulse);
			JoltSubsystem->GetBodyInterface()->AddImpulse(PlayerBodyID, PlayerToCursorDirection * -FireballKnockback);
		}
	
		
		PlayerCharacter.ShootingCooldownFramesLeft = ShootingCooldownInFrames;
		
		if (PlayerCharacter.BulletsLeft == MaxBullets)
		{
			PlayerCharacter.BulletReloadCooldownFramesLeft = GetBulletReloadCooldownInFrames(PlayerCharacter);
		}
		
		PlayerCharacter.BulletsLeft--;
		PlayerCharacter.ShootButtonHeldFramesNum = 0;

#if WITH_EDITOR
		if (GeneralData->bNoCooldowns)
		{
			PlayerCharacter.BulletsLeft++;
		}
#endif

		FString StringToHash = FString::Printf(TEXT("ShootProjectile%d"), InFrameId);
		const auto EffectHash = THashContainer::Generate(StringToHash);

		if (!UsedEffects.Contains(EffectHash))
		{
			if (PlayerCharacter.bLocalPlayer)
			{
				GameCamera->DirectionalShake(Units::FromJoltCoord(-PlayerToCursorDirection), 0.6f);
			}

			const FVector VFXSpawnLocation = Units::FromJoltPos(HandCurrentPos + PlayerToCursorDirection * 2.f);
			const FRotator VFXSpawnRotation = VecDirectionToFRotator(PlayerToCursorDirection);
			AActor* SpawnedSprite = SpawnSpriteActor(GeneralData->ShootFireballVFX, VFXSpawnLocation, VFXSpawnRotation);
			Cast<APlayAnimationOnceActor>(SpawnedSprite)->SetPlayerTeam(PlayerCharacter.PlayerTeam);

			SoundSubsystem->PlaySound("ShootFireball");

			FActorSpawnParameters ActorSpawnParameters;
			ActorSpawnParameters.bDeferConstruction = true;
			
			auto PlayerBodyVFX = Cast<APlayerBodyVFX>(GetWorld()->SpawnActor(GeneralData->PlayerShootFireballBodyVFX, 0, 0, ActorSpawnParameters));
			PlayerBodyVFX->PlayerHeadDynamicMaterial = PlayerCharacter.RenderData.HeadDynamicMaterial;
			PlayerBodyVFX->PlayerEyesDynamicMaterial = PlayerCharacter.RenderData.EyesDynamicMaterial;
			PlayerBodyVFX->PlayerBodyDynamicMaterial = PlayerCharacter.RenderData.BodyDynamicMaterial;
			PlayerBodyVFX->HammerDynamicMaterial = PlayerCharacter.RenderData.HammerDynamicMaterial;
			PlayerBodyVFX->StickDynamicMaterial = PlayerCharacter.RenderData.StickDynamicMaterial;
			PlayerBodyVFX->FinishSpawning({});
			PlayerCharacter.RenderData.ActivePlayerBodyVFX = PlayerBodyVFX;

			PlayerBodyVFX->AttachToActor(PlayerCharacter.PlayerRenderer, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			
			AddUsedEffect(InFrameId, EffectHash);
		}
	}

	if (!CharacterInput.bShoot)
	{
		PlayerCharacter.ShootButtonHeldFramesNum = 0;
	}

	/**
	 * RIGHT MOUSE CLICK, USE ABILITIES
	 */
	if (GameModeSettings.bAbilitiesEnabled && CharacterInput.bParry && PlayerCharacter.DashCooldownFramesLeft == 0)
	{
		if (PlayerCharacter.GetFirstItem() != ELootItem::None)
		{
			// TODO: we probably don't even need this cooldown. But if we want to keep it, at least rename
			PlayerCharacter.DashCooldownFramesLeft = 10;

			CleanupActiveSwordBody(InFrameId, PlayerCharacter);
		}

		//LOG_TEMPORARY(TEXT("[Frame %d] Use ability %s player %d"), InFrameId, *UEnum::GetValueAsString(PlayerCharacter.GetFirstItem()), PlayerCharacter.PlayerId);
		
		if (PlayerCharacter.HasItem(ELootItem::ProjectileShotgun))
		{
			PlayerCharacter.RemoveItem(ELootItem::ProjectileShotgun);

			TArray<Vec3> ProjectileImpulseDirections = {
				OffsetDirectionAngle(PlayerToCursorDirection, -10.f),
				OffsetDirectionAngle(PlayerToCursorDirection, -20.f),
				OffsetDirectionAngle(PlayerToCursorDirection, -30.f),
				
				PlayerToCursorDirection,
				
				OffsetDirectionAngle(PlayerToCursorDirection, 10.f),
				OffsetDirectionAngle(PlayerToCursorDirection, 20.f),
				OffsetDirectionAngle(PlayerToCursorDirection, 30.f),
			};

			for (const Vec3& ProjectileImpulseDirection : ProjectileImpulseDirections)
			{
				const FVector ProjectileSpawnLocation = Units::FromJoltPos(HandCurrentPos + ProjectileImpulseDirection * ProjectileSpawnForwardDistance);
				int SpawnedBodyIndex = SpawnerSubsystem->SpawnJoltBody(ShootActorClass, ProjectileSpawnLocation, Units::FromJoltCoord(ProjectileImpulseDirection).Rotation(), PlayerCharacter.PlayerId);
		
				PlayerCharacter.CreatedBodyIndexes.Add(SpawnedBodyIndex);
				JoltSubsystem->GetBodyInterface()->AddImpulse(BodyID(SpawnedBodyIndex), ProjectileImpulseDirection * GetDefaultProjectileImpulse(PlayerCharacter));
			}
			
			/*Vec3 PlayerToCursorDirectionPerpendicular = Vec3(PlayerToCursorDirection.GetY(), -PlayerToCursorDirection.GetX(), 0.f);
			TArray<FVector> ProjectileSpawnLocations = {
				Units::FromJoltPos(HandCurrentPos + PlayerToCursorDirection * ProjectileSpawnForwardDistance),
				
				//Units::FromJoltPos(HandCurrentPos + PlayerToCursorDirection * ProjectileSpawnForwardDistance + PlayerToCursorDirectionPerpendicular * 10.f),
				Units::FromJoltPos(HandCurrentPos + PlayerToCursorDirection * ProjectileSpawnForwardDistance + PlayerToCursorDirectionPerpendicular * 20.f),
				//Units::FromJoltPos(HandCurrentPos + PlayerToCursorDirection * ProjectileSpawnForwardDistance + PlayerToCursorDirectionPerpendicular * 30.f),
				Units::FromJoltPos(HandCurrentPos + PlayerToCursorDirection * ProjectileSpawnForwardDistance + PlayerToCursorDirectionPerpendicular * 40.f),

				//Units::FromJoltPos(HandCurrentPos + PlayerToCursorDirection * ProjectileSpawnForwardDistance + PlayerToCursorDirectionPerpendicular * -10.f),
				Units::FromJoltPos(HandCurrentPos + PlayerToCursorDirection * ProjectileSpawnForwardDistance + PlayerToCursorDirectionPerpendicular * -20.f),
				//Units::FromJoltPos(HandCurrentPos + PlayerToCursorDirection * ProjectileSpawnForwardDistance + PlayerToCursorDirectionPerpendicular * -30.f),
				Units::FromJoltPos(HandCurrentPos + PlayerToCursorDirection * ProjectileSpawnForwardDistance + PlayerToCursorDirectionPerpendicular * -40.f),
			};
			
			for (const FVector& ProjectileSpawnLocation : ProjectileSpawnLocations)
			{
				int SpawnedBodyIndex = SpawnerSubsystem->SpawnJoltBody(ShootActorClass, ProjectileSpawnLocation, Units::FromJoltCoord(PlayerToCursorDirection).Rotation(), PlayerCharacter.PlayerId);
		
				PlayerCharacter.CreatedBodyIndexes.Add(SpawnedBodyIndex);
				JoltSubsystem->GetBodyInterface()->AddImpulse(BodyID(SpawnedBodyIndex), PlayerToCursorDirection * GetDefaultProjectileImpulse(PlayerCharacter));
			}*/

			JoltSubsystem->GetBodyInterface()->AddImpulse(PlayerBodyID, PlayerToCursorDirection * -ProjectileKnockbackOnPlayer * 2.f);
		}
		else if (PlayerCharacter.HasItem(ELootItem::MachineGun))
		{
			PlayerCharacter.RemoveItem(ELootItem::MachineGun);
			PlayerCharacter.ActiveItem = ELootItem::MachineGun;
			PlayerCharacter.ItemActiveForFramesNum = 0;

			FLootItem ActiveItemData = UAssetManagerSubsystem::GetLootItemRow(GetWorld(), ELootItem::MachineGun);

			PlayerCharacter.ShootingCooldownFramesLeft = ActiveItemData.TotalFrameDuration + ShootingCooldownInFrames;
		}
		else if (PlayerCharacter.HasItem(ELootItem::Sword))
		{
			PlayerCharacter.RemoveItem(ELootItem::Sword);
			PlayerCharacter.ActiveItem = ELootItem::Sword;
			PlayerCharacter.ItemActiveForFramesNum = 0;
			
			if (PlayerCharacter.ActiveSwordBody.IsValid() && SpawnerSubsystem->DoesBodyExistInSimulation(PlayerCharacter.ActiveSwordBody->GetBodyID()))
			{
				SpawnerSubsystem->DespawnJoltBody(PlayerCharacter.ActiveSwordBody->GetBodyID());
			}

			FLootItem ActiveItemData = UAssetManagerSubsystem::GetLootItemRow(GetWorld(), ELootItem::Sword);
			const int SwordBodyIndex = SpawnerSubsystem->SpawnJoltBody(ActiveItemData.GetItemClass(), FTransform::Identity, PlayerCharacter.PlayerId);
			PlayerCharacter.ActiveSwordBody = SpawnerSubsystem->GetBodyActor<ASword>(SwordBodyIndex);
			checkf(PlayerCharacter.ActiveSwordBody, TEXT("Could not spawn sword"));

			FString StringToHash = FString::Printf(TEXT("SwordAppear%d%d"), InFrameId, PlayerCharacter.PlayerId);
			const auto EffectHash = THashContainer::Generate(StringToHash);

			if (!UsedEffects.Contains(EffectHash))
			{
				ASword* Sword = SpawnerSubsystem->GetBodyActor<ASword>(SwordBodyIndex);
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), Sword->AppearVFX, Units::FromJoltPos(HandCurrentPos + PlayerToCursorDirection * Sword->DistanceFromPlayer));
				
				SoundSubsystem->PlaySound("SwordAppear");
				
				AddUsedEffect(InFrameId, EffectHash);
			}
		}
		else if (PlayerCharacter.HasItem(ELootItem::GravityBomb))
		{
			PlayerCharacter.RemoveItem(ELootItem::GravityBomb);
			
			FLootItem ActiveItemData = UAssetManagerSubsystem::GetLootItemRow(GetWorld(), ELootItem::GravityBomb);
			AGravityBomb* GravityBombCDO = ActiveItemData.GetItemCDO<AGravityBomb>();
			
			const FVector ProjectileSpawnLocation = Units::FromJoltPos(HandCurrentPos + PlayerToCursorDirection * GravityBombCDO->ProjectileSpawnForwardDistance);
			int SpawnedBodyIndex = SpawnerSubsystem->SpawnJoltBody(ActiveItemData.GetItemClass(), ProjectileSpawnLocation, FRotator::ZeroRotator, PlayerCharacter.PlayerId);
			
			PlayerCharacter.CreatedBodyIndexes.Add(SpawnedBodyIndex);
			JoltSubsystem->GetBodyInterface()->AddImpulse(BodyID(SpawnedBodyIndex), PlayerToCursorDirection * GravityBombCDO->ProjectileImpulse);
			//JoltSubsystem->GetBodyInterface()->AddTorque(BodyID(SpawnedBodyIndex), Vec3(0, 0, 550000.f));
			
			JoltSubsystem->GetBodyInterface()->AddImpulse(PlayerBodyID, PlayerToCursorDirection * -GravityBombCDO->ProjectileKnockbackOnPlayer);

			PlayerCharacter.ShootingCooldownFramesLeft = ShootingCooldownInFrames;
		}
		else if (PlayerCharacter.HasItem(ELootItem::StickyBomb))
		{
			PlayerCharacter.RemoveItem(ELootItem::StickyBomb);

			FLootItem ActiveItemData = UAssetManagerSubsystem::GetLootItemRow(GetWorld(), ELootItem::StickyBomb);
			AStickyBomb* StickyBombCDO = ActiveItemData.GetItemCDO<AStickyBomb>();

			const float DegreesStep = 360.f / static_cast<float>(StickyBombCDO->NumBombs);
			const Vec3 BaseDir = Vec3(1.f, 0.f, 0.f);
			for (int i = 0; i < StickyBombCDO->NumBombs; i++)
			{
				const Vec3 LaunchDir = OffsetDirectionAngle(BaseDir, DegreesStep * static_cast<float>(i));
				const Vec3 SpawnPos = PlayerPos + LaunchDir * StickyBombCDO->SpawnRadius;

				const int BombIndex = SpawnerSubsystem->SpawnJoltBody(
					ActiveItemData.GetItemClass(),
					Units::FromJoltPos(SpawnPos),
					FRotator::ZeroRotator,
					PlayerCharacter.PlayerId
				);

				PlayerCharacter.CreatedBodyIndexes.Add(BombIndex);
				JoltSubsystem->GetBodyInterface()->AddImpulse(BodyID(BombIndex), LaunchDir * StickyBombCDO->LaunchImpulse);
			}

			if (StickyBombCDO->ReleaseVFX)
			{
				FString StringToHash = FString::Printf(TEXT("ReleaseStickyBombs%d%d"), InFrameId, PlayerCharacter.PlayerId);
				const auto EffectHash = THashContainer::Generate(StringToHash);
				if (!UsedEffects.Contains(EffectHash))
				{
					UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), StickyBombCDO->ReleaseVFX, Units::FromJoltPos(PlayerPos));
					GetWorld()->GetSubsystem<USoundSubsystem>()->PlaySound("ReleaseStickyBombs");
					AddUsedEffect(InFrameId, EffectHash);
				}
			}

			PlayerCharacter.ShootingCooldownFramesLeft = ShootingCooldownInFrames;
		}
		else if (PlayerCharacter.HasItem(ELootItem::ProjectileBounce))
		{
			PlayerCharacter.RemoveItem(ELootItem::ProjectileBounce);
			//PlayerCharacter.ActiveItem = ELootItem::ProjectileBounce;
			//PlayerCharacter.ItemActiveForFramesNum = 0;

			FLootItem ActiveItemData = UAssetManagerSubsystem::GetLootItemRow(GetWorld(), ELootItem::ProjectileBounce);
			ABananaProjectile* BananaCDO = ActiveItemData.GetItemCDO<ABananaProjectile>();
			
			const FVector ProjectileSpawnLocation = Units::FromJoltPos(HandCurrentPos + PlayerToCursorDirection * BananaCDO->ProjectileSpawnForwardDistance);
			int SpawnedBodyIndex = SpawnerSubsystem->SpawnJoltBody(ActiveItemData.GetItemClass(), ProjectileSpawnLocation, FRotator::ZeroRotator, PlayerCharacter.PlayerId);
			
			PlayerCharacter.CreatedBodyIndexes.Add(SpawnedBodyIndex);
			JoltSubsystem->GetBodyInterface()->AddImpulse(BodyID(SpawnedBodyIndex), PlayerToCursorDirection * BananaCDO->ProjectileImpulse);
			JoltSubsystem->GetBodyInterface()->AddTorque(BodyID(SpawnedBodyIndex), Vec3(0, 0, BananaCDO->ProjectileTorque));
			
			JoltSubsystem->GetBodyInterface()->AddImpulse(PlayerBodyID, PlayerToCursorDirection * -BananaCDO->ProjectileKnockbackOnPlayer);

			PlayerCharacter.ShootingCooldownFramesLeft = ShootingCooldownInFrames;
		}
		else if (PlayerCharacter.HasItem(ELootItem::Boomerang))
		{
			FLootItem ActiveItemData = UAssetManagerSubsystem::GetLootItemRow(GetWorld(), ELootItem::Boomerang);
			PlayerCharacter.RemoveItem(ActiveItemData.Type);

			ABoomerangProjectile* BoomerangCDO = ActiveItemData.GetItemCDO<ABoomerangProjectile>();
			
			const FVector ProjectileSpawnLocation = Units::FromJoltPos(HandCurrentPos + PlayerToCursorDirection * 	BoomerangCDO->ProjectileSpawnForwardDistance);
			int SpawnedBodyIndex = SpawnerSubsystem->SpawnJoltBody(ActiveItemData.GetItemClass(), ProjectileSpawnLocation, FRotator::ZeroRotator, PlayerCharacter.PlayerId);
			
			PlayerCharacter.CreatedBodyIndexes.Add(SpawnedBodyIndex);
			JoltSubsystem->GetBodyInterface()->AddImpulse(BodyID(SpawnedBodyIndex), PlayerToCursorDirection * BoomerangCDO->DefaultProjectileImpulse);
			JoltSubsystem->GetBodyInterface()->AddTorque(BodyID(SpawnedBodyIndex), Vec3(0, 0, BoomerangCDO->TorqueImpulse));
			
			JoltSubsystem->GetBodyInterface()->AddImpulse(PlayerBodyID, PlayerToCursorDirection * -BoomerangCDO->ProjectileKnockbackOnPlayer);

			PlayerCharacter.ShootingCooldownFramesLeft = ShootingCooldownInFrames;

			FString StringToHash = FString::Printf(TEXT("ThrowBoomerang%d"), InFrameId);
			const auto EffectHash = THashContainer::Generate(StringToHash);

			if (!UsedEffects.Contains(EffectHash))
			{
				auto NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), BoomerangCDO->ShootVFX, ProjectileSpawnLocation + FVector(0, 20000.f, 0));
				NiagaraComponent->SetVariableVec3(TEXT("Direction"), Units::FromJoltCoord(PlayerToCursorDirection));
				
				SoundSubsystem->PlaySound("ThrowBoomerang");

				AddUsedEffect(InFrameId, EffectHash);
			}
		}
		else if (PlayerCharacter.HasItem(ELootItem::HomingMissile))
		{
			PlayerCharacter.RemoveItem(ELootItem::HomingMissile);
			PlayerCharacter.ActiveItem = ELootItem::HomingMissile;
			PlayerCharacter.ItemActiveForFramesNum = 0;

			FLootItem ActiveItemData = UAssetManagerSubsystem::GetLootItemRow(GetWorld(), ELootItem::HomingMissile);

			AHomingMissile* HomingMissileCDO = ActiveItemData.GetItemCDO<AHomingMissile>();

			PlayerCharacter.ShootingCooldownFramesLeft = HomingMissileCDO->FrameDurationToShootAllRockets + ShootingCooldownInFrames;
		}
		else if (PlayerCharacter.HasItem(ELootItem::MineSweeper))
		{
			PlayerCharacter.RemoveItem(ELootItem::MineSweeper);
			PlayerCharacter.ActiveItem = ELootItem::MineSweeper;
			PlayerCharacter.ItemActiveForFramesNum = 0;

			FLootItem ActiveItemData = UAssetManagerSubsystem::GetLootItemRow(GetWorld(), ELootItem::MineSweeper);
			AMineSweeper* MineSweeperCDO = ActiveItemData.GetItemCDO<AMineSweeper>();

			PlayerCharacter.ShootingCooldownFramesLeft = MineSweeperCDO->FrameDurationToReleaseAllMines + ShootingCooldownInFrames;
		}
		else if (PlayerCharacter.HasItem(ELootItem::Turret))
		{
			FLootItem ActiveItemData = UAssetManagerSubsystem::GetLootItemRow(GetWorld(), ELootItem::Turret);
			PlayerCharacter.RemoveItem(ActiveItemData.Type);

			ASentryTurret* TurretCDO = ActiveItemData.GetItemCDO<ASentryTurret>();
			
			const FVector ProjectileSpawnLocation = Units::FromJoltPos(HandCurrentPos + PlayerToCursorDirection * 	TurretCDO->ProjectileSpawnForwardDistance);
			int SpawnedBodyIndex = SpawnerSubsystem->SpawnJoltBody(ActiveItemData.GetItemClass(), ProjectileSpawnLocation, FRotator::ZeroRotator, PlayerCharacter.PlayerId);
			
			PlayerCharacter.CreatedBodyIndexes.Add(SpawnedBodyIndex);

			PlayerCharacter.ShootingCooldownFramesLeft = ShootingCooldownInFrames;
		}
		else if (PlayerCharacter.HasItem(ELootItem::Dash))
		{
			PlayerCharacter.RemoveItem(ELootItem::Dash);
			
			JoltSubsystem->GetBodyInterface()->SetLinearVelocity(PlayerBodyID, PlayerToCursorDirection * DefaultDashImpulse);

			FVector EffectSpawnLocation = Units::FromJoltPos(PlayerPos);
			EffectSpawnLocation.Y = 10000.f;

			/*PlayerCharacter.DashCooldownFramesLeft = DashCooldownInFrames;
	#if WITH_EDITOR
			if (GeneralData->bNoCooldowns)
			{
				PlayerCharacter.DashCooldownFramesLeft = 10;
			}
	#endif*/
			
			FString StringToHash = FString::Printf(TEXT("Dash%d%s"), InFrameId, *EffectSpawnLocation.ToString());
			const auto EffectHash = THashContainer::Generate(StringToHash);

			if (!UsedEffects.Contains(EffectHash))
			{
				/*auto NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), GeneralData->DashVFX, EffectSpawnLocation);
				NiagaraComponent->SetVariableVec3(TEXT("Direction"), Units::FromJoltCoord(-PlayerToCursorDirection));
				NiagaraComponent->SetTranslucentSortPriority(250);*/

				const FRotator EffectSpawnRotation = VecDirectionToFRotator(-PlayerToCursorDirection);
				SpawnSpriteActor(GeneralData->PlayerDashVFX, EffectSpawnLocation, EffectSpawnRotation);

				SoundSubsystem->PlaySound("PlayerDash");

				if (PlayerCharacter.bLocalPlayer)
				{
					GameCamera->DirectionalShake(Units::FromJoltCoord(PlayerToCursorDirection));
				}

				if (PlayerCharacter.RenderData.ActivePlayerBodyVFX.IsValid())
				{
					PlayerCharacter.RenderData.ActivePlayerBodyVFX->FinishEarly();
				}

				FActorSpawnParameters ActorSpawnParameters;
				ActorSpawnParameters.bDeferConstruction = true;
			
				auto PlayerBodyVFX = Cast<APlayerBodyVFX>(GetWorld()->SpawnActor(GeneralData->PlayerBodyDashVFXClass, 0, 0, ActorSpawnParameters));
				PlayerBodyVFX->PlayerHeadDynamicMaterial = PlayerCharacter.RenderData.HeadDynamicMaterial;
				PlayerBodyVFX->PlayerEyesDynamicMaterial = PlayerCharacter.RenderData.EyesDynamicMaterial;
				PlayerBodyVFX->PlayerBodyDynamicMaterial = PlayerCharacter.RenderData.BodyDynamicMaterial;
				PlayerBodyVFX->HammerDynamicMaterial = PlayerCharacter.RenderData.HammerDynamicMaterial;
				PlayerBodyVFX->StickDynamicMaterial = PlayerCharacter.RenderData.StickDynamicMaterial;
				PlayerBodyVFX->FinishSpawning({});
				PlayerCharacter.RenderData.ActivePlayerBodyVFX = PlayerBodyVFX;

				PlayerBodyVFX->AttachToActor(PlayerCharacter.PlayerRenderer, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			
				AddUsedEffect(InFrameId, EffectHash);
			}
		}
		else if (PlayerCharacter.HasItem(ELootItem::OrbitalSpheres))
		{
			PlayerCharacter.RemoveItem(ELootItem::OrbitalSpheres);
			PlayerCharacter.ActiveItem = ELootItem::OrbitalSpheres;
			PlayerCharacter.ItemActiveForFramesNum = 0;

			FLootItem OrbitalItemData = UAssetManagerSubsystem::GetLootItemRow(GetWorld(), ELootItem::OrbitalSpheres);
			AOrbitalSphere* CDO = OrbitalItemData.GetItemCDO<AOrbitalSphere>();

			const Vec3 SpawnCenter = JoltSubsystem->GetBodyInterface()->GetCenterOfMassPosition(BodyID(PlayerCharacter.ShoulderBodyIndex));
			const FVector SpawnLoc = Units::FromJoltPos(SpawnCenter);

			for (int i = 0; i < CDO->SphereCount; i++)
			{
				const int SphereIdx = SpawnerSubsystem->SpawnJoltBody(OrbitalItemData.GetItemClass(), SpawnLoc, FRotator::ZeroRotator, PlayerCharacter.PlayerId);
				Cast<AOrbitalSphere>(SpawnerSubsystem->GetBodyActor(BodyID(SphereIdx)))->OrbitIndex = i;
				PlayerCharacter.CreatedBodyIndexes.Add(SphereIdx);
			}
		}
	}

	/**
	 * GRAPPLE ROPE - Right mouse button shoots a rope to the first hit physics body.
	 * The rope stays while RMB is held and despawns on release.
	 */
	/*if (GrappleRopeConstraintClass)
	{
		const bool bRopeActive = PlayerCharacter.GrappleRopeConstraintId != -1
			&& SpawnerSubsystem->DoesConstraintExistInSimulation(PlayerCharacter.GrappleRopeConstraintId);

		if (CharacterInput.bDash && !bRopeActive)
		{
			Vec3 OutRopeHitPos;
			BodyID OutRopeHitBodyID;
			RayObjectLayerFilter GrappleRayFilter({EObjectLayer::NonMoving, EObjectLayer::Moving});

			if (JoltSubsystem->CastRayFirstHit(HandCurrentPos, PlayerToCursorDirection * GrappleRopeMaxRange, OutRopeHitPos, OutRopeHitBodyID, &GrappleRayFilter))
			{
				if (OutRopeHitBodyID.GetIndex() != PlayerCharacter.GrabbedBodyIndex)
				{
					float RopeLength = (OutRopeHitPos - HandCurrentPos).Length();
					if (RopeLength > GrapplePullRange * 2.f)
					{
						RopeLength -= GrapplePullRange;
					}

					// Find the outer edge of the shoulder body in the shoot direction so
					// the rope torques the shoulder when it rotates rather than pulling from center.
					const BodyID ShoulderBodyID = BodyID(PlayerCharacter.ShoulderBodyIndex);
					Vec3 ShoulderPos = JoltSubsystem->GetBodyInterface()->GetCenterOfMassPosition(ShoulderBodyID);
					Quat ShoulderRot = JoltSubsystem->GetBodyInterface()->GetRotation(ShoulderBodyID);
					const BoxShape* ShoulderBoxShape = static_cast<const BoxShape*>(
						JoltSubsystem->GetBodyInterface()->GetShape(ShoulderBodyID).GetPtr()
					);
					Vec3 HalfExtent = ShoulderBoxShape->GetHalfExtent();
					Vec3 LocalShootDir = ShoulderRot.Conjugated() * PlayerToCursorDirection;
					Vec3 AbsDir(abs(LocalShootDir.GetX()), abs(LocalShootDir.GetY()), abs(LocalShootDir.GetZ()));
					float EdgeT = FLT_MAX;
					for (int i = 0; i < 3; i++)
						if (AbsDir[i] > 1e-6f) EdgeT = min(EdgeT, HalfExtent[i] / AbsDir[i]);
					Vec3 ShoulderEdgePoint = ShoulderPos + ShoulderRot * (LocalShootDir * EdgeT * GrappleShoulderAttachAlpha);

					FJoltConstraintData ConstraintData;
					ConstraintData.BodyID1 = OutRopeHitBodyID;
					ConstraintData.BodyID2 = ShoulderBodyID;
					ConstraintData.bHasBody2WorldAttachPoint = true;
					ConstraintData.Body2WorldAttachPoint = ShoulderEdgePoint;
					ConstraintData.MinDistance = 0.f;
					ConstraintData.MaxDistance = RopeLength;
					PlayerCharacter.GrappleCurrentMaxDistance = RopeLength;
					
					PlayerCharacter.GrappleRopeConstraintId = SpawnerSubsystem->SpawnJoltConstraint(
						GrappleRopeConstraintClass,
						Units::FromJoltPos(OutRopeHitPos),
						ConstraintData
					);

					const auto EffectHash = THashContainer::Generate(FString::Printf(TEXT("ShootRope%d%d"), InFrameId, PlayerCharacter.PlayerId));
					if (!UsedEffects.Contains(EffectHash))
					{
						GetWorld()->GetSubsystem<USoundSubsystem>()->PlaySound("ShootRope");
						AddUsedEffect(InFrameId, EffectHash);
					}
				}
			}
		}

		if (!CharacterInput.bDash && bRopeActive)
		{
			SpawnerSubsystem->DespawnJoltConstraint(PlayerCharacter.GrappleRopeConstraintId);
			PlayerCharacter.GrappleRopeConstraintId = -1;
			PlayerCharacter.GrappleCurrentMaxDistance = -1.f;
		}

		if (bRopeActive && PlayerCharacter.GrappleCurrentMaxDistance > GrappleMinDistance)
		{
			PlayerCharacter.GrappleCurrentMaxDistance = FMath::Max(
				GrappleMinDistance,
				PlayerCharacter.GrappleCurrentMaxDistance - GrappleShrinkRate
			);
			AJoltConstraintActor* ConstraintActor = SpawnerSubsystem->GetConstraintActor(PlayerCharacter.GrappleRopeConstraintId);
			checkf(ConstraintActor, TEXT("Grapple rope constraint actor doesnt exist"));
			ConstraintActor->GetConstraintInstance<DistanceConstraint>()->SetDistance(0.f, PlayerCharacter.GrappleCurrentMaxDistance);
		}
	}*/

	PlayerCharacter.RenderData.BodyLocation = Units::FromJoltPos(PlayerPos);
	PlayerCharacter.RenderData.MaxBullets = MaxBullets;
	PlayerCharacter.RenderData.SimulationFrameId = InFrameId;
	PlayerCharacter.RenderData.CursorLocation = Units::FromJoltPos(CursorPos);
}

void AMyPlayerController::StepCharacterSimulationTutorial(const int& InFrameId, FPlayerCharacter& PlayerCharacter)
{
	if (PlayerCharacter.bDead)
	{
		//LOG_TEMPORARY(TEXT("Player dead, so skipping"));
		if (UHelper::GetMyGameState(GetWorld())->GetFirstPlayerToDieId() != PlayerCharacter.PlayerId)
		{
			return;
		}
		
		PlayerCharacter.DeadFramesNum++;

		if (PlayerCharacter.DeadFramesNum == 1)
		{
			bool bAllPlayersDeadOnSameFrame = true;
			for (const auto& TempPlayerCharacter : PlayerCharacters)
			{
				if (!TempPlayerCharacter.bDead || (TempPlayerCharacter.DeadOnFrameId != PlayerCharacter.DeadOnFrameId))
				{
					bAllPlayersDeadOnSameFrame = false;
				}
			}

			if (bAllPlayersDeadOnSameFrame)
			{
				OnTeamsTie.Broadcast();
				RespawnPlayerCharacters();
				UHelper::GetMyGameState(GetWorld())->ClearRoundData();

				return;
			}
			else
			{
				UHelper::GetMyGameState(GetWorld())->EndRound();
			}
		}

		// At this point players death is 100% confirmed. Since it's outside rollback window.
		if (PlayerCharacter.DeadFramesNum > GetRollbackWindow() && PlayerCharacter.DeadFramesCurrentStep == 0)
		{
			PlayerCharacter.DeadFramesCurrentStep = 1;
			IncreaseOppositeTeamsScore(PlayerCharacter.PlayerTeam);
			LOG_NETWORKING(TEXT("Increase opposite team score %s"), *UEnum::GetValueAsString(PlayerCharacter.PlayerTeam));
		}

		return;
	}

	if (GetMyGameState()->GetPlayerIdThatHasCrown() == PlayerCharacter.PlayerId)
	{
		PlayerCharacter.CrownHeldFramesNum++;
	}

	StepCharacterSimulationMovementAndAbilities(InFrameId, PlayerCharacter);
}

void AMyPlayerController::StepCharacterSimulationTimeAttack(const int& InFrameId, FPlayerCharacter& PlayerCharacter)
{
	if (PlayerCharacter.bDead)
	{
		OnTeamsTie.Broadcast();
		RespawnPlayerCharacters();
		UHelper::GetMyGameState(GetWorld())->ClearRoundData();

		return;
	}

	if (bResetInputPressed && !GetMyGameState()->HasRoundEnded())
	{
		ReloadCurrentLevel();

		return;
	}

	StepCharacterSimulationMovementAndAbilities(InFrameId, PlayerCharacter);
}

void AMyPlayerController::StepCharacterSimulationDeathmatch(const int& InFrameId, FPlayerCharacter& PlayerCharacter)
{
	//LOG_TEMPORARY(TEXT("[Frame %d] StepCharacterSimulationDeathmatch for character %d"), InFrameId, PlayerCharacter.PlayerId);
	if (PlayerCharacter.bDead)
	{
		//LOG_TEMPORARY(TEXT("Player dead, so skipping"));
		if (UHelper::GetMyGameState(GetWorld())->GetFirstPlayerToDieId() != PlayerCharacter.PlayerId)
		{
			return;
		}

		PlayerCharacter.DeadFramesNum++;
		
		if (PlayerCharacter.DeadFramesNum == 1)
		{
			bool bAllPlayersDeadOnSameFrame = true;
			for (const auto& TempPlayerCharacter : PlayerCharacters)
			{
				if (!TempPlayerCharacter.bDead || (TempPlayerCharacter.DeadOnFrameId != PlayerCharacter.DeadOnFrameId))
				{
					bAllPlayersDeadOnSameFrame = false;
				}
			}

			if (bAllPlayersDeadOnSameFrame)
			{
				OnTeamsTie.Broadcast();
				RespawnPlayerCharacters();
				UHelper::GetMyGameState(GetWorld())->ClearRoundData();

				return;
			}
		}

		// At this point players death is 100% confirmed. Since it's outside rollback window.
		if (PlayerCharacter.DeadFramesNum == (GetRollbackWindow() + 1))
		{
			if (PlayerCharacter.DeadFramesCurrentStep == 0)
			{
				PlayerCharacter.DeadFramesCurrentStep = 1;
				IncreaseCurrentGameModeOppositeTeamsScore(PlayerCharacter.PlayerTeam);
			}

			if (CurrentGameModeTeamScores.GetTeamScore(FTeamScores::GetOppositeTeam(PlayerCharacter.PlayerTeam)) == GeneralData->DeathmatchMaxPoints)
			{
				UHelper::GetMyGameState(GetWorld())->EndRound();
			}
		}

		if (!GetMyGameState()->HasRoundEnded())
		{
			if (PlayerCharacter.DeadFramesNum == 180 - (GetRollbackWindow() + 1))
			{
				bLocalPlayerInputDisabled = true;
				AccumulatedMouseDelta = FVector2D::ZeroVector;
			}

			if (PlayerCharacter.DeadFramesNum == 180)
			{
				ReloadCurrentLevel();
			}
		}

		return;
	}

	StepCharacterSimulationMovementAndAbilities(InFrameId, PlayerCharacter);
}

void AMyPlayerController::StepCharacterSimulationRopedTogether(const int& InFrameId, FPlayerCharacter& PlayerCharacter)
{
	if (PlayerCharacter.bDead)
	{
		if (UHelper::GetMyGameState(GetWorld())->GetFirstPlayerToDieId() != PlayerCharacter.PlayerId)
		{
			return;
		}
		
		PlayerCharacter.DeadFramesNum++;

		if (PlayerCharacter.DeadFramesNum == 1)
		{
			UHelper::GetMyGameState(GetWorld())->EndRound();
		}

		// At this point players death should be confirmed. Since we won't rollback more than 20 frames.
		if (PlayerCharacter.DeadFramesNum == 20 && PlayerCharacter.DeadFramesCurrentStep == 0)
		{
			PlayerCharacter.DeadFramesCurrentStep = 1;

			if (PlayerCharacter.bLocalPlayer)
			{
				bLocalPlayerInputDisabled = true;
				AccumulatedMouseDelta = FVector2D::ZeroVector;
				LOG_NETWORKING(TEXT("Disable inputs for the player that died only"));
			}
		}

		return;
	}

	StepCharacterSimulationMovementAndAbilities(InFrameId, PlayerCharacter);
}

void AMyPlayerController::StepCharacterSimulationPullingRope(const int& InFrameId, FPlayerCharacter& PlayerCharacter)
{
	if (PlayerCharacter.bDead)
	{
		if (UHelper::GetMyGameState(GetWorld())->GetFirstPlayerToDieId() != PlayerCharacter.PlayerId)
		{
			return;
		}
		
		PlayerCharacter.DeadFramesNum++;

		if (PlayerCharacter.DeadFramesNum == 1)
		{
			UHelper::GetMyGameState(GetWorld())->EndRound();
		}

		if (PlayerCharacter.DeadFramesNum == 20 && PlayerCharacter.DeadFramesCurrentStep == 0)
		{
			PlayerCharacter.DeadFramesCurrentStep = 1;
			IncreaseOppositeTeamsScore(PlayerCharacter.PlayerTeam);
		}

		return;
	}

	StepCharacterSimulationMovementAndAbilities(InFrameId, PlayerCharacter);
}

void AMyPlayerController::StepCharacterSimulationCaptureTheFlag(const int& InFrameId, FPlayerCharacter& PlayerCharacter)
{
	if (PlayerCharacter.bDead)
	{
		PlayerCharacter.DeadFramesNum++;
		
		// At this point players death should be confirmed.
		if (PlayerCharacter.DeadFramesNum > GetRollbackWindow() && PlayerCharacter.DeadFramesCurrentStep == 0)
		{
			PlayerCharacter.DeadFramesCurrentStep = 1;

			if (PlayerCharacter.bLocalPlayer)
			{
				bLocalPlayerInputDisabled = true;
				AccumulatedMouseDelta = FVector2D::ZeroVector;
				LOG_NETWORKING(TEXT("Disable inputs for the player that died only"));
			}
		}

		if (PlayerCharacter.DeadFramesNum == GeneralData->CaptureTheFlagPlayerRespawnFrames)
		{
			if (PlayerCharacter.bLocalPlayer && PlayerCharacter.DeadFramesCurrentStep == 1)
			{
				bLocalPlayerInputDisabled = false;
				LOG_NETWORKING(TEXT("Re-enable inputs for the player that died only"));
				
				PlayerCharacter.DeadFramesCurrentStep = 2;
			}

			PlayerCharacter.InvulnerabilityFramesNum = InvulnerabilityFramesAfterRespawnNum;
			ResetPlayerCharacterToStartingPosition(PlayerCharacter);
		}
	
		return;
	}

	StepCharacterSimulationMovementAndAbilities(InFrameId, PlayerCharacter);
}

void AMyPlayerController::StepCharacterSimulationKingOfTheCrown(const int& InFrameId, FPlayerCharacter& PlayerCharacter)
{
	// If the winning amount of points was reached by a player, then we keep increasing the frames regardless
	// Even if he already lost the crown or died. This is purely to count how many frames have passed and to wait till we get out of rollback window
	if (PlayerCharacter.CrownHeldFramesNum >= GeneralData->HoldCrownFramesToWin)
	{
		PlayerCharacter.CrownHeldFramesNum++;

		const int FramesWaited = PlayerCharacter.CrownHeldFramesNum - GeneralData->HoldCrownFramesToWin;
		if (CrownModeEndStep == 0 && FramesWaited > GetRollbackWindow())
		{
			IncreaseTeamsScore(PlayerCharacter.PlayerTeam);
			CrownModeEndStep = 1;

			// This only happens when it's 100% confirmed and it cannot be rolled back, so we can just spawn the VFX without any checks
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), GeneralData->WinCrownVFX, PlayerCharacter.RenderData.BodyLocation);
		}
	}
	
	if (PlayerCharacter.bDead)
	{
		PlayerCharacter.DeadFramesNum++;
		
		// At this point players death should be confirmed. Since we won't rollback more than 20 frames.
		if (PlayerCharacter.DeadFramesNum == 20 && PlayerCharacter.DeadFramesCurrentStep == 0)
		{
			PlayerCharacter.DeadFramesCurrentStep = 1;

			if (PlayerCharacter.bLocalPlayer)
			{
				bLocalPlayerInputDisabled = true;
				AccumulatedMouseDelta = FVector2D::ZeroVector;
				LOG_NETWORKING(TEXT("Disable inputs for the player that died only"));
			}
		}

		// 5 second respawn timer
		if (PlayerCharacter.DeadFramesNum == GeneralData->KingOfTheCrownPlayerRespawnFrames)
		{
			if (PlayerCharacter.bLocalPlayer && PlayerCharacter.DeadFramesCurrentStep == 1)
			{
				bLocalPlayerInputDisabled = false;
				LOG_NETWORKING(TEXT("Re-enable inputs for the player that died only"));
				
				PlayerCharacter.DeadFramesCurrentStep = 2;
			}

			PlayerCharacter.InvulnerabilityFramesNum = InvulnerabilityFramesAfterRespawnNum;
			ResetPlayerCharacterToStartingPosition(PlayerCharacter);
		}
	
		return;
	}

	if (GetMyGameState()->GetPlayerIdThatHasCrown() == PlayerCharacter.PlayerId)
	{
		PlayerCharacter.CrownHeldFramesNum++;
		if (PlayerCharacter.CrownHeldFramesNum == GeneralData->HoldCrownFramesToWin)
		{
			UHelper::GetMyGameState(GetWorld())->EndRound();

			auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();
			
			FVector EffectSpawnLocation = JoltSubsystem->GetCenterOfMassPosition(BodyID(PlayerCharacter.PlayerId));
			EffectSpawnLocation.Y = 10000.f;
			FString StringToHash = FString::Printf(TEXT("PlayerWinKingOfCrown%d%s%d"), InFrameId, *EffectSpawnLocation.ToString(), PlayerCharacter.PlayerId);
			const auto EffectHash = THashContainer::Generate(StringToHash);
			if (!UsedEffects.Contains(EffectHash))
			{
				GameCamera->Shake(1.5f);

				GetWorld()->GetSubsystem<USoundSubsystem>()->PlaySound("PlayerWinKingOfCrown");

				auto NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), GeneralData->PlaceFlagVFX, EffectSpawnLocation);

				AddUsedEffect(InFrameId, EffectHash);
			}
		}
	}

	StepCharacterSimulationMovementAndAbilities(InFrameId, PlayerCharacter);
}

int AMyPlayerController::GetBulletReloadCooldownInFrames(const FPlayerCharacter& PlayerCharacter)
{
	if (GetCurrentGameModeType() == EGameMode::KingOfTheCrown && GetMyGameState()->GetPlayerIdThatHasCrown() == PlayerCharacter.PlayerId)
	{
		return BulletReloadCooldownInFramesWhenHoldingCrown;
	}

	return BulletReloadCooldownInFrames;
}

void AMyPlayerController::PlayerExplodeDirectional(const int& InFrameId, const AJoltBodyActor* PlayerBodyActor, const BodyID& PlayerBodyID, const Vec3& HitPosition, const Vec3& HitDirection, int KilledByPlayerId)
{
	auto SoundSubsystem = GetWorld()->GetSubsystem<USoundSubsystem>();
	
	//LOG_TEMPORARY(TEXT("[Frame %d] PlayerExplodeDirectional %d; HitPos %s; KilledByPlayerId %d;"), InFrameId, PlayerBodyID.GetIndex(), *Units::FromJoltPos(HitPosition).ToString(), KilledByPlayerId);
	
	FPlayerCharacter& ThisPlayerCharacter = GetPlayerCharacter(PlayerBodyActor->BelongsToPlayerId);
	if (ThisPlayerCharacter.InvulnerabilityFramesNum > 0)
	{
		return;
	}
	
	MarkPlayerCharacterAsDead(InFrameId, ThisPlayerCharacter);

	FVector EffectSpawnLocation = Units::FromJoltPos(HitPosition);
	EffectSpawnLocation.Y = 10000.f;
	FString StringToHash = FString::Printf(TEXT("PlayerExplodeDirectional%d%d"), InFrameId, PlayerBodyID.GetIndex());
	const auto EffectHash = THashContainer::Generate(StringToHash);
	FString VoiceLineName = SoundSubsystem->ChooseRandomVoiceLineDeterministic({ "V_Haha", "V_FeelTheBurn", "V_YoureFired" });
	
	if (!UsedEffects.Contains(EffectHash))
	{
		if (KilledByPlayerId >= 0)
		{
			GetWorld()->GetSubsystem<USoundSubsystem>()->PlayVoiceLine(VoiceLineName, GetPlayerRenderer(KilledByPlayerId));
		}
		
		GameCamera->DirectionalShake(Units::FromJoltCoord(HitDirection), 1.5f);

		GetWorld()->GetSubsystem<USoundSubsystem>()->PlaySound("PlayerDie");

		//auto NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), GeneralData->PlayerExplodeDirectional, EffectSpawnLocation);
		//NiagaraComponent->SetVariableVec3(TEXT("Direction"), Units::FromJoltCoord(HitDirection));
		//NiagaraComponent->SetVariableLinearColor(TEXT("ExplosionColor"), ThisPlayerCharacter.RenderData.ColorScheme.MainColor);

		const FRotator EffectSpawnRotation = VecDirectionToFRotator(HitDirection);
		//GetWorld()->SpawnActor(GeneralData->PlayerExplodeVFX, &EffectSpawnLocation);

		if (ThisPlayerCharacter.PlayerTeam == EPlayerTeam::Yellow)
		{
			SpawnSpriteActor(GeneralData->PlayerExplodeLightningVFX, EffectSpawnLocation, EffectSpawnRotation);
			SpawnSpriteActor(GeneralData->PlayerExplodeDirectionalVFX, EffectSpawnLocation, EffectSpawnRotation);
		}
		else if (ThisPlayerCharacter.PlayerTeam == EPlayerTeam::Blue)
		{
			SpawnSpriteActor(GeneralData->PlayerExplodeLightningBlueVFX, EffectSpawnLocation, EffectSpawnRotation);
			SpawnSpriteActor(GeneralData->PlayerExplodeDirectionalBlueVFX, EffectSpawnLocation, EffectSpawnRotation);
		}
		
		const auto ViewTargetLocation = GetViewTarget()->GetActorLocation();
		GetWorld()->SpawnActor(PlayerDieVFXActor, &ViewTargetLocation);

		AddUsedEffect(InFrameId, EffectHash);
	}
}

void AMyPlayerController::PlayerExplodeOmniDirectional(const int& InFrameId, const AJoltBodyActor* PlayerBodyActor, const BodyID& PlayerBodyID)
{
	auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();
	
	FPlayerCharacter& ThisPlayerCharacter = GetPlayerCharacter(PlayerBodyActor->BelongsToPlayerId);
	if (ThisPlayerCharacter.InvulnerabilityFramesNum > 0)
	{
		return;
	}

	// Need to fetch player position BEFORE marking him as dead, because that changes its position
	FVector EffectSpawnLocation = JoltSubsystem->GetCenterOfMassPosition(PlayerBodyID);
	MarkPlayerCharacterAsDead(InFrameId, ThisPlayerCharacter);

	EffectSpawnLocation.Y = 10000.f;
	FString StringToHash = FString::Printf(TEXT("PlayerExplodeOmniDirectional%d%s%d"), InFrameId, *EffectSpawnLocation.ToString(), PlayerBodyID.GetIndex());
	const auto EffectHash = THashContainer::Generate(StringToHash);
	if (!UsedEffects.Contains(EffectHash))
	{
		GameCamera->Shake();
		
		//auto NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), GeneralData->PlayerExplodeOmniDirectional, EffectSpawnLocation);
		//NiagaraComponent->SetVariableLinearColor(TEXT("ExplosionColor"), ThisPlayerCharacter.RenderData.ColorScheme.MainColor);

		//GetWorld()->SpawnActor(GeneralData->PlayerExplodeVFX, &EffectSpawnLocation);
		//SpawnSpriteActor(GeneralData->PlayerExplodeLightningVFX, EffectSpawnLocation, VecDirectionToFRotator(-JoltSubsystem->GetBodyInterface()->GetLinearVelocity(PlayerBodyID).NormalizedOr(Vec3::sZero())));
		
		GetWorld()->GetSubsystem<USoundSubsystem>()->PlaySound("PlayerDie");

		GetWorld()->SpawnActor(GeneralData->PlayerExplodeVFX, &EffectSpawnLocation);
		const auto ViewTargetLocation = GetViewTarget()->GetActorLocation();
		GetWorld()->SpawnActor(PlayerDieVFXActor, &ViewTargetLocation);

		AddUsedEffect(InFrameId, EffectHash);
	}
}

void AMyPlayerController::BulletHit(const int& InFrameId, const BodyID& InBulletBodyID, const Vec3& BulletLinearVelocity, const FContact& Contact)
{
	auto SpawnerSubsystem = GetWorld()->GetSubsystem<USpawnerSubsystem>();
	AJoltBodyActor* ProjectileBodyActor = nullptr;
	if (SpawnerSubsystem->DoesBodyExistInSimulation(InBulletBodyID))
	{
		ProjectileBodyActor = SpawnerSubsystem->GetBodyActor(InBulletBodyID);

		if (auto BananaProjectile = Cast<ABananaProjectile>(ProjectileBodyActor))
		{
			if (BananaProjectile->BouncesLeft > 0)
			{
				ProjectileBounce(InFrameId, BananaProjectile, BulletLinearVelocity, Contact.WorldNormal);
			}
		}
		
		if (ProjectileBodyActor->BelongsToPlayerId >= 0)
		{
			GetPlayerCharacter(ProjectileBodyActor->BelongsToPlayerId).CreatedBodyIndexes.Remove(InBulletBodyID.GetIndex());
		}

		SpawnerSubsystem->DespawnJoltBody(InBulletBodyID);
	}

	BodyID HitBodyID = Contact.BodyID2;
	if (HitBodyID == InBulletBodyID)
	{
		HitBodyID = Contact.BodyID1;
	}

	if (SpawnerSubsystem->DoesBodyExistInSimulation(HitBodyID))
	{
		auto HitBodyActor = SpawnerSubsystem->GetBodyActor(HitBodyID);
		if (Cast<ASentryTurret>(HitBodyActor))
		{
			SpawnerSubsystem->DespawnJoltBody(HitBodyID);
		}
		else if (Cast<AMineSweeper>(HitBodyActor))
		{
			SpawnerSubsystem->DespawnJoltBody(HitBodyID);
		}
		else if (HitBodyActor->BodyName == EBodyName::PracticeTarget)
		{
			// NOT SUPPORTING ROLLBACK. Since I assume targets will only be used in single player 
			GetWorld()->GetSubsystem<USoundSubsystem>()->PlaySound("PracticeTargetHit");

			const FVector EffectSpawnLocation = Units::FromJoltPos(GetWorld()->GetSubsystem<UJoltSubsystem>()->GetBodyInterface()->GetPosition(HitBodyID));
			auto NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), GeneralData->DashVFX, EffectSpawnLocation);
			NiagaraComponent->SetVariableVec3(TEXT("Direction"), Units::FromJoltCoord(BulletLinearVelocity.NormalizedOr(Vec3::sZero())));
			NiagaraComponent->SetTranslucentSortPriority(250);
			
			SpawnerSubsystem->DespawnJoltBodyWithRelatedConstraints(HitBodyID);
		}
		else if (HitBodyActor->BodyName == EBodyName::Destructible)
		{
			ADestructibleJoltBodyActor* DestructibleBodyActor = Cast<ADestructibleJoltBodyActor>(HitBodyActor);
			checkf(DestructibleBodyActor, TEXT("Body of type Destructible is not using the correct class"));

			if (!DestructibleBodyActor->IsDestroyedIntoSeparateParts())
			{
				DestructibleBodyActor->DestroyAndSeparateIntoParts(BulletLinearVelocity.NormalizedOr(-Vec3::sAxisY()) * DefaultProjectileImpulse, Contact.ContactWorldPosition);
			}
		}
	}
	
	FVector EffectSpawnLocation = Units::FromJoltPos(Contact.ContactWorldPosition);
	EffectSpawnLocation.Y = 10000.f;
	const float SquaredDistanceFromLocalPlayer = (LocalPlayerBody->GetCenterOfMassPosition() - Contact.ContactWorldPosition).LengthSq();
			
	FString StringToHash = FString::Printf(TEXT("Bullet%d%s%f"), InFrameId, *EffectSpawnLocation.ToString(), SquaredDistanceFromLocalPlayer);
	const auto EffectHash = THashContainer::Generate(StringToHash);

	if (!UsedEffects.Contains(EffectHash))
	{
		TObjectPtr<UNiagaraSystem> FireballImpactVFX = GeneralData->FireballImpactProceduralVFX;
		if (ProjectileBodyActor && ProjectileBodyActor->BelongsToPlayerId >=0 && GetPlayerCharacter(ProjectileBodyActor->BelongsToPlayerId).PlayerTeam == EPlayerTeam::Blue)
		{
			FireballImpactVFX = GeneralData->FireballBlueImpactProceduralVFX;
		}
		
		auto NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), FireballImpactVFX, EffectSpawnLocation);
		NiagaraComponent->SetVariableVec3(TEXT("Direction"), Units::FromJoltCoord(BulletLinearVelocity.NormalizedOr(Vec3::sZero())));

		//const FRotator EffectSpawnRotation = VecDirectionToFRotator(-BulletLinearVelocity.NormalizedOr(Vec3::sZero()));
		//SpawnSpriteActor(GeneralData->FireballImpactVFX, EffectSpawnLocation, EffectSpawnRotation);
		//SpawnSpriteActor(GeneralData->ShootFireballVFX, EffectSpawnLocation, EffectSpawnRotation);

		GetWorld()->GetSubsystem<USoundSubsystem>()->PlaySound("FireballImpact");
			
		constexpr float MaxDistanceToAffectCamera = 50.f;
		//DrawDebugSphere(GetWorld(), NiagaraSpawnLocation, MaxDistanceToAffectCamera * 100.f, 6, FColor::White, false, 0.016f, SDPG_World, 60.f);
				
		if (SquaredDistanceFromLocalPlayer < JPH::Square(MaxDistanceToAffectCamera))
		{
			const auto ViewTargetLocation = GetViewTarget()->GetActorLocation();
			GetWorld()->SpawnActor(BulletHitVFXActor, &ViewTargetLocation);
		}

		AddUsedEffect(InFrameId, EffectHash);
	}
}

void AMyPlayerController::ProjectileBounce(const int& InFrameId, ABananaProjectile* BananaProjectile, const Vec3& BodyLinearVel, const Vec3& ContactWorldNormal)
{
	auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();
	auto SpawnerSubsystem = GetWorld()->GetSubsystem<USpawnerSubsystem>();

	auto ItemData = UAssetManagerSubsystem::GetLootItemRow(GetWorld(), ELootItem::ProjectileBounce);
	
	const Vec3 BodyVelNormal = BodyLinearVel.NormalizedOr(Vec3::sZero());
	const Vec3 RotateAroundNormal = ContactWorldNormal;
	Vec3 BounceNormal = BodyVelNormal - 2 * BodyVelNormal.Dot(RotateAroundNormal) * RotateAroundNormal;

	const FVector ProjectileSpawnLocation = Units::FromJoltPos(JoltSubsystem->GetBodyInterface()->GetCenterOfMassPosition(BananaProjectile->GetBodyID()) + BounceNormal * 20.f);

	FPlayerCharacter& OwnerPlayerCharacter = GetPlayerCharacter(BananaProjectile->BelongsToPlayerId);
	float ProjectileShootImpulse = BananaProjectile->ProjectileImpulse * 0.5f;
	const int NewBouncesLeft = BananaProjectile->BouncesLeft - 1;
					
	for (int i = 0; i < BananaProjectile->SpawnNumOnBounce; i++)
	{
		int SpawnedBodyIndex = SpawnerSubsystem->SpawnJoltBody(ItemData.GetItemClass(), ProjectileSpawnLocation, FRotator::ZeroRotator, BananaProjectile->BelongsToPlayerId);
		Cast<ABananaProjectile>(SpawnerSubsystem->GetBodyActor(SpawnedBodyIndex))->BouncesLeft = NewBouncesLeft;
					
		const float RandomDegreesOffset = static_cast<float>(JoltSubsystem->RandomNumberGenerator.NextIntInRange(-BananaProjectile->DegreesOffsetOnBounce, BananaProjectile->DegreesOffsetOnBounce));
		Vec3 ProjectileShootDirection = OffsetDirectionAngle(BounceNormal, RandomDegreesOffset);

		OwnerPlayerCharacter.CreatedBodyIndexes.Add(SpawnedBodyIndex);
		
		const float RandomAlpha = JoltSubsystem->RandomNumberGenerator.NextFloat();
		const float BounceImpulse = FMath::Lerp(BananaProjectile->MinProjectileImpulseOnBounce, BananaProjectile->MaxProjectileImpulseOnBounce, RandomAlpha);
		JoltSubsystem->GetBodyInterface()->AddImpulse(BodyID(SpawnedBodyIndex), ProjectileShootDirection * BounceImpulse);
		
		JoltSubsystem->GetBodyInterface()->AddTorque(BodyID(SpawnedBodyIndex), Vec3(0, 0, BananaProjectile->ProjectileTorque));
	}
}

void AMyPlayerController::BombExplode(const int& InFrameId, const BodyID& BombBodyID)
{
	auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();
	auto SpawnerSubsystem = GetWorld()->GetSubsystem<USpawnerSubsystem>();
	auto SoundSubsystem = GetWorld()->GetSubsystem<USoundSubsystem>();

	const Vec3 BombPosition = JoltSubsystem->GetBodyInterface()->GetCenterOfMassPosition(BombBodyID);
	const FVector BombLocation = Units::FromJoltPos(BombPosition);
	FVector EffectSpawnLocation = BombLocation + FVector(0, 10000.f, 0);
	FString StringToHash = FString::Printf(TEXT("BombExplode%d%s"), InFrameId, *EffectSpawnLocation.ToString());
	const auto EffectHash = THashContainer::Generate(StringToHash);

	if (!UsedEffects.Contains(EffectHash))
	{
		// NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), GeneralData->ExplosiveVFX, EffectSpawnLocation);
		//NiagaraComponent->SetVariableVec3(TEXT("Direction"), Units::FromJoltCoord(-Contact.Body1LinearVelocity.Normalized()));
		//NiagaraComponent->SetTranslucentSortPriority(126);

		SpawnSpriteActor(GeneralData->BombExplodeVFX, EffectSpawnLocation);

		SoundSubsystem->PlaySound("BombExplode");
			
		AddUsedEffect(InFrameId, EffectHash);
	}

	SphereShape Sphere(80.f);
	Sphere.SetEmbedded();
	Vec3 CastPosition = BombPosition;
	RayObjectLayerFilter ObjectLayerFilter({EObjectLayer::PlayerTakeDamageSensor});
	auto CollectedHits = JoltSubsystem->CollideShape(Sphere, CastPosition, &ObjectLayerFilter);
				
	//DrawDebugSphere(GetWorld(), Units::FromJoltPos(CastPosition), Units::FromJoltUnits(Sphere.GetRadius()), 10.f, FColor::Red, false, 5.f, SDPG_World, 30.f);
	for (const CollideShapeResult& mHit : CollectedHits.mHits)
	{
		auto BodyActor = SpawnerSubsystem->GetBodyActor(mHit.mBodyID2.GetIndex());
		if (BodyActor->bIsPlayer)
		{
			//LOG_TEMPORARY(TEXT("[Frame %d] Player %d caught in bomb explosion %d"), InFrameId, BodyActor->BelongsToPlayerId, BombBodyID.GetIndex());
			const Vec3 PlayerPos = JoltSubsystem->GetBodyInterface()->GetCenterOfMassPosition(mHit.mBodyID2);
			const Vec3 ExplodeDirection = (PlayerPos - BombPosition).NormalizedOr(Vec3::sZero());
			PlayerExplodeDirectional(InFrameId, BodyActor, BodyID(BodyActor->BelongsToPlayerId), BombPosition, ExplodeDirection);
		}
	}
			
	SpawnerSubsystem->DespawnJoltBody(BombBodyID);
}

void AMyPlayerController::PlayerHandContactWithFlagBase(const int& InFrameId, AJoltBodyActor* FlagBaseBodyActor, FPlayerCharacter& InPlayerCharacter, const Vec3& ContactWorldPosition)
{
	auto SpawnerSubsystem = GetWorld()->GetSubsystem<USpawnerSubsystem>();
	auto SoundSubsystem = GetWorld()->GetSubsystem<USoundSubsystem>();

	if (UHelper::GetMyGameState(GetWorld())->HasRoundEnded())
	{
		return;
	}
	
	auto FlagBase = Cast<AFlagBase>(FlagBaseBodyActor);
	if (!FlagBase->IsFlagTaken() && FlagBase->BelongsToTeam != InPlayerCharacter.PlayerTeam)
	{
		InPlayerCharacter.CarryingFlagBodyIndex = FlagBase->TakeFlag();

		FVector EffectSpawnLocation = Units::FromJoltPos(ContactWorldPosition) + FVector(0, 10000.f, 0);
		FString StringToHash = FString::Printf(TEXT("TakeFlag%d%s"), InFrameId, *EffectSpawnLocation.ToString());
		const auto EffectHash = THashContainer::Generate(StringToHash);

		if (!UsedEffects.Contains(EffectHash))
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), GeneralData->TakeFlagVFX, EffectSpawnLocation)->SetTranslucentSortPriority(124);
			SoundSubsystem->PlaySound("StealFlag");
			
			AddUsedEffect(InFrameId, EffectHash);
		}
	}

	// Can't put opponents flag in, if your own flag is missing
	if (InPlayerCharacter.CarryingFlagBodyIndex > -1 && !FlagBase->IsFlagTaken())
	{
		auto Flag = SpawnerSubsystem->GetBodyActor<AFlag>(InPlayerCharacter.CarryingFlagBodyIndex);
		if (!Flag->IsPlacedInBase() && FlagBase->BelongsToTeam != Flag->BelongsToTeam)
		{
			Flag->PlaceInBase();
			InPlayerCharacter.CarryingFlagBodyIndex = -1;

			//TODO: These VFX could give player a FALSE positive with a rollback. We could play them from inside Flag body when the placement is CONFIRMED
			FVector EffectSpawnLocation = Units::FromJoltPos(ContactWorldPosition) + FVector(0, 10000.f, 0);
			FString StringToHash = FString::Printf(TEXT("PlaceFlag%d%s"), InFrameId, *EffectSpawnLocation.ToString());
			const auto EffectHash = THashContainer::Generate(StringToHash);
			FString VoiceLineName = SoundSubsystem->ChooseRandomVoiceLineDeterministic({ "V_SmokingCompetition", "V_Boring", "V_IsItWarm" });

			if (!UsedEffects.Contains(EffectHash))
			{
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), GeneralData->PlaceFlagVFX, EffectSpawnLocation)->SetTranslucentSortPriority(124);
				SoundSubsystem->PlaySound("DeliverFlag");
			
				SoundSubsystem->PlayVoiceLine(VoiceLineName, InPlayerCharacter.PlayerRenderer);
			
				AddUsedEffect(InFrameId, EffectHash);
			}
		}
	}
}

void AMyPlayerController::BoomerangHitPlayer(const int& InFrameId, AJoltBodyActor* BoomerangBodyActor, AJoltBodyActor* PlayerTakeDamageSensorBodyActor, const Vec3& ContactWorldPosition)
{
	auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();
	auto SpawnerSubsystem = GetWorld()->GetSubsystem<USpawnerSubsystem>();
	
	ABoomerangProjectile* BoomerangProjectile = Cast<ABoomerangProjectile>(BoomerangBodyActor);
	checkf(BoomerangProjectile, TEXT("Couldn't cast to boomerang"));

	if (GetPlayerTeam(BoomerangBodyActor->BelongsToPlayerId) != GetPlayerTeam(PlayerTakeDamageSensorBodyActor->BelongsToPlayerId))
	{
		//LOG_TEMPORARY(TEXT("[Frame %d] Player %d collide with boomerang"), InFrameId, PlayerCharacter.PlayerId);
		const Vec3 PlayerPos = JoltSubsystem->GetBodyInterface()->GetCenterOfMassPosition(PlayerTakeDamageSensorBodyActor->GetBodyID());
		const Vec3 BoomerangPos = JoltSubsystem->GetBodyInterface()->GetCenterOfMassPosition(BoomerangBodyActor->GetBodyID());
		PlayerExplodeDirectional(InFrameId, PlayerTakeDamageSensorBodyActor, PlayerTakeDamageSensorBodyActor->GetBodyID(), ContactWorldPosition, (PlayerPos - BoomerangPos).NormalizedOr(Vec3::sZero()), BoomerangBodyActor->BelongsToPlayerId);
	}
	else if (BoomerangBodyActor->BelongsToPlayerId == PlayerTakeDamageSensorBodyActor->BelongsToPlayerId)
	{
		SpawnerSubsystem->DespawnJoltBody(BoomerangBodyActor->GetBodyID());

		FString StringToHash = FString::Printf(TEXT("CatchBoomerang%d%d"), InFrameId, PlayerTakeDamageSensorBodyActor->BelongsToPlayerId);
		const auto EffectHash = THashContainer::Generate(StringToHash);

		if (!UsedEffects.Contains(EffectHash))
		{
			GetWorld()->GetSubsystem<USoundSubsystem>()->PlaySound("CatchBoomerang");

			AddUsedEffect(InFrameId, EffectHash);
		}
	}
}

void AMyPlayerController::CleanupActiveSwordBody(const int& InFrameId, FPlayerCharacter& InPlayerCharacter)
{
	if (!InPlayerCharacter.ActiveSwordBody.IsValid())
	{
		return;
	}
	
	auto SpawnerSubsystem = GetWorld()->GetSubsystem<USpawnerSubsystem>();
	if (!SpawnerSubsystem->DoesBodyExistInSimulation(InPlayerCharacter.ActiveSwordBody->GetBodyID()))
	{
		return;
	}
	
	FString StringToHash = FString::Printf(TEXT("SwordDisappear%d%d"), InFrameId, InPlayerCharacter.PlayerId);
	const auto EffectHash = THashContainer::Generate(StringToHash);

	if (!UsedEffects.Contains(EffectHash))
	{
		ASword* Sword = SpawnerSubsystem->GetBodyActor<ASword>(InPlayerCharacter.ActiveSwordBody->GetBodyID().GetIndex());
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), Sword->DisappearVFX, GetWorld()->GetSubsystem<UJoltSubsystem>()->GetBodyLocation(Sword->GetBodyID()));
			
		GetWorld()->GetSubsystem<USoundSubsystem>()->PlaySound("SwordDisappear");
			
		AddUsedEffect(InFrameId, EffectHash);
	}
	
	SpawnerSubsystem->DespawnJoltBody(InPlayerCharacter.ActiveSwordBody->GetBodyID());
}

void AMyPlayerController::ExplosiveBarrelHit(const int& InFrameId, const BodyID& InBarrelBodyID)
{
	auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();
	auto SpawnerSubsystem = GetWorld()->GetSubsystem<USpawnerSubsystem>();
	auto SoundSubsystem = GetWorld()->GetSubsystem<USoundSubsystem>();

	const Vec3 BarrelPos = JoltSubsystem->GetBodyInterface()->GetCenterOfMassPosition(InBarrelBodyID);
	FVector EffectSpawnLocation = Units::FromJoltPos(BarrelPos) + FVector(0, 10000.f, 0);
	FString StringToHash = FString::Printf(TEXT("BarrelExplode%d%s"), InFrameId, *EffectSpawnLocation.ToString());
	const auto EffectHash = THashContainer::Generate(StringToHash);

	if (!UsedEffects.Contains(EffectHash))
	{
		SpawnSpriteActor(GeneralData->BarrelExplodeVFX, EffectSpawnLocation);

		SoundSubsystem->PlaySound("BarrelExplode");
			
		AddUsedEffect(InFrameId, EffectHash);
	}
	
	SpawnerSubsystem->DespawnJoltBody(InBarrelBodyID);

	SphereShape Sphere(100.f);
	Sphere.SetEmbedded();
	RayObjectLayerFilter ObjectLayerFilter({EObjectLayer::PlayerTakeDamageSensor, EObjectLayer::Moving});
	auto CollectedHits = JoltSubsystem->CollideShape(Sphere, BarrelPos, &ObjectLayerFilter);

#ifdef DEBUG_RENDER_ALL_BODIES
	DrawDebugSphere(GetWorld(), Units::FromJoltPos(BarrelPos), Units::FromJoltUnits(Sphere.GetRadius()), 10.f, FColor::Red, false, 1.f, SDPG_World, 30.f);
#endif
	for (const CollideShapeResult& mHit : CollectedHits.mHits)
	{
		auto BodyActor = SpawnerSubsystem->GetBodyActor(mHit.mBodyID2.GetIndex());
		if (BodyActor->bIsPlayer)
		{
			//LOG_TEMPORARY(TEXT("[Frame %d] Player %d caught in barrel explosion %d"), InFrameId, BodyActor->BelongsToPlayerId, InBarrelBodyID.GetIndex());
			const Vec3 PlayerPos = JoltSubsystem->GetBodyInterface()->GetCenterOfMassPosition(mHit.mBodyID2);
			Vec3 ExplosionDirection = (PlayerPos - BarrelPos).NormalizedOr(Vec3::sZero());
			PlayerExplodeDirectional(InFrameId, BodyActor, BodyID(BodyActor->BelongsToPlayerId), PlayerPos, ExplosionDirection);
		}
		else if (BodyActor->BodyName == EBodyName::ExplosiveBarrel)
		{
			Cast<AExplosiveBarrel>(BodyActor)->Explode(InFrameId);
		}
	}
}

void AMyPlayerController::MineExpire(const BodyID& MineBodyID)
{
	auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();
	auto SpawnerSubsystem = GetWorld()->GetSubsystem<USpawnerSubsystem>();
	const int SteppingFrameId = GetWorld()->GetSubsystem<UGameLoopSubsystem>()->GetSteppingFrameId();
	
	const int MineBodyIndex = MineBodyID.GetIndex();
	FVector EffectSpawnLocation = JoltSubsystem->GetCenterOfMassPosition(MineBodyID);
	EffectSpawnLocation.Y = 10000.f;
	
	FString StringToHash = FString::Printf(TEXT("MineExpire%d%d"), SteppingFrameId, MineBodyIndex);
	const auto EffectHash = THashContainer::Generate(StringToHash);

	if (!UsedEffects.Contains(EffectHash) && SpawnerSubsystem->DoesBodyExistInSimulation(MineBodyID))
	{
		auto MineBodyActor = SpawnerSubsystem->GetBodyActor(MineBodyID);
		TObjectPtr<UNiagaraSystem> FireballImpactVFX = GeneralData->FireballImpactProceduralVFX;
		if (MineBodyActor->BelongsToPlayerId >= 0 && GetPlayerCharacter(MineBodyActor->BelongsToPlayerId).PlayerTeam == EPlayerTeam::Blue)
		{
			FireballImpactVFX = GeneralData->FireballBlueImpactProceduralVFX;
		}
		
		auto NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), FireballImpactVFX, EffectSpawnLocation);
		const Vec3 RandomDirection = RandomUnitVector(UKismetMathLibrary::RandomFloat());
		NiagaraComponent->SetVariableVec3(TEXT("Direction"), Units::FromJoltCoord(RandomDirection));

		GetWorld()->GetSubsystem<USoundSubsystem>()->PlaySound("FireballImpact");
			
		AddUsedEffect(SteppingFrameId, EffectHash);
	}
}

void AMyPlayerController::StickyBombExplode(const BodyID& InBombBodyID, float InExplosionRadius)
{
	auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();
	auto SpawnerSubsystem = GetWorld()->GetSubsystem<USpawnerSubsystem>();
	const int SteppingFrameId = GetWorld()->GetSubsystem<UGameLoopSubsystem>()->GetSteppingFrameId();

	const Vec3 BombPos = JoltSubsystem->GetBodyInterface()->GetCenterOfMassPosition(InBombBodyID);
	const int BombOwnerPlayerId = SpawnerSubsystem->GetBodyActor(InBombBodyID)->BelongsToPlayerId;

	SphereShape Sphere(InExplosionRadius);
	Sphere.SetEmbedded();
	RayObjectLayerFilter ObjectLayerFilter({EObjectLayer::PlayerTakeDamageSensor});
	auto CollectedHits = JoltSubsystem->CollideShape(Sphere, BombPos, &ObjectLayerFilter);

	for (const CollideShapeResult& Hit : CollectedHits.mHits)
	{
		if (!SpawnerSubsystem->DoesBodyExistInSimulation(Hit.mBodyID2))
		{
			continue;
		}

		auto HitBodyActor = SpawnerSubsystem->GetBodyActor(Hit.mBodyID2.GetIndex());
		if (!HitBodyActor->bIsPlayer)
		{
			continue;
		}

		const int HitPlayerId = HitBodyActor->BelongsToPlayerId;
		if (HitPlayerId == BombOwnerPlayerId)
		{
			continue;
		}

		FPlayerCharacter& HitPlayer = GetPlayerCharacter(HitPlayerId);
		if (HitPlayer.bDead)
		{
			continue;
		}

		const Vec3 PlayerPos = JoltSubsystem->GetBodyInterface()->GetCenterOfMassPosition(Hit.mBodyID2);
		const Vec3 BlastDir = (PlayerPos - BombPos).NormalizedOr(Vec3::sAxisX());
		PlayerExplodeDirectional(SteppingFrameId, HitBodyActor, Hit.mBodyID2, BombPos, BlastDir);
	}

	if (AStickyBomb* Bomb = Cast<AStickyBomb>(SpawnerSubsystem->GetBodyActor(InBombBodyID)))
	{
#if WITH_EDITOR
		if (Bomb->bDrawDebug)
		{
			DrawDebugSphere(GetWorld(), Units::FromJoltPos(BombPos), Units::FromJoltUnits(InExplosionRadius), 16, FColor::Red, false, 2.f, SDPG_World, 30.f);
		}
#endif

		if (Bomb->ExplodeVFX)
		{
			FString StringToHash = FString::Printf(TEXT("StickyBombExplode%d%d"), SteppingFrameId, InBombBodyID.GetIndex());
			const auto EffectHash = THashContainer::Generate(StringToHash);
			if (!UsedEffects.Contains(EffectHash))
			{
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), Bomb->ExplodeVFX, Units::FromJoltPos(BombPos));
				GetWorld()->GetSubsystem<USoundSubsystem>()->PlaySound("StickyBombExplode");
				AddUsedEffect(SteppingFrameId, EffectHash);
			}
		}
	}
}

FVector2D AMyPlayerController::GetMouseDelta()
{
	/*constexpr float MaxAccumulatedMagnitude = 5.f;
	if (AccumulatedMouseDelta.SquaredLength() > FMath::Square(MaxAccumulatedMagnitude))
	{
		AccumulatedMouseDelta = AccumulatedMouseDelta.GetSafeNormal() * MaxAccumulatedMagnitude;
	}*/
	const FVector2D MouseDelta = FVector2D(AccumulatedMouseDelta.X, AccumulatedMouseDelta.Y);
	AccumulatedMouseDelta = FVector2D::ZeroVector;

	return MouseDelta;
}

void AMyPlayerController::InitializeJoltBodiesAndConstraints()
{
	auto SpawnerSubsystem = GetWorld()->GetSubsystem<USpawnerSubsystem>();
	for (auto JoltBodyActor : GetJoltBodyActorsSortedBySpawnOrder())
	{
		// We might pick up actors that were already spawned by our code earlier. Like player spawning.
		// We shouldn't touch them
		if (JoltBodyActor->IsInitialized())
		{
			continue;
		}
		
		check(JoltBodyActor);
		SpawnerSubsystem->AddJoltBody(JoltBodyActor);
		CurrentStageJoltBodyActors.Add(JoltBodyActor);
		
		if (JoltBodyActor->bMirror)
		{
			AJoltBodyActor* MirroredBodyActor = SpawnMirroredActor<AJoltBodyActor>(JoltBodyActor);
			if (AFlagBase* MirroredFlagBase = Cast<AFlagBase>(MirroredBodyActor))
			{
				MirroredFlagBase->UpdateTeam(FTeamScores::GetOppositeTeam(MirroredFlagBase->BelongsToTeam));
			}

			check(MirroredBodyActor);
			SpawnerSubsystem->AddJoltBody(MirroredBodyActor);
			CurrentStageJoltBodyActors.Add(MirroredBodyActor);
		}

		if (APortal* Portal = Cast<APortal>(JoltBodyActor))
		{
			const FVector ReturnPortalLocation = Portal->GetActorLocation() + Portal->TeleportDestination;
			APortal* ReturnPortal = GetWorld()->SpawnActor<APortal>(Portal->GetClass(), FTransform(ReturnPortalLocation));
			ReturnPortal->TeleportDestination = -Portal->TeleportDestination;
			Portal->DestinationPortal = ReturnPortal;
			ReturnPortal->DestinationPortal = Portal;
			SpawnerSubsystem->AddJoltBody(ReturnPortal);
			CurrentStageJoltBodyActors.Add(ReturnPortal);
		}
	}
	
	for (auto JoltContainerActor : GetJoltContainerActorsSortedBySpawnOrder())
	{
		TArray<UJoltBodyComponent*> BodyComponentsSortedBySpawnOrder = JoltContainerActor->GetBodyComponentsSortedBySpawnOrder();
		if (JoltContainerActor->bCompoundShape)
		{
			auto RootBodyComponent = BodyComponentsSortedBySpawnOrder[0];

			if (JoltContainerActor->bDestructible)
			{
				auto SpawnedBodyActor = Cast<AJoltBodyActor>(GetWorld()->SpawnActor(ADestructibleJoltBodyActor::StaticClass(), &JoltContainerActor->GetActorTransform()));
				check(SpawnedBodyActor);

				SpawnedBodyActor->BodyName = EBodyName::Destructible;
				
				auto JoltDestructibleComponent = Cast<UJoltDestructibleCompoundComponent>(SpawnedBodyActor->AddComponentByClass(UJoltDestructibleCompoundComponent::StaticClass(), false, FTransform::Identity, false));
				check(JoltDestructibleComponent);

				JoltDestructibleComponent->CopyRootBodyComponentProperties(RootBodyComponent);

				for (auto BodyComponent : BodyComponentsSortedBySpawnOrder)
				{
					JoltDestructibleComponent->AddJoltBodyComponent(BodyComponent);
				}

				/*TArray<USceneComponent*> ChildrenComponents;
				RootBodyComponent->GetChildrenComponents(false, ChildrenComponents);
				for (USceneComponent* Child : ChildrenComponents)
				{
					Child->AttachToComponent(JoltDestructibleComponent, FAttachmentTransformRules::KeepWorldTransform);
					MoveComponentHierarchyToAnotherActor(Child, SpawnedBodyActor);
				}*/

				// This list is only used for constraints after. Since we only have one compound body here, we only need that body in that list.
				BodyComponentsSortedBySpawnOrder.Empty();
				BodyComponentsSortedBySpawnOrder.Add(JoltDestructibleComponent);

				SpawnerSubsystem->AddJoltBody(SpawnedBodyActor);
				CurrentStageJoltBodyActors.Add(SpawnedBodyActor);
			}
			else
			{
				auto SpawnedBodyActor = Cast<AJoltBodyActor>(GetWorld()->SpawnActor(RootBodyComponent->GetJoltBodyClass(), &JoltContainerActor->GetActorTransform()));
				check(SpawnedBodyActor);

				SpawnedBodyActor->BodyName = RootBodyComponent->BodyName;
				
				auto JoltCompoundComponent = Cast<UJoltStaticCompoundComponent>(SpawnedBodyActor->AddComponentByClass(UJoltStaticCompoundComponent::StaticClass(), false, FTransform::Identity, false));
				check(JoltCompoundComponent);

				JoltCompoundComponent->CopyRootBodyComponentProperties(RootBodyComponent);

				for (auto BodyComponent : BodyComponentsSortedBySpawnOrder)
				{
					JoltCompoundComponent->AddJoltBodyComponent(BodyComponent);
				}

				TArray<USceneComponent*> ChildrenComponents;
				RootBodyComponent->GetChildrenComponents(false, ChildrenComponents);
				for (USceneComponent* Child : ChildrenComponents)
				{
					Child->AttachToComponent(JoltCompoundComponent, FAttachmentTransformRules::KeepWorldTransform);
					MoveComponentHierarchyToAnotherActor(Child, SpawnedBodyActor);
				}

				// This list is only used for constraints after. Since we only have one compound body here, we only need that body in that list.
				BodyComponentsSortedBySpawnOrder.Empty();
				BodyComponentsSortedBySpawnOrder.Add(JoltCompoundComponent);
				
				SpawnerSubsystem->AddJoltBody(SpawnedBodyActor);
				CurrentStageJoltBodyActors.Add(SpawnedBodyActor);
			}
		}
		else
		{
			for (auto BodyComponent : BodyComponentsSortedBySpawnOrder)
			{
				auto SpawnedBodyActor = Cast<AJoltBodyActor>(GetWorld()->SpawnActor(BodyComponent->GetJoltBodyClass(), &JoltContainerActor->GetActorTransform()));
				check(SpawnedBodyActor);
			
				SpawnedBodyActor->BodyName = BodyComponent->BodyName;

				BodyComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
			
				MoveComponentHierarchyToAnotherActor(BodyComponent, SpawnedBodyActor);
				SpawnedBodyActor->SetRootComponent(BodyComponent);
				//BodyComponent->AttachToComponent(SpawnedBodyActor->GetDefaultAttachComponent(), FAttachmentTransformRules::KeepWorldTransform);
				
				SpawnerSubsystem->AddJoltBody(SpawnedBodyActor);
				CurrentStageJoltBodyActors.Add(SpawnedBodyActor);
			}
		}

		/** Initially constraints were handled in a separate method, but there was a bit of a problem.
		 * We need body IDs to be initialized first, because constraints need to know the body ID.
		 * BUT constraints also needed the body components to still be in their original JoltContainer and not moved to newly spawned JoltBody actors.
		 * So the workaround was to move constraint init here and store pointers to all the needed components before moving them and pass them here in BodyComponents TArray.
		 */
		for (auto ConstraintComponent : JoltContainerActor->GetConstraintComponentsSortedBySpawnOrder())
		{
			auto SpawnedConstraintActor = Cast<AJoltConstraintActor>(GetWorld()->SpawnActor(AJoltConstraintActor::StaticClass(), &JoltContainerActor->GetActorTransform()));
			check(SpawnedConstraintActor);
			ConstraintComponent->PreInitialize(BodyComponentsSortedBySpawnOrder);

			ConstraintComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);

			MoveComponentHierarchyToAnotherActor(ConstraintComponent, SpawnedConstraintActor);
			ConstraintComponent->AttachToComponent(SpawnedConstraintActor->GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);

			SpawnerSubsystem->AddJoltConstraint(SpawnedConstraintActor);
		}

		JoltContainerActor->Destroy();
	}
}

TArray<AJoltBodyActor*> AMyPlayerController::GetJoltBodyActorsSortedBySpawnOrder()
{
	TArray<AActor*> JoltBodyActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AJoltBodyActor::StaticClass(), JoltBodyActors);
	
	TMap<FString, AJoltBodyActor*> JoltBodyActorsBySpawnOrder;
	for (const auto Actor : JoltBodyActors)
	{
		auto JoltBodyActor = Cast<AJoltBodyActor>(Actor);
		check(JoltBodyActor);

		FString BodyNameHash = JoltBodyActor->GetPathName();
		
		check(!JoltBodyActorsBySpawnOrder.Contains(BodyNameHash));

		JoltBodyActorsBySpawnOrder.Add(BodyNameHash, JoltBodyActor);
	}

	TArray<FString> SpawnOrder;
	JoltBodyActorsBySpawnOrder.GetKeys(SpawnOrder);
	SpawnOrder.Sort();

	TArray<AJoltBodyActor*> JoltBodyActorsSorted;
	for (const FString& CurrentOrderId : SpawnOrder)
	{
		JoltBodyActorsSorted.Add(JoltBodyActorsBySpawnOrder[CurrentOrderId]);
	}

	return JoltBodyActorsSorted;
}

TArray<AJoltContainerActor*> AMyPlayerController::GetJoltContainerActorsSortedBySpawnOrder()
{
	TArray<AActor*> JoltContainerActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AJoltContainerActor::StaticClass(), JoltContainerActors);
	
	TMap<FString, AJoltContainerActor*> JoltContainerActorsBySpawnOrder;
	for (const auto Actor : JoltContainerActors)
	{
		auto JoltContainerActor = Cast<AJoltContainerActor>(Actor);
		check(JoltContainerActor);

		FString ContainerHash = JoltContainerActor->GetPathName();
		
		//check(JoltContainerActor->SpawnOrder >= 0);
		//check(!JoltContainerActorsBySpawnOrder.Contains(JoltContainerActor->SpawnOrder));
		check(!JoltContainerActorsBySpawnOrder.Contains(ContainerHash));

		JoltContainerActorsBySpawnOrder.Add(ContainerHash, JoltContainerActor);
	}

	TArray<FString> SpawnOrder;
	JoltContainerActorsBySpawnOrder.GetKeys(SpawnOrder);
	SpawnOrder.Sort();

	TArray<AJoltContainerActor*> JoltContainerActorsSorted;
	for (const FString& CurrentOrderId : SpawnOrder)
	{
		AJoltContainerActor* JoltContainerActor = JoltContainerActorsBySpawnOrder[CurrentOrderId];
		JoltContainerActorsSorted.Add(JoltContainerActor);

		if (JoltContainerActor->bMirror)
		{
			JoltContainerActorsSorted.Add(SpawnMirroredActor<AJoltContainerActor>(JoltContainerActor));
		}
	}

	return JoltContainerActorsSorted;
}

void AMyPlayerController::MoveComponentHierarchyToAnotherActor(USceneComponent* Component, AActor* NewOwner)
{
	// Change ownership
	Component->Rename(nullptr, NewOwner);

	TArray<USceneComponent*> ChildrenComponents;
	Component->GetChildrenComponents(false, ChildrenComponents);
	for (USceneComponent* Child : ChildrenComponents)
	{
		MoveComponentHierarchyToAnotherActor(Child, NewOwner);
	}
}

void AMyPlayerController::AddToCurrentStageJoltBodyActors(AJoltBodyActor* BodyActorToAdd)
{
	CurrentStageJoltBodyActors.Add(BodyActorToAdd);
}

template <typename T>
T* AMyPlayerController::SpawnMirroredActor(AActor* InActor)
{
	FObjectDuplicationParameters DuplicationParameters(InActor, GetWorld());
	AActor* Template = Cast<AActor>(
		StaticDuplicateObjectEx(DuplicationParameters)
	);

	FActorSpawnParameters Params;
	Params.Template = Template;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	constexpr float XAxisMiddle = 100000.f;
	FTransform MirrorTransform = Template->GetActorTransform();
	const FVector OriginalTransformLocation = MirrorTransform.GetLocation();
	const FVector OriginalTransformScale = MirrorTransform.GetScale3D();
			
	MirrorTransform.SetLocation(FVector(
		XAxisMiddle + (XAxisMiddle - OriginalTransformLocation.X),
		OriginalTransformLocation.Y,
		OriginalTransformLocation.Z
	));
			
	MirrorTransform.SetScale3D(FVector(
		OriginalTransformScale.X * -1.f,
		OriginalTransformScale.Y,
		OriginalTransformScale.Z
	));

	auto SpawnedMirrorActor = GetWorld()->SpawnActor<AActor>(
		Template->GetClass(),
		MirrorTransform,
		Params
	);

	/**
	 * For mirrored actors we need to find any child that might be using UMultipleMaterialStaticMeshComponent
	 * And switch to 2nd material. That's how we make mirrored sides look different, without having to place everything manually.
	*/
	TArray<USceneComponent*> ChildComponents;
	SpawnedMirrorActor->GetComponents(ChildComponents, true);
	for (USceneComponent* ChildComponent : ChildComponents)
	{
		if (UMultipleMaterialStaticMeshComponent* MultiMaterialMeshComp = Cast<UMultipleMaterialStaticMeshComponent>(ChildComponent))
		{	
			MultiMaterialMeshComp->SwitchToSecondMaterial();
		}
	}
			
	return Cast<T>(SpawnedMirrorActor);
}

AActor* AMyPlayerController::SpawnSpriteActor(TSubclassOf<AActor> InActorClass, const FVector& InSpawnLocation, const FRotator& InSpawnRotation)
{
	if (InSpawnRotation.Pitch >= -90.f && InSpawnRotation.Pitch <= 90.f)
	{
		return GetWorld()->SpawnActor(InActorClass, &InSpawnLocation, &InSpawnRotation);
	}

	// Handles flipping the actor on X axis to avoid flipping sprite upside down when rotating 180 degrees
	FTransform EffectSpawnTransform = FTransform(
		InSpawnRotation + FRotator(180.f, 0, 0),
		InSpawnLocation,
		FVector(-1.f, 1.f, 1.f)
	);

	return GetWorld()->SpawnActor(InActorClass, &EffectSpawnTransform);
}

AActor* AMyPlayerController::SpawnSpriteActor(TSubclassOf<AActor> InActorClass, const FVector& InSpawnLocation)
{
	return SpawnSpriteActor(InActorClass, InSpawnLocation, FRotator::ZeroRotator);
}

FTransform AMyPlayerController::GetTransformWithMirroredAxis(const FVector& InSpawnLocation,
                                                             const FRotator& InSpawnRotation) const
{
	if (InSpawnRotation.Pitch >= -90.f && InSpawnRotation.Pitch <= 90.f)
	{
		return FTransform(
			InSpawnRotation + FRotator(180.f, 0, 0),
			InSpawnLocation,
			FVector(1.f, 1.f, 1.f)
		);
	}

	// Handles flipping the actor on X axis to avoid flipping sprite upside down when rotating 180 degrees
	return FTransform(
			InSpawnRotation + FRotator(180.f, 0, 0),
			InSpawnLocation,
			FVector(-1.f, 1.f, 1.f)
		);
}

void AMyPlayerController::DesyncDetected(const int& InFrameId)
{
	DesyncDetectedOnFrameId = InFrameId;

#if !UE_BUILD_SHIPPING
	OnDesyncDetected.Broadcast(InFrameId);
#endif
}

void AMyPlayerController::DesyncFixed(const int& InFrameId)
{
	DesyncDetectedOnFrameId = -1;

#if !UE_BUILD_SHIPPING
	OnDesyncFixed.Broadcast(InFrameId);
#endif
}

int AMyPlayerController::GetDesyncDetectedFrameId() const
{
	return DesyncDetectedOnFrameId;
}

int AMyPlayerController::CreateCharacter(APlayerSpawn* InPlayerSpawn)
{
	auto& PlayerCharacter = PlayerCharacters.AddDefaulted_GetRef();
	PlayerCharacter.PlayerIndex = PlayerCharacters.Num() - 1;

	checkf(GeneralData->PossiblePlayerColorSchemes.IsValidIndex(PlayerCharacters.Num() - 1), TEXT("Invalid player index for color scheme"));
	const auto ColorScheme = GeneralData->PossiblePlayerColorSchemes[PlayerCharacters.Num() - 1];

	PlayerCharacter.RenderData.ColorScheme = ColorScheme;
	PlayerCharacter.PlayerTeam = ColorScheme.Team;
	
	const FRotator ZeroRotator = FRotator::ZeroRotator;
	const FVector CharacterSpawnLocation = InPlayerSpawn->GetActorLocation();
	AJoltContainerActor* JoltContainerActor = Cast<AJoltContainerActor>(GetWorld()->SpawnActor(PlayerBodyClass, &CharacterSpawnLocation, &ZeroRotator));
	check(JoltContainerActor);

	auto SpawnerSubsystem = GetWorld()->GetSubsystem<USpawnerSubsystem>();
	auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();

	const FPlayerMovementConfig& MovementConfig = PlayerMovementConfigData->Get();
	
	TArray<UJoltBodyComponent*> BodyComponentsSortedBySpawnOrder = JoltContainerActor->GetBodyComponentsSortedBySpawnOrder();
	for (auto BodyComponent : BodyComponentsSortedBySpawnOrder)
	{
		auto SpawnedBodyActor = Cast<AJoltBodyActor>(GetWorld()->SpawnActor(AJoltBodyActor::StaticClass(), &JoltContainerActor->GetActorTransform()));
		check(SpawnedBodyActor);

		BodyComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
			
		MoveComponentHierarchyToAnotherActor(BodyComponent, SpawnedBodyActor);
		BodyComponent->AttachToComponent(SpawnedBodyActor->GetDefaultAttachComponent(), FAttachmentTransformRules::KeepWorldTransform);
		
		//***** UPDATE PLAYER MOVEMENT PHYSICS VALUES BEFORE INITIALIZING ANY JOLT BODIES *******//
		if (BodyComponent->GetSpawnOrder() == 0)
		{
			UJoltBoxComponent* PlayerBody = Cast<UJoltBoxComponent>(BodyComponent);

			PlayerBody->PhysicsProperties.Mass = MovementConfig.BodyConfig.BodyMass;
			PlayerBody->PhysicsProperties.Friction = MovementConfig.BodyConfig.BodyFriction;
			PlayerBody->PhysicsProperties.LinearDamping = MovementConfig.BodyConfig.BodyLinearDamping;
			PlayerBody->PhysicsProperties.MaxLinearVelocity = MovementConfig.MaxLinearVelocity;
			PlayerBody->PhysicsProperties.GravityFactor = MovementConfig.Gravity;
		}

		if (BodyComponent->GetSpawnOrder() == 1)
		{
			UJoltBoxComponent* PlayerShoulder = Cast<UJoltBoxComponent>(BodyComponent);

			PlayerShoulder->PhysicsProperties.Mass = MovementConfig.ShoulderConfig.ShoulderMass;
			PlayerShoulder->PhysicsProperties.Friction = MovementConfig.ShoulderConfig.ShoulderFriction;
			PlayerShoulder->PhysicsProperties.LinearDamping = MovementConfig.ShoulderConfig.ShoulderLinearDamping;
			PlayerShoulder->PhysicsProperties.MaxLinearVelocity = MovementConfig.MaxLinearVelocity;
			PlayerShoulder->PhysicsProperties.GravityFactor = MovementConfig.Gravity;
		}

		if (BodyComponent->GetSpawnOrder() == 2)
		{
			UJoltBoxComponent* PlayerHand = Cast<UJoltBoxComponent>(BodyComponent);

			PlayerHand->PhysicsProperties.Mass = MovementConfig.HandConfig.HandMass;
			PlayerHand->PhysicsProperties.Friction = MovementConfig.HandConfig.HandFriction;
			PlayerHand->PhysicsProperties.LinearDamping = MovementConfig.HandConfig.HandLinearDamping;
			PlayerHand->PhysicsProperties.MaxLinearVelocity = MovementConfig.MaxLinearVelocity;
			PlayerHand->PhysicsProperties.GravityFactor = MovementConfig.Gravity;
		}
		
		//***** INITIALIZE JOLT BODY ******//
		const int AssignedBodyIndex = SpawnerSubsystem->AddJoltBody(SpawnedBodyActor);
		if (BodyComponent->GetSpawnOrder() == 0)
		{
			PlayerCharacter.PlayerId = AssignedBodyIndex;
			SpawnedBodyActor->BelongsToPlayerId = PlayerCharacter.PlayerId;
			SpawnedBodyActor->BodyName = EBodyName::PlayerBody;
			SpawnedBodyActor->bIsPlayer = true;
		}
		else if (BodyComponent->GetSpawnOrder() == 1)
		{
			PlayerCharacter.ShoulderBodyIndex = AssignedBodyIndex;
			SpawnedBodyActor->BelongsToPlayerId = PlayerCharacter.PlayerId;
		}
		else if (BodyComponent->GetSpawnOrder() == 2)
		{
			PlayerCharacter.HandBodyIndex = AssignedBodyIndex;
			SpawnedBodyActor->bIsPlayer = true;
			SpawnedBodyActor->BelongsToPlayerId = PlayerCharacter.PlayerId;
			SpawnedBodyActor->BodyName = EBodyName::PlayerHammer;
			PlayerCharacter.RenderData.HandStaticMeshComponent = Cast<UStaticMeshComponent>(BodyComponent->GetChildComponent(1));
			check(PlayerCharacter.RenderData.HandStaticMeshComponent);
			
			PlayerCharacter.RenderData.HammerDynamicMaterial = PlayerCharacter.RenderData.HandStaticMeshComponent->CreateDynamicMaterialInstance(0, ColorScheme.HammerMaterial);
		}
		else if (BodyComponent->GetSpawnOrder() == 110)
		{
			PlayerCharacter.HiddenHandBodyIndex = AssignedBodyIndex;
		}
		else if (BodyComponent->GetSpawnOrder() == 111)
		{
			PlayerCharacter.HiddenVelocityBodyIndex = AssignedBodyIndex;
		}
		else if (BodyComponent->GetSpawnOrder() == 100)
		{
			PlayerCharacter.PlayerVisualRootBodyIndex = AssignedBodyIndex;

			TArray<USceneComponent*> ChildrenComps;
			// Index 0 is the auto created mesh comp by Jolt component. If we ever get rid of that, we will need to adjsut all these places
			BodyComponent->GetChildrenComponents(true, ChildrenComps);
			
			/*auto PlayerBodyStaticMesh = Cast<UStaticMeshComponent>(ChildrenComps[1]);
			check(PlayerBodyStaticMesh);
			
			PlayerCharacter.RenderData.BodyDynamicMaterial = PlayerBodyStaticMesh->CreateDynamicMaterialInstance(0, ColorScheme.BodyMaterial);*/

			
			auto OutlineMeshComp = Cast<UStaticMeshComponent>(ChildrenComps[2]);
			auto BodyMeshComp = Cast<UStaticMeshComponent>(ChildrenComps[3]);

			OutlineMeshComp->SetMaterial(0, ColorScheme.OutlineMaterial);
			PlayerCharacter.RenderData.BodyDynamicMaterial = BodyMeshComp->CreateDynamicMaterialInstance(0, ColorScheme.UpperArmMaterial);
		}
		else if (BodyComponent->GetSpawnOrder() >= 101 && BodyComponent->GetSpawnOrder() < 110)
		{
			PlayerCharacter.PlayerVisualBodyIndexes.Add(AssignedBodyIndex);
			if (BodyComponent->GetSpawnOrder() == 101)
			{
				PlayerCharacter.PlayerVisualHeadBodyIndex = AssignedBodyIndex;
				
				auto HeadAnimationComponent = Cast<UAnimationComponent>(BodyComponent->GetChildComponent(1));
				check(HeadAnimationComponent);
				PlayerCharacter.RenderData.HeadAnimationComp = HeadAnimationComponent;

				auto EyesAnimationComponent = Cast<UEyesAnimationComponent>(BodyComponent->GetChildComponent(2));
				check(EyesAnimationComponent);
				PlayerCharacter.RenderData.EyesAnimationComp = EyesAnimationComponent;

				PlayerCharacter.RenderData.EyesDynamicMaterial = EyesAnimationComponent->CreateDynamicMaterialInstance(0, ColorScheme.EyesMaterial);
				PlayerCharacter.RenderData.HeadDynamicMaterial = HeadAnimationComponent->CreateDynamicMaterialInstance(0, ColorScheme.HeadMaterial);
			}
		}
		else if (BodyComponent->GetSpawnOrder() == 1000)
		{
			PlayerCharacter.TakeDamageSensorBodyIndex = AssignedBodyIndex;
			SpawnedBodyActor->bIsPlayer = true;
			SpawnedBodyActor->BelongsToPlayerId = PlayerCharacter.PlayerId;
		}
	}

	const auto& BodyInterface = JoltSubsystem->GetPhysicsSystem()->GetBodyLockInterfaceNoLock();
	Ref<GroupFilterTable> DisableCollisionFilter = new GroupFilterTable();
	BodyInterface.TryGetBody(BodyID(PlayerCharacter.PlayerId))->SetCollisionGroup(CollisionGroup(DisableCollisionFilter, PlayerCharacter.PlayerId, 0));
	BodyInterface.TryGetBody(BodyID(PlayerCharacter.HandBodyIndex))->SetCollisionGroup(CollisionGroup(DisableCollisionFilter, PlayerCharacter.PlayerId, 0));
	BodyInterface.TryGetBody(BodyID(PlayerCharacter.TakeDamageSensorBodyIndex))->SetCollisionGroup(CollisionGroup(DisableCollisionFilter, PlayerCharacter.PlayerId, 0));
	for (const int& VisualBodyIndex : PlayerCharacter.PlayerVisualBodyIndexes)
	{
		BodyInterface.TryGetBody(BodyID(VisualBodyIndex))->SetCollisionGroup(CollisionGroup(DisableCollisionFilter, PlayerCharacter.PlayerId, 0));
	}
	
	for (auto ConstraintComponent : JoltContainerActor->GetConstraintComponentsSortedBySpawnOrder())
	{
		auto SpawnedConstraintActor = Cast<AJoltConstraintActor>(GetWorld()->SpawnActor(AJoltConstraintActor::StaticClass(), &JoltContainerActor->GetActorTransform()));
		check(SpawnedConstraintActor);
		
		if (ConstraintComponent->SpawnOrder == 0)
		{
			UJoltHingeConstraint* PlayerHingeConstraint = Cast<UJoltHingeConstraint>(ConstraintComponent);
			check(PlayerHingeConstraint);

			PlayerHingeConstraint->ConstraintSettings.CustomMotorSettings.TorqueLimit = MovementConfig.HingeConfig.HingeMotorTorqueLimit;
			PlayerHingeConstraint->ConstraintSettings.CustomMotorSettings.MotorSpringSettings.Damping = MovementConfig.HingeConfig.HingeMotorDamping;
			PlayerHingeConstraint->ConstraintSettings.CustomMotorSettings.MotorSpringSettings.Frequency = MovementConfig.HingeConfig.HingeMotorFrequency;
		}

		if (ConstraintComponent->SpawnOrder == 1)
		{
			UJoltSliderConstraint* PlayerSliderConstraint = Cast<UJoltSliderConstraint>(ConstraintComponent);
			check(PlayerSliderConstraint);

			PlayerSliderConstraint->ConstraintSettings.LimitsMax = MovementConfig.SliderConfig.SliderDistanceMax;
			PlayerSliderConstraint->ConstraintSettings.CustomMotorSettings.ForceLimit = MovementConfig.SliderConfig.SliderMotorForceLimit;
			PlayerSliderConstraint->ConstraintSettings.CustomMotorSettings.MotorSpringSettings.Damping = MovementConfig.SliderConfig.SliderMotorDamping;
			PlayerSliderConstraint->ConstraintSettings.CustomMotorSettings.MotorSpringSettings.Frequency = MovementConfig.SliderConfig.SliderMotorFrequency;
		}
		
		ConstraintComponent->PreInitialize(BodyComponentsSortedBySpawnOrder);

		ConstraintComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);

		MoveComponentHierarchyToAnotherActor(ConstraintComponent, SpawnedConstraintActor);
		ConstraintComponent->AttachToComponent(SpawnedConstraintActor->GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);
			
		SpawnerSubsystem->AddJoltConstraint(SpawnedConstraintActor);

		if (ConstraintComponent->SpawnOrder == 0 && ConstraintComponent->IsConstraintInitialized())
		{
			PlayerCharacter.HandHingeConstraint = SpawnedConstraintActor->GetConstraintInstance<HingeConstraint>();
		}
		else if (ConstraintComponent->SpawnOrder == 1 && ConstraintComponent->IsConstraintInitialized())
		{
			PlayerCharacter.HandSliderConstraint = SpawnedConstraintActor->GetConstraintInstance<SliderConstraint>();
		}
		else if (ConstraintComponent->SpawnOrder == 100)
		{
			PlayerCharacter.HeadHingeConstraint = SpawnedConstraintActor->GetConstraintInstance<HingeConstraint>();
		}
	}

	const FTransform RendererSpawnTransform(FRotator(0.f, 0.f, 0.f));
	PlayerCharacter.PlayerRenderer = Cast<APlayerRenderer>(GetWorld()->SpawnActor(PlayerRendererClass, &RendererSpawnTransform));
	PlayerCharacter.PlayerRenderer->LowerArmMeshL->SetMaterial(0, ColorScheme.LowerArmMaterial);
	PlayerCharacter.PlayerRenderer->LowerArmMeshR->SetMaterial(0, ColorScheme.LowerArmMaterial);
	PlayerCharacter.PlayerRenderer->UpperArmMeshL->SetMaterial(0, ColorScheme.UpperArmMaterial);
	PlayerCharacter.PlayerRenderer->UpperArmMeshR->SetMaterial(0, ColorScheme.UpperArmMaterial);

	Cast<UStaticMeshComponent>(PlayerCharacter.PlayerRenderer->LowerArmMeshL->GetChildComponent(0))->SetMaterial(0, ColorScheme.OutlineMaterial);
	Cast<UStaticMeshComponent>(PlayerCharacter.PlayerRenderer->LowerArmMeshR->GetChildComponent(0))->SetMaterial(0, ColorScheme.OutlineMaterial);
	Cast<UStaticMeshComponent>(PlayerCharacter.PlayerRenderer->UpperArmMeshL->GetChildComponent(0))->SetMaterial(0, ColorScheme.OutlineMaterial);
	Cast<UStaticMeshComponent>(PlayerCharacter.PlayerRenderer->UpperArmMeshR->GetChildComponent(0))->SetMaterial(0, ColorScheme.OutlineMaterial);

	PlayerCharacter.RenderData.StickDynamicMaterial = PlayerCharacter.PlayerRenderer->StickMesh->CreateDynamicMaterialInstance(0, ColorScheme.StickMaterial);

	//check(PlayerCharacter.HandHingeConstraint);
	//check(PlayerCharacter.HandSliderConstraint);

	JoltContainerActor->Destroy();

	InPlayerSpawn->Init(PlayerCharacter.PlayerId);

	return PlayerCharacter.PlayerId;
}

APlayerSpawn* AMyPlayerController::GetPlayerSpawn(const int& CharacterIndex, const TArray<AActor*>& InPossibleSpawns)
{
	for (const auto& PossibleSpawn : InPossibleSpawns)
	{
		auto PlayerSpawn = Cast<APlayerSpawn>(PossibleSpawn);
		check(PlayerSpawn);

		if (PlayerSpawn->SpawnIndex == CharacterIndex)
		{
			return PlayerSpawn;
		}
	}

	checkNoEntry();

	return nullptr;
}

void AMyPlayerController::StepCharacterSimulation(const int& InFrameId, FPlayerCharacter& PlayerCharacter, const float& FixedStepTime)
{
	//LOG_TEMPORARY(TEXT("[Frame %d] Current game mode %s"), InFrameId, *UEnum::GetValueAsString(GameModeSettings.Mode));
	if (GameModeSettings.Mode == EGameMode::Tutorial)
	{
		StepCharacterSimulationTutorial(InFrameId, PlayerCharacter);
	}

	if (GameModeSettings.Mode == EGameMode::TimeAttack)
	{
		StepCharacterSimulationTimeAttack(InFrameId, PlayerCharacter);
	}
	
	if (GameModeSettings.Mode == EGameMode::Lobby)
	{
		StepCharacterSimulationDeathmatch(InFrameId, PlayerCharacter);
	}
	
	if (GameModeSettings.Mode == EGameMode::Deathmatch)
	{
		StepCharacterSimulationDeathmatch(InFrameId, PlayerCharacter);
	}
	
	if (GameModeSettings.Mode == EGameMode::RopedTogether)
	{
		StepCharacterSimulationRopedTogether(InFrameId, PlayerCharacter);
	}

	if (GameModeSettings.Mode == EGameMode::PullingRope)
	{
		StepCharacterSimulationPullingRope(InFrameId, PlayerCharacter);
	}

	if (GameModeSettings.Mode == EGameMode::CaptureTheFlag)
	{
		StepCharacterSimulationCaptureTheFlag(InFrameId, PlayerCharacter);
	}

	if (GameModeSettings.Mode == EGameMode::KingOfTheCrown)
	{
		StepCharacterSimulationKingOfTheCrown(InFrameId, PlayerCharacter);
	}
}

void AMyPlayerController::AddUsedEffect(const int& InFrameId, const uint64& InEffectHash)
{
	if (bReplayMode)
	{
		return;
	}

	UsedEffects.Add(InFrameId, InEffectHash);
}

void AMyPlayerController::LoadAssetsForGame()
{
	UAssetManagerSubsystem::LoadLootItemsData(GetWorld());
}

#pragma region NetworkClockSync

float AMyPlayerController::GetServerWorldTimeDelta() const
{
	return ServerWorldTimeDelta;
}

float AMyPlayerController::GetServerWorldTime() const
{
	if (!GetWorld())
	{
		return 0.f;
	}
	
	return GetWorld()->GetTimeSeconds() + ServerWorldTimeDelta;
}

float AMyPlayerController::GetOneWayTripTime() const
{
	return OneWayTripTime;
}

float AMyPlayerController::GetRoundTripTime() const
{
	return RoundTripTime;
}

float AMyPlayerController::BP_GetServerWorldTimeDelta()
{
	return GetServerWorldTimeDelta();
}

float AMyPlayerController::BP_GetServerWorldTime()
{
	return GetServerWorldTime();
}

float AMyPlayerController::BP_GetOneWayLatency()
{
	return GetOneWayTripTime();
}

float AMyPlayerController::BP_GetRoundTripLatency()
{
	return GetRoundTripTime();
}

void AMyPlayerController::PostNetInit()
{
	Super::PostNetInit();
	if (GetLocalRole() != ROLE_Authority)
	{
		RequestWorldTime_Internal();
		if (NetworkClockUpdateFrequency > 0.f)
		{
			FTimerHandle TimerHandle;
			GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &ThisClass::RequestWorldTime_Internal, NetworkClockUpdateFrequency, true);
		}
	}
}

void AMyPlayerController::RequestWorldTime_Internal()
{
	ServerRequestWorldTime(GetWorld()->GetTimeSeconds(), RoundTripTime);
}

void AMyPlayerController::ClientUpdateWorldTime_Implementation(float ClientTimestamp, float ServerTimestamp)
{
	const float RequestRoundTripTime = GetWorld()->GetTimeSeconds() - ClientTimestamp;
	UE_LOG(LogTemp, Warning, TEXT("RTT %f"), RequestRoundTripTime);
	RTTCircularBuffer.Add(RequestRoundTripTime);
	
	float FinalRTT = 0;
	if (RTTCircularBuffer.Num() == 10)
	{
		TArray<float> CircularBufferCopy = RTTCircularBuffer;
		CircularBufferCopy.Sort();
		// We skip the first and last elements as outliers
		for (int i = 1; i < 9; ++i)
		{
			FinalRTT += CircularBufferCopy[i];
		}

		// Divide by 8 to get the average RTT, since we skipped 2 out of 10
		FinalRTT /= 8;
		RTTCircularBuffer.RemoveAt(0);
	}
	else
	{
		FinalRTT = RequestRoundTripTime;
	}

	RoundTripTime = FinalRTT;
	OneWayTripTime = FinalRTT / 2.f;
	ServerWorldTimeDelta = ServerTimestamp - (ClientTimestamp + OneWayTripTime);
}

void AMyPlayerController::ServerRequestWorldTime_Implementation(float ClientTimestamp, float RTT)
{
	const float Timestamp = GetWorld()->GetTimeSeconds();
	ClientUpdateWorldTime(ClientTimestamp, Timestamp);
	RoundTripTime = RTT;
	OneWayTripTime = RoundTripTime / 2.f;
}

#pragma endregion 
