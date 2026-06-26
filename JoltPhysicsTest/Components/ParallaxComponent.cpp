// Fill out your copyright notice in the Description page of Project Settings.


#include "ParallaxComponent.h"

void UParallaxComponent::Init(FVector InOriginLocation)
{
	CameraOriginLocation = InOriginLocation;
	ParallaxRelativeLocation = GetRelativeLocation();
	const int TranslucencySort = -(ParallaxRelativeLocation.X / 1000.f);
	SetTranslucentSortPriority(TranslucencySort);
}

void UParallaxComponent::Update(FVector OwnerLocation, const float& InDeltaSeconds)
{
	FVector Delta = OwnerLocation - CameraOriginLocation;
	Delta.Y = 0.f;

	FVector NewLocation = Delta * FVector(MoveSpeedRelativeToOwner.X, 0.f, MoveSpeedRelativeToOwner.Y) + CameraOriginLocation;// + DefaultRelativeLocation;
	NewLocation.Y = -ParallaxRelativeLocation.X;
	
	if (bConstantMove)
	{
		
		AccumulatedConstantMove += FVector(ConstantMoveSpeed.X, 0.f, ConstantMoveSpeed.Y) * InDeltaSeconds;

		// Yes, relative location is supposed to use Y instead of X here. Because of stupid reasons how the blueprint was setup...
		const FVector RelativeLocationToOwner = FVector(GetRelativeLocation().Y, 0, GetRelativeLocation().Z);
		if (RelativeLocationToOwner.X > LocationMoreThanXToTriggerReset || RelativeLocationToOwner.X < LocationLessThanXToTriggerReset)
		{
			AccumulatedConstantMove = FVector(ResetRelativeLocation.X, 0, ResetRelativeLocation.Y) - FVector(ParallaxRelativeLocation.Y, 0.f, ParallaxRelativeLocation.Z);
		}
	}
	
	SetWorldLocation(NewLocation + AccumulatedConstantMove + FVector(ParallaxRelativeLocation.Y, 0.f, ParallaxRelativeLocation.Z));
}
