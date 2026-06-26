#pragma once

#include "JoltPhysicsTest/Jolt/JoltSubsystem.h"
#include "PhysicsPropertiesData.generated.h"

UCLASS()
class JOLTPHYSICSTEST_API UPhysicsPropertiesData : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:

	UPROPERTY(EditDefaultsOnly)
	FPhysicsProperties PhysicsProperties;

	UPROPERTY(EditDefaultsOnly, Meta = (DevelopmentOnly))
	bool bForceDebugDraw = false;
};