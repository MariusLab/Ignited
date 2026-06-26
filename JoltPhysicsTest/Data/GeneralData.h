// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LootItems.h"
#include "Engine/DataAsset.h"
#include "NiagaraSystem.h"
#include "JoltPhysicsTest/Player/PlayerBodyVFX.h"
#include "JoltPhysicsTest/Player/PlayerTeamsEnum.h"
#include "JoltPhysicsTest/Game/GameModesEnum.h"
#include "JoltPhysicsTest/Game/GameModeSettings.h"
#include "JoltPhysicsTest/Data/MapData.h"
#include "GeneralData.generated.h"

USTRUCT()
struct FPlayerColorScheme
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	EPlayerTeam Team = EPlayerTeam::None;

	UPROPERTY(EditDefaultsOnly)
	FString DefaultPlayerName = "Player";
	
	UPROPERTY(EditDefaultsOnly)
	FLinearColor MainColor;
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UMaterialInterface> BodyMaterial = nullptr;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UMaterialInterface> StickMaterial = nullptr;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UMaterialInterface> HammerMaterial = nullptr;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UMaterialInterface> HeadMaterial = nullptr;
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UMaterialInterface> EyesMaterial = nullptr;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UMaterialInterface> UpperArmMaterial = nullptr;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UMaterialInterface> LowerArmMaterial = nullptr;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UMaterialInterface> OutlineMaterial = nullptr;
};

USTRUCT()
struct FLevelLoadSettings
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly)
	int TweenLevelFramesNum = 120;

	UPROPERTY(EditDefaultsOnly)
	bool bPlayLevelIntroSound = true;
};

inline FArchive& operator<<(FArchive& Ar, FLevelLoadSettings& Data)
{
	Ar << Data.TweenLevelFramesNum;
	Ar << Data.bPlayLevelIntroSound;
		
	return Ar;
}

USTRUCT()
struct FLevelData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	FGameModeSettings GameModeSettings;
	
	UPROPERTY(EditDefaultsOnly)
	FLevelLoadSettings LevelLoadSettings;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UMapData> MapData = nullptr;
};

inline FArchive& operator<<(FArchive& Ar, FLevelData& Data)
{
	Ar << Data.GameModeSettings;

	FString MapDataPath;
	if (Ar.IsSaving())
	{
		MapDataPath = Data.MapData ? FSoftObjectPath(Data.MapData).ToString() : FString();
	}

	Ar << MapDataPath;

	if (Ar.IsLoading() && !MapDataPath.IsEmpty())
	{
		Data.MapData = TSoftObjectPtr<UMapData>(FSoftObjectPath(MapDataPath)).LoadSynchronous();
	}

	return Ar;
}

UCLASS()
class JOLTPHYSICSTEST_API UGeneralData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category = "GameMode")
	bool bBuildRandomLevelSequenceFromPool = false;
	
	// Random levels rotation will be drawn from this pool
	UPROPERTY(EditDefaultsOnly, Category = "GameMode", Meta = (EditCondition = "bBuildRandomLevelSequenceFromPool"))
	TArray<FLevelData> LevelsPool;
	
	UPROPERTY(EditDefaultsOnly, Category = "GameMode")
	TArray<FLevelData> LevelsData;
	
	UPROPERTY(EditDefaultsOnly, Category = "GameMode")
	TArray<FLevelData> TutorialLevels;

	UPROPERTY(EditDefaultsOnly, Category = "GameMode")
	TArray<FLevelData> TimeAttackLevels;

	UPROPERTY(EditDefaultsOnly, Category = "Config|Abilities")
	int AbilitySpinnerTakesFramesNum = 260;

	// You can figure this one out by enabling the print string in UI_LootSpinner
	UPROPERTY(EditDefaultsOnly, Category = "Config|Abilities")
	int SpinnerStopsOnItemIndex = 18;

	UPROPERTY(EditDefaultsOnly, Category = "Config|Camera")
	float MinOrthoWidth = 70000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Config|Camera")
	float MaxOrthoWidth = 100000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Config|Camera")
	float SinglePlayerOrthoWidth = 75000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Config|Camera")
	float CameraPaddingX = 30000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Config|Camera")
	float CameraPaddingZ = 15000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Config|Camera")
	FVector CameraLocationOffset = FVector(0, 0, 500.f);
	
	UPROPERTY(EditDefaultsOnly, Category = "Config|Meshes")
	UStaticMesh* BoxStaticMesh = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Config|Meshes")
	UStaticMesh* SphereStaticMesh = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Config|Materials")
	UMaterialInterface* PlatformMaterial = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Config|Materials")
	UMaterialInterface* RopeMaterial = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Config|PostProcess")
	FPostProcessSettings PostProcessSettings;

	UPROPERTY(EditDefaultsOnly, Category = "Config|Players")
	TArray<FPlayerColorScheme> PossiblePlayerColorSchemes;

	UPROPERTY(EditDefaultsOnly, Category = "Config|VFX")
	TObjectPtr<UNiagaraSystem> DashVFX = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Config|VFX")
	TSubclassOf<APlayerBodyVFX> PlayerBodyDashVFXClass;

	UPROPERTY(EditDefaultsOnly, Category = "Config|VFX")
	TSubclassOf<APlayerBodyVFX> PlayerShootFireballBodyVFX;
	
	UPROPERTY(EditDefaultsOnly, Category = "Config|VFX")
	TObjectPtr<UNiagaraSystem> PlayerExplodeDirectional;

	UPROPERTY(EditDefaultsOnly, Category = "Config|VFX")
	TObjectPtr<UNiagaraSystem> PlayerExplodeOmniDirectional;

	UPROPERTY(EditDefaultsOnly, Category = "Config|VFX")
	TObjectPtr<UNiagaraSystem> HandContactDust;

	UPROPERTY(EditDefaultsOnly, Category = "Config|VFX")
	TObjectPtr<UNiagaraSystem> TakeFlagVFX;

	UPROPERTY(EditDefaultsOnly, Category = "Config|VFX")
	TObjectPtr<UNiagaraSystem> PlaceFlagVFX;

	UPROPERTY(EditDefaultsOnly, Category = "Config|VFX")
	TObjectPtr<UNiagaraSystem> WinCrownVFX;

	UPROPERTY(EditDefaultsOnly, Category = "Config|VFX")
	TSubclassOf<AActor> ShootFireballVFX;

	UPROPERTY(EditDefaultsOnly, Category = "Config|VFX")
	TSubclassOf<AActor> FireballImpactVFX;

	UPROPERTY(EditDefaultsOnly, Category = "Config|VFX")
	TObjectPtr<UNiagaraSystem> FireballImpactProceduralVFX;

	UPROPERTY(EditDefaultsOnly, Category = "Config|VFX")
	TObjectPtr<UNiagaraSystem> FireballBlueImpactProceduralVFX;

	UPROPERTY(EditDefaultsOnly, Category = "Config|VFX")
	TSubclassOf<AActor> PlayerDashVFX;

	UPROPERTY(EditDefaultsOnly, Category = "Config|VFX")
	TSubclassOf<AActor> PlayerExplodeVFX;
	
	UPROPERTY(EditDefaultsOnly, Category = "Config|VFX")
	TSubclassOf<AActor> PlayerExplodeDirectionalVFX;

	UPROPERTY(EditDefaultsOnly, Category = "Config|VFX")
	TSubclassOf<AActor> PlayerExplodeDirectionalBlueVFX;

	UPROPERTY(EditDefaultsOnly, Category = "Config|VFX")
	TSubclassOf<AActor> PlayerExplodeLightningVFX;

	UPROPERTY(EditDefaultsOnly, Category = "Config|VFX")
	TSubclassOf<AActor> PlayerExplodeLightningBlueVFX;

	UPROPERTY(EditDefaultsOnly, Category = "Config|VFX")
	TSubclassOf<AActor> PickupLootSpinnerVFX;

	UPROPERTY(EditDefaultsOnly, Category = "Config|VFX")
	TSubclassOf<AActor> PlayerHasAbilityVFX;
	
	UPROPERTY(EditDefaultsOnly, Category = "Config|VFX")
	TSubclassOf<AActor> GrantAbilityVFX;

	UPROPERTY(EditDefaultsOnly, Category = "Config|VFX")
	TSubclassOf<AActor> BombExplodeVFX;
	
	UPROPERTY(EditDefaultsOnly, Category = "Config|VFX")
	TSubclassOf<AActor> BarrelExplodeVFX;

	UPROPERTY(EditDefaultsOnly, Category = "Data")
	TObjectPtr<UDataTable> LootItemsDataTable;

	UPROPERTY(EditDefaultsOnly, Category = "Data")
	TObjectPtr<UDataTable> SoundEffectsDataTable;

	UPROPERTY(EditDefaultsOnly, Category = "Debug", Meta = (DevelopmentOnly))
	bool bKeepCameraAtMaximumZoomOut = false;

	UPROPERTY(EditDefaultsOnly, Category = "Debug", Meta = (DevelopmentOnly))
	bool bNoCooldowns = false;

	UPROPERTY(EditDefaultsOnly, Category = "Debug", Meta = (DevelopmentOnly))
	ELootItem StartPlayerOneWithAbility = ELootItem::None;

	UPROPERTY(EditDefaultsOnly, Category = "Debug", Meta = (DevelopmentOnly))
	ELootItem StartPlayerTwoWithAbility = ELootItem::None;

	UPROPERTY(EditDefaultsOnly, Category = "Debug", Meta = (DevelopmentOnly))
	bool bDebugTutorial = false;

	UPROPERTY(EditDefaultsOnly, Category = "Debug", Meta = (DevelopmentOnly, EditCondition = "bDebugTutorial"))
	int DebugTutorialLevelIndex = 0;

	UPROPERTY(EditDefaultsOnly, Category = "Config|Deathmatch")
	int DeathmatchPlayerRespawnFrames = 300;

	// How many points do you need to score to win a round of deathmatch
	UPROPERTY(EditDefaultsOnly, Category = "Config|Deathmatch")
	uint8 DeathmatchMaxPoints = 3;

	// How many points do you need to score to win a round of capture the flag
	UPROPERTY(EditDefaultsOnly, Category = "Config|CaptureTheFlag")
	uint8 CaptureTheFlagMaxPoints = 3;

	UPROPERTY(EditDefaultsOnly, Category = "Config|CaptureTheFlag")
	int CaptureTheFlagPlayerRespawnFrames = 300;

	// How many frames must a player hold the crown to win (locked to 60 frames per second)
	UPROPERTY(EditDefaultsOnly, Category = "Config|KingOfTheCrown")
	int HoldCrownFramesToWin = 600;

	UPROPERTY(EditDefaultsOnly, Category = "Config|KingOfTheCrown")
	int KingOfTheCrownPlayerRespawnFrames = 300;

	UPROPERTY(EditDefaultsOnly, Category = "Config|KingOfTheCrown")
	int KingOfTheCrownHolderMaxBullets = 1;

	// How many points do you need to score to win the whole match
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	uint8 PointsNeededToWinTheMatch = 10;
};
