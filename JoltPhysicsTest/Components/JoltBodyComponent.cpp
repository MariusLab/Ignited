// Fill out your copyright notice in the Description page of Project Settings.

#include "JoltBodyComponent.h"

#include "JoltPhysicsTest/Game/GameLoopSubsystem.h"
#include "JoltPhysicsTest/Jolt/JoltBodyActor.h"
#include "JoltPhysicsTest/Jolt/JoltContainerActor.h"
#include "JoltPhysicsTest/Jolt/JoltSubsystem.h"

UJoltBodyComponent::UJoltBodyComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	
	bCanEverAffectNavigation = false;
	bMultiBodyOverlap = false;
	bApplyImpulseOnDamage = false;
	bReplicatePhysicsToAutonomousProxy = false;
	BodyInstance.bEnableGravity = false;
	BodyInstance.bAutoWeld = false;
	BodyInstance.bUpdateMassWhenScaleChanges = false;
	BodyInstance.bSimulatePhysics = false;
	BodyInstance.bEnableGravity = false;
	BodyInstance.SetCollisionProfileNameDeferred("NoCollision");
	
	bCastStaticShadow = false;
	bCastCinematicShadow = false;
	bCastHiddenShadow = false;
	bCastContactShadow = false;
	CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;

	CastShadow = false;
	bAffectDynamicIndirectLighting = false;
	bAffectDistanceFieldLighting = false;
	bCastDynamicShadow = false;
}

void UJoltBodyComponent::OnRegister()
{
	Super::OnRegister();

#if WITH_EDITOR
	// Automatically set SpawnOrder to a higher value than all other body components in this container
	AJoltContainerActor* JoltContainerActor = Cast<AJoltContainerActor>(GetOwner());
	if (!JoltContainerActor)
	{
		return;
	}
	
	check(JoltContainerActor);

	int HighestSpawnOrderFound = 0;
	bool bOtherBodyHasSameGeneratedId = false;

	TArray<UJoltBodyComponent*> JoltBodyComponents;
	JoltContainerActor->GetComponents(UJoltBodyComponent::StaticClass(), JoltBodyComponents);
	for (auto BodyComponent : JoltBodyComponents)
	{
		if (BodyComponent->GetUniqueID() == GetUniqueID())
		{
			continue;
		}
		
		if (BodyComponent->GeneratedSpawnOrder > HighestSpawnOrderFound)
		{
			HighestSpawnOrderFound = BodyComponent->GeneratedSpawnOrder;
		}

		if (BodyComponent->GeneratedSpawnOrder == GeneratedSpawnOrder)
		{
			bOtherBodyHasSameGeneratedId = true;
		}
	}

	if (bOtherBodyHasSameGeneratedId)
	{
		GeneratedSpawnOrder = HighestSpawnOrderFound + 1;
	}
#endif
}

void UJoltBodyComponent::OnRender()
{
	bRemovedFromSimulationLastRenderFrameValue = bRemovedFromSimulation;
	if (bRemovedFromSimulation)
	{
		return;
	}

	const auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();
	
	if (bUpdateComponentPositionFromJoltPhysics)
	{
		CenterOfMassLocation = JoltSubsystem->GetCenterOfMassPosition(JoltBodyID);
		SetWorldLocation(CenterOfMassLocation);
		SetWorldRotation(JoltSubsystem->GetBodyRotation(JoltBodyID));
	}
}
                                                                                                                                                                        
void UJoltBodyComponent::InitBody(const FJoltBodyData& JoltBodyData)
{
	check(!bBodyInitialized);
	bBodyInitialized = true;
	bRemovedFromSimulation = false;
	bDestroyed = false;

	auto World = GetWorld();
	check(World);
	auto GameLoopSubsystem = GetWorld()->GetSubsystem<UGameLoopSubsystem>();
	check(GameLoopSubsystem);
	GameLoopSubsystem->OnRenderFrame.AddUniqueDynamic(this, &UJoltBodyComponent::OnRender);

	SetVisibility(false, true);

#ifdef DEBUG_RENDER_ALL_BODIES
	bDebugDraw = true;
#endif
}

void UJoltBodyComponent::AddBody()
{
	check(!JoltBodyID.IsInvalid());
	check(bBodyInitialized);
	check(bRemovedFromSimulation);

	auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();
	JoltSubsystem->AddBody(JoltBodyID);
	bRemovedFromSimulation = false;
}

void UJoltBodyComponent::RemoveBody()
{
	check(!JoltBodyID.IsInvalid());
	check(bBodyInitialized);
	check(!bRemovedFromSimulation);

	auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();
	JoltSubsystem->RemoveBody(JoltBodyID);
	bRemovedFromSimulation = true;
}

void UJoltBodyComponent::DestroyBody()
{
	check(!JoltBodyID.IsInvalid());
	check(bBodyInitialized);
	check(bRemovedFromSimulation);
	check(!bDestroyed);

	auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();
	JoltSubsystem->DestroyBody(JoltBodyID);
	bBodyInitialized = false;
	bDestroyed = true;
}

void UJoltBodyComponent::RemoveAndDestroyBody()
{
	check(!JoltBodyID.IsInvalid());
	check(bBodyInitialized);
	check(!bDestroyed);

	auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();
	JoltSubsystem->RemoveAndDestroyBody(JoltBodyID);
	bBodyInitialized = false;
	bRemovedFromSimulation = true;
	bDestroyed = true;
}

bool UJoltBodyComponent::ShouldDrawPrimitive() const
{
	return !bDestroyed && !bRemovedFromSimulation && (bDebugDraw || GetWorld()->WorldType == EWorldType::Editor || GetWorld()->WorldType == EWorldType::EditorPreview);
}

BodyID UJoltBodyComponent::GetBodyID()
{
	return JoltBodyID;
}

bool UJoltBodyComponent::IsBodyDestroyed() const
{
	return bDestroyed;
}

void UJoltBodyComponent::StepSimulation()
{
	if (bOrientTowardsVelocity && !bRemovedFromSimulation)
	{
		auto JoltSubSystem = GetWorld()->GetSubsystem<UJoltSubsystem>();
		const auto VelDirection = JoltSubSystem->GetBodyInterface()->GetLinearVelocity(JoltBodyID).NormalizedOr(Vec3::sZero());
		const float TargetAngleInRadians = JPH::ATan2(VelDirection.GetX(), VelDirection.GetY()) * -1.f;
		
		if (!VelDirection.IsNearZero())
		{
			JoltSubSystem->GetBodyInterface()->SetRotation(JoltBodyID, Quat::sRotation(Vec3::sAxisZ(), TargetAngleInRadians), EActivation::Activate);
		}
	}
}

int UJoltBodyComponent::GetSpawnOrder() const
{
	if (SpawnOrder == -1)
	{
		return GeneratedSpawnOrder;
	}

	return SpawnOrder;
}

TArray<FVector2D> UJoltBodyComponent::GetAttachedMeshVertices() const
{
	return LocalPositionsToWorld(ShapeVertices);
}

UClass* UJoltBodyComponent::GetJoltBodyClass() const
{
	if (JoltBodyClass.Get())
	{
		return JoltBodyClass.Get();
	}
	
	return AJoltBodyActor::StaticClass();
}

FVector2D UJoltBodyComponent::LocalPosToWorld(const float& PosX, const float& PosY) const
{
	const FVector TransformedPosition = GetComponentTransform().TransformPosition(FVector(PosX, 0, PosY));

	return FVector2D(TransformedPosition.X, TransformedPosition.Z);
}

TArray<FVector2D> UJoltBodyComponent::LocalPositionsToWorld(const TArray<FVector2D>& Positions) const
{
	TArray<FVector2D> WorldPositions;

	const auto ComponentTransform = GetComponentTransform();
	for (const auto& Position : Positions)
	{
		const auto TransformedPosition = ComponentTransform.TransformPosition(FVector(Position.X, 0, Position.Y));
		WorldPositions.Add(FVector2D(TransformedPosition.X, TransformedPosition.Z));
	}

	return WorldPositions;
}
