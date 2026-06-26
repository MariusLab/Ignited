// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JoltPhysicsTest/Jolt/JoltBodyActor.h"
#include "ConveyorBelt.generated.h"

UCLASS()
class JOLTPHYSICSTEST_API AConveyorBelt : public AJoltBodyActor
{
	GENERATED_BODY()

public:
	virtual void InitializeBody(const FJoltBodyData& JoltBodyData) override;
	
	Vec3 ConveyorVel = Vec3(0.f, 0, 0);
};
