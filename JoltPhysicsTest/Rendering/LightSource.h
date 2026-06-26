// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "GameFramework/Actor.h"
#include "LightSource.generated.h"

UCLASS()
class JOLTPHYSICSTEST_API ALightSource : public AActor
{
	GENERATED_BODY()

public:
	ALightSource();
	
	void UpdateMesh(const TArray<FVector>& Locations);

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UProceduralMeshComponent> ProceduralMeshComponent = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Material")
	TObjectPtr<UMaterialInterface> LightMaterial = nullptr;

	/** Determines how far rays will travel */
	UPROPERTY(EditAnywhere, Category = "Light")
	float BackgroundSize = 2000.f;
	
protected:
	virtual void BeginPlay() override;
};
