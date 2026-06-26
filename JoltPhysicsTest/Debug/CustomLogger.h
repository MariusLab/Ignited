#pragma once

#pragma float_control(precise, on)
#pragma float_control(except, on)
#pragma fenv_access(on)
#pragma fp_contract(off)
#define _CRT_SECURE_NO_WARNINGS

PRAGMA_PUSH_PLATFORM_DEFAULT_PACKING
#include "mimalloc.h"
#include "Jolt/Jolt.h"
#include "Jolt/RegisterTypes.h"
#include "Jolt/Math/Quat.h"
#include "Jolt/Math/Vec3.h"
#include "Jolt/Physics/PhysicsSystem.h"
#include "Jolt/Core/Factory.h"
#include "Jolt/Core/TempAllocator.h"
#include "Jolt/Core/JobSystemThreadPool.h"
#include "Jolt/Physics/PhysicsSettings.h"
#include "Jolt/Physics/Collision/Shape/BoxShape.h"
#include "Jolt/Physics/Collision/Shape/SphereShape.h"
#include "Jolt/Physics/Collision/Shape/MeshShape.h"
#include "Jolt/Physics/Collision/Shape/ConvexHullShape.h"
#include "Jolt/Physics/Collision/Shape/StaticCompoundShape.h"
#include "Jolt/Physics/Body/BodyCreationSettings.h"
#include "Jolt/Physics/Body/BodyActivationListener.h"
#include "Jolt/Physics/Collision/Shape/CapsuleShape.h"
#include "Jolt/Physics/Collision/Shape/MeshShape.h"
#include "Jolt/Physics/Character/CharacterBase.h"
#include "Jolt/Physics/Character/CharacterVirtual.h"
#include "Jolt/Physics/Character/Character.h"
#include "Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h"
#include "Jolt/ConfigurationString.h"
#include "Jolt/Physics/StateRecorder.h"
#include "Jolt/Physics/StateRecorderImpl.h"
#include <Jolt/Physics/Collision/GroupFilterTable.h>
#include <Jolt/Physics/Constraints/Constraint.h>
#include <Jolt/Physics/Constraints/PointConstraint.h>
#include "Jolt/Physics/Constraints/DistanceConstraint.h"
#include "Jolt/Physics/Constraints/SwingTwistConstraint.h"
#include "Jolt/Physics/Constraints/HingeConstraint.h"
#include "Jolt/Physics/Constraints/SliderConstraint.h"
#include "Jolt/Physics/Collision/RayCast.h"
#include "Jolt/Physics/Collision/CastResult.h"
#include "Jolt/Physics/Collision/CollisionCollectorImpl.h"
#include "Jolt/Physics/Collision/CollideShape.h"
#include "Jolt/Physics/Collision/ShapeCast.h"
// Disable common warnings triggered by Jolt, you can use JPH_SUPPRESS_WARNING_PUSH / JPH_SUPPRESS_WARNING_POP to store and restore the warning state
//JPH_SUPPRESS_WARNINGS
PRAGMA_POP_PLATFORM_DEFAULT_PACKING

using namespace JPH;

#include "Engine/Engine.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerController.h"
#include "CoreMinimal.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"

static uint32 FloatToHex(float Value)
{
	union
	{
		float f;
		uint32 i;
	} u;

	u.f = Value;
	return u.i;
}

static FString FloatToHexString(const float& InFloat)
{
	return FString::Printf(TEXT("0x%08X "), FloatToHex(InFloat));
}

static FString IntToString(const int& InInt)
{
	return FString::Printf(TEXT("%d "), InInt);
}

static FString IntToHexString(const int& InInt)
{
	return FString::Printf(TEXT("0x%08X "), FloatToHex(InInt));
}

static FString IntArrayToHexString(const TArray<int>& InArray)
{
	FString ReturnString;
	for (const int& Value : InArray)
	{
		ReturnString += FString::Printf(TEXT("0x%08X "), FloatToHex(Value));
	}
	
	return ReturnString;
}

static FString IntArrayToString(const TArray<int>& InArray)
{
	FString ReturnString;
	for (const int& Value : InArray)
	{
		ReturnString += FString::Printf(TEXT("%d, "), Value);
	}
	
	return ReturnString;
}

static FString IntsMapToString(const TMap<int, int>& InMap)
{
	FString ReturnString;
	for (const auto& pair : InMap)
	{
		ReturnString += FString::Printf(TEXT("%d - %d, "), pair.Key, pair.Value);
	}
	
	return ReturnString;
}

static FString VectorToHexString(const FVector& InVector)
{
	return
		FString::Printf(TEXT("X=0x%08X "), FloatToHex(InVector.X))
		+= FString::Printf(TEXT("Y=0x%08X "), FloatToHex(InVector.Y))
		+= FString::Printf(TEXT("Z=0x%08X "), FloatToHex(InVector.Z));
}

static FString Vector2DToHexString(const FVector2D& InVector)
{
	return
		FString::Printf(TEXT("X=0x%08X "), FloatToHex(InVector.X))
		+= FString::Printf(TEXT("Y=0x%08X "), FloatToHex(InVector.Y));
}

static FString QuatToHexString(const FQuat& InQuat)
{
	return
		FString::Printf(TEXT("X=0x%08X "), FloatToHex(InQuat.X))
		+= FString::Printf(TEXT("Y=0x%08X "), FloatToHex(InQuat.Y))
		+= FString::Printf(TEXT("Z=0x%08X "), FloatToHex(InQuat.Z))
		+= FString::Printf(TEXT("W=0x%08X "), FloatToHex(InQuat.W));
}

static FString QuatToString(const Quat& InQuat)
{
	return
		FString::Printf(TEXT("X=%f "), InQuat.GetX())
		+= FString::Printf(TEXT("Y=%f "), InQuat.GetY())
		+= FString::Printf(TEXT("Z=%f "), InQuat.GetZ())
		+= FString::Printf(TEXT("W=%f "), InQuat.GetW());
}

static FString QuatToHexString(const Quat& InQuat)
{
	return
		FString::Printf(TEXT("X=0x%08X "), FloatToHex(InQuat.GetX()))
		+= FString::Printf(TEXT("Y=0x%08X "), FloatToHex(InQuat.GetY()))
		+= FString::Printf(TEXT("Z=0x%08X "), FloatToHex(InQuat.GetZ()))
		+= FString::Printf(TEXT("W=0x%08X "), FloatToHex(InQuat.GetW()));
}

static FString VecToString(const Vec3& InVec)
{
	return
		FString::Printf(TEXT("X=%f "), InVec.GetX())
		+= FString::Printf(TEXT("Y=%f "), InVec.GetY())
		+= FString::Printf(TEXT("Z=%f "), InVec.GetZ());
}

static FString VecToHexString(const Vec3& InVec)
{
	return
		FString::Printf(TEXT("X=0x%08X "), FloatToHex(InVec.GetX()))
		+= FString::Printf(TEXT("Y=0x%08X "), FloatToHex(InVec.GetY()))
		+= FString::Printf(TEXT("Z=0x%08X "), FloatToHex(InVec.GetZ()));
}

static FRotator VecDirectionToFRotator(const Vec3& InVecDirection)
{
	const float YawRad = JPH::ATan2(InVecDirection.GetY(), InVecDirection.GetX());
	const float YawDeg = JPH::RadiansToDegrees(YawRad);

	return {YawDeg, 0, 0};
}

// RandomFloat = 0.f - 1.f
static Vec3 RandomUnitVector(const float& RandomFloat)
{
	float theta = JPH_PI * RandomFloat;
	float phi = 2.0f * JPH_PI * RandomFloat;
	return Vec3::sUnitSpherical(theta, phi);
}

static Vec3 OffsetDirectionAngle(const Vec3& InVecDirection, const float& InDegreesOffset)
{
	float TargetAngle = JPH::ATan2(InVecDirection.GetY(), InVecDirection.GetX());
	TargetAngle += JPH::DegreesToRadians(InDegreesOffset);
			
	return Vec3(
		JPH::Cos(TargetAngle),
		JPH::Sin(TargetAngle),
		0.0f
	);
}

#define LOG(Format, ...) \
{ \
	FString RoleTag = TEXT("[Unknown] "); \
	UWorld* World = nullptr; \
	if (GEngine && GEngine->GetWorldContexts().Num() > 0) \
	{ \
		for (const FWorldContext& Context : GEngine->GetWorldContexts()) \
		{ \
			if (Context.World() && Context.World()->IsGameWorld()) \
			{ \
				World = Context.World(); \
				break; \
			} \
		} \
	} \
	if (World) \
	{ \
		switch (World->GetNetMode()) \
		{ \
			case NM_Standalone: RoleTag = TEXT("[Standalone] "); break; \
			case NM_ListenServer: RoleTag = TEXT("[Server] "); break; \
			case NM_DedicatedServer: RoleTag = TEXT("[DedicatedServer] "); break; \
			case NM_Client: \
			{ \
				APlayerController* LogPC = World->GetFirstPlayerController(); \
				if (LogPC && LogPC->PlayerState) \
				{ \
					int32 LogPlayerId = LogPC->PlayerState->GetPlayerId(); \
					RoleTag = FString::Printf(TEXT("[Client %d] "), LogPlayerId); \
				} \
				else \
				{ \
					RoleTag = TEXT("[Client] "); \
				} \
				break; \
			} \
		} \
	} \
	UE_LOG(LogTemp, Warning, TEXT("%s") Format, *RoleTag, ##__VA_ARGS__); \
}


// Usage: LOG_TO_FILE("MyLog.txt", TEXT("Value = %.2f"), SomeValue);
#define LOG_TO_FILE(FileName, Format, ...) \
{ \
FString Message = FString::Printf(TEXT("") Format, ##__VA_ARGS__); \
\
FString FullPath = FPaths::ProjectSavedDir() / TEXT("CustomLogs") / FileName; \
IFileManager& FileSys = IFileManager::Get(); \
FileSys.MakeDirectory(*FPaths::GetPath(FullPath), /*Tree=*/true); \
FFileHelper::SaveStringToFile(Message + "\n\n", *FullPath, FFileHelper::EEncodingOptions::AutoDetect, &FileSys, EFileWrite::FILEWRITE_Append); \
}

/** For logging calls to Jolt physics. To check if all clients calls go in the same order */
#define LOG_JOLT(Format, ...) \
{ \
	FString FileName = (GetWorld() && GetWorld()->GetNetMode() == NM_ListenServer) ? TEXT("ServerJolt.txt") : TEXT("ClientJolt.txt"); \
	LOG_TO_FILE(*FileName, Format, ##__VA_ARGS__); \
}

#define LOG_REPLAY(Format, ...) \
{ \
FString FileName = (GetWorld() && GetWorld()->GetNetMode() == NM_ListenServer) ? TEXT("ServerReplay.txt") : TEXT("ClientReplay.txt"); \
LOG_TO_FILE(*FileName, Format, ##__VA_ARGS__); \
}

#define LOG_NETWORKING(Format, ...) \
{ \
FString FileName = (GetWorld() && GetWorld()->GetNetMode() == NM_ListenServer) ? TEXT("NetworkingServer.txt") : TEXT("NetworkingClient.txt"); \
LOG_TO_FILE(*FileName, Format, ##__VA_ARGS__); \
}

#define LOG_TEMPORARY(Format, ...) \
{ \
FString FileName = (GetWorld() && GetWorld()->GetNetMode() == NM_ListenServer) ? TEXT("TemporaryServer.txt") : TEXT("TemporaryClient.txt"); \
LOG_TO_FILE(*FileName, Format, ##__VA_ARGS__); \
}

#define LOG_CONSTRAINT(Format, ...) \
{ \
FString FileName = (GetWorld() && GetWorld()->GetNetMode() == NM_ListenServer) ? TEXT("ServerConstraint.txt") : TEXT("ClientConstraint.txt"); \
LOG_TO_FILE(*FileName, Format, ##__VA_ARGS__); \
}

#define DELETE_LOG_FILE_IF_EXISTS(FileName) \
{ \
FString FullPath = FPaths::ProjectSavedDir() / TEXT("CustomLogs") / FileName; \
if (FPaths::FileExists(FullPath)) \
{ \
IFileManager::Get().Delete(*FullPath); \
} \
}

#define MY_DEBUG_RENDERING 1
/** Normally you have to tick a checkbox on each specific body you want to debug render. This allows to render ALL automatically */
//#define DEBUG_RENDER_ALL_BODIES 1;
#define MY_DEBUG_VALIDATE_CONSTRAINTS_GET_REMOVED_BEFORE_BODY 1
/** Debug desyncs in game state between players by comparing logs Server.txt and Client.txt */
//#define MY_DEBUG_GAME_STATE 1
/** Dump all calls to Jolt to ClientJolt.txt and ServerJolt.txt */
//#define MY_DEBUG_LOG_JOLT_CALLS 1

/** Perform max rollback every frame */
//#define MY_DEBUG_ROLLBACK 1
/** This should only be on when testing in SINGLE PLAYER. In multiplayer misspredictions happen and then rollback is SUPPOSED TO produce a different state */
//#define MY_DEBUG_VALIDATE_ROLLBACK_DETERMINISM 1

/** Perform each physics step twice, to make sure that both steps produce identical bit for bit results */
//#define MY_DEBUG_VALIDATE_DETERMINISM 1

#undef LOG
#define LOG(Format, ...)

/*#undef NO_LOGGING
#define NO_LOGGING 1*/

#undef LOG_CONSTRAINT
#define LOG_CONSTRAINT(Format, ...)

#undef LOG_REPLAY
#define LOG_REPLAY(Format, ...)

#undef LOG_NETWORKING
#define LOG_NETWORKING(Format, ...)

/*#undef LOG_TEMPORARY
#define LOG_TEMPORARY(Format, ...)*/


#if UE_BUILD_SHIPPING
	#undef MY_DEBUG_RENDERING
	#undef MY_DEBUG_VALIDATE_CONSTRAINTS_GET_REMOVED_BEFORE_BODY
	#undef MY_DEBUG_GAME_STATE
	#undef MY_DEBUG_LOG_JOLT_CALLS

	#undef MY_DEBUG_ROLLBACK
	#undef MY_DEBUG_VALIDATE_ROLLBACK_DETERMINISM
	#undef MY_DEBUG_VALIDATE_DETERMINISM
#endif

// Disables all our LOGS. Especially useful for stress testing. As excessive logging can drag down CPU performance a lot.
#if UE_BUILD_SHIPPING
	#undef LOG
	#define LOG(Format, ...)

	#undef LOG_TO_FILE
	#define LOG_TO_FILE(Format, ...)

	#undef LOG_CONSTRAINT
	#define LOG_CONSTRAINT(Format, ...)

	#undef LOG_JOLT
	#define LOG_JOLT(Format, ...)

	#undef LOG_REPLAY
    #define LOG_REPLAY(Format, ...)

	#undef LOG_NETWORKING
	#define LOG_NETWORKING(Format, ...)

	#undef LOG_TEMPORARY
	#define LOG_TEMPORARY(Format, ...)

	#undef NO_LOGGING
	#define NO_LOGGING 1
#endif