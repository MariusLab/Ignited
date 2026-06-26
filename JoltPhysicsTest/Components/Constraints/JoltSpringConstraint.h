// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ConstraintStructs.h"
#include "JoltPhysicsTest/Debug/CustomLogger.h"
#include "JoltConstraintComponent.h"
#include "JoltPhysicsTest/Components/JoltBodyComponent.h"
#include "JoltSpringConstraint.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class JOLTPHYSICSTEST_API UJoltSpringConstraint : public UJoltConstraintComponent
{
	GENERATED_BODY()

public:
	UJoltSpringConstraint();
	
	virtual void InitConstraintComponent() override;

	virtual void RecreateConstraint() override;

	virtual void DestroyConstraint() override;

	UPROPERTY(EditAnywhere, Category = "Constraint")
	FDistanceConstraintSettings ConstraintSettings;

	UPROPERTY(EditAnywhere, Category = "Constraint")
	bool bAttachStaticMesh = false;

	UPROPERTY(EditAnywhere, Category = "Constraint", Meta = (EditCondition = "bAttachStaticMesh"))
	TObjectPtr<UMaterialInterface> CustomRopeMaterial = nullptr;
	
	DistanceConstraint* DistanceConstraintInstance = nullptr;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

#ifdef MY_DEBUG_RENDERING
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
#endif

private:
	UPROPERTY()
	UStaticMeshComponent* AttachedMesh = nullptr;
};
