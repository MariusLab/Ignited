// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JoltPhysicsTest/Debug/CustomLogger.h"
#include "OptimizedStaticMeshComponent.h"
#include "ParallaxComponent.generated.h"

UCLASS(meta=(BlueprintSpawnableComponent))
class JOLTPHYSICSTEST_API UParallaxComponent : public UOptimizedStaticMeshComponent
{
	GENERATED_BODY()
public:
	// MoveSpeed of 1.f means no parallax effect. It will move the exact same amount as the owner.
	UPROPERTY(EditDefaultsOnly, Category = "ParallaxComponent")
	FVector2D MoveSpeedRelativeToOwner = FVector2D::ZeroVector;

	UPROPERTY(EditDefaultsOnly, Category = "ParallaxComponent")
	bool bConstantMove = false;
	
	// Have this component move constantly in a given direction at this speed per second.
	UPROPERTY(EditDefaultsOnly, Category = "ParallaxComponent", Meta = (EditCondition = "bConstantMove"))
	FVector2D ConstantMoveSpeed = FVector2D::ZeroVector;

	// Reset location relative to owning actor
	UPROPERTY(EditDefaultsOnly, Category = "ParallaxComponent", Meta = (EditCondition = "bConstantMove"))
	FVector2D ResetRelativeLocation = FVector2D::ZeroVector;

	UPROPERTY(EditDefaultsOnly, Category = "ParallaxComponent", Meta = (EditCondition = "bConstantMove"))
	float LocationLessThanXToTriggerReset = -100000.f;

	UPROPERTY(EditDefaultsOnly, Category = "ParallaxComponent", Meta = (EditCondition = "bConstantMove"))
	float LocationMoreThanXToTriggerReset = 100000.f;

	void Init(FVector InOriginLocation);
	void Update(FVector OwnerLocation, const float& InDeltaSeconds);

private:
	FVector CameraOriginLocation = FVector::ZeroVector;
	FVector ParallaxRelativeLocation = FVector::ZeroVector;
	FVector AccumulatedConstantMove = FVector::ZeroVector;
};
