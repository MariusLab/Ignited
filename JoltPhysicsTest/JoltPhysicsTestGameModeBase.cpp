// Fill out your copyright notice in the Description page of Project Settings.


#include "JoltPhysicsTestGameModeBase.h"

#include "Debug/CustomLogger.h"
#include "FunctionLibrary/Helper.h"
#include "Game/MyGameInstance.h"
#include "Player/MyPlayerController.h"


bool AJoltPhysicsTestGameModeBase::ReadyToStartMatch_Implementation()
{
	if (bDelayedStart) {
		return false;
	}
	
	if (GetMatchState() != MatchState::WaitingToStart) {
		return false;
	}

	if (GetNetMode() == NM_Standalone)
	{
		UHelper::GetLocalPlayerController(GetWorld())->LocalControllerInit();
		UHelper::GetLocalPlayerController(GetWorld())->OnMatchHasStarted();

		return true;
	}

	auto MyGameInstance = UHelper::GetMyGameInstance(GetWorld());
	const int ExpectedPlayersNum = MyGameInstance->ExpectedPlayersNum;

	if (NumPlayers < ExpectedPlayersNum) {
		LOG(TEXT("Waiting for more players.\nExpected: %d\nActual: %d"), ExpectedPlayersNum, NumPlayers);
		
		return false;
	}

	int PlayersFullyReplicatedAllPlayersStates = 0;
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		AMyPlayerController* MyPlayerController = Cast<AMyPlayerController>(Iterator->Get());
		if (!MyPlayerController)
		{
			// Player controllers might not be swapped for new ones yet, so we wait
			return false;
		}
		MyPlayerController->OnStartLoading.Broadcast();
		if (MyPlayerController->GetPlayerNetworkData().PlayerStatesReplicated == ExpectedPlayersNum)
		{
			PlayersFullyReplicatedAllPlayersStates++;
		}
	}

	LOG_NETWORKING(TEXT("Waiting for clients to replicate all player states. Players ready: %d"), PlayersFullyReplicatedAllPlayersStates);

	// We subtract 1, because the server client already has all players states
	if (PlayersFullyReplicatedAllPlayersStates < ExpectedPlayersNum - 1)
	{
		return false;
	}

	UHelper::GetLocalPlayerController(GetWorld())->LocalControllerInit();

	//****************************** WAIT FOR PLAYER IDS TO REPLICATE ***********************************//
	int PlayersFullyReplicatedAllCharactersIds = 0;
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		AMyPlayerController* MyPlayerController = Cast<AMyPlayerController>(Iterator->Get());
		if (MyPlayerController->GetPlayerNetworkData().CharacterIdsReplicated == ExpectedPlayersNum)
		{
			PlayersFullyReplicatedAllCharactersIds++;
		}
	}

	LOG_NETWORKING(TEXT("Waiting for clients to replicate all characters ids. Players ready: %d"), PlayersFullyReplicatedAllCharactersIds);

	if (PlayersFullyReplicatedAllCharactersIds < ExpectedPlayersNum - 1)
	{
		return false;
	}
	//****************************** END ***********************************//

	//****************************** WAIT FOR PLAYERS SETTINGS TO REPLICATE ***********************************//
	int PlayersExchangedAllSettings = 0;
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		AMyPlayerController* MyPlayerController = Cast<AMyPlayerController>(Iterator->Get());
		if (MyPlayerController->GetPlayerNetworkData().PlayersSettingsReplicated == (ExpectedPlayersNum - 1))
		{
			PlayersExchangedAllSettings++;
		}
	}

	LOG_NETWORKING(TEXT("Waiting for players to exchange their settings. Players ready: %d"), PlayersExchangedAllSettings);

	if (PlayersExchangedAllSettings < ExpectedPlayersNum)
	{
		return false;
	}
	//****************************** END ***********************************//

	
	int PlayersReadyToStartGameLoop = 0;
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		AMyPlayerController* MyPlayerController = Cast<AMyPlayerController>(Iterator->Get());
		if (MyPlayerController->GetPlayerNetworkData().bReadyToStartGameLoop)
		{
			PlayersReadyToStartGameLoop++;
		}
		else
		{
			MyPlayerController->ClientInitializeController();
		}
	}

	if (PlayersReadyToStartGameLoop < ExpectedPlayersNum)
	{
		return false;
	}

	// Clients will get this called in AMyGameState::HandleMatchHasStarted
	UHelper::GetLocalPlayerController(GetWorld())->OnMatchHasStarted();

	return true;
}

void AJoltPhysicsTestGameModeBase::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);
}

void AJoltPhysicsTestGameModeBase::OnPostLogin(AController* NewPlayer)
{
	Super::OnPostLogin(NewPlayer);

}

void AJoltPhysicsTestGameModeBase::GenericPlayerInitialization(AController* C)
{
	Super::GenericPlayerInitialization(C);

}
