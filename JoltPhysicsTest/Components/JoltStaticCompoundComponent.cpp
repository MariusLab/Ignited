// Fill out your copyright notice in the Description page of Project Settings.


#include "JoltStaticCompoundComponent.h"
#include "JoltBoxComponent.h"
#include "JoltConvexHullComponent.h"
#include "JoltSphereComponent.h"
#include "JoltPhysicsTest/Debug/CustomLogger.h"
#include "JoltPhysicsTest/Jolt/JoltConverter.h"
#include "JoltPhysicsTest/Jolt/JoltSubsystem.h"

UJoltStaticCompoundComponent::UJoltStaticCompoundComponent()
{
	bHiddenInGame = false;
	bNeverDistanceCull = true;
	bUseEditorCompositing = true;

	// Jolt cannot calculate mass automatically for shapes like Convex hull, triangle, mesh etc.
	PhysicsProperties.bOverrideMass = 1;
	PhysicsProperties.Mass = 100;
}

void UJoltStaticCompoundComponent::AddJoltBodyComponent(UJoltBodyComponent* JoltBodyComponent)
{
	if (auto JoltBox = Cast<UJoltBoxComponent>(JoltBodyComponent))
	{
		JoltBox->StaticMeshComp->DestroyComponent(false);
		JoltBodiesSettings.Add(TJoltBodySettings(JoltBox->GetComponentLocation(), JoltBox->GetComponentQuat(), JoltBox->BoxExtent));
		return;
	}

	if (auto JoltSphere = Cast<UJoltSphereComponent>(JoltBodyComponent))
	{
		JoltSphere->StaticMeshComp->DestroyComponent(false);
		JoltBodiesSettings.Add(TJoltBodySettings(JoltSphere->GetComponentLocation(), JoltSphere->GetComponentQuat(), JoltSphere->SphereRadius));
		return;
	}

	if (auto JoltConvexHull = Cast<UJoltConvexHullComponent>(JoltBodyComponent))
	{
		const int AddedIndex = JoltBodiesSettings.Add(TJoltBodySettings(JoltConvexHull->GetComponentLocation(), JoltConvexHull->GetComponentQuat(), JoltConvexHull->GetConvexHullPointsForJolt()));

#ifdef MY_DEBUG_RENDERING
		JoltBodiesSettings[AddedIndex].ConvexHullPointsForRendering = JoltConvexHull->GetConvexHullPointsForDebugRendering();
#endif

		return;
	}

	checkNoEntry();
}

void UJoltStaticCompoundComponent::CopyRootBodyComponentProperties(UJoltBodyComponent* JoltBodyComponent)
{
	bDebugDraw = JoltBodyComponent->bDebugDraw;
	bOrientTowardsVelocity = JoltBodyComponent->bOrientTowardsVelocity;
	SpawnOrder = JoltBodyComponent->SpawnOrder;
	GeneratedSpawnOrder = JoltBodyComponent->GeneratedSpawnOrder;
	BodyName = JoltBodyComponent->BodyName;
	JoltBodyClass = JoltBodyComponent->JoltBodyClass;
	
	if (auto JoltBox = Cast<UJoltBoxComponent>(JoltBodyComponent))
	{
		PhysicsProperties = JoltBox->GetPhysicsProperties();

#ifdef MY_DEBUG_RENDERING
		JoltBox->OverwriteDebugDraw();
		bDebugDraw = JoltBox->bDebugDraw;
#endif
		
		return;
	}

	if (auto JoltSphere = Cast<UJoltSphereComponent>(JoltBodyComponent))
	{
		PhysicsProperties = JoltSphere->GetPhysicsProperties();

#ifdef MY_DEBUG_RENDERING
		JoltSphere->OverwriteDebugDraw();
		bDebugDraw = JoltSphere->bDebugDraw;
#endif
		
		return;
	}

	if (auto JoltConvexHull = Cast<UJoltConvexHullComponent>(JoltBodyComponent))
	{
		PhysicsProperties = JoltConvexHull->GetPhysicsProperties();

#ifdef MY_DEBUG_RENDERING
		JoltConvexHull->OverwriteDebugDraw();
		bDebugDraw = JoltConvexHull->bDebugDraw;
#endif
		
		return;
	}

	checkNoEntry();
}

void UJoltStaticCompoundComponent::InitBody(const FJoltBodyData& JoltBodyData)
{
	Super::InitBody(JoltBodyData);
	
	auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();
	JoltBodyID = JoltSubsystem->AddStaticCompound(JoltBodyData.BodyIndex, GetComponentLocation(), GetComponentQuat(), JoltBodiesSettings, PhysicsProperties);
}

void UJoltStaticCompoundComponent::OnRender()
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

const TArray<TJoltBodySettings>& UJoltStaticCompoundComponent::GetJoltBodiesSettings() const
{
	return JoltBodiesSettings;
}

#ifdef MY_DEBUG_RENDERING
FBoxSphereBounds UJoltStaticCompoundComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	return FBoxSphereBounds( FVector::ZeroVector, FVector(100000.f), 100000.f).TransformBy(LocalToWorld);
}

FPrimitiveSceneProxy* UJoltStaticCompoundComponent::CreateSceneProxy()
{
	class FBoxSceneProxy final : public FPrimitiveSceneProxy
	{
	public:
		SIZE_T GetTypeHash() const override
		{			
			static size_t UniquePointer;
			return reinterpret_cast<size_t>(&UniquePointer);
		}

		FBoxSceneProxy(UJoltStaticCompoundComponent* InComponent)
			:	FPrimitiveSceneProxy(InComponent)
		{
			bWillEverBeLit = false;

			JoltBodiesSettings = InComponent->GetJoltBodiesSettings();
			bShouldDrawPrimitive = InComponent->ShouldDrawPrimitive();
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
					const FSceneView* View = Views[ViewIndex];

					FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);
					DrawWireSphere(PDI, LocalToWorld.GetOrigin(), FColor::Red, 500.f, 8, SDPG_MAX, 30.f);
					for (const TJoltBodySettings& JoltBodySettings : JoltBodiesSettings)
					{
						if (JoltBodySettings.JoltBodyShape == EJoltBodyShape::Box)
						{
							FMatrix BoxMatrix = FTransform(JoltBodySettings.Rotation, JoltBodySettings.Location, FVector::OneVector).ToMatrixNoScale() * LocalToWorld;
							DrawOrientedWireBox(PDI, BoxMatrix.GetOrigin(), BoxMatrix.GetScaledAxis( EAxis::X ), BoxMatrix.GetScaledAxis( EAxis::Y ), BoxMatrix.GetScaledAxis( EAxis::Z ), JoltBodySettings.BoxExtent, DebugColor, SDPG_MAX, DebugLineThickness);
						}

						if (JoltBodySettings.JoltBodyShape == EJoltBodyShape::Sphere)
						{
							FMatrix SphereMatrix = FTransform(JoltBodySettings.Rotation, JoltBodySettings.Location, FVector::OneVector).ToMatrixNoScale() * LocalToWorld;
							DrawWireSphere(PDI, FTransform(SphereMatrix.Rotator(), SphereMatrix.GetOrigin(), FVector::OneVector), DebugColor, JoltBodySettings.SphereRadius, 16, SDPG_MAX, DebugLineThickness);
						}

						if (JoltBodySettings.JoltBodyShape == EJoltBodyShape::ConvexHull)
						{
							FMatrix FirstHullPointMatrix;
							FMatrix LastHullPointMatrix;
							uint8 i = 0;
							for (const auto& HullPoint : JoltBodySettings.ConvexHullPointsForRendering)
							{
								const FMatrix CurrentHullPointMatrix = FTransform(FRotator::ZeroRotator, HullPoint, FVector::OneVector).ToMatrixNoScale() * FTransform(JoltBodySettings.Rotation, JoltBodySettings.Location, FVector::OneVector).ToMatrixNoScale() * LocalToWorld;
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
		FColor DebugColor = FColor::Blue;
		float DebugLineThickness = 100.f;
		TArray<TJoltBodySettings> JoltBodiesSettings;
		bool bShouldDrawPrimitive = false;
		
	};

	return new FBoxSceneProxy( this );
}
#endif

