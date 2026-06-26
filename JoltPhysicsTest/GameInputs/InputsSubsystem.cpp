// Fill out your copyright notice in the Description page of Project Settings.


#include "InputsSubsystem.h"

#include "JoltPhysicsTest/Debug/CustomLogger.h"

bool UInputsSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (!Super::ShouldCreateSubsystem(Outer))
	{
		return false;
	}

	const TArray AllowedNetModes{NM_ListenServer, NM_Client, NM_Standalone};	

	const UWorld* World = Cast<UWorld>(Outer);
	if (World == nullptr)
	{
		return false;
	}

	if (World->WorldType != EWorldType::Game && World->WorldType != EWorldType::PIE)
	{
		return false;
	}

	return AllowedNetModes.Contains(World->GetNetMode());
}

void UInputsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	GameInputs = NewObject<UGameInputs>(GetWorld());
	LOG(TEXT("[UInputsSubsystem] Created GameInputs instance"));
}
