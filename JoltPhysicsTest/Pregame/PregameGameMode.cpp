// Fill out your copyright notice in the Description page of Project Settings.


#include "PregameGameMode.h"

#include "PregamePlayerController.h"

bool APregameGameMode::ReadyToStartMatch_Implementation()
{
	if (bDelayedStart) {
		return false;
	}
	
	if (GetMatchState() != MatchState::WaitingToStart) {
		return false;
	}
	
	if (NumPlayers != 2)
	{
		return false;
	}

	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		APregamePlayerController* PregamePlayerController = Cast<APregamePlayerController>(Iterator->Get());
		if (PregamePlayerController->GetRemoteRole() == ROLE_AutonomousProxy)
		{
			PregamePlayerController->ClientReadyToPlay();
		}
		else
		{
			PregamePlayerController->ServerReadyToPlay();
		}
	}
	
	return true;
}

void APregameGameMode::OnPostLogin(AController* NewPlayer)
{
	Super::OnPostLogin(NewPlayer);

}

