// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JoltBodyComponent.h"
#include "JoltPhysicsTest/Debug/CustomLogger.h"
#include "Components/PrimitiveComponent.h"
#include "JoltPhysicsTest/Data/PhysicsPropertiesData.h"
#include "JoltPhysicsTest/Jolt/JoltEnumsAndStructs.h"
#include "JoltSphereComponent.generated.h"


enum class EObjectMotion : uint8;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class JOLTPHYSICSTEST_API UJoltSphereComponent : public UJoltBodyComponent
{
	GENERATED_BODY()

public:
	UJoltSphereComponent();

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> StaticMeshComp = nullptr;

	UPROPERTY(EditAnywhere, Category = "Shape")
	float SphereRadius;

	UPROPERTY(EditAnywhere, Category = "Shape")
	FColor	SphereColor;

	UPROPERTY(EditAnywhere, Category = "Shape")
	float LineThickness;

	UPROPERTY(EditAnywhere, Category = "Shape", Meta = (EditCondition = "PhysicsPropertiesDataAsset == nullptr"))
	FPhysicsProperties PhysicsProperties;

	UPROPERTY(EditAnywhere, Category = "Shape")
	TObjectPtr<UPhysicsPropertiesData> PhysicsPropertiesDataAsset = nullptr;

	UPROPERTY(EditAnywhere, Category = "Shape")
	bool bAttachStaticMesh = false;

	virtual void InitBody(const FJoltBodyData& JoltBodyData) override;
	
	FPhysicsProperties GetPhysicsProperties() const;
#ifdef MY_DEBUG_RENDERING
	void OverwriteDebugDraw();
#endif

	virtual void OnRender() override;

	virtual TArray<FVector2D> GetAttachedMeshVertices() const override;

#ifdef MY_DEBUG_RENDERING
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
#endif

private:
	float GetSphereMeshScale() const;
};
