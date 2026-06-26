// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PlayerBodyVFX.generated.h"

UCLASS()
class JOLTPHYSICSTEST_API APlayerBodyVFX : public AActor
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "PlayerBodyVFX")
	TObjectPtr<UMaterialInstanceDynamic> PlayerHeadDynamicMaterial = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "PlayerBodyVFX")
	TObjectPtr<UMaterialInstanceDynamic> PlayerEyesDynamicMaterial = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "PlayerBodyVFX")
	TObjectPtr<UMaterialInstanceDynamic> PlayerBodyDynamicMaterial = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "PlayerBodyVFX")
	TObjectPtr<UMaterialInstanceDynamic> HammerDynamicMaterial = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "PlayerBodyVFX")
	TObjectPtr<UMaterialInstanceDynamic> StickDynamicMaterial = nullptr;

	UFUNCTION(BlueprintImplementableEvent)
	void FinishEarly();
};
