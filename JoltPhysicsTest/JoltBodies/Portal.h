// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JoltPhysicsTest/Jolt/JoltBodyActor.h"
#include "Portal.generated.h"

UCLASS()
class JOLTPHYSICSTEST_API APortal : public AJoltBodyActor
{
	GENERATED_BODY()

public:
	APortal();

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<class USceneComponent> RootScene = nullptr;
	
	UPROPERTY(EditAnywhere, Category = "Portal")
	FVector TeleportDestination = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, Category = "Portal")
	int CooldownFrames = 60;

	UPROPERTY(EditDefaultsOnly, Category = "Portal")
	bool bReverseVelocityOnTeleport = false;

	UPROPERTY()
	APortal* DestinationPortal = nullptr;

	bool CanTeleportBody(int32 InBodyIndex, int32 CurrentFrame) const;
	void RecordTeleport(int32 InBodyIndex, int32 CurrentFrame);
	void BodyEntered(const BodyID& InBodyID);

	virtual void Save(StateRecorderImpl& InStream) override;
	virtual void Load(StateRecorderImpl& InStream) override;

	virtual void OnConstruction(const FTransform& Transform) override;

private:
	TMap<int32, int32> BodyLastTeleportFrame;
#if WITH_EDITORONLY_DATA
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<class USphereComponent> DestinationMarker;
#endif
};
