// Fill out your copyright notice in the Description page of Project Settings.


#include "MyGameState.h"

#include "GameLoopSubsystem.h"
#include "JoltPhysicsTest/Data/AssetManagerSubsystem.h"
#include "JoltPhysicsTest/FunctionLibrary/Helper.h"
#include "JoltPhysicsTest/Player/MyPlayerController.h"
#include "Kismet/GameplayStatics.h"

int AMyGameState::GetCaptureTheFlagMaxPoints()
{
	return UAssetManagerSubsystem::GetGeneralData(GetWorld())->CaptureTheFlagMaxPoints;
}

int AMyGameState::GetDeathmatchMaxPoints()
{
	return UAssetManagerSubsystem::GetGeneralData(GetWorld())->DeathmatchMaxPoints;
}

int AMyGameState::GetPointsNeededToWinTheMatch()
{
	return UAssetManagerSubsystem::GetGeneralData(GetWorld())->PointsNeededToWinTheMatch;
}

bool AMyGameState::BP_HasRoundEnded()
{
	return HasRoundEnded();
}

void AMyGameState::SaveState(StateRecorderImpl& InStream)
{
	InStream.Write(FirstPlayerToDieThisRoundId);
	InStream.Write(RoundEndedFramesNum);
	InStream.Write(PlayerIdThatHasCrown);
}

void AMyGameState::RestoreState(StateRecorderImpl& InStream)
{
	InStream.Read(FirstPlayerToDieThisRoundId);
	InStream.Read(RoundEndedFramesNum);
	InStream.Read(PlayerIdThatHasCrown);
}

void AMyGameState::SetFirstPlayerToDieId(const int& InPlayerId)
{
	if (FirstPlayerToDieThisRoundId > -1)
	{
		return;
	}
	
	FirstPlayerToDieThisRoundId = InPlayerId;
}

int AMyGameState::GetFirstPlayerToDieId() const
{
	return FirstPlayerToDieThisRoundId;
}

void AMyGameState::SetPlayerIdThatHasCrown(const int& InPlayerId)
{
	if (InPlayerId == PlayerIdThatHasCrown)
	{
		return;
	}

	PlayerIdThatHasCrown = InPlayerId;
}

int AMyGameState::GetPlayerIdThatHasCrown() const
{
	return PlayerIdThatHasCrown;
}

void AMyGameState::ClearRoundData()
{
	FirstPlayerToDieThisRoundId = -1;
	RoundEndedFramesNum = -1;
	PlayerIdThatHasCrown = -1;
	bConfirmedRoundEnd = false;
}

void AMyGameState::EndRound()
{
	RoundEndedFramesNum = 0;
}

bool AMyGameState::HasRoundEnded() const
{
	return RoundEndedFramesNum >= 0;
}

int AMyGameState::GetRoundEndedFramesNum() const
{
	return RoundEndedFramesNum;
}

void AMyGameState::IncrementRoundEndedFramesNum()
{
	RoundEndedFramesNum++;

	if (!bConfirmedRoundEnd && RoundEndedFramesNum > UHelper::GetLocalPlayerController(GetWorld())->GetRollbackWindow())
	{
		bConfirmedRoundEnd = true;
		OnConfirmedRoundEnd.Broadcast();
	}
}

void AMyGameState::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

	auto LocalPlayerController = UHelper::GetLocalPlayerController(GetWorld());

	if (GetNetMode() == NM_Client)
	{
		LocalPlayerController->LocalControllerInit();
		LocalPlayerController->OnMatchHasStarted();
	}

	GetWorld()->GetSubsystem<UGameLoopSubsystem>()->StartLoop();
}
