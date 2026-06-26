// Fill out your copyright notice in the Description page of Project Settings.


#include "MultipleMaterialStaticMeshComponent.h"

void UMultipleMaterialStaticMeshComponent::SwitchToSecondMaterial()
{
	if (!Material2)
	{
		return;
	}
	
	SetMaterial(0, Material2);
	FVector NewRelativeScale = GetRelativeScale3D();
	NewRelativeScale.X *= -1.f;
	//SetRelativeScale3D(NewRelativeScale);
}
