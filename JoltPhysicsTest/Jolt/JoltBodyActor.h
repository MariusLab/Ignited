// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JoltBodyDataStruct.h"
#include "JoltBodyNameEnum.h"
#include "GameFramework/Actor.h"
#include "JoltPhysicsTest/Components/JoltBodyComponent.h"
#include "JoltBodyActor.generated.h"

UCLASS()
class JOLTPHYSICSTEST_API AJoltBodyActor : public AActor
{
	GENERATED_BODY()

public:
	AJoltBodyActor();

	virtual void StepSimulation();

	TArray<FVector2D> GetAttachedMeshVertices();

	virtual void InitializeBody(const FJoltBodyData& JoltBodyData);

	/** Re-adds previously removed body. The actor must already be initialized */
	void AddBody();

	/** Only removes bodies from physics simulation, but does not destroy them */
	void RemoveBody();

	UFUNCTION(BlueprintImplementableEvent)
	void BP_OnInitBody();

	UFUNCTION(BlueprintImplementableEvent)
	void BP_OnRemoveFromSimulation();

	UFUNCTION(BlueprintImplementableEvent)
	void BP_OnAddToSimulation();

	virtual void DestroyBody();

	/** Used during rollback */
	void RemoveAndDestroyBody();

	bool IsInitialized() const;
	void DestroyActor();

	BodyID GetBodyID();

	void RecordActorLocation();
	FVector GetRecordedActorLocation() const;

	void EnableUpdateLocationAndRotationFromPhysics();
	void DisableUpdateLocationAndRotationFromPhysics();

	virtual void Save(StateRecorderImpl& InStream);

	virtual void Load(StateRecorderImpl& InStream);

	bool ShouldSkipTweening();

	bool bIsPlayer = false;
	int BelongsToPlayerId = -1;

	UPROPERTY(EditDefaultsOnly, Category = "Body")
	EBodyName BodyName = EBodyName::None;

	UPROPERTY(EditAnywhere, Category = "Config")
	bool bMirror = false;

	// Used only for visual tweening when level loads in.
	FVector OffsetLocation = FVector::ZeroVector;
	
	// Each actor has some random offset to make all platforms come in at slightly different times.
	float RandomTweeningOffset = 0.f;
protected:
	UJoltBodyComponent* GetBodyComponent();
private:
	bool bInitialized = false;
	uint32 BodyIndex = 0;

	UPROPERTY()
	TObjectPtr<UJoltBodyComponent> CachedBodyComponent = nullptr;

	FVector RecordedActorLocation = FVector::ZeroVector;
};
