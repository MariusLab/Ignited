// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "JoltPhysicsTest/Debug/CustomLogger.h"
#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"
#include "JoltPhysicsTest/Jolt/JoltBodyNameEnum.h"
#include "JoltPhysicsTest/Jolt/JoltBodyDataStruct.h"
#include "JoltBodyComponent.generated.h"

UCLASS(Abstract)
class JOLTPHYSICSTEST_API UJoltBodyComponent : public UPrimitiveComponent
{
	GENERATED_BODY()

public:
	UJoltBodyComponent();
	
	virtual void OnRegister() override;

	UFUNCTION()
	virtual void OnRender();
	
	virtual void InitBody(const FJoltBodyData& JoltBodyData);

	virtual void AddBody();
	
	/** Only removes this physics body from the simulation, BUT DOES NOT DESTROY IT. Which keeps its BodyID intact and reserved */
	virtual void RemoveBody();
	
	virtual void DestroyBody();
	
	virtual void RemoveAndDestroyBody();

	bool ShouldDrawPrimitive() const;

	BodyID GetBodyID();

	bool IsBodyDestroyed() const;

	void StepSimulation();

	int GetSpawnOrder() const;

	// Return the points in 2D space that represent the corners of this shape and should be used for calculating where shadows are
	virtual TArray<FVector2D> GetAttachedMeshVertices() const;

	/** This controls the order in which body components will be spawned. Very important to keep them deterministic for all players */
	UPROPERTY(EditAnywhere, Category = "Shape")
	int SpawnOrder = -1;

	/** This gets generated automatically and will be used instead of SpawnOrder, if SpawnOrder == -1 */
	UPROPERTY(VisibleAnywhere, Category = "Shape")
	int GeneratedSpawnOrder = 0;

	/** Rotate body to match its velocity direction */
	UPROPERTY(EditAnywhere, Category = "Shape")
	bool bOrientTowardsVelocity = false;

	/** Should this body draw its shape in debug and development builds  */
	UPROPERTY(EditAnywhere, Category = "Shape")
	bool bDebugDraw = false;

	UPROPERTY(EditAnywhere, Category = "Shape")
	EBodyName BodyName = EBodyName::None;

	UPROPERTY(EditAnywhere, Category = "Shape")
	TSubclassOf<class AJoltBodyActor> JoltBodyClass;

	/** Manually define the shape vertices that will be used for shadows */
	UPROPERTY(EditAnywhere, Category = "Shadows")
	TArray<FVector2D> ShapeVertices;

	/** Should this body be skipped from lerping into position when loading the level */
	UPROPERTY(EditAnywhere, Category = "Visual")
	bool bNoTweening = false;

	virtual UClass* GetJoltBodyClass() const;

	bool bUpdateComponentPositionFromJoltPhysics = true;

	// If you want to create a body as static and later set it to dynamic, this needs to be set to TRUE
	// So that Jolt can internally creat mMotionProperties for a static body in preparation for this switch
	bool bAllowDynamicOrKinematic = false;
protected:
	BodyID JoltBodyID;
	FVector CenterOfMassLocation = FVector::ZeroVector;
	bool bBodyInitialized = false;
	bool bRemovedFromSimulation = false;
	bool bDestroyed = false;

	// Needed to know for rendering when bRemovedFromSimulation changed value from last render frame
	bool bRemovedFromSimulationLastRenderFrameValue = true;

	FVector2D LocalPosToWorld(const float& PosX, const float& PosY) const;
	TArray<FVector2D> LocalPositionsToWorld(const TArray<FVector2D>& Positions) const;
};
