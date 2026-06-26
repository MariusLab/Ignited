// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JoltBodyComponent.h"
#include "Components/PrimitiveComponent.h"
#include "JoltPhysicsTest/Data/PhysicsPropertiesData.h"
#include "JoltPhysicsTest/Jolt/JoltEnumsAndStructs.h"
#include "JoltConveyorBeltComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class JOLTPHYSICSTEST_API UJoltConveyorBeltComponent : public UJoltBodyComponent
{
	GENERATED_BODY()

public:
	UJoltConveyorBeltComponent();

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UStaticMeshComponent> StaticMeshComp = nullptr;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Conveyor")
	float ConveyorVelocity = 100.f;
	
	FVector ConveyorVelocityVector = FVector(1.f, 0, 0);

	UPROPERTY(EditAnywhere, Category = "Shape")
	FVector	BoxExtent;

	UPROPERTY(EditAnywhere, Category = "Shape")
	FColor BoxColor;

	UPROPERTY(EditAnywhere, Category = "Shape")
	float LineThickness;

	UPROPERTY(EditAnywhere, Category = "Shape", Meta = (EditCondition = "PhysicsPropertiesDataAsset == nullptr"))
	FPhysicsProperties PhysicsProperties;

	UPROPERTY(EditAnywhere, Category = "Shape")
	TObjectPtr<UPhysicsPropertiesData> PhysicsPropertiesDataAsset = nullptr;

	UPROPERTY(EditAnywhere, Category = "Shape")
	bool bAttachStaticMesh = false;

	UPROPERTY(EditAnywhere, Category = "Shape", Meta = (EditCondition = "bAttachStaticMesh"))
	bool bAttachedStaticMeshCastShadow = false;

	// Optional material override. If not specified, the default platform material will be used
	UPROPERTY(EditAnywhere, Category = "Shape", Meta = (EditCondition = "bAttachStaticMesh"))
	TObjectPtr<UMaterialInterface> AttachedMeshMaterial = nullptr;

	virtual void InitBody(const FJoltBodyData& JoltBodyData) override;

	void ReverseConveyorVelocity();

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

	virtual UClass* GetJoltBodyClass() const override;
};
