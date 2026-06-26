#pragma once

UENUM()
enum class EJoltBodyShape : uint8
{
	None,
	Box,
	Sphere,
	ConvexHull
};

struct TJoltBodySettings
{
	TJoltBodySettings(
		const FVector& InLocation,
		const FQuat& InRotation,
		const FVector& InBoxExtent
	)
	{
		JoltBodyShape = EJoltBodyShape::Box;
		Location = InLocation;
		Rotation = InRotation;
		BoxExtent = InBoxExtent;
	}

	TJoltBodySettings(
		const FVector& InLocation,
		const FQuat& InRotation,
		const float& InSphereRadius
	)
	{
		JoltBodyShape = EJoltBodyShape::Sphere;
		Location = InLocation;
		Rotation = InRotation;
		SphereRadius = InSphereRadius;
	}

	TJoltBodySettings(
		const FVector& InLocation,
		const FQuat& InRotation,
		const TArray<FVector>& Points
	)
	{
		JoltBodyShape = EJoltBodyShape::ConvexHull;
		Location = InLocation;
		Rotation = InRotation;
		ConvexHullPoints = Points;
	}

	EJoltBodyShape JoltBodyShape = EJoltBodyShape::None;
	
	FVector Location = FVector::ZeroVector;
	FQuat Rotation = FQuat::Identity;

	FVector BoxExtent = FVector::ZeroVector;
	float SphereRadius = 0.f;
	TArray<FVector> ConvexHullPoints;

#ifdef MY_DEBUG_RENDERING
	TArray<FVector> ConvexHullPointsForRendering;
#endif
};