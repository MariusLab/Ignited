// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Actor.h"
#include "GameCameraLimits.generated.h"

UCLASS()
class JOLTPHYSICSTEST_API AGameCameraLimits : public AActor
{
	GENERATED_BODY()

public:
	AGameCameraLimits();

	UPROPERTY(EditAnywhere, Category = "GameCameraLimits")
	TObjectPtr<UBoxComponent> BoxComponent;

	UPROPERTY(EditAnywhere, Category = "Config")
	bool bUseCustomOrthoLimits = false;
	
	UPROPERTY(EditAnywhere, Category = "Config", Meta = (EditCondition = "bUseCustomOrthoLimits"))
	float MinOrthoWidth = 80000.f;
	
	UPROPERTY(EditAnywhere, Category = "Config", Meta = (EditCondition = "bUseCustomOrthoLimits"))
	float MaxOrthoWidth = 100000.f;

	UPROPERTY(EditAnywhere, Category = "Config")
	bool bStaticCamera = false;
};
