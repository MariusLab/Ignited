// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "JoltPhysicsTest/Debug/CustomLogger.h"
#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "MyGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnConfirmedRoundEnd);

UCLASS()
class JOLTPHYSICSTEST_API AMyGameState : public AGameState
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintAssignable, Category = "MyGameState")
	FOnConfirmedRoundEnd OnConfirmedRoundEnd;
	
	UFUNCTION(BlueprintPure, Category = "MyGameState")
	int GetCaptureTheFlagMaxPoints();

	UFUNCTION(BlueprintPure, Category = "MyGameState")
	int GetDeathmatchMaxPoints();

	UFUNCTION(BlueprintPure, Category = "MyGameState")
	int GetPointsNeededToWinTheMatch();

	UFUNCTION(BlueprintPure, Category = "MyGameState")
	bool BP_HasRoundEnded();
	
	void SaveState(StateRecorderImpl& InStream);
	void RestoreState(StateRecorderImpl& InStream);

	void SetFirstPlayerToDieId(const int& InPlayerId);
	int GetFirstPlayerToDieId() const;
	void SetPlayerIdThatHasCrown(const int& InPlayerId);
	int GetPlayerIdThatHasCrown() const;
	void ClearRoundData();

	void EndRound();
	bool HasRoundEnded() const;
	int GetRoundEndedFramesNum() const;
	void IncrementRoundEndedFramesNum();
protected:
	virtual void HandleMatchHasStarted() override;
private:
	int FirstPlayerToDieThisRoundId = -1;
	int RoundEndedFramesNum = -1;
	int PlayerIdThatHasCrown = -1;
	bool bConfirmedRoundEnd = false;
};
