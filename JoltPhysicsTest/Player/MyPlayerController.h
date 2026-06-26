 // Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "JoltPhysicsTest/Debug/CustomLogger.h"
#include "CoreMinimal.h"
#include "InputAction.h"
#include "MyPlayerState.h"
#include "NiagaraSystem.h"
#include "PlayerCharacterRenderDataStruct.h"
#include "PlayerRenderer.h"
#include "PlayerTeamsEnum.h"
#include "GameFramework/PlayerController.h"
#include "JoltPhysicsTest/Data/HashStruct.h"
#include "JoltPhysicsTest/Data/PlayerMovementConfigData.h"
#include "JoltPhysicsTest/Game/GameCamera.h"
#include "JoltPhysicsTest/Game/GameCameraLimits.h"
#include "JoltPhysicsTest/Game/GameModesEnum.h"
#include "JoltPhysicsTest/Game/GameModeSettings.h"
#include "JoltPhysicsTest/GameInputs/GameInputs.h"
#include "JoltPhysicsTest/Jolt/JoltConstraintActor.h"
#include "JoltPhysicsTest/Jolt/JoltBodyActor.h"
#include "JoltPhysicsTest/Jolt/JoltSubsystem.h"
#include "JoltPhysicsTest/JoltBodies/PlayerSpinner.h"

#include "MyPlayerController.generated.h"

class AJoltContainerActor;
class AMyGameState;
class ACheckpoint;

 USTRUCT(BlueprintType)
 struct FTeamScores
 {
	 GENERATED_BODY()

 	UPROPERTY(BlueprintReadOnly)
 	uint8 YellowTeamScore = 0;

 	UPROPERTY(BlueprintReadOnly)
 	uint8 BlueTeamScore = 0;
 	
 	void IncreaseScore(EPlayerTeam Team)
 	{
 		if (Team == EPlayerTeam::Blue)
 		{
 			BlueTeamScore++;
 		}

 		if (Team == EPlayerTeam::Yellow)
 		{
 			YellowTeamScore++;
 		}
 	}

 	uint8 GetTeamScore(EPlayerTeam Team)
 	{
 		if (Team == EPlayerTeam::Blue)
 		{
 			return BlueTeamScore;
 		}

 		if (Team == EPlayerTeam::Yellow)
 		{
 			return YellowTeamScore;
 		}

 		return 0;
 	}

 	uint8 GetOppositeTeamScore(EPlayerTeam PlayerTeam)
 	{
 		return GetTeamScore(FTeamScores::GetOppositeTeam(PlayerTeam));
 	}

 	static EPlayerTeam GetOppositeTeam(EPlayerTeam InPlayerTeam)
 	{
 		if (InPlayerTeam == EPlayerTeam::Blue)
 		{
 			return EPlayerTeam::Yellow;
 		}

 		if (InPlayerTeam == EPlayerTeam::Yellow)
 		{
 			return EPlayerTeam::Blue;
 		}

 		checkNoEntry();

 		return EPlayerTeam::None;
 	}
 };

USTRUCT()
struct FPlayerNetworkData
{
	GENERATED_BODY()

	UPROPERTY()
	uint8 PlayerStatesReplicated = 0;

	UPROPERTY()
	uint8 CharacterIdsReplicated = 0;

	UPROPERTY()
	FString GameSeed = "";

	UPROPERTY()
	uint8 PlayersSettingsReplicated = 0;

	UPROPERTY()
	bool bReadyToStartGameLoop = false;
};

USTRUCT()
struct FPlayerCharacter
{
	GENERATED_BODY()

	/** This is data that needs to be saved each frame and support rollback */
	int BulletReloadCooldownFramesLeft = 0;
	int ShootingCooldownFramesLeft = 0;
	int DashCooldownFramesLeft = 0;
	int BulletsLeft = 3;
	int ShootButtonHeldFramesNum = 0;
	int HandContactBodyIndex = -1;
	int GrabbedBodyIndex = -1;
	int GrappleRopeConstraintId = -1;
	float GrappleCurrentMaxDistance = -1.f;
	bool bDead = false;
	int DeadFramesNum = 0;
	int InvulnerabilityFramesNum = 0;
	int CrownHeldFramesNum = 0;
	int CarryingFlagBodyIndex = -1;
	ELootItem ActiveItem = ELootItem::None;
	int ItemActiveForFramesNum = 0;
	/** This is an invisible and "fake" cursor that is moved by the code based on the mouse deltas coming from input. But this cursor can have boundaries and speed limits */
	FVector2D CursorPosition = FVector2D::ZeroVector;
	TArray<int> CreatedBodyIndexes;
	TArray<int> CreatedConstraintsIds;
	TArray<ELootItem> PlayerItems;

	/** This data never changes from the start of the game to the end, so it also doesn't participate in rollback */
	bool bLocalPlayer = false;
	int PlayerId = -1;
	int PlayerIndex = -1;
	int ShoulderBodyIndex = -1;
	int HandBodyIndex = -1;
	int TakeDamageSensorBodyIndex = -1;
	EPlayerTeam PlayerTeam = EPlayerTeam::None;
	// Copies players actual hand position every single frame and is used to attach bodies to it that wouldn't affect players movement at all
	int HiddenHandBodyIndex = -1;
	// Used for checking the velocity of players hand, especially when throwing objects
	int HiddenVelocityBodyIndex = -1;
	int PlayerVisualRootBodyIndex = -1;
	TArray<int> PlayerVisualBodyIndexes;
	TArray<Vec3> PlayerVisualBodyRelativeLocation;
	int PlayerVisualHeadBodyIndex = -1;

	HingeConstraint* HandHingeConstraint = nullptr;
	SliderConstraint* HandSliderConstraint = nullptr;
	// Used for measuring crushing forces put on the player
	HingeConstraint* HeadHingeConstraint = nullptr;

	int DeadFramesCurrentStep = 0;
	FVector DeadLocation = FVector::ZeroVector;

	/** This data changes, but is only used for rendering and therefor doesn't participate in rollback */
	FPlayerCharacterRenderData RenderData;

	UPROPERTY()
	TObjectPtr<APlayerRenderer> PlayerRenderer = nullptr;

	AActor* PlayerHasAbilityAnimationActor = nullptr;
	
	TSoftObjectPtr<class ASword> ActiveSwordBody = nullptr;

	int DeadOnFrameId = -1;

	int GetItemAmount(ELootItem ItemToCheck) const
	{
		int Amount = 0;
		for (ELootItem Item : PlayerItems)
		{
			if (Item == ItemToCheck)
			{
				Amount++;
			}
		}

		return Amount;
	}

	bool HasItem(ELootItem ItemToCheck) const
	{
		return GetItemAmount(ItemToCheck) >= 1;
	}

	ELootItem GetFirstItem() const
	{
		if (PlayerItems.IsEmpty())
		{
			return ELootItem::None;
		}

		return PlayerItems[0];
	}

	void RemoveItem(ELootItem ItemToRemove)
	{
		check(PlayerItems.Contains(ItemToRemove));
		PlayerItems.Remove(ItemToRemove);

		if (PlayerHasAbilityAnimationActor && !PlayerHasAbilityAnimationActor->IsHidden())
		{
			PlayerHasAbilityAnimationActor->SetActorHiddenInGame(true);
		}
	}

	void AddItem(ELootItem LootItem)
	{
		// For now we will only allow 1 item at a time. If this ends up being the final design, we should refactor this TArray into a simple enum var
		PlayerItems.Empty();
		PlayerItems.Add(LootItem);

		if (PlayerHasAbilityAnimationActor && PlayerHasAbilityAnimationActor->IsHidden())
		{
			PlayerHasAbilityAnimationActor->SetActorHiddenInGame(false);
		}
	}

	void RemoveAllItems()
	{
		PlayerItems.Empty();
		
		if (PlayerHasAbilityAnimationActor && !PlayerHasAbilityAnimationActor->IsHidden())
		{
			PlayerHasAbilityAnimationActor->SetActorHiddenInGame(true);
		}
	}
};

USTRUCT(BlueprintType)
struct FTimeAttackLevelInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int32 LevelIndex = -1;

	UPROPERTY(BlueprintReadOnly)
	TSoftObjectPtr<UWorld> Level;

	UPROPERTY(BlueprintReadOnly)
	FString LevelName;

	UPROPERTY(BlueprintReadOnly)
	FString LeaderboardName;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UTexture2D> Thumbnail = nullptr;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDesyncDetected, int, FrameId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDesyncFixed, int, FrameId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTeamWinsWholeGame, EPlayerTeam, WinningTeam);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTeamScoresChanged, const FTeamScores&, TeamScores);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCurrentGameModeTeamScoreChanged, const FTeamScores&, TeamScores);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTeamWin, EPlayerTeam, WinningTeam);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTeamsTie);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLocalPlayerItemsChanged, const TArray<ELootItem>&, Items, const TArray<int>&, Amounts);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRoundCountdownUpdate, int, SecondsLeft);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRoundCountdownFinished);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStartLoading);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFinishLoading);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnNewLevelLoaded);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMouseSensitivityUpdated, int, NewMouseSensitivity, float, NewMouseSensitivityPercentage);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCheckpointCollected, int, CheckpointIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTimeAttackLevelIndexChanged, int32, NewIndex);

UCLASS()
class JOLTPHYSICSTEST_API AMyPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	
	UPROPERTY(BlueprintAssignable, Category = "Player")
	FOnDesyncDetected OnDesyncDetected;

	UPROPERTY(BlueprintAssignable, Category = "Player")
	FOnDesyncFixed OnDesyncFixed;

	UPROPERTY(BlueprintAssignable, Category = "Player")
	FOnTeamWinsWholeGame OnTeamWinsWholeGame;

	UPROPERTY(BlueprintAssignable, Category = "Player")
	FOnTeamScoresChanged OnTeamScoresChanged;

	UPROPERTY(BlueprintAssignable, Category = "Player")
	FOnCurrentGameModeTeamScoreChanged OnCurrentGameModeTeamScoreChanged;
	
	UPROPERTY(BlueprintAssignable, Category = "Player")
	FOnTeamWin OnTeamWin;
	
	UPROPERTY(BlueprintAssignable, Category = "Player")
	FOnTeamsTie OnTeamsTie;

	UPROPERTY(BlueprintAssignable, Category = "Player")
	FOnLocalPlayerItemsChanged OnLocalPlayerItemsChanged;

	UPROPERTY(BlueprintAssignable, Category = "Player")
	FOnRoundCountdownUpdate OnRoundCountdownUpdate;

	UPROPERTY(BlueprintAssignable, Category = "Player")
	FOnRoundCountdownFinished OnRoundCountdownFinished;

	UPROPERTY(BlueprintAssignable, Category = "Player")
	FOnStartLoading OnStartLoading;
	
	UPROPERTY(BlueprintAssignable, Category = "Player")
	FOnFinishLoading OnFinishLoading;

	UPROPERTY(BlueprintAssignable, Category = "Player")
	FOnNewLevelLoaded OnNewLevelLoaded;

	UPROPERTY(BlueprintAssignable, Category = "Player")
	FOnMouseSensitivityUpdated OnMouseSensitivityUpdated;

	UPROPERTY(BlueprintAssignable, Category = "TimeAttack")
	FOnCheckpointCollected OnCheckpointCollected;

	UPROPERTY(BlueprintAssignable, Category = "TimeAttack")
	FOnTimeAttackLevelIndexChanged OnTimeAttackLevelIndexChanged;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	// The inputs that are exchanged are ALWAYS confirmed inputs. It would make no sense to exchange predicted inputs.
	UFUNCTION(Unreliable, Server)
	void ServerSendInputs(const int& CharacterId, const TArray<FCommandFrame>& ConfirmedFrames);

	// The inputs that are exchanged are ALWAYS confirmed inputs. It would make no sense to exchange predicted inputs.
	UFUNCTION(Unreliable, Client)
	void ClientSendInputs(const int& ServerFrameId, const TArray<FCommandFrame>& ConfirmedFrames);

	// Send game state hash to server every now and then to make sure the simulations didn't de-sync
	UFUNCTION(Unreliable, Server)
	void ServerSendGameStateHash(const int& FrameId, const int64& GameStateHash);

	/** This is mostly used for initial setup to make sure all clients have all the needed data replicated to start the game */
	UFUNCTION(Reliable, Server)
	void ServerUpdatePlayerNetworkData(const FPlayerNetworkData& InPlayerNetworkData);

	UFUNCTION(Reliable, Server)
	void ServerUpdatePlayerSettings(const FPlayerSettings& InPlayerSettings);

	UFUNCTION(Unreliable, Client)
	void ClientInitializeController();

	UFUNCTION(Reliable, Client)
	void ClientSyncGameStateWithServer(const int& FrameId, const TArray<uint8>& BinaryGameState, const FSpawnerSubsystemState& SpawnerSubsystemState);

	UFUNCTION(Reliable, Server)
	void ServerSyncGameStateWithServer(const int& FrameId, const TArray<uint8>& BinaryGameState, const FSpawnerSubsystemState& SpawnerSubsystemState);

	UFUNCTION(Reliable, Client)
	void ClientConfirmStateSynced();
	
	UFUNCTION(Reliable, Server)
	void ServerConfirmStateSynced();

	UFUNCTION(BlueprintCallable, Meta = (DevelopmentOnly))
	void RequestClientToSyncState();

	UFUNCTION(BlueprintCallable, Meta = (DevelopmentOnly))
	void RequestServerToSyncState();

	UFUNCTION(BlueprintCallable, Meta = (DevelopmentOnly))
	void DesyncOnPurpose();

	UFUNCTION(BlueprintCallable, Category = "Player")
	void IncreaseMouseSensitivity();

	UFUNCTION(BlueprintCallable, Category = "Player")
	void DecreaseMouseSensitivity();

	UFUNCTION(BlueprintCallable, Category = "Player")
	void SetMouseSensitivty(float InPercentage);

	UFUNCTION(BlueprintPure, Category = "Player")
	float GetMouseSensitivity();

	UFUNCTION(BlueprintPure, Category = "Player")
	float GetMouseSensitivityPercentage();

	UFUNCTION(BlueprintPure, Category = "Player")
	EGameMode GetCurrentGameModeType();

	UFUNCTION(BlueprintPure, Category = "Player")
	FString GetLeaderboardName();

	UFUNCTION(BlueprintCallable, Category = "Player", Meta = (ExpandBoolAsExecs = "ReturnValue"))
	bool IsCurrentGameModeType(EGameMode CheckGameModeType);
	
	UFUNCTION(BlueprintCallable, Category = "Player")
	void DisableLocalPlayerInputs();
	
	UFUNCTION(BlueprintCallable, Category = "Player")
	void EnableLocalPlayerInputs();

	AMyPlayerState* GetMyPlayerState();
	AMyGameState* GetMyGameState();

	AMyPlayerState* GetMyPlayerState(const int& InPlayerId);

	float GetPing() const;

	UFUNCTION(BlueprintPure, Meta = (DisplayName = "Get Ping"))
	float BP_GetPing();

	TArray<FPlayerCharacterRenderData> GetCharactersRenderData() const;
	
	virtual void Tick(float DeltaSeconds) override;

	FPlayerNetworkData& GetPlayerNetworkData();
	void OnMatchHasStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Player", Meta = (DisplayName = "On Match Has Started"))
	void BP_OnMatchHasStarted();

	UFUNCTION(BlueprintPure, Category = "Player")
	APlayerRenderer* GetLocalPlayerRenderer();
	
	UFUNCTION(BlueprintPure, Category = "Player")
	APlayerRenderer* GetPlayerRenderer(int PlayerId);

	UFUNCTION(BlueprintPure, Category = "Player")
	FLinearColor GetTeamColor(EPlayerTeam Team);

	UFUNCTION(BlueprintPure, Category = "Player")
	EPlayerTeam GetPlayerTeam(int PlayerId);
	
	UFUNCTION(BlueprintPure, Category = "Player")
	EPlayerTeam GetLocalPlayerTeam();
	
	UFUNCTION(BlueprintPure, Category = "Player")
	int GetLocalPlayerId();

	UFUNCTION(BlueprintPure, Category = "Player")
	FTeamScores GetTeamScores();

	UFUNCTION(BlueprintPure, Category = "Player")
	FTeamScores GetCurrentGameModeTeamScores();

	int GetCurrentGameModeRespawnFrames() const;

	UFUNCTION()
	void OnRender();
	
	// This needs to be called ONLY for local player controllers. This means the server should call it only on their local controller.
	void LocalControllerInit();

	// Only used for initializing the lobby level, so it differs in ways like it doesn't do anything with replay subsystem, it supports 1 player
	void LocalControllerLobbyInit();

	// Initializes tutorial levels. You have to do them once and then everything else in the game unlocks. No replay system, supports only 1 player
	void LocalControllerTutorialInit();

	// Initializes a single-player time attack run. Loads the configured map, collects checkpoints, starts the timer.
	void LocalControllerTimeAttackInit();

	bool CanTakeCheckpoint(const ACheckpoint* InCheckpoint) const;
	void OnCheckpointReached(ACheckpoint* InCheckpoint);

	UFUNCTION(BlueprintPure, Category = "TimeAttack")
	float GetTimeAttackElapsedSeconds() const;

	UFUNCTION(BlueprintPure, Category = "TimeAttack")
	int GetTimeAttackTotalCheckpoints() const { return TimeAttackTotalCheckpoints; }

	UFUNCTION(BlueprintPure, Category = "TimeAttack")
	int GetTimeAttackFinishTimeMilliseconds() const { return FMath::RoundToInt(TimeAttackFinishTime * 1000.f); }

	UFUNCTION(BlueprintPure, Category = "TimeAttack")
	TArray<FTimeAttackLevelInfo> GetTimeAttackLevels() const;

	UFUNCTION(BlueprintPure, Category = "TimeAttack")
	FTimeAttackLevelInfo GetCurrentTimeAttackLevelInfo() const;

	UFUNCTION(BlueprintCallable, Category = "TimeAttack")
	void SetCurrentTimeAttackLevelIndex(int32 Index);

	UFUNCTION(BlueprintImplementableEvent, Category = "TimeAttack")
	void BP_OnTimeAttackComplete(float ElapsedSeconds, int ElapsedMilliseconds, const FString& LevelName);

	bool IsInitialized() const;
	void HandleLocalInputs(const int& InFrameId);
	void SendInputsToServer(const int& InFrameId);
	void SendInputsToClients(const int& InFrameId);

	void StepCharactersSimulation(const int& InFrameId, const float& FixedStepTime);

	void ProcessCollisions(const int& InFrameId);

	FPlayerCharacter& GetPlayerCharacter(const int& InPlayerId);
	FPlayerCharacter& GetEnemyPlayerCharacter(EPlayerTeam FriendlyTeam);

	/** THIS METHOD DOES NOT SUPPORT ROLLBACK. Should only be called when reward is fully confirmed by being outside the rollback window. */
	void GrantPlayerItem(const int& InPlayerId, ELootItem LootItem);

	bool IsPlayerDead(const int& InPlayerId);
	
	void SpawnGrantAbilityVFX(const FVector& EffectSpawnLocation, const int& BelongsToPlayerId);
	void SpawnShootCannonVFX(const FVector& EffectSpawnLocation);

	TArray<FPlayerCharacter> PlayerCharacters;
	Body* LocalPlayerBody = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Replay")
	bool bReplayMode = false;

	UPROPERTY(EditDefaultsOnly, Category = "Config|Replay")
	bool bRecordReplays = false;

	UPROPERTY(EditDefaultsOnly, Category = "Config|Renderer")
	TSubclassOf<APlayerRenderer> PlayerRendererClass;

	UPROPERTY(EditDefaultsOnly, Category = "Config|VFX")
	UNiagaraSystem* BulletHitVFX = nullptr;

	// Handles stuff like post process effects and anything else needed
	UPROPERTY(EditDefaultsOnly, Category = "Config|VFX")
	TSubclassOf<AActor> BulletHitVFXActor;
	
	UPROPERTY(EditDefaultsOnly, Category = "Config|VFX")
	TSubclassOf<AActor> PlayerDieVFXActor;

	UPROPERTY()
	TObjectPtr<class ALightSource> LightSource = nullptr;

	UPROPERTY()
	TArray<TObjectPtr<AActor>> PossibleSpawns;

	void CleanupFrame(const int& InFrameId);

	bool bPhysicsSimulationPaused = false;
	bool bLocalPlayerInputDisabled = false;

	// Delay turning the simulation back on until next frame
	bool bPendingPhysicsSimulationTurnPauseOff = false;

	UFUNCTION(BlueprintCallable, Category = "MyPlayerController")
	void BP_ReloadCurrentLevel();
	
	void ReloadCurrentLevel();
	void LoadNextLevel();
	void StartMatchAfterStageLoaded();

	void TearDownCurrentLevel();
	void LoadCurrentLevelImpl();

	int GetMaxBullets(const FPlayerCharacter& InPlayerCharacter);
	float GetDefaultProjectileImpulse(const FPlayerCharacter& InPlayerCharacter) const;

	void SetGameModeSettings(const FGameModeSettings& NewGameModeSettings);
	
	// Every game mode should handle its own cleanup here if it's own initializing
	void InitGameMode(const FGameModeSettings& NewGameModeSettings, const FGameModeSettings& InPreviousGameModeSettings);

	UPROPERTY(EditDefaultsOnly, Category = "GameModes|CaptureTheFlag")
	TSubclassOf<AJoltBodyActor> FlagClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "GameModes|CaptureTheFlag")
	TSubclassOf<AJoltConstraintActor> FlagConstraintClass;

	UPROPERTY(EditDefaultsOnly, Category = "GameModes|KingOfTheCrown")
	TSubclassOf<AJoltBodyActor> CrownClass;

	UPROPERTY(EditDefaultsOnly, Category = "GameModes|KingOfTheCrown")
	float CrownImpulseAfterBeingDropped = 100.f;

	// Because these score methods broadcast events to UI, they should only be called when the event is outside of rollback window and is 100% confirmed
	// This also means you can't put anything inside these methods that needs to be rollback compliant, since it won't be called again
	void IncreaseOppositeTeamsScore(EPlayerTeam PlayerTeam);

	void IncreaseCurrentGameModeOppositeTeamsScore(EPlayerTeam PlayerTeam);
	void IncreaseTeamsScore(EPlayerTeam WinningTeam);

	UFUNCTION(BlueprintCallable, Meta = (DevelopmentOnly))
	void DevelopmentIncreaseTeamScore(EPlayerTeam InPlayerTeam);

	UFUNCTION(BlueprintCallable, Meta = (DevelopmentOnly))
	void DevelopmentIncreaseCurrentGameModeScore(EPlayerTeam InPlayerTeam);

	UFUNCTION(BlueprintCallable, Category = "MyPlayerController|Tutorial")
	void CompleteCurrentTutorialLevel();
	
	UFUNCTION(BlueprintCallable, Category = "MyPlayerController|Tutorial")
	void FinishTutorials();

	int GetRollbackWindow() const;

	UFUNCTION(BlueprintPure, Category = "MyPlayerController|KingOfTheCrown")
	int GetHoldCrownFramesToWin() const;

	FTransform GetTransformWithMirroredAxis(const FVector& InSpawnLocation, const FRotator& InSpawnRotation) const;

	void DesyncDetected(const int& InFrameId);
	void DesyncFixed(const int& InFrameId);
	int GetDesyncDetectedFrameId() const;

	void ExplosiveBarrelHit(const int& InFrameId, const BodyID& InBarrelBodyID);
	void MineExpire(const BodyID& MineBodyID);
	void StickyBombExplode(const BodyID& InBombBodyID, float InExplosionRadius);

	void MoveComponentHierarchyToAnotherActor(USceneComponent* Component, AActor* NewOwner);
	void AddToCurrentStageJoltBodyActors(AJoltBodyActor* BodyActorToAdd);
protected:
	UPROPERTY(EditDefaultsOnly, Category = "MyPlayerController|Input")
	TObjectPtr<UInputAction> InputShoot;
	
	UPROPERTY(EditDefaultsOnly, Category = "MyPlayerController|Input")
	TObjectPtr<UInputAction> InputReset;
	
	UPROPERTY(EditDefaultsOnly, Category = "MyPlayerController|Input")
	TObjectPtr<UInputAction> InputDash;

	UPROPERTY(EditDefaultsOnly, Category = "MyPlayerController|Input")
	TObjectPtr<UInputAction> InputParry;
	
	UPROPERTY(EditDefaultsOnly, Category = "MyPlayerController|Player")
	TSubclassOf<AJoltContainerActor> PlayerBodyClass;

	UPROPERTY(EditDefaultsOnly, Category = "MyPlayerController|Abilities")
	TSubclassOf<AJoltBodyActor> ShootActorClass;

	UPROPERTY(EditDefaultsOnly, Category = "MyPlayerController|Abilities")
	TSubclassOf<AJoltBodyActor> SpawnBoxActorClass;

	UPROPERTY(EditDefaultsOnly, Category = "MyPlayerController|Abilities")
	TSubclassOf<AJoltConstraintActor> PlayerToPlayerRopeConstraintClass;

	UPROPERTY(EditDefaultsOnly, Category = "MyPlayerController|Abilities")
	TSubclassOf<AJoltConstraintActor> PullingRopeConstraintClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "MyPlayerController|Abilities")
	TSubclassOf<AJoltConstraintActor> JavelinConstraintActorClass;

	UPROPERTY(EditDefaultsOnly, Category = "MyPlayerController|Abilities")
	TSubclassOf<AJoltConstraintActor> GrappleRopeConstraintClass;

	UPROPERTY(EditDefaultsOnly, Category = "MyPlayerController|Abilities")
	float GrappleRopeMaxRange = 80.f;

	// Shorten rope by this amount so player feels some pull
	UPROPERTY(EditDefaultsOnly, Category = "MyPlayerController|Abilities")
	float GrapplePullRange = 10.f;

	// 0 = attach at shoulder center, 1 = attach at the outer edge in the shoot direction
	UPROPERTY(EditDefaultsOnly, Category = "MyPlayerController|Abilities", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float GrappleShoulderAttachAlpha = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = "MyPlayerController|Abilities")
	float GrappleShrinkRate = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "MyPlayerController|Abilities")
	float GrappleMinDistance = 5.f;

	UPROPERTY(EditDefaultsOnly, Category = "MyPlayerController")
	TSubclassOf<AJoltBodyActor> TombstoneClass;

	UPROPERTY(EditDefaultsOnly, Category = "MyPlayerController")
	TSubclassOf<APlayerSpinner> PlayerSpinnerClass;

	UPROPERTY(EditDefaultsOnly, Category = "MyPlayerController|Levels")
	TObjectPtr<UCurveFloat> PlatformsMoveIntoScreenCurve = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "MyPlayerController|Levels")
	TObjectPtr<UCurveFloat> PlatformsMoveOutOfScreenCurve = nullptr;

	// How far away should platforms spawn from target position when loading in a level and then lerp to target
	UPROPERTY(EditDefaultsOnly, Category = "MyPlayerController|Levels")
	float DistanceAwayFromTargetPositionForTweening = 30000.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "MyPlayerController|Levels")
	float TweeningAlphaVariation = 0.f;
	
	virtual void SetupInputComponent() override;

	void OnShootInputStarted(const FInputActionValue& InputActionValue);
	void OnShootInputFinished(const FInputActionValue& InputActionValue);

	void OnResetInputStarted(const FInputActionValue& InputActionValue);
	void OnResetInputFinished(const FInputActionValue& InputActionValue);
	
	void OnDashInputStarted(const FInputActionValue& InputActionValue);
	void OnDashInputFinished(const FInputActionValue& InputActionValue);

	void OnParryInputStarted(const FInputActionValue& InputActionValue);
	void OnParryInputFinished(const FInputActionValue& InputActionValue);
private:
	float SecondsCountdownBeforeRoundStart = 0.f;

	FLevelData CurrentLevelData;
	
	TArray<FLevelData> BuildLevelsSequenceForMatch() const;
	TArray<FLevelData> BuildLevelsSequenceForTutorial();
	TArray<FLevelData> BuildLevelsSequenceForTimeAttack();

	int TimeAttackStartFrame = -1;
	int TimeAttackNextCheckpointIndex = 0;
	float TimeAttackFinishTime = -1.f;
	int TimeAttackTotalCheckpoints = 0;
	
	class UMyGameSave* GetCurrentGameSave();
	bool bSyncingStateInProgress = false;
	int DesyncDetectedOnFrameId = -1;
	bool bGameOver = false;
	
	int RollbackWindow = 0;
	FVector CrownSpawnLocation = FVector::ZeroVector;
	int CrownModeEndStep = 0;
	void ReinitGameCamera();
	bool bTurnOffDamageBetweenPlayers = false;
	
	FGameModeSettings GameModeSettings;
	FGameModeSettings PreviousGameModeSettings;

	int RopeConstraintId = -1;
	
	FTeamScores TeamScores;
	FTeamScores CurrentGameModeTeamScores;

	void RespawnPlayerCharacters();
	void ResetPlayerCharacterToStartingPosition(FPlayerCharacter& PlayerCharacter);

	int StartMatchOnFrame = -1;
	void DestroySpawnLocationActors();

	int CurrentLevelIndex = 0;

	void MarkPlayerCharacterAsDead(const int& InFrameId, FPlayerCharacter& PlayerCharacter);
	
	void StepCharacterSimulationMovementAndAbilities(const int& InFrameId, FPlayerCharacter& PlayerCharacter);
	
	void StepCharacterSimulationTutorial(const int& InFrameId, FPlayerCharacter& PlayerCharacter);

	void StepCharacterSimulationTimeAttack(const int& InFrameId, FPlayerCharacter& PlayerCharacter);
	
	void StepCharacterSimulationDeathmatch(const int& InFrameId, FPlayerCharacter& PlayerCharacter);

	void StepCharacterSimulationRopedTogether(const int& InFrameId, FPlayerCharacter& PlayerCharacter);

	void StepCharacterSimulationPullingRope(const int& InFrameId, FPlayerCharacter& PlayerCharacter);

	void StepCharacterSimulationCaptureTheFlag(const int& InFrameId, FPlayerCharacter& PlayerCharacter);
	
	void StepCharacterSimulationKingOfTheCrown(const int& InFrameId, FPlayerCharacter& PlayerCharacter);

	int GetBulletReloadCooldownInFrames(const FPlayerCharacter& PlayerCharacter);
	
	UPROPERTY()
	TArray<FLevelData> LevelsSequence;

	UPROPERTY()
	TArray<class AJoltBodyActor*> CurrentStageJoltBodyActors;

	UPROPERTY()
	TMap<class AJoltBodyActor*, float> JoltBodyActorTweeningRandomAlphaOffset;

	UPROPERTY()
	AGameCamera* GameCamera = nullptr;

	UPROPERTY()
	AGameCameraLimits* GameCameraLimits = nullptr;
	
	UPROPERTY()
	class AGameBackground* GameBackground = nullptr;
	
	UPROPERTY()
	TObjectPtr<UGeneralData> GeneralData = nullptr;
	THashContainer UsedEffects;
	
	void PlayerExplodeDirectional(const int& InFrameId, const AJoltBodyActor* PlayerBodyActor, const BodyID& PlayerBodyID, const Vec3& HitPosition, const Vec3& HitDirection, int KilledByPlayerId = -1);
	void PlayerExplodeOmniDirectional(const int& InFrameId, const AJoltBodyActor* PlayerBodyActor, const BodyID& PlayerBodyID);
	void BulletHit(const int& InFrameId, const BodyID& InBulletBodyID, const Vec3& BulletLinearVelocity, const FContact& Contact);
	void ProjectileBounce(const int& InFrameId, class ABananaProjectile* BananaProjectile, const Vec3& BodyLinearVel, const Vec3& ContactWorldNormal);
	void BombExplode(const int& InFrameId, const BodyID& BombBodyID);
	void PlayerHandContactWithFlagBase(const int& InFrameId, AJoltBodyActor* FlagBaseBodyActor, FPlayerCharacter& InPlayerCharacter, const Vec3& ContactWorldPosition);
	void BoomerangHitPlayer(const int& InFrameId, AJoltBodyActor* BoomerangBodyActor, AJoltBodyActor* PlayerTakeDamageSensorBodyActor, const Vec3& ContactWorldPosition);
	void CleanupActiveSwordBody(const int& InFrameId, FPlayerCharacter& InPlayerCharacter);
	FVector2D GetMouseDelta();
	FVector2D LastMousePosition = FVector2D::ZeroVector;
	FVector2D AccumulatedMouseDelta;
	
	void InitializeJoltBodiesAndConstraints();

	TArray<AJoltBodyActor*> GetJoltBodyActorsSortedBySpawnOrder();
	TArray<AJoltContainerActor*> GetJoltContainerActorsSortedBySpawnOrder();
	

	// Super useful for only designing one half of a mirrored level and then automatically spawning the other half in-game
	template<typename T>
	T* SpawnMirroredActor(AActor* InActor);

	// The reason for both the methods below is to handle flipping sprite on X axis by setting scale to -1.f, instead of flipping the sprite upside down
	AActor* SpawnSpriteActor(TSubclassOf<AActor> InActorClass, const FVector& InSpawnLocation, const FRotator& InSpawnRotation);
	AActor* SpawnSpriteActor(TSubclassOf<AActor> InActorClass, const FVector& InSpawnLocation);
	

	int CreateCharacter(class APlayerSpawn* InPlayerSpawn);
	class APlayerSpawn* GetPlayerSpawn(const int& CharacterIndex, const TArray<AActor*>& InPossibleSpawns);
	void StepCharacterSimulation(const int& InFrameId, FPlayerCharacter& InPlayerCharacter, const float& FixedStepTime);

	void AddUsedEffect(const int& InFrameId, const uint64& InEffectHash);

	void LoadAssetsForGame();
	
	bool bInitialized = false;
	bool bShootInputPressed = false;
	bool bResetInputPressed = false;
	bool bDashInputPressed = false;
	bool bParryInputPressed = false;

	/** This is basically the fire rate of the weapon. How quickly can you shoot one bullet after another */
	UPROPERTY(EditDefaultsOnly, Meta = (AllowPrivateAccess = "true"), Category = "Config|Weapon")
    int ShootingCooldownInFrames = 1;

	UPROPERTY(EditDefaultsOnly, Meta = (AllowPrivateAccess = "true"), Category = "Config|Weapon")
	int BulletReloadCooldownInFrames = 1;

	UPROPERTY(EditDefaultsOnly, Meta = (AllowPrivateAccess = "true"), Category = "Config|Weapon")
	int BulletReloadCooldownInFramesWhenHoldingCrown = 1;

	UPROPERTY(EditDefaultsOnly, Meta = (AllowPrivateAccess = "true"), Category = "Config|Weapon")
	int StartingMaxBullets = 3;

	UPROPERTY(EditDefaultsOnly, Meta = (AllowPrivateAccess = "true"), Category = "Config|Weapon")
	int DashCooldownInFrames = 1;

	UPROPERTY(EditDefaultsOnly, Meta = (AllowPrivateAccess = "true"), Category = "Config|Weapon")
	float DefaultProjectileImpulse = 30000.f;

	UPROPERTY(EditDefaultsOnly, Meta = (AllowPrivateAccess = "true"), Category = "Config|Weapon")
	float ProjectileKnockbackOnPlayer = 30000.f;

	UPROPERTY(EditDefaultsOnly, Meta = (AllowPrivateAccess = "true"), Category = "Config|Weapon")
	float FireballImpulseMin = 10000.f;

	UPROPERTY(EditDefaultsOnly, Meta = (AllowPrivateAccess = "true"), Category = "Config|Weapon")
	float FireballImpulseMax = 30000.f;

	UPROPERTY(EditDefaultsOnly, Meta = (AllowPrivateAccess = "true"), Category = "Config|Weapon")
	float FireballImpulse = 12000.f;

	UPROPERTY(EditDefaultsOnly, Meta = (AllowPrivateAccess = "true"), Category = "Config|Weapon")
	float FireballKnockback = 30000.f;

	// Max frames the player needs to hold shoot button to reach maximum knockback power on release
	UPROPERTY(EditDefaultsOnly, Meta = (AllowPrivateAccess = "true"), Category = "Config|Weapon")
	int MaxFramesHoldShootButton = 60;

	UPROPERTY(EditDefaultsOnly, Meta = (AllowPrivateAccess = "true"), Category = "Config|Weapon")
	float ShootKnockbackMin = 1000.f;

	UPROPERTY(EditDefaultsOnly, Meta = (AllowPrivateAccess = "true"), Category = "Config|Weapon")
	float ShootKnockbackMax = 30000.f;

	UPROPERTY(EditDefaultsOnly, Meta = (AllowPrivateAccess = "true"), Category = "Config|Weapon")
	bool bUseChargeShooting = false;

	UPROPERTY(EditDefaultsOnly, Meta = (AllowPrivateAccess = "true", EditCondition = "bUseChargeShooting"), Category = "Config|Weapon")
	bool bUseHandExtensionToCalculateShootPower = false;

	// How far from the hammer to spawn projectile
	UPROPERTY(EditDefaultsOnly, Category = "Config|Weapon")
	float ProjectileSpawnForwardDistance = 2.f;

	UPROPERTY(EditDefaultsOnly, Meta = (AllowPrivateAccess = "true"), Category = "Config|Weapon")
	float ProjectileApplyImpulseOnHit = 30000.f;

	UPROPERTY(EditDefaultsOnly, Meta = (AllowPrivateAccess = "true"), Category = "Config|Weapon")
	float DefaultDashImpulse = 400.f;

	UPROPERTY(EditDefaultsOnly, Meta = (AllowPrivateAccess = "true"), Category = "Config|Movement")
	TObjectPtr<UPlayerMovementConfigData> PlayerMovementConfigData = nullptr;

	UPROPERTY(EditDefaultsOnly, Meta = (AllowPrivateAccess = "true"), Category = "Config|Movement")
	float ForceToApplyPerFrame = 10000000.f;

	UPROPERTY(EditDefaultsOnly, Meta = (AllowPrivateAccess = "true"), Category = "Config|Movement")
	float MinMouseSensitivity = 0.1f;

	UPROPERTY(EditDefaultsOnly, Meta = (AllowPrivateAccess = "true"), Category = "Config|Movement")
	float MaxMouseSensitivity = 2.f;

	UPROPERTY(EditDefaultsOnly, Meta = (AllowPrivateAccess = "true"), Category = "Config|Respawn")
	int InvulnerabilityFramesAfterRespawnNum = 60;

	/**
	 * Each player controller will have a copy of this.
	 * Clients will RPC this struct to the server to inform about their current game state.
	 * Such as whether their player state already replicated to them, whether they've loaded everything etc.
	 * Then the server just needs to iterate all player controllers and check that they're all ready before moving on to the next thing.
	 */
	FPlayerNetworkData PlayerNetworkData;

#pragma region NetworkClockSync

public:

	float GetServerWorldTimeDelta() const;
	float GetServerWorldTime() const;
	float GetOneWayTripTime() const;
	float GetRoundTripTime() const;

	UFUNCTION(BlueprintPure, Category = "GameState", Meta = (DisplayName = "Get Server World Time Delta"))
	float BP_GetServerWorldTimeDelta();

	UFUNCTION(BlueprintPure, Category = "GameState", Meta = (DisplayName = "Get Server World Time"))
	float BP_GetServerWorldTime();

	UFUNCTION(BlueprintPure, Category = "GameState", Meta = (DisplayName = "Get One Way Latency"))
	float BP_GetOneWayLatency();

	UFUNCTION(BlueprintPure, Category = "GameState", Meta = (DisplayName = "Get Round Trip Latency"))
	float BP_GetRoundTripLatency();

	virtual void PostNetInit() override;

protected:

	/** Frequency that the client requests to adjust it's local clock. Set to zero to disable periodic updates. */
	UPROPERTY(EditDefaultsOnly, Category=GameState)
	float NetworkClockUpdateFrequency = 1.f;
private:
	FFloatInterval LevelBoundsX;
	FFloatInterval LevelBoundsZ;
	bool bLevelBoundsInitialized = false;
	
	float ServerWorldTimeDelta = 0.f;
	float OneWayTripTime = 0.f;
	float RoundTripTime = 0.f;
	TArray<float> RTTCircularBuffer;
	
	void RequestWorldTime_Internal();

	// This is only called on clients player controllers on the server
	// So if it's a listen server, the player on the server will NOT get this message and therefor will have no RTT time set at all
	// And it makes sense, because unless it's a 1v1 game, what would the RTT even mean for the server? It can have a different one for each of the connected clients.
	UFUNCTION(Server, Unreliable)
	void ServerRequestWorldTime(float ClientTimestamp, float RTT);
	
	UFUNCTION(Client, Unreliable)
	void ClientUpdateWorldTime(float ClientTimestamp, float ServerTimestamp);

#pragma endregion NetworkClockSync
};

