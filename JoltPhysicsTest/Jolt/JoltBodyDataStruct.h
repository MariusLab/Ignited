#pragma once

#include "JoltPhysicsTest//Debug/CustomLogger.h"

// This is a struct used to carry data from higher level spawner down to the Body components
// Such as the assigned body index/id.
struct FJoltBodyData
{
	FJoltBodyData() {};

	FJoltBodyData(const int& InBodyIndex)
	{
		BodyIndex = InBodyIndex;
	}
	
	JPH::uint32 BodyIndex = 0;
};