// Fill out your copyright notice in the Description page of Project Settings.


#include "PregamePlayerController.h"

void APregamePlayerController::ServerReadyToPlay()
{
	checkf(HasAuthority(), TEXT("Server function called on client"));

	BP_OnServerReadyToPlay();
	OnAllPlayersConnected.Broadcast();
}

void APregamePlayerController::ClientReadyToPlay_Implementation()
{
	BP_OnClientReadyToPlay();
	OnAllPlayersConnected.Broadcast();
}


