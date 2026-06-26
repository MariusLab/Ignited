// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MyGameSave.h"
#include "SIK_SharedFile.h"
#include "Engine/GameInstance.h"
#include "MyGameInstance.generated.h"

UCLASS()
class JOLTPHYSICSTEST_API UMyGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	TObjectPtr<USoundClass> SoundEffectsMaster = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	TObjectPtr<USoundClass> MusicMaster = nullptr;
	
	virtual void Init() override;

	UFUNCTION(BlueprintCallable)
	void SetMusicVolume(float NewVolume);
	
	UFUNCTION(BlueprintCallable)
	void SetSoundsVolume(float NewVolume);
	
	UFUNCTION(BlueprintCallable)
    void SetResolution(int ResX, int ResY);
    	
    UFUNCTION(BlueprintCallable)
    void SetFullscreen(bool bFullscreen);

	UFUNCTION(BlueprintCallable)
	void SetVSync(bool bVSync);

	UFUNCTION(BlueprintCallable)
	void SetLanguage(FString NewLanguage);

	UFUNCTION(BlueprintPure)
	bool IsTutorialCompleted();

	UFUNCTION(BlueprintPure)
	FGameSettings GetGameSettings();

	UFUNCTION(BlueprintCallable)
	void BP_SaveGame();

	UMyGameSave* GetCurrentGameSave();

	void SaveGame();
	
	UPROPERTY(EditDefaultsOnly)
	int ExpectedPlayersNum = 2;
	
	UPROPERTY(BlueprintReadOnly)
	bool bReplayMode = false;

	UPROPERTY(BlueprintReadOnly)
	bool bProcessReplayMode = false;

	UPROPERTY(BlueprintReadOnly)
	FString ReplayName;

	UPROPERTY(BlueprintReadWrite)
	FSIK_SteamId CurrentLobbyId;

	UPROPERTY(BlueprintReadWrite, Category = "TimeAttack")
	int32 CurrentTimeAttackLevelIndex = 0;

	virtual void BeginDestroy() override;

private:
	UPROPERTY()
	TObjectPtr<UMyGameSave> MyGameSave = nullptr;
};
