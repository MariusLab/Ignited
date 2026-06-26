// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerSpawn.h"


APlayerSpawn::APlayerSpawn()
{
	PrimaryActorTick.bCanEverTick = false;
}

void APlayerSpawn::Init(const int& InPlayerId)
{
	BelongsToPlayerId = InPlayerId;
	BP_OnInit(InPlayerId);
}


