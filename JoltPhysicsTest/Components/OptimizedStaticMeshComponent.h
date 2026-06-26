// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "OptimizedStaticMeshComponent.generated.h"

UCLASS(meta=(BlueprintSpawnableComponent))
class JOLTPHYSICSTEST_API UOptimizedStaticMeshComponent : public UStaticMeshComponent
{
	GENERATED_BODY()
public:
	UOptimizedStaticMeshComponent();
};
