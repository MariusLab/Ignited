// Fill out your copyright notice in the Description page of Project Settings.


#include "JoltBodyActor.h"

AJoltBodyActor::AJoltBodyActor()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
}

void AJoltBodyActor::StepSimulation()
{
	check(bInitialized);
	GetBodyComponent()->StepSimulation();
}

TArray<FVector2D> AJoltBodyActor::GetAttachedMeshVertices()
{
	check(bInitialized);
	return GetBodyComponent()->GetAttachedMeshVertices();
}

void AJoltBodyActor::InitializeBody(const FJoltBodyData& JoltBodyData)
{
	GetBodyComponent()->InitBody(JoltBodyData);

	bInitialized = true;
	BP_OnInitBody();
}

void AJoltBodyActor::AddBody()
{
	GetBodyComponent()->AddBody();
	BP_OnAddToSimulation();
}

void AJoltBodyActor::RemoveBody()
{
	GetBodyComponent()->RemoveBody();
	BP_OnRemoveFromSimulation();
}

void AJoltBodyActor::DestroyBody()
{
	GetBodyComponent()->DestroyBody();

	bInitialized = false;
}

void AJoltBodyActor::RemoveAndDestroyBody()
{
	GetBodyComponent()->RemoveAndDestroyBody();

	bInitialized = false;
}

bool AJoltBodyActor::IsInitialized() const
{
	return bInitialized;
}

void AJoltBodyActor::DestroyActor()
{
	check(GetBodyComponent()->IsBodyDestroyed());

	Destroy();
}

BodyID AJoltBodyActor::GetBodyID()
{
	return GetBodyComponent()->GetBodyID();
}

void AJoltBodyActor::Save(StateRecorderImpl& InStream)
{
}

void AJoltBodyActor::Load(StateRecorderImpl& InStream)
{
}

bool AJoltBodyActor::ShouldSkipTweening()
{
	return GetBodyComponent()->bNoTweening;
}


void AJoltBodyActor::RecordActorLocation()
{
	RecordedActorLocation = GetActorLocation();
}

FVector AJoltBodyActor::GetRecordedActorLocation() const
{
	return RecordedActorLocation;
}

void AJoltBodyActor::EnableUpdateLocationAndRotationFromPhysics()
{
	GetBodyComponent()->bUpdateComponentPositionFromJoltPhysics = true;
}

void AJoltBodyActor::DisableUpdateLocationAndRotationFromPhysics()
{
	GetBodyComponent()->bUpdateComponentPositionFromJoltPhysics = false;
}

UJoltBodyComponent* AJoltBodyActor::GetBodyComponent()
{
	if (CachedBodyComponent)
	{
		return CachedBodyComponent;
	}

	TArray<UJoltBodyComponent*> JoltBodyComponents;
	GetComponents(UJoltBodyComponent::StaticClass(), JoltBodyComponents);
	check(JoltBodyComponents.Num() == 1);

	CachedBodyComponent = JoltBodyComponents.Pop(false);

	return CachedBodyComponent;
}
