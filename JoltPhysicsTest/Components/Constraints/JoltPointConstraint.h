// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JoltPhysicsTest/Debug/CustomLogger.h"
#include "JoltConstraintComponent.h"
#include "JoltPhysicsTest/Components/JoltBodyComponent.h"
#include "JoltPointConstraint.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class JOLTPHYSICSTEST_API UJoltPointConstraint : public UJoltConstraintComponent
{
	GENERATED_BODY()

public:
	virtual void InitConstraintComponent() override;

	virtual void RecreateConstraint() override;

#ifdef MY_DEBUG_RENDERING
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
#endif
};
