// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JoltPhysicsTest/Debug/CustomLogger.h"
#include "JoltBodyComponent.h"
#include "Components/PrimitiveComponent.h"
#include "JoltPhysicsTest/Data/PhysicsPropertiesData.h"
#include "JoltPhysicsTest/Jolt/JoltEnumsAndStructs.h"
#include "JoltCapsuleComponent.generated.h"


enum class EObjectMotion : uint8;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class JOLTPHYSICSTEST_API UJoltCapsuleComponent : public UJoltBodyComponent
{
	GENERATED_BODY()

public:
	UJoltCapsuleComponent();
	
	UPROPERTY(EditAnywhere, Category = "Shape")
	float CapsuleRadius;

	UPROPERTY(EditAnywhere, Category = "Shape")
	float CapsuleHalfHeight;

	UPROPERTY(EditAnywhere, Category = "Shape")
	FColor	CapsuleColor;

	UPROPERTY(EditAnywhere, Category = "Shape")
	float LineThickness;

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

#ifdef MY_DEBUG_RENDERING
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
#endif
};
