// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OptimizedStaticMeshComponent.h"
#include "MultipleMaterialStaticMeshComponent.generated.h"

UCLASS(meta=(BlueprintSpawnableComponent))
class JOLTPHYSICSTEST_API UMultipleMaterialStaticMeshComponent : public UOptimizedStaticMeshComponent
{
	GENERATED_BODY()
public:

	UPROPERTY(EditAnywhere, Category = "Materials")
	TObjectPtr<UMaterialInterface> Material2 = nullptr;

	void SwitchToSecondMaterial();
};
