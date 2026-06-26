// Fill out your copyright notice in the Description page of Project Settings.


#include "JoltConvexHullComponent.h"

#include "JoltPhysicsTest/Debug/CustomLogger.h"
#include "JoltPhysicsTest/Jolt/JoltConverter.h"
#include "JoltPhysicsTest/Jolt/JoltSubsystem.h"

UJoltConvexHullComponent::UJoltConvexHullComponent()
{
	Points = {
		FVector2D(0.f, 0.f),
		FVector2D(3000.f, 0.f),
		FVector2D(3000.f, -3000.f),
		FVector2D(0.f, -3000.f),
	};

	bHiddenInGame = false;
	bNeverDistanceCull = true;
	bUseEditorCompositing = true;

	// Jolt cannot calculate mass automatically for shapes like Convex hull, triangle, mesh etc.
	PhysicsProperties.bOverrideMass = 1;
	PhysicsProperties.Mass = 100;
}

void UJoltConvexHullComponent::InitBody(const FJoltBodyData& JoltBodyData)
{
	Super::InitBody(JoltBodyData);

	PhysicsProperties = GetPhysicsProperties();
	PhysicsProperties.bAllowDynamicOrKinematic = bAllowDynamicOrKinematic;
	
#ifdef MY_DEBUG_RENDERING
	OverwriteDebugDraw();
#endif
	
	auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();
	JoltBodyID = JoltSubsystem->AddConvexHull(JoltBodyData.BodyIndex, GetComponentLocation(), GetComponentQuat(), GetConvexHullPointsForJolt(), PhysicsProperties);
}

FPhysicsProperties UJoltConvexHullComponent::GetPhysicsProperties() const
{
	if (PhysicsPropertiesDataAsset)
	{
		return PhysicsPropertiesDataAsset->PhysicsProperties;
	}

	return PhysicsProperties;
}

#ifdef MY_DEBUG_RENDERING
void UJoltConvexHullComponent::OverwriteDebugDraw()
{
	if (PhysicsPropertiesDataAsset && PhysicsPropertiesDataAsset->bForceDebugDraw)
	{
		bDebugDraw = true;
	}
}
#endif

void UJoltConvexHullComponent::OnRender()
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

	bRemovedFromSimulationLastRenderFrameValue = bRemovedFromSimulation;
	if (bRemovedFromSimulation)
	{
		return;
	}

	const auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();

	if (bUpdateComponentPositionFromJoltPhysics)
	{
		// Usually with other shapes we use shapes center of mass as the components location
		// However for convex hull that screws everything up. Instead we use the first convex hull point as the comp location
		FVector BodyLocation = JoltSubsystem->GetBodyLocation(JoltBodyID);
		SetWorldLocation(BodyLocation);
		SetWorldRotation(JoltSubsystem->GetBodyRotation(JoltBodyID));
	}

#ifdef MY_DEBUG_RENDERING
	if (bDebugDraw)
	{
		CenterOfMassLocation = JoltSubsystem->GetCenterOfMassPosition(JoltBodyID);
		DrawDebugString(GetWorld(), CenterOfMassLocation, FString::Printf(TEXT("ID: %d"), JoltBodyID.GetIndex()), 0, FColor::White, 0.016f);

		if (PhysicsProperties.Motion == EObjectMotion::Dynamic)
		{
			const auto VelocityMagnitude = Units::FromJoltPos(JoltSubsystem->GetBodyInterface()->GetLinearVelocity(JoltBodyID)).Size();
			const auto VelocityDirection = Units::FromJoltPos(JoltSubsystem->GetBodyInterface()->GetLinearVelocity(JoltBodyID)).GetSafeNormal();
			DrawDebugLine(GetWorld(), CenterOfMassLocation, CenterOfMassLocation + VelocityDirection * VelocityMagnitude, FColor::Yellow, false, 0.016f, SDPG_MAX, 10.f);
		}
	}
#endif
}

TArray<FVector> UJoltConvexHullComponent::GetConvexHullPointsForJolt() const
{
	TArray<FVector> ConvexHullPoints;
	for (const auto& Point : Points)
	{
		ConvexHullPoints.Add(FVector(Point.X * GetComponentScale().X, -1000.f, Point.Y * GetComponentScale().Z));
		ConvexHullPoints.Add(FVector(Point.X * GetComponentScale().X, 1000.f, Point.Y * GetComponentScale().Z));
	}

	return ConvexHullPoints;
}

#ifdef MY_DEBUG_RENDERING
TArray<FVector> UJoltConvexHullComponent::GetConvexHullPointsForDebugRendering() const
{
	TArray<FVector> ConvexHullPoints;
	for (const auto& Point : Points)
	{
		ConvexHullPoints.Add(FVector(Point.X, 1000.f, Point.Y));
	}

	return ConvexHullPoints;
}

FBoxSphereBounds UJoltConvexHullComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	return FBoxSphereBounds( FVector::ZeroVector, FVector(100000.f), 100000.f).TransformBy(LocalToWorld);
}

FPrimitiveSceneProxy* UJoltConvexHullComponent::CreateSceneProxy()
{
	class FBoxSceneProxy final : public FPrimitiveSceneProxy
	{
	public:
		SIZE_T GetTypeHash() const override
		{			
			static size_t UniquePointer;
			return reinterpret_cast<size_t>(&UniquePointer);
		}

		FBoxSceneProxy(UJoltConvexHullComponent* InComponent)
			:	FPrimitiveSceneProxy(InComponent)
		{
			bWillEverBeLit = false;
			bShouldDrawPrimitive = InComponent->ShouldDrawPrimitive();
			ConvexHullPoints = InComponent->GetConvexHullPointsForDebugRendering();
		}

		virtual bool IsDrawnInGame() const override
		{
			return true;
		}

		virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override
		{
			if (!bShouldDrawPrimitive)
			{
				return;
			}
			
			const FMatrix& LocalToWorld = GetLocalToWorld();
			
			for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
			{
				if (VisibilityMap & (1 << ViewIndex))
				{
					FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);
					DrawWireSphere(PDI, LocalToWorld.GetOrigin(), FColor::Red, 500.f, 8, SDPG_MAX, 30.f);

					FMatrix FirstHullPointMatrix;
					FMatrix LastHullPointMatrix;
					uint8 i = 0;
					for (const auto& HullPoint : ConvexHullPoints)
					{
						const FMatrix CurrentHullPointMatrix = FTransform(FRotator::ZeroRotator, HullPoint, FVector::OneVector).ToMatrixNoScale() * LocalToWorld;
						FColor PointColor = FColor::Emerald;
						if (i == 0)
						{
							FirstHullPointMatrix = CurrentHullPointMatrix;
							PointColor = FColor::Orange;
						}
						DrawWireSphere(PDI, FTransform(CurrentHullPointMatrix.Rotator(), CurrentHullPointMatrix.GetOrigin(), FVector::OneVector), PointColor, 500.f, 8, SDPG_MAX, 30.f);

						if (i > 0)
						{
							PDI->DrawLine(LastHullPointMatrix.GetOrigin(), CurrentHullPointMatrix.GetOrigin(), DebugColor, SDPG_MAX, DebugLineThickness);
						}

						LastHullPointMatrix = CurrentHullPointMatrix;
						i++;
					}

					PDI->DrawLine(LastHullPointMatrix.GetOrigin(), FirstHullPointMatrix.GetOrigin(), DebugColor, SDPG_MAX, DebugLineThickness);
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
		const FColor DebugColor = FColor::Blue;
		const float DebugLineThickness = 100.f;
		bool bShouldDrawPrimitive = false;
		TArray<FVector> ConvexHullPoints;
	};

	return new FBoxSceneProxy( this );
}
#endif

