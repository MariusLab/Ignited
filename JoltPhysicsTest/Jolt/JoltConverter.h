#pragma once

PRAGMA_PUSH_PLATFORM_DEFAULT_PACKING
#include "Jolt/Math/Quat.h"
#include "Jolt/Math/Vec3.h"
#include "Jolt/Core/Color.h"
PRAGMA_POP_PLATFORM_DEFAULT_PACKING

namespace Units
{
	using namespace JPH;
	
	FORCEINLINE static FColor FromJoltColor(const ColorArg& InColor) { return FColor(InColor.r, InColor.g, InColor.b, InColor.a); }
	
	//Jolt Physics uses a right handed coordinate system with Y-up; While Unreal uses left handed Z-up; So we need to convert
	FORCEINLINE static FVector FromJoltCoord(const Vec3& InVec) { return FVector(InVec.GetX(), InVec.GetZ(), InVec.GetY()); }
	FORCEINLINE static Vec3 ToJoltCoord(const FVector& InVector) { return Vec3(InVector.X, InVector.Z, InVector.Y); }
	
	FORCEINLINE static FVector FromJoltPos(const Vec3& InVec) { return FromJoltCoord(InVec * 100.f); }
	FORCEINLINE static Vec3 ToJoltPos(const FVector& InVector) { return ToJoltCoord(InVector) * 0.01f; }

	FORCEINLINE static Vec3 ToJoltUnits(const FVector& InVector) { return Vec3(InVector.X, InVector.Z, InVector.Y) * 0.01f; }
	FORCEINLINE static float ToJoltUnits(const float& InFloat) { return InFloat * 0.01f; }
	
	FORCEINLINE static float FromJoltUnits(const float& InFloat) { return InFloat * 100.f; }
	
	FORCEINLINE static FQuat FromJoltQuat(const Quat& InQuat) { return FQuat(-InQuat.GetX(), -InQuat.GetZ(), -InQuat.GetY(), InQuat.GetW()); }
	FORCEINLINE static Quat ToJoltQuat(const FQuat& InQuat) { return Quat(-InQuat.X, -InQuat.Z, -InQuat.Y, InQuat.W); }
}