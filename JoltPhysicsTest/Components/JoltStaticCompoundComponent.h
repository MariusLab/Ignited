// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JoltPhysicsTest/Debug/CustomLogger.h"
#include "JoltBodyComponent.h"
#include "Components/PrimitiveComponent.h"
#include "JoltPhysicsTest/JoltBodies/JoltBodySettings.h"
#include "JoltPhysicsTest/Jolt/JoltEnumsAndStructs.h"
#include "JoltStaticCompoundComponent.generated.h"

UCLASS()
class JOLTPHYSICSTEST_API UJoltStaticCompoundComponent : public UJoltBodyComponent
{
	GENERATED_BODY()

public:
	UJoltStaticCompoundComponent();

	void AddJoltBodyComponent(UJoltBodyComponent* JoltBodyComponent);
	void CopyRootBodyComponentProperties(UJoltBodyComponent* JoltBodyComponent);

	virtual void InitBody(const FJoltBodyData& JoltBodyData) override;

	virtual void OnRender() override;

	const TArray<TJoltBodySettings>& GetJoltBodiesSettings() const;

#ifdef MY_DEBUG_RENDERING
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
#endif

private:
	FPhysicsProperties PhysicsProperties;
	TArray<TJoltBodySettings> JoltBodiesSettings;
};
