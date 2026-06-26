// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "JoltPhysicsTest/Debug/CustomLogger.h"
#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SplineComponent.h"
#include "JoltPhysicsTest/Components/JoltBodyComponent.h"
#include "JoltPhysicsTest/Jolt/JoltConstraintDataStruct.h"
#include "JoltConstraintComponent.generated.h"

UCLASS(Abstract)
class JOLTPHYSICSTEST_API UJoltConstraintComponent : public USplineComponent
{
	GENERATED_BODY()

public:
	UJoltConstraintComponent();

	/** This is used for constraints loaded together with the map. It needs to set BodyIds differently than at runtime */
	virtual void PreInitialize(TArray<UJoltBodyComponent*> BodyComponents);

	virtual void StepSimulation();
	
	void InitConstraint(const FJoltConstraintData& InConstraintData);

	virtual void InitConstraintComponent();

	virtual void SaveConstraint(StateRecorderImpl& InStream);
	virtual void LoadConstraint(StateRecorderImpl& InStream);

	virtual void RecreateConstraint();
	
	virtual void DestroyConstraint();

	void EnableConstraint();

	void DisableConstraint();
	
	bool ShouldDrawPrimitive() const;

	void Save(StateRecorderImpl& InStream);
	void Load(StateRecorderImpl& InStream);

	bool IsConstraintInitialized() const;

	Constraint* GetConstraintInstance();
	
	FJoltConstraintData GetConstraintData() const;

	/** This controls the order in which all the constraints will be spawned. Very important to keep them deterministic for all players */
	UPROPERTY(EditAnywhere, Category = "Constraint")
	int SpawnOrder = 0;

	/** Will disable collision between the constrained bodies */
	UPROPERTY(EditAnywhere, Category = "Constraint")
	bool bDisableCollisionBetweenBodies = false;

	/** Allows to disable the constraint without deleting it. It will simply never initialize */
	UPROPERTY(EditAnywhere, Category = "Constraint")
	bool bDisabled = false;

	/** Should this constraint draw itself in debug and development builds  */
	UPROPERTY(EditAnywhere, Category = "Constraint")
	bool bDebugDraw = false;

	UPROPERTY(EditAnywhere, Category = "Constraint")
	bool bConstrainToImmovableWorld = false;

	bool IsConstraintDestroyed() const;

	/** This is a bit of a hack to find the right component, because Unreal editor doesn't allow to select an instance of a component directly, only that of an actor */
	
	// The exact spawn order for the body component we want to target
	UPROPERTY(EditAnywhere, Category = "Constraint|Shape1", Meta = (EditCondition = "!bConstrainToImmovableWorld"))
	int SpawnOrderOfBody1 = -1;

	// The exact spawn order for the body component we want to target
	UPROPERTY(EditAnywhere, Category = "Constraint|Shape2")
	int SpawnOrderOfBody2 = -1;

#ifdef MY_DEBUG_RENDERING
	/** Prevent from getting frustum culled too early */
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
#endif

protected:
	void UpdateCollisionBetweenBodies(Body* Body1, Body* Body2);
	Constraint* JoltConstraint = nullptr;
	FJoltConstraintData ConstraintData;

	StateRecorderImpl ConstraintSettingsRecorded;

	UPROPERTY()
	UJoltBodyComponent* BodyComponent1 = nullptr;
private:
	bool bConstraintInitialized = false;
	bool bDestroyed = false;
};
