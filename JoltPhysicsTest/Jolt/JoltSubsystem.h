// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "JoltPhysicsTest/Debug/CustomLogger.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Subsystems/WorldSubsystem.h"
#include <iostream>

#include "JoltEnumsAndStructs.h"

#include "JoltPhysicsTest/Components/JoltStaticCompoundComponent.h"
#include "JoltPhysicsTest/Game/DeterministicRNG.h"
#include "JoltPhysicsTest/Game/GameStateManager.h"
#include "JoltPhysicsTest/Game/SpawnerSubsystem.h"
#include "JoltPhysicsTest/JoltBodies/ConveyorBelt.h"
#include "JoltSubsystem.generated.h"

using namespace std;

// Callback for traces, connect this to your own trace function if you have one
static void TraceImpl(const char *inFMT, ...)
{
	// Format the message
	va_list list;
	va_start(list, inFMT);
	char buffer[1024];
	vsnprintf(buffer, sizeof(buffer), inFMT, list);
	va_end(list);

	// Print to the TTY
	std::cout << buffer << std::endl;
}

#ifdef JPH_ENABLE_ASSERTS

// Callback for asserts, connect this to your own assert handler if you have one
static bool AssertFailedImpl(const char *inExpression, const char *inMessage, const char *inFile, uint inLine)
{
	// Print to the TTY
	std::cout << inFile << ":" << inLine << ": (" << inExpression << ") " << (inMessage != nullptr? inMessage : "") << std::endl;

	// Breakpoint
	return true;
};

#endif // JPH_ENABLE_ASSERTS

/// Class that determines if two object layers can collide
/**
 * You must configure each layer case comparing with every other layer case. In other words the comparison has to work both ways, not just from one side.
 * For example No collision layer has to return false, but Player layer ALSO has to return false on NoCollision comparison.
 */
class ObjectLayerPairFilterImpl : public ObjectLayerPairFilter
{
public:
	virtual bool					ShouldCollide(ObjectLayer inObject1, ObjectLayer inObject2) const override
	{
		switch (inObject1)
		{
		case Layers::NO_COLLISION:
			return false;
		case Layers::NON_MOVING:
			return inObject2 == Layers::MOVING
				|| inObject2 == Layers::PROJECTILE
				|| inObject2 == Layers::FLAG
				|| inObject2 == Layers::CROWN
				|| inObject2 == Layers::BLACKHOLE;
		case Layers::MOVING:
			return
				inObject2 == Layers::NON_MOVING
				|| inObject2 == Layers::MOVING
				|| inObject2 == Layers::PLAYER
				|| inObject2 == Layers::PLAYER2
				|| inObject2 == Layers::PICKUP
				|| inObject2 == Layers::PROJECTILE
				|| inObject2 == Layers::OUTOFBOUNDS
				|| inObject2 == Layers::FLAG
				|| inObject2 == Layers::CROWN
				|| inObject2 == Layers::BLACKHOLE
				|| inObject2 == Layers::PORTAL;
		case Layers::FLAG:
			return
				inObject2 == Layers::NON_MOVING
				|| inObject2 == Layers::MOVING
				|| inObject2 == Layers::PICKUP
				|| inObject2 == Layers::FLAGBASE;
		case Layers::CROWN:
			return
				inObject2 == Layers::NON_MOVING
				|| inObject2 == Layers::MOVING
				|| inObject2 == Layers::PICKUP
				|| inObject2 == Layers::PLAYER
				|| inObject2 == Layers::PLAYER2
				|| inObject2 == Layers::OUTOFBOUNDS;
		case Layers::LOOTBOX:
			return
				inObject2 == Layers::PLAYER ||
				inObject2 == Layers::PLAYER2 ||
				inObject2 == Layers::PLAYERTAKEDAMAGESENSOR;
		case Layers::PLAYER:
			return
				inObject2 == Layers::NON_MOVING
				|| inObject2 == Layers::MOVING
				|| inObject2 == Layers::PLAYER
				|| inObject2 == Layers::PICKUP
				|| inObject2 == Layers::FLAGBASE
				|| inObject2 == Layers::LOOTBOX
				|| inObject2 == Layers::CROWN
				|| inObject2 == Layers::PLAYERSENSOR;
		case Layers::FLAGBASE:
			return
				inObject2 == Layers::PLAYER
				|| inObject2 == Layers::FLAG;
		case Layers::PLAYERSENSOR:
			return
				inObject2 == Layers::PLAYER
				|| inObject2 == Layers::PLAYER2;
		case Layers::PLAYER2:
			return
				inObject2 == Layers::NON_MOVING
				|| inObject2 == Layers::MOVING
				|| inObject2 == Layers::LOOTBOX
				|| inObject2 == Layers::CROWN
				|| inObject2 == Layers::PLAYERSENSOR;
		case Layers::PICKUP:
			return
				inObject2 == Layers::NON_MOVING
				|| inObject2 == Layers::MOVING
				|| inObject2 == Layers::PLAYER
				|| inObject2 == Layers::PLAYER2
				|| inObject2 == Layers::PICKUP
				|| inObject2 == Layers::OUTOFBOUNDS
				|| inObject2 == Layers::FLAG
				|| inObject2 == Layers::CROWN;
		case Layers::PROJECTILE:
			return
				inObject2 == Layers::MOVING
				|| inObject2 == Layers::NON_MOVING
				|| inObject2 == Layers::OUTOFBOUNDS
				|| inObject2 == Layers::PLAYERTAKEDAMAGESENSOR
				|| inObject2 == Layers::PLAYER
				|| inObject2 == Layers::PROJECTILE
				|| inObject2 == Layers::BOOMERANG
				|| inObject2 == Layers::SWORD
				|| inObject2 == Layers::PORTAL;
		case Layers::OUTOFBOUNDS:
			return inObject2 == Layers::PLAYERTAKEDAMAGESENSOR
				|| inObject2 == Layers::PROJECTILE
				|| inObject2 == Layers::CROWN;
		case Layers::BOOMERANG:
			return
				inObject2 == Layers::PROJECTILE
				|| inObject2 == Layers::PLAYERTAKEDAMAGESENSOR
				|| inObject2 == Layers::PORTAL;
		case Layers::BLADE:
			return inObject2 == Layers::PLAYERTAKEDAMAGESENSOR;
		case Layers::SWORD:
			return
				inObject2 == Layers::PROJECTILE
				|| inObject2 == Layers::PLAYERTAKEDAMAGESENSOR
				|| inObject2 == Layers::SWORD;
		case Layers::PLAYERTAKEDAMAGESENSOR:
			return inObject2 == Layers::PROJECTILE
				|| inObject2 == Layers::OUTOFBOUNDS
				|| inObject2 == Layers::LOOTBOX
				|| inObject2 == Layers::BOOMERANG
				|| inObject2 == Layers::BLADE
				|| inObject2 == Layers::SWORD
				|| inObject2 == Layers::PORTAL;
		case Layers::BLACKHOLE:
			 return inObject2 == Layers::MOVING
				|| inObject2 == Layers::NON_MOVING;
		case Layers::DEBRIS:
			return inObject2 == Layers::DEBRIS;
		case Layers::PORTAL:
			return inObject2 == Layers::PLAYERTAKEDAMAGESENSOR
				|| inObject2 == Layers::MOVING
				|| inObject2 == Layers::PROJECTILE
				|| inObject2 == Layers::BOOMERANG;
		default:
			JPH_ASSERT(false);
			return false;
		}
	}
};

// Each broadphase layer results in a separate bounding volume tree in the broad phase. You at least want to have
// a layer for non-moving and moving objects to avoid having to update a tree full of static objects every frame.
// You can have a 1-on-1 mapping between object layers and broadphase layers (like in this case) but if you have
// many object layers you'll be creating many broad phase trees, which is not efficient. If you want to fine tune
// your broadphase layers define JPH_TRACK_BROADPHASE_STATS and look at the stats reported on the TTY.
namespace BroadPhaseLayers
{
	static constexpr BroadPhaseLayer NON_MOVING(0);
	static constexpr BroadPhaseLayer MOVING(1);
	static constexpr uint NUM_LAYERS(2);
};

// BroadPhaseLayerInterface implementation
// This defines a mapping between object and broadphase layers.
class BPLayerInterfaceImpl final : public BroadPhaseLayerInterface
{
public:
	BPLayerInterfaceImpl()
	{
		// Create a mapping table from object to broad phase layer
		mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
		mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
		mObjectToBroadPhase[Layers::NO_COLLISION] = BroadPhaseLayers::NON_MOVING;
		mObjectToBroadPhase[Layers::PLAYER] = BroadPhaseLayers::MOVING;
		mObjectToBroadPhase[Layers::PLAYER2] = BroadPhaseLayers::MOVING;
		mObjectToBroadPhase[Layers::PICKUP] = BroadPhaseLayers::MOVING;
		mObjectToBroadPhase[Layers::PROJECTILE] = BroadPhaseLayers::MOVING;
		mObjectToBroadPhase[Layers::OUTOFBOUNDS] = BroadPhaseLayers::MOVING;
		mObjectToBroadPhase[Layers::PLAYERTAKEDAMAGESENSOR] = BroadPhaseLayers::MOVING;
		mObjectToBroadPhase[Layers::FLAGBASE] = BroadPhaseLayers::NON_MOVING;
		mObjectToBroadPhase[Layers::FLAG] = BroadPhaseLayers::MOVING;
		mObjectToBroadPhase[Layers::CROWN] = BroadPhaseLayers::MOVING;
		mObjectToBroadPhase[Layers::LOOTBOX] = BroadPhaseLayers::NON_MOVING;
		mObjectToBroadPhase[Layers::PLAYERSENSOR] = BroadPhaseLayers::NON_MOVING;
		mObjectToBroadPhase[Layers::BOOMERANG] = BroadPhaseLayers::MOVING;
		mObjectToBroadPhase[Layers::BLADE] = BroadPhaseLayers::MOVING;
		mObjectToBroadPhase[Layers::SWORD] = BroadPhaseLayers::MOVING;
		mObjectToBroadPhase[Layers::BLACKHOLE] = BroadPhaseLayers::MOVING;
		mObjectToBroadPhase[Layers::DEBRIS] = BroadPhaseLayers::MOVING;
		mObjectToBroadPhase[Layers::PORTAL] = BroadPhaseLayers::MOVING;
	}

	virtual uint GetNumBroadPhaseLayers() const override
	{
		return BroadPhaseLayers::NUM_LAYERS;
	}

	virtual BroadPhaseLayer GetBroadPhaseLayer(ObjectLayer inLayer) const override
	{
		JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
		return mObjectToBroadPhase[inLayer];
	}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
	virtual const char* GetBroadPhaseLayerName(BroadPhaseLayer inLayer) const override
	{
		switch ((BroadPhaseLayer::Type)inLayer)
		{
		case (BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING:	return "NON_MOVING";
		case (BroadPhaseLayer::Type)BroadPhaseLayers::MOVING:		return "MOVING";
		default:													JPH_ASSERT(false); return "INVALID";
		}
	}
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

private:
	BroadPhaseLayer	mObjectToBroadPhase[Layers::NUM_LAYERS];
};

/// Class that determines if an object layer can collide with a broadphase layer
class ObjectVsBroadPhaseLayerFilterImpl : public ObjectVsBroadPhaseLayerFilter
{
public:
	virtual bool ShouldCollide(ObjectLayer inLayer1, BroadPhaseLayer inLayer2) const override
	{
		switch (inLayer1)
		{
		case Layers::NO_COLLISION:
			return false;
		case Layers::NON_MOVING:
			return inLayer2 == BroadPhaseLayers::MOVING;
		case Layers::MOVING:
			return true;
		case Layers::PLAYER:
			return true;
		case Layers::PLAYER2:
			return true;
		case Layers::PICKUP:
			return true;
		case Layers::PROJECTILE:
			return true;
		case Layers::OUTOFBOUNDS:
			return inLayer2 == BroadPhaseLayers::MOVING;
		case Layers::PLAYERTAKEDAMAGESENSOR:
			return inLayer2 == BroadPhaseLayers::MOVING;
		case Layers::FLAGBASE:
			return inLayer2 == BroadPhaseLayers::MOVING;
		case Layers::PLAYERSENSOR:
			return inLayer2 == BroadPhaseLayers::MOVING;
		case Layers::BOOMERANG:
			return inLayer2 == BroadPhaseLayers::MOVING;
		case Layers::SWORD:
			return inLayer2 == BroadPhaseLayers::MOVING;
		case Layers::BLADE:
			return true;
		case Layers::FLAG:
			return true;
		case Layers::CROWN:
			return true;
		case Layers::LOOTBOX:
			return inLayer2 == BroadPhaseLayers::MOVING;
		case Layers::BLACKHOLE:
			return true;
		case Layers::DEBRIS:
			return inLayer2 == BroadPhaseLayers::MOVING;
		case Layers::PORTAL:
			return true;
		default:
			JPH_ASSERT(false);
			return false;
		}
	}
};

struct FContact
{
	FContact() {}

	BodyID BodyID1;
	BodyID BodyID2;
	Vec3 ContactWorldPosition;
	Vec3 Body1LinearVelocity;
	Vec3 Body2LinearVelocity;
	Vec3 WorldNormal;
	float PenetrationDepth = 0.f;
};

/**
 * !!!!!!!!!!!!!! CONTACTS ARE NOT GUARANTEED TO ARRIVE IN A DETERMINISTIC ORDER !!!!!!!!!!!!!!!!!
 * Make sure you sort them before processing them or otherwise ensure that collisions are handled in a deterministic way.
 */
class MyContactListener : public ContactListener
{
public:
	void Initialize(UWorld* InWorld, BodyInterface* InBodyInterface)
	{
		World = InWorld;
		BodyInterfacePtr = InBodyInterface;
	}
	
	// See: ContactListener
	virtual ValidateResult OnContactValidate(const Body &inBody1, const Body &inBody2, RVec3Arg inBaseOffset, const CollideShapeResult &inCollisionResult) override
	{
		//cout << "Contact validate callback" << endl;

		// Allows you to ignore a contact before it is created (using layers to not make objects collide is cheaper!)
		return ValidateResult::AcceptAllContactsForThisBodyPair;
	}

	virtual void OnContactAdded(const Body &inBody1, const Body &inBody2, const ContactManifold &inManifold, ContactSettings &ioSettings) override
	{
		LOG(TEXT("[JoltSubsystem] Registered contact; Body1 %d; Body2: %d; ContactWorldPosition: %s"), inBody1.GetID().GetIndex(), inBody2.GetID().GetIndex(), *VecToString(inManifold.GetWorldSpaceContactPointOn1(0)));
		/*if (inBody1.GetID().GetIndex() == 18)
		{
			ioSettings.mInvMassScale1 = 2.f;
			ioSettings.mInvInertiaScale1 = 2.f;
		}

		if (inBody2.GetID().GetIndex() == 18)
		{
			ioSettings.mInvMassScale2 = 2.f;
			ioSettings.mInvInertiaScale2 = 2.f;
		}*/
		FContact NewContact;
		NewContact.BodyID1 = inBody1.GetID();
		NewContact.BodyID2 = inBody2.GetID();
		NewContact.ContactWorldPosition = inManifold.GetWorldSpaceContactPointOn1(0);
		NewContact.Body1LinearVelocity = inBody1.GetLinearVelocity();
		NewContact.Body2LinearVelocity = inBody2.GetLinearVelocity();
		NewContact.WorldNormal = inManifold.mWorldSpaceNormal;
		NewContact.PenetrationDepth = inManifold.mPenetrationDepth;
		
		ContactsAdded.Enqueue(NewContact);
	}

	virtual void OnContactPersisted(const Body &inBody1, const Body &inBody2, const ContactManifold &inManifold, ContactSettings &ioSettings) override
	{
		auto SpawnerSubsystem = World->GetSubsystem<USpawnerSubsystem>();
		if (SpawnerSubsystem->DoesBodyExistInSimulation(inBody2.GetID()))
		{
			auto BodyActor = SpawnerSubsystem->GetBodyActor(inBody2.GetID());
			check(BodyActor);

			if (auto ConveyorBelt = Cast<AConveyorBelt>(BodyActor))
			{
				ioSettings.mRelativeLinearSurfaceVelocity = ConveyorBelt->ConveyorVel;
			}
		}

		if (SpawnerSubsystem->DoesBodyExistInSimulation(inBody1.GetID()))
		{
			auto BodyActor = SpawnerSubsystem->GetBodyActor(inBody1.GetID());
			check(BodyActor);

			if (auto ConveyorBelt = Cast<AConveyorBelt>(BodyActor))
			{
				ioSettings.mRelativeLinearSurfaceVelocity = ConveyorBelt->ConveyorVel;
			}
		}

		return;
		
		FContact NewContact;
		NewContact.BodyID1 = inBody1.GetID();
		NewContact.BodyID2 = inBody2.GetID();
		NewContact.ContactWorldPosition = inManifold.GetWorldSpaceContactPointOn1(0);
		NewContact.Body1LinearVelocity = inBody1.GetLinearVelocity();
		NewContact.Body2LinearVelocity = inBody2.GetLinearVelocity();
		NewContact.WorldNormal = inManifold.mWorldSpaceNormal;
		NewContact.PenetrationDepth = inManifold.mPenetrationDepth;
		
		ContactsPersisted.Enqueue(NewContact);
	}

	virtual void OnContactRemoved(const SubShapeIDPair &inSubShapePair) override
	{
		LOG(TEXT("[JoltSubsystem] Removed contact; Body1 %d; Body2: %d;"), inSubShapePair.GetBody1ID().GetIndex(), inSubShapePair.GetBody2ID().GetIndex());
		
		FContact RemovedContact;
		RemovedContact.BodyID1 = inSubShapePair.GetBody1ID();
		RemovedContact.BodyID2 = inSubShapePair.GetBody2ID();
		
		ContactsRemoved.Enqueue(RemovedContact);
	}
		
	TQueue<FContact, EQueueMode::Mpsc>& GetContactsAdded()
	{
		return ContactsAdded;
	}

	TQueue<FContact, EQueueMode::Mpsc>& GetContactsRemoved()
	{
		return ContactsRemoved;
	}

	/*TQueue<FContact, EQueueMode::Mpsc>& GetContactsPersisted()
	{
		return ContactsPersisted;
	}*/

	void ResetContacts()
	{
		ContactsAdded.Empty();
		ContactsRemoved.Empty();
		ContactsPersisted.Empty();
	}

private:
	TQueue<FContact, EQueueMode::Mpsc> ContactsAdded;
	TQueue<FContact, EQueueMode::Mpsc> ContactsRemoved;
	TQueue<FContact, EQueueMode::Mpsc> ContactsPersisted;

	UWorld* World = nullptr;
	BodyInterface* BodyInterfacePtr = nullptr;
};

// An example activation listener
class MyBodyActivationListener : public BodyActivationListener
{
public:
	virtual void OnBodyActivated(const BodyID &inBodyID, uint64 inBodyUserData) override
	{
		cout << "A body got activated" << endl;
	}

	virtual void OnBodyDeactivated(const BodyID &inBodyID, uint64 inBodyUserData) override
	{
		cout << "A body went to sleep" << endl;
	}
};

class RayObjectLayerFilter : public ObjectLayerFilter
{
public:
	RayObjectLayerFilter() {}
	
	explicit RayObjectLayerFilter(const TArray<EObjectLayer>& InObjectLayers)
	{
		ObjectLayers = InObjectLayers;
	}
	
	virtual bool ShouldCollide(ObjectLayer InLayer) const override
	{
		return ObjectLayers.IsEmpty() || ObjectLayers.Contains(static_cast<EObjectLayer>(InLayer));
	}

private:
	TArray<EObjectLayer> ObjectLayers;
};

#ifdef MY_DEBUG_ROLLBACK
	struct FJoltFrame
	{
		int FrameId = 0;
		TArray<FString> Logs;
	};
#endif

UCLASS()
class JOLTPHYSICSTEST_API UJoltSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	void SaveState(StateRecorderImpl& InStream, CustomStateRecorderFilter* RecorderFilter);
	bool RestoreState(StateRecorder &InStream);

	void StepSimulation(const float& FixedStepTime);

	TArray<FContact> GetContactsAddedFromLastStep();
	TArray<FContact> GetContactsRemovedFromLastStep();

	PhysicsSystem* GetPhysicsSystem();
	BodyInterface* GetBodyInterface();
	
	FVector GetCenterOfMassPosition(const BodyID& InBodyID);
	FVector GetBodyLocation(const BodyID& InBodyID);
	FQuat GetBodyRotation(const BodyID& InBodyID);
	
	bool CastRayFirstHit(const Vec3& FromLocation, const Vec3& Direction, Vec3& OutHitPosition, BodyID& OutBodyID, RayObjectLayerFilter* InRayObjectLayerFilter = nullptr) const;
	bool CastShapeFirstHit(const RefConst<Shape>& InShape, const Vec3& FromLocation, const Vec3& Direction, Vec3& OutHitPosition, BodyID& OutBodyID, RayObjectLayerFilter* InRayObjectLayerFilter = nullptr) const;

	ClosestHitPerBodyCollisionCollector<CollideShapeCollector> CollideShape(const Shape& InShape, const Vec3& CastPosition, RayObjectLayerFilter* InRayObjectLayerFilter) const;
	
	EObjectLayer GetObjectLayer(const uint32& InBodyIndex) const;
	void SetObjectLayer(const uint32& InBodyIndex, EObjectLayer InObjectLayer);
	void SetMotionType(const uint32& InBodyIndex, EMotionType InMotionType);

	BodyID AddBox(
		const int& BodyIndex,
		const FVector& InLocation,
		const FQuat& InRotation,
		const FVector& InHalfExtent,
		const FPhysicsProperties& PhysicsProperties
	);
	
	BodyID AddSphere(
		const int& BodyIndex,
		const FVector& InLocation,
		const FQuat& InRotation,
		const float& InRadius,
		const FPhysicsProperties& PhysicsProperties
	);
	
	BodyID AddCapsule(
		const int& BodyIndex,
		const FVector& InLocation,
		const FQuat& InRotation,
		const float& InRadius,
		const float& InHalfHeight,
		const FPhysicsProperties& PhysicsProperties
	);

	BodyID AddConvexHull(
		const int& BodyIndex,
		const FVector& InLocation,
		const FQuat& InRotation,
		const TArray<FVector>& InPoints,
		const FPhysicsProperties& PhysicsProperties
	);

	BodyID AddStaticCompound(
		const int& BodyIndex,
		const FVector& InLocation,
		const FQuat& InRotation,
		TArray<TJoltBodySettings> JoltBodySettings,
		const FPhysicsProperties& PhysicsProperties
	);
	
	void AddConstraint(Constraint* InConstraint);
	void RemoveConstraint(Constraint* InConstraint);

	void AddBody(const BodyID& InBodyID);
	void RemoveBody(const BodyID& InBodyID);
	void DestroyBody(const BodyID& InBodyID);
	void RemoveAndDestroyBody(const BodyID& InBodyID);

	bool IsRandomNumberGeneratorInitialized() const;

	void InitRandomNumberGenerator(const uint64_t& InSeed);

	DeterministicRNG RandomNumberGenerator;

#ifdef MY_DEBUG_LOG_JOLT_CALLS
	void PrepareToLogFrame(const int& InFrameId);
#endif
private:
	BodyID CreateAndAddBody(const int& BodyIndex, const BodyCreationSettings& BodySettings);

	void DestroyBody_internal(const BodyID& InBodyID);

	void UpdateAllowedDOFs_Internal(BodyCreationSettings& BodySettings, const FPhysicsProperties& PhysicsProperties);
	void UpdateIsSensor_Internal(BodyCreationSettings& BodySettings, const FPhysicsProperties& PhysicsProperties);
	void UpdateMass_Internal(BodyCreationSettings& BodySettings, const FPhysicsProperties& PhysicsProperties);

	bool bInitialized = false;
	bool bRandomNumberGeneratorInitialized = false;

	TSharedPtr<TempAllocatorImpl> JoltAllocator = nullptr;
	TSharedPtr<JobSystemThreadPool> JoltJobSystem = nullptr;
	TSharedPtr<PhysicsSystem> JoltPhysicsSystem = nullptr;

	const FVector Gravity = FVector(0, 0, -980.f);

	// How many collision steps Jolt should perform during single frame
	const int JoltCollisionSteps = 2;

	// Used for temporary storage by Jolt during simulation
	const uint AllocationArenaSize = 100 * 1024 * 1024;
	
	// This is the max amount of rigid bodies that you can add to the physics system. If you try to add more you'll get an error.
	// Note: This value is low because this is a simple test. For a real project use something in the order of 65536.
	const uint cMaxBodies = 1000000;

	// This determines how many mutexes to allocate to protect rigid bodies from concurrent access. Set it to 0 for the default settings.
	const uint cNumBodyMutexes = 0;

	// This is the max amount of body pairs that can be queued at any time (the broad phase will detect overlapping
	// body pairs based on their bounding boxes and will insert them into a queue for the narrowphase). If you make this buffer
	// too small the queue will fill up and the broad phase jobs will start to do narrow phase work. This is slightly less efficient.
	// Note: This value is low because this is a simple test. For a real project use something in the order of 65536.
	const uint cMaxBodyPairs = 1000000;

	// This is the maximum size of the contact constraint buffer. If more contacts (collisions between bodies) are detected than this
	// number then these contacts will be ignored and bodies will start interpenetrating / fall through the world.
	// Note: This value is low because this is a simple test. For a real project use something in the order of 10240.
	const uint cMaxContactConstraints = 2048;

	// A contact listener gets notified when bodies (are about to) collide, and when they separate again.
	// Note that this is called from a job so whatever you do here needs to be thread safe.
	// Registering one is entirely optional.
	MyContactListener ContactListenerInstance;

	// A body activation listener gets notified when bodies activate and go to sleep
	// Note that this is called from a job so whatever you do here needs to be thread safe.
	// Registering one is entirely optional.
	TSharedPtr<MyBodyActivationListener> BodyActivationListener;

	// Create mapping table from object layer to broadphase layer
	// Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
	BPLayerInterfaceImpl BroadPhaseLayerInterface;

	// Create class that filters object vs broadphase layers
	// Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
	ObjectVsBroadPhaseLayerFilterImpl ObjectVSBroadPhaseLayerFilterInterface;

	// Create class that filters object vs object layers
	// Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
	ObjectLayerPairFilterImpl ObjectVSObjectLayerFilterInterface;


#ifdef MY_DEBUG_LOG_JOLT_CALLS
	TMap<int, FJoltFrame> DebugJoltFrames;
	void DebugLog(FString Message);
	void DumpToLogs();
	int CurrentlyLoggingFrameId = -1;
#endif
};
