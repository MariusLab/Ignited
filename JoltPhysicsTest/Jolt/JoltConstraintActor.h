// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "JoltPhysicsTest/Debug/CustomLogger.h"
#include "CoreMinimal.h"
#include "JoltConstraintDataStruct.h"
#include "JoltConstraintTypeEnum.h"
#include "GameFramework/Actor.h"
#include "JoltPhysicsTest/Components/Constraints/JoltConstraintComponent.h"
#include "JoltConstraintActor.generated.h"

UCLASS()
class JOLTPHYSICSTEST_API AJoltConstraintActor : public AActor
{
	GENERATED_BODY()

public:
	AJoltConstraintActor();

	UPROPERTY(EditAnywhere)
	EJoltConstraintType JoltConstraintType = EJoltConstraintType::None;

	void StepSimulation();

	void InitializeConstraint(const FJoltConstraintData& ConstraintData);

	/** Re-adds previously removed constraint. The actor must already be initialized */
	void RecreateConstraint();

	void DestroyConstraint();

	void EnableConstraint();

	void DisableConstraint();

	bool IsInitialized() const;
	
	void DestroyActor();

	void Save(StateRecorderImpl& InStream);

	void Load(StateRecorderImpl& InStream);

	template <typename T>
	T* GetConstraintInstance() { return static_cast<T*>(GetConstraintComponent()->GetConstraintInstance()); }

	FJoltConstraintData GetConstraintData();

private:
	bool bInitialized = false;
	uint32 BodyIndex = 0;

	UJoltConstraintComponent* GetConstraintComponent();

	UPROPERTY()
	TObjectPtr<UJoltConstraintComponent> CachedConstraintComponent = nullptr;
};
