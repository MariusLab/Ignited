// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PregamePlayerController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAllPlayersConnected);

UCLASS()
class JOLTPHYSICSTEST_API APregamePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnAllPlayersConnected OnAllPlayersConnected;
	
	UFUNCTION(BlueprintImplementableEvent)
	void BP_OnServerReadyToPlay();

	UFUNCTION(BlueprintImplementableEvent)
	void BP_OnClientReadyToPlay();

	void ServerReadyToPlay();

	UFUNCTION(Client, Reliable)
	void ClientReadyToPlay();
};
