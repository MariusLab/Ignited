// Fill out your copyright notice in the Description page of Project Settings.


#include "JoltPointConstraint.h"

#include "JoltPhysicsTest/Jolt/JoltConverter.h"
#include "JoltPhysicsTest/Jolt/JoltBodyActor.h"
#include "JoltPhysicsTest/Jolt/JoltSubsystem.h"

void UJoltPointConstraint::InitConstraintComponent()
{
	auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();

	auto Body1 = JoltSubsystem->GetPhysicsSystem()->GetBodyLockInterface().TryGetBody(ConstraintData.BodyID1);
	auto Body2 = JoltSubsystem->GetPhysicsSystem()->GetBodyLockInterface().TryGetBody(ConstraintData.BodyID2);

	UpdateCollisionBetweenBodies(Body1, Body2);

	PointConstraintSettings Settings;
	Settings.mConstraintPriority = ConstraintData.ConstraintPriority;
	Settings.mPoint1 = Settings.mPoint2 = Units::ToJoltPos(GetComponentLocation());

	JoltConstraint = Settings.Create(*Body1, *Body2);
	JoltSubsystem->AddConstraint(JoltConstraint);
}

void UJoltPointConstraint::RecreateConstraint()
{
	Super::RecreateConstraint();

	auto Result = ConstraintSettings::sRestoreFromBinaryState(ConstraintSettingsRecorded);
	check(!Result.HasError());

	auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();
	auto Body1 = JoltSubsystem->GetPhysicsSystem()->GetBodyLockInterfaceNoLock().TryGetBody(ConstraintData.BodyID1);
	auto Body2 = JoltSubsystem->GetPhysicsSystem()->GetBodyLockInterfaceNoLock().TryGetBody(ConstraintData.BodyID2);
	check(Body1);
	check(Body2);
	
	UpdateCollisionBetweenBodies(Body1, Body2);

	JoltConstraint = static_cast<PointConstraintSettings*>(Result.Get().GetPtr())->Create(*Body1, *Body2);
	JoltSubsystem->AddConstraint(JoltConstraint);
}

#ifdef MY_DEBUG_RENDERING
FPrimitiveSceneProxy* UJoltPointConstraint::CreateSceneProxy()
{
	class FConstraintSceneProxy final : public FPrimitiveSceneProxy
	{
	public:
		SIZE_T GetTypeHash() const override
		{			
			static size_t UniquePointer;
			return reinterpret_cast<size_t>(&UniquePointer);
		}

		FConstraintSceneProxy(UJoltPointConstraint* InComponent)
			:	FPrimitiveSceneProxy(InComponent)
			,	JoltPointConstraint( InComponent )
		{
			bWillEverBeLit = false;
		}

		virtual bool IsDrawnInGame() const override
		{
			return true;
		}

		virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override
		{
			if (JoltPointConstraint.IsStale() || !JoltPointConstraint->ShouldDrawPrimitive())
			{
				return;
			}
			
			const FMatrix& LocalToWorld = GetLocalToWorld();
			
			for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
			{
				if (VisibilityMap & (1 << ViewIndex))
				{
					const FSceneView* View = Views[ViewIndex];

					const FLinearColor DrawColor = GetViewSelectionColor(FColor::Red, *View, IsSelected(), IsHovered(), false, IsIndividuallySelected() );

					FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);
					DrawWireSphere(PDI, LocalToWorld.GetOrigin(), DrawColor, 30.f, 16, SDPG_World, 10.f);
				}
			}
		}

		virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override
		{
			FPrimitiveViewRelevance Result;
			Result.bDrawRelevance = true;
			Result.bDynamicRelevance = true;
			Result.bShadowRelevance = false;
			return Result;
		}
		
		virtual uint32 GetMemoryFootprint( void ) const override { return( sizeof( *this ) + GetAllocatedSize() ); }
		uint32 GetAllocatedSize( void ) const { return( FPrimitiveSceneProxy::GetAllocatedSize() ); }

	private:
		TWeakObjectPtr<UJoltPointConstraint> JoltPointConstraint = nullptr;
	};

	return new FConstraintSceneProxy( this );
}
#endif