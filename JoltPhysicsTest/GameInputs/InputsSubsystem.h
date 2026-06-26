// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameInputs.h"
#include "Subsystems/WorldSubsystem.h"
#include "InputsSubsystem.generated.h"

UCLASS()
class JOLTPHYSICSTEST_API UInputsSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UPROPERTY()
	TObjectPtr<UGameInputs> GameInputs = nullptr;
};
