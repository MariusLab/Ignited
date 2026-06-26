// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JoltBodyComponent.h"
#include "JoltPhysicsTest/Jolt/JoltEnumsAndStructs.h"
#include "JoltPhysicsTest/JoltBodies/JoltBodySettings.h"
#include "JoltDestructibleCompoundComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class JOLTPHYSICSTEST_API UJoltDestructibleCompoundComponent : public UJoltBodyComponent
{
	GENERATED_BODY()

public:
	UJoltDestructibleCompoundComponent();

	void DestroyAndSeparateIntoParts(const Vec3& Impulse, const Vec3& ImpactPoint);
	void RollbackDestroy();


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
	void AddDestructibleComponent(UJoltBodyComponent* JoltBodyComponent);
	FPhysicsProperties PhysicsProperties;
	TArray<TJoltBodySettings> JoltBodiesSettings;

	UPROPERTY()
	TArray<UJoltBodyComponent*> DestructibleBodyComponents;
};
