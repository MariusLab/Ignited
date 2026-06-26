// Fill out your copyright notice in the Description page of Project Settings.


#include "DestructibleJoltBodyActor.h"

#include "JoltPhysicsTest/Components/JoltDestructibleCompoundComponent.h"

void ADestructibleJoltBodyActor::DestroyAndSeparateIntoParts(const Vec3& Impulse, const Vec3& ImpactPoint)
{
	check(!bDestroyedIntoSeparateParts);
	
	UJoltDestructibleCompoundComponent* DestructibleComponent = Cast<UJoltDestructibleCompoundComponent>(GetBodyComponent());
	checkf(DestructibleComponent, TEXT("Destructible actor could not find destructible component"));

	DestructibleComponent->DestroyAndSeparateIntoParts(Impulse, ImpactPoint);
	bDestroyedIntoSeparateParts = true;
}

void ADestructibleJoltBodyActor::RollbackDestroy()
{
	check(bDestroyedIntoSeparateParts);
	
	UJoltDestructibleCompoundComponent* DestructibleComponent = Cast<UJoltDestructibleCompoundComponent>(GetBodyComponent());
	checkf(DestructibleComponent, TEXT("Destructible actor could not find destructible component"));

	DestructibleComponent->RollbackDestroy();
}

void ADestructibleJoltBodyActor::Save(StateRecorderImpl& InStream)
{
	Super::Save(InStream);

	InStream.Write(bDestroyedIntoSeparateParts);
}

void ADestructibleJoltBodyActor::Load(StateRecorderImpl& InStream)
{
	Super::Load(InStream);

	bool bNewValue = false;
	InStream.Read(bNewValue);

	// I think we only need to worry about rolling back destruction here, but we don't need to handle going from not destroyed -> destroyed
	if (bDestroyedIntoSeparateParts != bNewValue && !bNewValue)
	{
		RollbackDestroy();
	}

	bDestroyedIntoSeparateParts = bNewValue;
}
