// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OptimizedStaticMeshComponent.h"
#include "AnimationComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAnimationFinished);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAnimationFrameRendered, int, AnimationFrameId);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class JOLTPHYSICSTEST_API UAnimationComponent : public UOptimizedStaticMeshComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnAnimationFinished OnAnimationFinished;

	UPROPERTY(BlueprintAssignable)
	FOnAnimationFrameRendered OnAnimationFrameRendered;
	
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	int MaxFrames = 6;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	int FramesPerSecond = 12;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	bool bUpdateAnimationManually = false;

	UFUNCTION(BlueprintCallable, Category = "Animation")
	void SetNewMaterial(UMaterialInterface* NewMaterial);

	virtual void Update(const int& SimulationFrameId);

	virtual void BeginPlay() override;

	UFUNCTION()
	void OnRender();
protected:
	int StartedOnSimulationFrameId = -1;
	int FrameNum = 0;
	
	class UMaterialInstanceDynamic* GetDynamicMaterial();

	UPROPERTY()
	class UMaterialInstanceDynamic* MaterialInstanceDynamic = nullptr;
};
