// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ConstraintStructs.h"
#include "JoltConstraintComponent.h"
#include "JoltSliderConstraint.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class JOLTPHYSICSTEST_API UJoltSliderConstraint : public UJoltConstraintComponent
{
	GENERATED_BODY()
public:
	virtual void StepSimulation() override;
	
	virtual void InitConstraintComponent() override;

	virtual void RecreateConstraint() override;

	virtual void SaveConstraint(StateRecorderImpl& InStream) override;
	virtual void LoadConstraint(StateRecorderImpl& InStream) override;

	UPROPERTY(EditAnywhere, Category = "Constraint")
	FSliderConstraintSettings ConstraintSettings;

	UPROPERTY(EditAnywhere, Category = "Constraint")
	bool bSwitchDirectionAutomatically = false;
	
	/** Only applies when motor is set to "Position" */
	UPROPERTY(EditAnywhere, Category = "Constraint", Meta = (EditCondition = "bSwitchDirectionAutomatically"))
	int SwitchDirectionEveryXFrames = 60;
	
	SliderConstraint* SliderConstraintInstance = nullptr;

#ifdef MY_DEBUG_RENDERING
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
#endif
private:
	int FramesMovedInSameDirection = 0;
	// Used when switching directions for moving platforms.
	float NextTargetPosition = 0.f;
};
