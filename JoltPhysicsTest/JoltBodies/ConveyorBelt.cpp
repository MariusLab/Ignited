// Fill out your copyright notice in the Description page of Project Settings.


#include "ConveyorBelt.h"

#include "JoltPhysicsTest/Components/JoltConveyorBeltComponent.h"
#include "JoltPhysicsTest/Jolt/JoltConverter.h"


void AConveyorBelt::InitializeBody(const FJoltBodyData& JoltBodyData)
{
	Super::InitializeBody(JoltBodyData);

	auto* ConveyorComp = Cast<UJoltConveyorBeltComponent>(GetBodyComponent());
	ConveyorVel = -Units::ToJoltPos(ConveyorComp->ConveyorVelocityVector);

	if (GetActorScale3D().X < 0.f)
	{
		ConveyorVel = Vec3(-ConveyorVel.GetX(), ConveyorVel.GetY(), ConveyorVel.GetZ());
		ConveyorComp->ReverseConveyorVelocity();
	}
}
