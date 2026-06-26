// Fill out your copyright notice in the Description page of Project Settings.


#include "JoltSubsystem.h"
#include "JoltConverter.h"
#include "JoltPhysicsTest/Debug/DebugGameStateManager.h"

bool UJoltSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (!Super::ShouldCreateSubsystem(Outer))
	{
		return false;
	}

	const TArray AllowedNetModes{NM_ListenServer, NM_Client, NM_Standalone};

	const UWorld* World = Cast<UWorld>(Outer);
	if (World == nullptr)
	{
		return false;
	}

	// For some reason in standalone builds when you launch the game, it first creates this subsystem on some "Untitled" world
	// Which doesn't even have a game instance, so everything goes to hell.
	// So we just skip creating it if that's the case
	if (!World->GetGameInstance())
	{
		return false;
	}

	if (World->WorldType != EWorldType::Game && World->WorldType != EWorldType::PIE)
	{
		return false;
	}

	return AllowedNetModes.Contains(World->GetNetMode());
}

void UJoltSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	if (bInitialized)
	{
		return;
	}

	bInitialized = true;
	
	LOG(TEXT("[JoltSubsystem] Init called"));
	// This RESETS the character IDs to start from 1 again.
	// If we don't do this, then we risk not having deterministic IDs across different instances of the game
	// ALWAYS reset before a new game
	CharacterID::sSetNextCharacterID();

	// Register allocation hook. In this example we'll just let Jolt use malloc / free but you can override these if you want (see Memory.h).
	// This needs to be done before any other Jolt function is called.
	RegisterDefaultAllocator();

	// Install trace and assert callbacks
	Trace = TraceImpl;
	JPH_IF_ENABLE_ASSERTS(AssertFailed = AssertFailedImpl;)

	// Create a factory, this class is responsible for creating instances of classes based on their name or hash and is mainly used for deserialization of saved data.
	// It is not directly used in this example but still required.
	Factory::sInstance = new Factory();

	// Register all physics types with the factory and install their collision handlers with the CollisionDispatch class.
	// If you have your own custom shape types you probably need to register their handlers with the CollisionDispatch before calling this function.
	// If you implement your own default material (PhysicsMaterial::sDefault) make sure to initialize it before this function or else this function will create one for you.
	RegisterTypes();

	// We need a temp allocator for temporary allocations during the physics update. We're
	// pre-allocating 10 MB to avoid having to do allocations during the physics update.
	// B.t.w. 10 MB is way too much for this example but it is a typical value you can use.
	// If you don't want to pre-allocate you can also use TempAllocatorMalloc to fall back to
	// malloc / free.
	JoltAllocator = MakeShareable(new TempAllocatorImpl(AllocationArenaSize));

	// We need a job system that will execute physics jobs on multiple threads. Typically
	// you would implement the JobSystem interface yourself and let Jolt Physics run on top
	// of your own job scheduler. JobSystemThreadPool is an example implementation.
	JoltJobSystem = MakeShareable(new JobSystemThreadPool(cMaxPhysicsJobs, cMaxPhysicsBarriers, thread::hardware_concurrency() - 1));

	
	// Now we can create the actual physics system.
	JoltPhysicsSystem = MakeShareable(new PhysicsSystem());
	JoltPhysicsSystem->Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, BroadPhaseLayerInterface, ObjectVSBroadPhaseLayerFilterInterface, ObjectVSObjectLayerFilterInterface);
	
	JoltPhysicsSystem->SetBodyActivationListener(BodyActivationListener.Get());
	JoltPhysicsSystem->SetContactListener(&ContactListenerInstance);
	
	JoltPhysicsSystem->SetGravity(Units::ToJoltPos(Gravity));

	ContactListenerInstance.Initialize(GetWorld(), &JoltPhysicsSystem->GetBodyInterface());

	LOG(TEXT("[JoltSubsystem] Finished initializing."));

#ifdef MY_DEBUG_LOG_JOLT_CALLS
	// Initialization logic technically doesn't run within any frame, so we log it to -1 frame id
	DebugJoltFrames.Add(-1, {});
#endif
}

void UJoltSubsystem::Deinitialize()
{
	Super::Deinitialize();

	LOG(TEXT("[JoltSubsystem] Deinitialize."));

#ifdef MY_DEBUG_LOG_JOLT_CALLS
	DumpToLogs();
#endif
}

void UJoltSubsystem::SaveState(StateRecorderImpl& InStream, CustomStateRecorderFilter* RecorderFilter)
{
	InStream.Write(RandomNumberGenerator.GetState());
	
	GetPhysicsSystem()->SaveState(InStream, EStateRecorderState::Global | EStateRecorderState::Bodies | EStateRecorderState::Contacts, RecorderFilter);
}

bool UJoltSubsystem::RestoreState(StateRecorder& InStream)
{
	ContactListenerInstance.ResetContacts();

	uint64_t RandomNumberGeneratorState;
	InStream.Read(RandomNumberGeneratorState);
	RandomNumberGenerator.RestoreState(RandomNumberGeneratorState);
	
	return GetPhysicsSystem()->RestoreState(InStream);
}

void UJoltSubsystem::StepSimulation(const float& FixedStepTime)
{
	ContactListenerInstance.ResetContacts();
	const auto Result = JoltPhysicsSystem->Update(FixedStepTime, JoltCollisionSteps, JoltAllocator.Get(), JoltJobSystem.Get());
	check(Result == EPhysicsUpdateError::None);

#ifdef MY_DEBUG_LOG_JOLT_CALLS
	DebugLog(FString::Printf(TEXT("Step simulation")));
#endif
}

TArray<FContact> UJoltSubsystem::GetContactsAddedFromLastStep()
{
	TArray<FContact> SortedContacts;
	auto& ContactsQueue = ContactListenerInstance.GetContactsAdded();

	FContact Contact;
	while (ContactsQueue.Dequeue(Contact))
	{
		SortedContacts.Add(Contact);
	}

	SortedContacts.Sort([](const FContact& A, const FContact& B)
	{
		return A.BodyID1.GetIndex() < B.BodyID1.GetIndex() || A.BodyID2.GetIndex() < B.BodyID2.GetIndex();
	});

	return SortedContacts;
}

TArray<FContact> UJoltSubsystem::GetContactsRemovedFromLastStep()
{
	TArray<FContact> SortedContacts;
	auto& ContactsQueue = ContactListenerInstance.GetContactsRemoved();

	FContact Contact;
	while (ContactsQueue.Dequeue(Contact))
	{
		SortedContacts.Add(Contact);
	}

	SortedContacts.Sort([](const FContact& A, const FContact& B)
	{
		return A.BodyID1.GetIndex() < B.BodyID1.GetIndex() || A.BodyID2.GetIndex() < B.BodyID2.GetIndex();
	});

	return SortedContacts;
}

PhysicsSystem* UJoltSubsystem::GetPhysicsSystem()
{
	return JoltPhysicsSystem.Get();
}

BodyInterface* UJoltSubsystem::GetBodyInterface()
{
	return &JoltPhysicsSystem->GetBodyInterface();
}

FVector UJoltSubsystem::GetCenterOfMassPosition(const BodyID& InBodyID)
{
	return Units::FromJoltPos(GetBodyInterface()->GetCenterOfMassPosition(InBodyID));
}

FVector UJoltSubsystem::GetBodyLocation(const BodyID& InBodyID)
{
	return Units::FromJoltPos(GetBodyInterface()->GetPosition(InBodyID));
}

FQuat UJoltSubsystem::GetBodyRotation(const BodyID& InBodyID)
{
	return Units::FromJoltQuat(GetBodyInterface()->GetRotation(InBodyID));
}

bool UJoltSubsystem::CastRayFirstHit(const Vec3& FromLocation, const Vec3& Direction, Vec3& OutHitPosition, BodyID& OutBodyID, RayObjectLayerFilter* InRayObjectLayerFilter) const
{
	RRayCast Ray { FromLocation, Direction };
	
	RayCastResult CastResult;

	RayObjectLayerFilter EmptyFilter;
	RayObjectLayerFilter* RayFilter = &EmptyFilter;
	if (InRayObjectLayerFilter)
	{
		RayFilter = InRayObjectLayerFilter;
	}
	
	const bool bHit = JoltPhysicsSystem->GetNarrowPhaseQuery().CastRay(Ray, CastResult, {}, *RayFilter);
	LOG(TEXT("[JoltSubsystem] Narrowphase RayCast from %s in direction of %s"), *VecToHexString(FromLocation), *VecToHexString(Direction));

	if (bHit)
	{
		OutBodyID = CastResult.mBodyID;
		
		OutHitPosition = Ray.GetPointOnRay(CastResult.mFraction);
		LOG(TEXT("[JoltSubsystem] RayCast hit location %s; Early out fraction: %s"), *VecToHexString(OutHitPosition), *FloatToHexString(CastResult.mFraction));

		return true;
	}

	return false;
}

bool UJoltSubsystem::CastShapeFirstHit(const RefConst<Shape>& InShape, const Vec3& FromLocation, const Vec3& Direction, Vec3& OutHitPosition, BodyID& OutBodyID, RayObjectLayerFilter* InRayObjectLayerFilter) const
{
	RayObjectLayerFilter EmptyFilter;
	RayObjectLayerFilter* CastFilter = InRayObjectLayerFilter ? InRayObjectLayerFilter : &EmptyFilter;

	ShapeCastSettings Settings;
	RShapeCast ShapeCast(InShape, Vec3::sOne(), RMat44::sTranslation(FromLocation), Direction);

	ClosestHitCollisionCollector<CastShapeCollector> Collector;

	JoltPhysicsSystem->GetNarrowPhaseQuery().CastShape(ShapeCast, Settings, RVec3::sZero(), Collector, {}, *CastFilter);

	if (Collector.HadHit())
	{
		OutBodyID = Collector.mHit.mBodyID2;
		OutHitPosition = FromLocation + Direction * Collector.mHit.mFraction;
		return true;
	}

	return false;
}

ClosestHitPerBodyCollisionCollector<CollideShapeCollector> UJoltSubsystem::CollideShape(const Shape& InShape, const Vec3& CastPosition, RayObjectLayerFilter* InRayObjectLayerFilter) const
{
	RayObjectLayerFilter EmptyFilter;
	RayObjectLayerFilter* CastObjectLayerFilter = &EmptyFilter;
	if (InRayObjectLayerFilter)
	{
		CastObjectLayerFilter = InRayObjectLayerFilter;
	}

	ClosestHitPerBodyCollisionCollector<CollideShapeCollector> CollideShapeCollector;
	CollideShapeSettings settings;

	JoltPhysicsSystem->GetNarrowPhaseQuery().CollideShape(
		&InShape,
		Vec3::sOne(),
		RMat44::sTranslation(CastPosition),
		settings,
		RVec3::sZero(),
		CollideShapeCollector,
		{ },
		*CastObjectLayerFilter,
		{ }
	);

	return CollideShapeCollector;
}

EObjectLayer UJoltSubsystem::GetObjectLayer(const uint32& InBodyIndex) const
{
	return static_cast<EObjectLayer>(JoltPhysicsSystem->GetBodyInterface().GetObjectLayer(BodyID(InBodyIndex)));
}

void UJoltSubsystem::SetObjectLayer(const uint32& InBodyIndex, EObjectLayer InObjectLayer)
{
	JoltPhysicsSystem->GetBodyInterface().SetObjectLayer(BodyID(InBodyIndex), static_cast<ObjectLayer>(InObjectLayer));
}

void UJoltSubsystem::SetMotionType(const uint32& InBodyIndex, EMotionType InMotionType)
{
	JoltPhysicsSystem->GetBodyInterface().SetMotionType(BodyID(InBodyIndex), InMotionType, EActivation::Activate);
}

BodyID UJoltSubsystem::AddBox(
	const int& BodyIndex,
	const FVector& InLocation,
	const FQuat& InRotation,
	const FVector& InHalfExtent,
	const FPhysicsProperties& PhysicsProperties
)
{
	ObjectLayer BoxLayer = static_cast<ObjectLayer>(PhysicsProperties.Layer);
	if (PhysicsProperties.Layer == EObjectLayer::NoCollision)
	{
		BoxLayer = Layers::NO_COLLISION;
	}
	
	BodyCreationSettings BoxSettings(new BoxShape(
		Units::ToJoltUnits(InHalfExtent)),
		Units::ToJoltPos(InLocation),
		Units::ToJoltQuat(InRotation),
		static_cast<EMotionType>(PhysicsProperties.Motion),
		BoxLayer
	);

	BoxSettings.mMotionQuality = static_cast<EMotionQuality>(PhysicsProperties.MotionQuality);

	UpdateAllowedDOFs_Internal(BoxSettings, PhysicsProperties);
	UpdateIsSensor_Internal(BoxSettings, PhysicsProperties);
	UpdateMass_Internal(BoxSettings, PhysicsProperties);
	
	BoxSettings.mAllowSleeping = false;

	// These values don't get converted. It's easier to specify them in the intended units, rather than converting to Unreals centimeters.
	BoxSettings.mFriction = PhysicsProperties.Friction;
	BoxSettings.mRestitution = PhysicsProperties.Restitution;
	BoxSettings.mLinearDamping = PhysicsProperties.LinearDamping;
	BoxSettings.mAngularDamping = PhysicsProperties.AngularDamping;
	BoxSettings.mMaxLinearVelocity = PhysicsProperties.MaxLinearVelocity;
	BoxSettings.mMaxAngularVelocity = PhysicsProperties.MaxAngularVelocity;
	BoxSettings.mGravityFactor = PhysicsProperties.GravityFactor;

	BoxSettings.mAllowDynamicOrKinematic = PhysicsProperties.bAllowDynamicOrKinematic;

#ifdef MY_DEBUG_LOG_JOLT_CALLS
	DebugLog(FString::Printf(TEXT("Added box")));
#endif

	return CreateAndAddBody(BodyIndex, BoxSettings);
}

BodyID UJoltSubsystem::AddSphere(
	const int& BodyIndex,
	const FVector& InLocation,
	const FQuat& InRotation,
	const float& InRadius,
	const FPhysicsProperties& PhysicsProperties
)
{
	BodyCreationSettings SphereSettings(
		new SphereShape(Units::ToJoltUnits(InRadius)),
		Units::ToJoltPos(InLocation),
		Units::ToJoltQuat(InRotation),
		static_cast<EMotionType>(PhysicsProperties.Motion),
		static_cast<ObjectLayer>(PhysicsProperties.Layer)
	);

	SphereSettings.mMotionQuality = static_cast<EMotionQuality>(PhysicsProperties.MotionQuality);
		
	UpdateAllowedDOFs_Internal(SphereSettings, PhysicsProperties);
	UpdateIsSensor_Internal(SphereSettings, PhysicsProperties);
	UpdateMass_Internal(SphereSettings, PhysicsProperties);
	
	SphereSettings.mAllowSleeping = false;
	
	SphereSettings.mFriction = PhysicsProperties.Friction;
	SphereSettings.mRestitution = PhysicsProperties.Restitution;
	SphereSettings.mLinearDamping = PhysicsProperties.LinearDamping;
	SphereSettings.mAngularDamping = PhysicsProperties.AngularDamping;
	SphereSettings.mMaxLinearVelocity = PhysicsProperties.MaxLinearVelocity;
	SphereSettings.mMaxAngularVelocity = PhysicsProperties.MaxAngularVelocity;
	SphereSettings.mGravityFactor = PhysicsProperties.GravityFactor;

	SphereSettings.mAllowDynamicOrKinematic = PhysicsProperties.bAllowDynamicOrKinematic;

#ifdef MY_DEBUG_LOG_JOLT_CALLS
	DebugLog(FString::Printf(TEXT("Added sphere")));
#endif

	return CreateAndAddBody(BodyIndex, SphereSettings);
}

BodyID UJoltSubsystem::AddCapsule(
	const int& BodyIndex,
	const FVector& InLocation,
	const FQuat& InRotation,
	const float& InRadius,
	const float& InHalfHeight,
	const FPhysicsProperties& PhysicsProperties
)
{
	BodyCreationSettings CapsuleSettings(
		new CapsuleShape(Units::ToJoltUnits(InHalfHeight), Units::ToJoltUnits(InRadius)),
		Units::ToJoltPos(InLocation),
		Units::ToJoltQuat(InRotation),
		static_cast<EMotionType>(PhysicsProperties.Motion),
		static_cast<ObjectLayer>(PhysicsProperties.Layer)
	);

	CapsuleSettings.mMotionQuality = static_cast<EMotionQuality>(PhysicsProperties.MotionQuality);

	UpdateAllowedDOFs_Internal(CapsuleSettings, PhysicsProperties);
	UpdateIsSensor_Internal(CapsuleSettings, PhysicsProperties);
	UpdateMass_Internal(CapsuleSettings, PhysicsProperties);
	
	CapsuleSettings.mAllowSleeping = false;

	CapsuleSettings.mFriction = PhysicsProperties.Friction;
	CapsuleSettings.mRestitution = PhysicsProperties.Restitution;
	CapsuleSettings.mLinearDamping = PhysicsProperties.LinearDamping;
	CapsuleSettings.mAngularDamping = PhysicsProperties.AngularDamping;
	CapsuleSettings.mMaxLinearVelocity = PhysicsProperties.MaxLinearVelocity;
	CapsuleSettings.mMaxAngularVelocity = PhysicsProperties.MaxAngularVelocity;
	CapsuleSettings.mGravityFactor = PhysicsProperties.GravityFactor;

#ifdef MY_DEBUG_LOG_JOLT_CALLS
	DebugLog(FString::Printf(TEXT("Added capsule")));
#endif

	return CreateAndAddBody(BodyIndex, CapsuleSettings);
}

BodyID UJoltSubsystem::AddConvexHull(const int& BodyIndex, const FVector& InLocation, const FQuat& InRotation,
	const TArray<FVector>& InPoints, const FPhysicsProperties& PhysicsProperties)
{
	ObjectLayer ShapeLayer = static_cast<ObjectLayer>(PhysicsProperties.Layer);
	if (PhysicsProperties.Layer == EObjectLayer::NoCollision)
	{
		ShapeLayer = Layers::NO_COLLISION;
	}
	
	Array<Vec3> ConvexHullPoints;
	for (const auto& InPoint : InPoints)
	{
		ConvexHullPoints.push_back(Units::ToJoltPos(InPoint));
	}
	
	BodyCreationSettings CreationSettings(new ConvexHullShapeSettings(ConvexHullPoints, 0.05f),
		Units::ToJoltPos(InLocation),
		Units::ToJoltQuat(InRotation),
		static_cast<EMotionType>(PhysicsProperties.Motion),
		ShapeLayer
	);

	CreationSettings.mMotionQuality = static_cast<EMotionQuality>(PhysicsProperties.MotionQuality);

	UpdateAllowedDOFs_Internal(CreationSettings, PhysicsProperties);
	UpdateIsSensor_Internal(CreationSettings, PhysicsProperties);
	UpdateMass_Internal(CreationSettings, PhysicsProperties);
	
	CreationSettings.mAllowSleeping = false;

	// These values don't get converted. It's easier to specify them in the intended units, rather than converting to Unreals centimeters.
	CreationSettings.mFriction = PhysicsProperties.Friction;
	CreationSettings.mRestitution = PhysicsProperties.Restitution;
	CreationSettings.mLinearDamping = PhysicsProperties.LinearDamping;
	CreationSettings.mAngularDamping = PhysicsProperties.AngularDamping;
	CreationSettings.mMaxLinearVelocity = PhysicsProperties.MaxLinearVelocity;
	CreationSettings.mMaxAngularVelocity = PhysicsProperties.MaxAngularVelocity;
	CreationSettings.mGravityFactor = PhysicsProperties.GravityFactor;

	CreationSettings.mAllowDynamicOrKinematic = PhysicsProperties.bAllowDynamicOrKinematic;

#ifdef MY_DEBUG_LOG_JOLT_CALLS
	DebugLog(FString::Printf(TEXT("Added convex hull")));
#endif

	return CreateAndAddBody(BodyIndex, CreationSettings);
}

BodyID UJoltSubsystem::AddStaticCompound(const int& BodyIndex, const FVector& InLocation, const FQuat& InRotation,
	TArray<TJoltBodySettings> JoltBodySettings, const FPhysicsProperties& PhysicsProperties)
{
	Ref<StaticCompoundShapeSettings> CompoundShapeSettings = new StaticCompoundShapeSettings();
	for (const TJoltBodySettings& BodySettings : JoltBodySettings)
	{
		if (BodySettings.JoltBodyShape == EJoltBodyShape::Box)
		{
			CompoundShapeSettings->AddShape(Units::ToJoltPos(BodySettings.Location), Units::ToJoltQuat(BodySettings.Rotation), new BoxShape(Units::ToJoltUnits(BodySettings.BoxExtent)));
			continue;
		}

		if (BodySettings.JoltBodyShape == EJoltBodyShape::Sphere)
		{
			CompoundShapeSettings->AddShape(Units::ToJoltPos(BodySettings.Location), Units::ToJoltQuat(BodySettings.Rotation), new SphereShape(Units::ToJoltUnits(BodySettings.SphereRadius)));
			continue;
		}

		if (BodySettings.JoltBodyShape == EJoltBodyShape::ConvexHull)
		{
			Array<Vec3> ConvexHullPoints;
			for (const auto& InPoint : BodySettings.ConvexHullPoints)
			{
				ConvexHullPoints.push_back(Units::ToJoltPos(InPoint));
			}
	
			CompoundShapeSettings->AddShape(Units::ToJoltPos(BodySettings.Location), Units::ToJoltQuat(BodySettings.Rotation), new ConvexHullShapeSettings(ConvexHullPoints, 0.05f));
			continue;
		}

		checkNoEntry();
	}
	
	BodyCreationSettings BodySettings(
		CompoundShapeSettings,
		Units::ToJoltPos(InLocation),
		Units::ToJoltQuat(InRotation),
		static_cast<EMotionType>(PhysicsProperties.Motion),
		static_cast<ObjectLayer>(PhysicsProperties.Layer)
	);

	BodySettings.mMotionQuality = static_cast<EMotionQuality>(PhysicsProperties.MotionQuality);

	UpdateAllowedDOFs_Internal(BodySettings, PhysicsProperties);
	UpdateIsSensor_Internal(BodySettings, PhysicsProperties);
	UpdateMass_Internal(BodySettings, PhysicsProperties);
	
	BodySettings.mAllowSleeping = false;

	BodySettings.mFriction = PhysicsProperties.Friction;
	BodySettings.mRestitution = PhysicsProperties.Restitution;
	BodySettings.mLinearDamping = PhysicsProperties.LinearDamping;
	BodySettings.mAngularDamping = PhysicsProperties.AngularDamping;
	BodySettings.mMaxLinearVelocity = PhysicsProperties.MaxLinearVelocity;
	BodySettings.mMaxAngularVelocity = PhysicsProperties.MaxAngularVelocity;
	BodySettings.mGravityFactor = PhysicsProperties.GravityFactor;

#ifdef MY_DEBUG_LOG_JOLT_CALLS
	DebugLog(FString::Printf(TEXT("Added static compound")));
#endif

	return CreateAndAddBody(BodyIndex, BodySettings);
}

void UJoltSubsystem::AddConstraint(Constraint* InConstraint)
{
	JoltPhysicsSystem->AddConstraint(InConstraint);
	LOG(TEXT("[JoltSubsystem] Added constraint. Total constraints num %d"), JoltPhysicsSystem->GetConstraints().size());

#ifdef MY_DEBUG_LOG_JOLT_CALLS
	DebugLog(FString::Printf(TEXT("Add constraint. Total constraints num %d"), JoltPhysicsSystem->GetConstraints().size()));
#endif
}

void UJoltSubsystem::RemoveConstraint(Constraint* InConstraint)
{
	JoltPhysicsSystem->RemoveConstraint(InConstraint);
	
	LOG(TEXT("[JoltSubsystem] Removed constraint. Total constraints num %d"), JoltPhysicsSystem->GetConstraints().size());

#ifdef MY_DEBUG_LOG_JOLT_CALLS
	DebugLog(FString::Printf(TEXT("Remove constraint. Total constraints num %d"), JoltPhysicsSystem->GetConstraints().size()));
#endif
}

void UJoltSubsystem::AddBody(const BodyID& InBodyID)
{
	auto& BodyInterface = JoltPhysicsSystem->GetBodyInterface();
	check(!InBodyID.IsInvalid());
	
	BodyInterface.AddBody(InBodyID, EActivation::Activate);
	LOG(TEXT("[JoltSubsystem] Added body with ID %d"), InBodyID.GetIndex());
	
#ifdef MY_DEBUG_LOG_JOLT_CALLS
	DebugLog(FString::Printf(TEXT("Added body with ID %d"), InBodyID.GetIndex()));
#endif
}

void UJoltSubsystem::RemoveBody(const BodyID& InBodyID)
{
	auto& BodyInterface = JoltPhysicsSystem->GetBodyInterface();
	BodyInterface.RemoveBody(InBodyID);
	LOG(TEXT("[JoltSubsystem] Removed body with ID %d"), InBodyID.GetIndex());
	
#ifdef MY_DEBUG_LOG_JOLT_CALLS
	DebugLog(FString::Printf(TEXT("Removed body with ID %d"), InBodyID.GetIndex()));
#endif
}

void UJoltSubsystem::DestroyBody(const BodyID& InBodyID)
{
	DestroyBody_internal(InBodyID);
	LOG(TEXT("[JoltSubsystem] Destroyed body with ID %d"), InBodyID.GetIndex());

#ifdef MY_DEBUG_LOG_JOLT_CALLS
	DebugLog(FString::Printf(TEXT("Destroyed body with ID %d"), InBodyID.GetIndex()));
#endif
}

void UJoltSubsystem::RemoveAndDestroyBody(const BodyID& InBodyID)
{
	auto& BodyInterface = JoltPhysicsSystem->GetBodyInterface();
	BodyInterface.RemoveBody(InBodyID);
	DestroyBody_internal(InBodyID);
	LOG(TEXT("[JoltSubsystem] Removed and destroyed body with ID %d"), InBodyID.GetIndex());

#ifdef MY_DEBUG_LOG_JOLT_CALLS
	DebugLog(FString::Printf(TEXT("Removed and destroyed body with ID %d"), InBodyID.GetIndex()));
#endif
}

bool UJoltSubsystem::IsRandomNumberGeneratorInitialized() const
{
	return bRandomNumberGeneratorInitialized;
}

void UJoltSubsystem::InitRandomNumberGenerator(const uint64_t& InSeed)
{;
	//LOG_TEMPORARY(TEXT("Init random nubmer generator: %llu"), InSeed)
	
	RandomNumberGenerator.Seed(InSeed);
	bRandomNumberGeneratorInitialized = true;
}

BodyID UJoltSubsystem::CreateAndAddBody(const int& BodyIndex, const BodyCreationSettings& BodySettings)
{
	auto& BodyInterface = JoltPhysicsSystem->GetBodyInterface();
	
	const Body* NewBody = BodyInterface.CreateBodyWithID(BodyID(BodyIndex), BodySettings);
	check(NewBody);
	
	BodyInterface.AddBody(NewBody->GetID(), EActivation::Activate);
	LOG(TEXT("[JoltSubsystem] Created and added body with ID %d"), NewBody->GetID().GetIndex());
	
#ifdef MY_DEBUG_LOG_JOLT_CALLS
	DebugLog(FString::Printf(TEXT("Created and added body with ID %d"), NewBody->GetID().GetIndex()));
#endif

	return NewBody->GetID();
}

void UJoltSubsystem::DestroyBody_internal(const BodyID& InBodyID)
{
	GetBodyInterface()->DestroyBody(InBodyID);
}

void UJoltSubsystem::UpdateAllowedDOFs_Internal(BodyCreationSettings& BodySettings,
	const FPhysicsProperties& PhysicsProperties)
{
	BodySettings.mAllowedDOFs = EAllowedDOFs::Plane2D;
	if (PhysicsProperties.bDisableRotation)
	{
		BodySettings.mAllowedDOFs = EAllowedDOFs::TranslationX | EAllowedDOFs::TranslationY;
	}
}

void UJoltSubsystem::UpdateIsSensor_Internal(BodyCreationSettings& BodySettings,
	const FPhysicsProperties& PhysicsProperties)
{
	BodySettings.mIsSensor = PhysicsProperties.bIsSensor;
}

void UJoltSubsystem::UpdateMass_Internal(BodyCreationSettings& BodySettings,
	const FPhysicsProperties& PhysicsProperties)
{
	BodySettings.mOverrideMassProperties = EOverrideMassProperties::CalculateMassAndInertia;
	
	if (PhysicsProperties.bOverrideMass)
	{
		BodySettings.mOverrideMassProperties = EOverrideMassProperties::CalculateInertia;
		BodySettings.mMassPropertiesOverride = MassProperties(PhysicsProperties.Mass);
	}
}

#ifdef MY_DEBUG_LOG_JOLT_CALLS
void UJoltSubsystem::PrepareToLogFrame(const int& InFrameId)
{
	CurrentlyLoggingFrameId = InFrameId;
	if (DebugJoltFrames.Contains(InFrameId))
	{
		DebugJoltFrames[InFrameId].Logs.Empty();
	}
	else
	{
		DebugJoltFrames.Add(InFrameId);
	}
}

void UJoltSubsystem::DebugLog(FString Message)
{
	DebugJoltFrames[CurrentlyLoggingFrameId].Logs.Add(Message);
}

void UJoltSubsystem::DumpToLogs()
{
	const bool bIsServer = GetWorld()->GetNetMode() == NM_ListenServer;
	
	if (bIsServer)
	{
		DELETE_LOG_FILE_IF_EXISTS("ServerJolt.txt");
	}
	else
	{
		DELETE_LOG_FILE_IF_EXISTS("ClientJolt.txt");
	}
	
	for (auto& pair : DebugJoltFrames)
	{
		const int& DebugFrameId = pair.Key;
		const auto& FrameLogs = pair.Value;

		for (int i = 0; i < FrameLogs.Logs.Num(); i++)
		{
			if (bIsServer)
			{
				LOG_TO_FILE("ServerJolt.txt", TEXT("Frame %d; %s"), DebugFrameId, *FrameLogs.Logs[i]);
			}
			else
			{
				LOG_TO_FILE("ClientJolt.txt", TEXT("Frame %d; %s"), DebugFrameId, *FrameLogs.Logs[i]);
			}
		}
	}
}
#endif
