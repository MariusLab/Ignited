#pragma once

#include "ConstraintStructs.h"
#include "ConstraintData.generated.h"

UCLASS()
class JOLTPHYSICSTEST_API UHingeConstraintSettingsData : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:

	UPROPERTY(EditDefaultsOnly)
	FHingeConstraintSettings HingeConstraintSettings;
};