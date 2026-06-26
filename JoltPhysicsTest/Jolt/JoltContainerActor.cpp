// Fill out your copyright notice in the Description page of Project Settings.


#include "JoltContainerActor.h"


AJoltContainerActor::AJoltContainerActor()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
}

TArray<UJoltBodyComponent*> AJoltContainerActor::GetBodyComponentsSortedBySpawnOrder() const
{
	TMap<int, UJoltBodyComponent*> BodyComponentsBySpawnOrder;
	
	TArray<UJoltBodyComponent*> JoltBodyComponents;
	GetComponents(UJoltBodyComponent::StaticClass(), JoltBodyComponents);

	for (auto BodyComponent : JoltBodyComponents)
	{
		check(!BodyComponentsBySpawnOrder.Contains(BodyComponent->GetSpawnOrder()));
		
		BodyComponentsBySpawnOrder.Add(BodyComponent->GetSpawnOrder(), BodyComponent);
	}

	// Process the components in a deterministic order
	TArray<int> ComponentsSpawnOrder;
	BodyComponentsBySpawnOrder.GetKeys(ComponentsSpawnOrder);
	ComponentsSpawnOrder.Sort();

	TArray<UJoltBodyComponent*> SortedComponents;
	for (const int& CurrentOrderId : ComponentsSpawnOrder)
	{
		SortedComponents.Add(BodyComponentsBySpawnOrder[CurrentOrderId]);
	}

	return SortedComponents;
}

TArray<UJoltConstraintComponent*> AJoltContainerActor::GetConstraintComponentsSortedBySpawnOrder() const
{
	TMap<int, UJoltConstraintComponent*> ConstraintComponentsBySpawnOrder;
	
	TArray<UJoltConstraintComponent*> JoltConstraintComponents;
	GetComponents(UJoltConstraintComponent::StaticClass(), JoltConstraintComponents);

	for (auto ConstraintComponent : JoltConstraintComponents)
	{
		check(!ConstraintComponentsBySpawnOrder.Contains(ConstraintComponent->SpawnOrder));
		
		ConstraintComponentsBySpawnOrder.Add(ConstraintComponent->SpawnOrder, ConstraintComponent);
	}

	// Process the components in a deterministic order
	TArray<int> ComponentsSpawnOrder;
	ConstraintComponentsBySpawnOrder.GetKeys(ComponentsSpawnOrder);
	ComponentsSpawnOrder.Sort();

	TArray<UJoltConstraintComponent*> SortedComponents;
	for (const int& CurrentOrderId : ComponentsSpawnOrder)
	{
		SortedComponents.Add(ConstraintComponentsBySpawnOrder[CurrentOrderId]);
	}

	return SortedComponents;
}