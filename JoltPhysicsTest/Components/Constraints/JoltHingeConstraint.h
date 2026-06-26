// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ConstraintData.h"
#include "ConstraintStructs.h"
#include "JoltConstraintComponent.h"
#include "JoltHingeConstraint.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class JOLTPHYSICSTEST_API UJoltHingeConstraint : public UJoltConstraintComponent
{
	GENERATED_BODY()
public:
	virtual void InitConstraintComponent() override;

	virtual void RecreateConstraint() override;

	UPROPERTY(EditAnywhere, Category = "Constraint", Meta = (EditCondition = "ConstraintSettingsData == nullptr"))
	FHingeConstraintSettings ConstraintSettings;

	UPROPERTY(EditAnywhere, Category = "Constraint")
	TObjectPtr<UHingeConstraintSettingsData> ConstraintSettingsData = nullptr;
	
	HingeConstraint* HingeConstraintInstance = nullptr;

#ifdef MY_DEBUG_RENDERING
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
#endif
};
