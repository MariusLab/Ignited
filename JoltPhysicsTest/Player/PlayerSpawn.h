// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PlayerSpawn.generated.h"

UCLASS()
class JOLTPHYSICSTEST_API APlayerSpawn : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	APlayerSpawn();

	void Init(const int& InPlayerId);

	UFUNCTION(BlueprintImplementableEvent, Category = "Spawn")
	void BP_OnInit(int PlayerId);

	/** Determines in what order the spawn points will be used. Lower ones go first */
	UPROPERTY(EditAnywhere, Category = "Spawn")
	int SpawnIndex = 0;

	UPROPERTY(EditAnywhere, Category = "Spawn")
	bool bMirror = false;

	UPROPERTY(BlueprintReadOnly, Category = "Spawn")
	int BelongsToPlayerId = -1;
};
