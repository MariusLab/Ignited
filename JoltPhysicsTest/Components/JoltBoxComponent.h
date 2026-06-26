// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JoltPhysicsTest/Debug/CustomLogger.h"
#include "JoltBodyComponent.h"
#include "Components/PrimitiveComponent.h"
#include "JoltPhysicsTest/Data/PhysicsPropertiesData.h"
#include "JoltPhysicsTest/Jolt/JoltEnumsAndStructs.h"
#include "JoltBoxComponent.generated.h"


enum class EObjectMotion : uint8;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class JOLTPHYSICSTEST_API UJoltBoxComponent : public UJoltBodyComponent
{
	GENERATED_BODY()

public:
	UJoltBoxComponent();

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> StaticMeshComp = nullptr;

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
};
