#pragma once

#include "JoltPhysicsTest//Debug/CustomLogger.h"

// This is a struct used to carry data from higher level spawner down to the constraint components
// Data that can't be configured beforehand. Such as raycast hitting a specific Body ID and we need it to attach a constraint.
struct FJoltConstraintData
{
	FJoltConstraintData() {};

	FJoltConstraintData(const BodyID& InBodyID1, const BodyID& InBodyID2)
	{
		BodyID1 = InBodyID1;
		BodyID2 = InBodyID2;
	}

	FJoltConstraintData(const BodyID& InBodyID1, const BodyID& InBodyID2, const float& InMinDistance, const float& InMaxDistance)
	{
		BodyID1 = InBodyID1;
		BodyID2 = InBodyID2;
		
		MinDistance = InMinDistance;
		MaxDistance = InMaxDistance;
	}
	
	// DO NOT PASS POINTERS HERE, made that mistake already with Jolt bodies
	// This data has to be valid between bodies getting created/destroyed during rollback
	// So we pass only IDs that we then use to fetch the needed data
	BodyID BodyID1 = BodyID();
	BodyID BodyID2 = BodyID();

	bool bConstrainToImmovableWorld = false;

	// When set, Body2 is attached at this world-space point instead of its center of mass.
	// Jolt converts it to local space at constraint creation, so it follows body rotation correctly.
	bool bHasBody2WorldAttachPoint = false;
	Vec3 Body2WorldAttachPoint = Vec3::sZero();

	float MinDistance = 0.f;
	float MaxDistance = 0.f;

	JPH::uint32 ConstraintPriority = 0;
};