// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JoltPhysicsTest/Debug/CustomLogger.h"
#include "JoltBodyComponent.h"
#include "Components/PrimitiveComponent.h"
#include "JoltPhysicsTest/Data/PhysicsPropertiesData.h"
#include "JoltPhysicsTest/Jolt/JoltEnumsAndStructs.h"
#include "JoltConvexHullComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class JOLTPHYSICSTEST_API UJoltConvexHullComponent : public UJoltBodyComponent
{
	GENERATED_BODY()

public:
	UJoltConvexHullComponent();

	UPROPERTY(EditAnywhere, Category = "Shape")
	TArray<FVector2D> Points;

	UPROPERTY(EditAnywhere, Category = "Shape", Meta = (EditCondition = "PhysicsPropertiesDataAsset == nullptr"))
	FPhysicsProperties PhysicsProperties;

	UPROPERTY(EditAnywhere, Category = "Shape")
	TObjectPtr<UPhysicsPropertiesData> PhysicsPropertiesDataAsset = nullptr;

	virtual void InitBody(const FJoltBodyData& JoltBodyData) override;

	FPhysicsProperties GetPhysicsProperties() const;
#ifdef MY_DEBUG_RENDERING
	void OverwriteDebugDraw();
#endif

	virtual void OnRender() override;

	TArray<FVector> GetConvexHullPointsForJolt() const;
#ifdef MY_DEBUG_RENDERING
	TArray<FVector> GetConvexHullPointsForDebugRendering() const;
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
#endif
};
