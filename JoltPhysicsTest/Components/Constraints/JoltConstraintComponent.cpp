// Fill out your copyright notice in the Description page of Project Settings.


#include "JoltConstraintComponent.h"

#include "JoltPhysicsTest/Jolt/JoltSubsystem.h"

UJoltConstraintComponent::UJoltConstraintComponent()
{
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

	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UJoltConstraintComponent::PreInitialize(TArray<UJoltBodyComponent*> BodyComponents)
{
	if (bConstrainToImmovableWorld)
	{
		// If body 2 was already provided, then we don't need to look for it
		if (!ConstraintData.BodyID2.IsInvalid())
		{
			return;
		}
		
		check(SpawnOrderOfBody2 >= 0);

		UJoltBodyComponent* BodyComponent2 = nullptr;
		for (const auto BodyComponent : BodyComponents)
		{
			if (BodyComponent->GetSpawnOrder() == SpawnOrderOfBody2)
			{
				BodyComponent2 = BodyComponent;
			}
		}

		check(BodyComponent2);

		ConstraintData.BodyID2 = BodyComponent2->GetBodyID();
		ConstraintData.bConstrainToImmovableWorld = true;
		
		return;
	}

	
	// If both bodies were already provided, then we don't need to look for them
	if (!ConstraintData.BodyID1.IsInvalid() && !ConstraintData.BodyID2.IsInvalid())
	{
		return;
	}

	check(SpawnOrderOfBody1 >= 0);
	check(SpawnOrderOfBody2 >= 0);
	check(SpawnOrderOfBody1 != SpawnOrderOfBody2);

	UJoltBodyComponent* BodyComponent2 = nullptr;

	for (const auto BodyComponent : BodyComponents)
	{
		if (BodyComponent->GetSpawnOrder() == SpawnOrderOfBody1)
		{
			BodyComponent1 = BodyComponent;
		}

		if (BodyComponent->GetSpawnOrder() == SpawnOrderOfBody2)
		{
			BodyComponent2 = BodyComponent;
		}
	}

	check(BodyComponent1);
	check(BodyComponent2);

	ConstraintData.BodyID1 = BodyComponent1->GetBodyID();
	ConstraintData.BodyID2 = BodyComponent2->GetBodyID();
}

void UJoltConstraintComponent::StepSimulation()
{
}

void UJoltConstraintComponent::InitConstraint(const FJoltConstraintData& InConstraintData)
{
	if (bDisabled)
	{
		return;
	}

#ifdef DEBUG_RENDER_ALL_BODIES
	bDebugDraw = true;
#endif

	check(!bConstraintInitialized);
	bConstraintInitialized = true;
	bDestroyed = false;

	ConstraintData = InConstraintData;

#ifdef MY_DEBUG_RENDERING
	// Have the constraint debug rendering follow the shapes
	AttachToComponent(BodyComponent1, FAttachmentTransformRules::KeepWorldTransform);
#endif

	InitConstraintComponent();
}

void UJoltConstraintComponent::InitConstraintComponent()
{
}

void UJoltConstraintComponent::SaveConstraint(StateRecorderImpl& InStream)
{
}

void UJoltConstraintComponent::LoadConstraint(StateRecorderImpl& InStream)
{
}

void UJoltConstraintComponent::RecreateConstraint()
{
	check(!JoltConstraint);
	check(bConstraintInitialized);
	check(bDestroyed);
	
	bDestroyed = false;
}

void UJoltConstraintComponent::DestroyConstraint()
{
	check(JoltConstraint);
	check(bConstraintInitialized);
	check(!bDestroyed);

	JoltConstraint->GetConstraintSettings()->SaveBinaryState(ConstraintSettingsRecorded);

	auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();
	JoltSubsystem->RemoveConstraint(JoltConstraint);

	// We don't need to delete, because it uses RefTarget<Constraint> for reference counting and will free the memory once it gets destroyed
	JoltConstraint = nullptr;
	
	bDestroyed = true;
}

void UJoltConstraintComponent::EnableConstraint()
{
	check(JoltConstraint);
	JoltConstraint->SetEnabled(true);
}

void UJoltConstraintComponent::DisableConstraint()
{
	check(JoltConstraint);
	JoltConstraint->SetEnabled(false);
}

bool UJoltConstraintComponent::ShouldDrawPrimitive() const
{
	return !bDestroyed && (bDebugDraw || GetWorld()->WorldType == EWorldType::Editor || GetWorld()->WorldType == EWorldType::EditorPreview);
}

void UJoltConstraintComponent::Save(StateRecorderImpl& InStream)
{
	if (bDisabled)
	{
		return;
	}
	
	JoltConstraint->SaveState(InStream);
	SaveConstraint(InStream);
}

void UJoltConstraintComponent::Load(StateRecorderImpl& InStream)
{
	if (bDisabled)
	{
		return;
	}
	
	JoltConstraint->RestoreState(InStream);
	LoadConstraint(InStream);
}

bool UJoltConstraintComponent::IsConstraintInitialized() const
{
	return bConstraintInitialized;
}

Constraint* UJoltConstraintComponent::GetConstraintInstance()
{
	check(JoltConstraint);
	
	return JoltConstraint;
}

FJoltConstraintData UJoltConstraintComponent::GetConstraintData() const
{
	return ConstraintData;
}

bool UJoltConstraintComponent::IsConstraintDestroyed() const
{
	return bDestroyed;
}

void UJoltConstraintComponent::UpdateCollisionBetweenBodies(Body* Body1, Body* Body2)
{
	if (bDisableCollisionBetweenBodies)
	{
		Ref<GroupFilterTable> DisableCollisionFilter = new GroupFilterTable();

		// We know constraint priority will always be unique for each constraint component. Because it basically functions like an ID
		// This ensures we only disable collision between these 2 bodies and not with some other bodies who happen to be in the same GroupID
		Body1->SetCollisionGroup(CollisionGroup(DisableCollisionFilter, ConstraintData.ConstraintPriority, 0));
		Body2->SetCollisionGroup(CollisionGroup(DisableCollisionFilter, ConstraintData.ConstraintPriority, 0));
	}
}

#ifdef MY_DEBUG_RENDERING
FBoxSphereBounds UJoltConstraintComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	return FBoxSphereBounds( FVector::ZeroVector, FVector(5000.f), 5000.f).TransformBy(LocalToWorld);
}
#endif
