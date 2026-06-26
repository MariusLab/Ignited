// Fill out your copyright notice in the Description page of Project Settings.


#include "JoltCapsuleComponent.h"
#include "JoltPhysicsTest/Debug/CustomLogger.h"
#include "JoltPhysicsTest/Jolt/JoltConverter.h"
#include "JoltPhysicsTest/Jolt/JoltSubsystem.h"


UJoltCapsuleComponent::UJoltCapsuleComponent()
{
	CapsuleRadius = 50.f;
	CapsuleHalfHeight = 200.f;
	CapsuleColor = FColor::Blue;
	LineThickness = 100.f;

	bHiddenInGame = false;
	bNeverDistanceCull = true;
	bUseEditorCompositing = true;
}

void UJoltCapsuleComponent::InitBody(const FJoltBodyData& JoltBodyData)
{
	Super::InitBody(JoltBodyData);

	PhysicsProperties = GetPhysicsProperties();
#ifdef MY_DEBUG_RENDERING
		OverwriteDebugDraw();
#endif
	
	auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();
	JoltBodyID = JoltSubsystem->AddCapsule(JoltBodyData.BodyIndex, GetComponentLocation(), GetComponentQuat(), CapsuleRadius, CapsuleHalfHeight, PhysicsProperties);
	CenterOfMassLocation = JoltSubsystem->GetCenterOfMassPosition(JoltBodyID);
}

FPhysicsProperties UJoltCapsuleComponent::GetPhysicsProperties() const
{
	if (PhysicsPropertiesDataAsset)
	{
		return PhysicsPropertiesDataAsset->PhysicsProperties;
	}

	return PhysicsProperties;
}

#ifdef MY_DEBUG_RENDERING
void UJoltCapsuleComponent::OverwriteDebugDraw()
{
	if (PhysicsPropertiesDataAsset && PhysicsPropertiesDataAsset->bForceDebugDraw)
	{
		bDebugDraw = true;
	}
}
#endif

void UJoltCapsuleComponent::OnRender()
{
	if (bRemovedFromSimulationLastRenderFrameValue != bRemovedFromSimulation)
	{
		if (bRemovedFromSimulation)
		{
			SetVisibility(false, true);
		}
		else
		{
			SetVisibility(true, true);
		}
	}

	// We purposely run the code above before calling the parent, because the parent overwrites bRemovedFromSimulationLastRenderFrameValue
	Super::OnRender();
	
#ifdef MY_DEBUG_RENDERING
	if (bDebugDraw)
	{
		auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();
		DrawDebugString(GetWorld(), CenterOfMassLocation, FString::Printf(TEXT("ID: %d"), JoltBodyID.GetIndex()), 0, FColor::White, 0.02f);

		if (PhysicsProperties.Motion == EObjectMotion::Dynamic)
		{
			const auto VelocityMagnitude = Units::FromJoltPos(JoltSubsystem->GetBodyInterface()->GetLinearVelocity(JoltBodyID)).Size();
			const auto VelocityDirection = Units::FromJoltPos(JoltSubsystem->GetBodyInterface()->GetLinearVelocity(JoltBodyID)).GetSafeNormal();
			DrawDebugLine(GetWorld(), CenterOfMassLocation, CenterOfMassLocation + VelocityDirection * VelocityMagnitude, FColor::Yellow, false, 0.02f, SDPG_World, 10.f);
		}
	}
#endif
}


#ifdef MY_DEBUG_RENDERING
FBoxSphereBounds UJoltCapsuleComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	return FBoxSphereBounds( FVector::ZeroVector, FVector(CapsuleRadius), CapsuleRadius ).TransformBy(LocalToWorld);
}

FPrimitiveSceneProxy* UJoltCapsuleComponent::CreateSceneProxy()
{
	class FCapsuleSceneProxy final : public FPrimitiveSceneProxy
	{
	public:
		SIZE_T GetTypeHash() const override
		{			
			static size_t UniquePointer;
			return reinterpret_cast<size_t>(&UniquePointer);
		}

		FCapsuleSceneProxy(UJoltCapsuleComponent* InComponent)
			:	FPrimitiveSceneProxy(InComponent)
			,   CapsuleRadius( InComponent->CapsuleRadius )
			,   CapsuleHalfHeight( InComponent->CapsuleHalfHeight )
			,	CapsuleColor( InComponent->CapsuleColor )
			,	LineThickness( InComponent->LineThickness )
			,	JoltCapsuleComponent( InComponent )
		{
			bWillEverBeLit = false;
		}

		virtual bool IsDrawnInGame() const override
		{
			return true;
		}

		virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override
		{
			if (JoltCapsuleComponent.IsStale() || !JoltCapsuleComponent->ShouldDrawPrimitive())
			{
				return;
			}
			
			const FMatrix& LocalToWorld = GetLocalToWorld();
			
			for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
			{
				if (VisibilityMap & (1 << ViewIndex))
				{
					const FSceneView* View = Views[ViewIndex];

					const FLinearColor DrawColor = GetViewSelectionColor(CapsuleColor, *View, IsSelected(), IsHovered(), false, IsIndividuallySelected() );

					FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);
					DrawWireCapsule(PDI, LocalToWorld.GetOrigin(), LocalToWorld.GetScaledAxis( EAxis::X ), LocalToWorld.GetScaledAxis( EAxis::Y ), LocalToWorld.GetScaledAxis( EAxis::Z ), DrawColor, CapsuleRadius, CapsuleHalfHeight, 16, SDPG_World, LineThickness);
					DrawWireStar(PDI, JoltCapsuleComponent->CenterOfMassLocation, 15.f, FColor::Red, SDPG_World);
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
		const float CapsuleRadius;
		const float CapsuleHalfHeight;
		const FColor CapsuleColor;
		const float LineThickness;
		TWeakObjectPtr<UJoltCapsuleComponent> JoltCapsuleComponent = nullptr;
	};

	return new FCapsuleSceneProxy( this );
}
#endif
