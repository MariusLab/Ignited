#pragma once

#include "SpringSettingsData.generated.h"

UCLASS()
class JOLTPHYSICSTEST_API USpringSettingsData : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly)
	float Damping = 1.f;

	/** Frequency = 0, means the spring is disabled. Can only specify Frequency OR Stiffness. But not both. */
	UPROPERTY(EditDefaultsOnly)
	float Frequency = 0.f;

	/** Can only specify Frequency OR Stiffness. But not both */
	UPROPERTY(EditDefaultsOnly)
	float Stiffness = 0.f;
};