// Fill out your copyright notice in the Description page of Project Settings.


#include "JoltConstraintActor.h"

AJoltConstraintActor::AJoltConstraintActor()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
}

void AJoltConstraintActor::StepSimulation()
{
	check(bInitialized);
	GetConstraintComponent()->StepSimulation();
}

void AJoltConstraintActor::InitializeConstraint(const FJoltConstraintData& ConstraintData)
{
	GetConstraintComponent()->InitConstraint(ConstraintData);

	bInitialized = GetConstraintComponent()->IsConstraintInitialized();
}

void AJoltConstraintActor::RecreateConstraint()
{
	GetConstraintComponent()->RecreateConstraint();
}

void AJoltConstraintActor::DestroyConstraint()
{
	GetConstraintComponent()->DestroyConstraint();
}

void AJoltConstraintActor::EnableConstraint()
{
	GetConstraintComponent()->EnableConstraint();
}

void AJoltConstraintActor::DisableConstraint()
{
	GetConstraintComponent()->DisableConstraint();
}

bool AJoltConstraintActor::IsInitialized() const
{
	return bInitialized;
}

void AJoltConstraintActor::DestroyActor()
{
	check(GetConstraintComponent()->IsConstraintDestroyed());

	Destroy();
}

void AJoltConstraintActor::Save(StateRecorderImpl& InStream)
{
	GetConstraintComponent()->Save(InStream);
}

void AJoltConstraintActor::Load(StateRecorderImpl& InStream)
{
	GetConstraintComponent()->Load(InStream);
}

FJoltConstraintData AJoltConstraintActor::GetConstraintData()
{
	return GetConstraintComponent()->GetConstraintData();
}

UJoltConstraintComponent* AJoltConstraintActor::GetConstraintComponent()
{
	if (CachedConstraintComponent)
	{
		return CachedConstraintComponent;
	}
	
	TArray<UJoltConstraintComponent*> JoltConstraintComponents;
	GetComponents(UJoltConstraintComponent::StaticClass(), JoltConstraintComponents);
	check(JoltConstraintComponents.Num() == 1);

	CachedConstraintComponent = JoltConstraintComponents.Pop(false);

	return CachedConstraintComponent;
}
