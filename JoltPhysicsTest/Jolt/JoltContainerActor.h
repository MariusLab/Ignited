// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "JoltPhysicsTest/Components/JoltBodyComponent.h"
#include "JoltPhysicsTest/Components/Constraints/JoltConstraintComponent.h"
#include "JoltContainerActor.generated.h"

/** This actor is used only as a container for Jolt bodies/constraints components
 * to be placed in the map and loaded upon game start.
 * Upon load all Jolt components on this actor will get attached to other appropriate actors and this one will get destroyed.
 */
UCLASS()
class JOLTPHYSICSTEST_API AJoltContainerActor : public AActor
{
	GENERATED_BODY()

public:
	AJoltContainerActor();

	// Mirror this container horizontally where X = 100,000 is the middle
	UPROPERTY(EditAnywhere, Category = "Container")
	bool bMirror = false;

	UPROPERTY(EditAnywhere, Category = "Container")
	bool bCompoundShape = false;

	UPROPERTY(EditAnywhere, Category = "Container", Meta = (EditCondition = "bCompoundShape"))
	bool bDestructible = false;

	TArray<UJoltBodyComponent*> GetBodyComponentsSortedBySpawnOrder() const;
	TArray<UJoltConstraintComponent*> GetConstraintComponentsSortedBySpawnOrder() const;
};
