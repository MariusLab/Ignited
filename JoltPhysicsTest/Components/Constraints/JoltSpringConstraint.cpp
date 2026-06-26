// Fill out your copyright notice in the Description page of Project Settings.

#include "JoltSpringConstraint.h"

#include "Engine/AssetManager.h"
#include "JoltPhysicsTest/Data/AssetManagerSubsystem.h"
#include "JoltPhysicsTest/Data/GeneralData.h"
#include "JoltPhysicsTest/Jolt/JoltConverter.h"
#include "JoltPhysicsTest/Jolt/JoltSubsystem.h"
#include "JoltPhysicsTest/Player/MyPlayerController.h"

UJoltSpringConstraint::UJoltSpringConstraint()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UJoltSpringConstraint::InitConstraintComponent()
{
	DistanceConstraintSettings Settings;
	Settings.mConstraintPriority = ConstraintData.ConstraintPriority;
	
	Settings.mMinDistance = ConstraintSettings.MinDistance;
	Settings.mMaxDistance = ConstraintSettings.MaxDistance;
	
	if (ConstraintData.MaxDistance > 0.f)
	{
		Settings.mMinDistance = ConstraintData.MinDistance;
		Settings.mMaxDistance = ConstraintData.MaxDistance;
	}

	Settings.mLimitsSpringSettings = ConstraintSettings.CustomSpringSettings.ToJoltSpringSettings();

	auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();

	if (GetNumberOfSplinePoints() == 2 && ConstraintData.MaxDistance == 0.f)
	{
		Settings.mPoint1 = Units::ToJoltPos(GetLocationAtSplinePoint(0, ESplineCoordinateSpace::World));
		Settings.mPoint2 = Units::ToJoltPos(GetLocationAtSplinePoint(1, ESplineCoordinateSpace::World));
	}
	else
	{
		Settings.mPoint1 = Units::ToJoltPos(GetComponentLocation());
		Settings.mPoint2 = ConstraintData.bHasBody2WorldAttachPoint
			? ConstraintData.Body2WorldAttachPoint
			: JoltSubsystem->GetBodyInterface()->GetPosition(ConstraintData.BodyID2);
	}

	if (!ConstraintData.bConstrainToImmovableWorld)
	{
		auto Body1 = JoltSubsystem->GetPhysicsSystem()->GetBodyLockInterfaceNoLock().TryGetBody(ConstraintData.BodyID1);
		auto Body2 = JoltSubsystem->GetPhysicsSystem()->GetBodyLockInterfaceNoLock().TryGetBody(ConstraintData.BodyID2);
		check(Body1);
		check(Body2);

		UpdateCollisionBetweenBodies(Body1, Body2);
	
		JoltConstraint = Settings.Create(*Body1, *Body2);
	}
	else
	{
		auto Body2 = JoltSubsystem->GetPhysicsSystem()->GetBodyLockInterfaceNoLock().TryGetBody(ConstraintData.BodyID2);
		check(Body2);

		JoltConstraint = Settings.Create(Body::sFixedToWorld, *Body2);
	}

	
	DistanceConstraintInstance = static_cast<DistanceConstraint*>(JoltConstraint);
	JoltSubsystem->AddConstraint(JoltConstraint);

	LOG(TEXT("[JoltSpringConstraint] Created between bodies %d and %d"), Body1->GetID().GetIndex(), Body2->GetID().GetIndex());

	if (bAttachStaticMesh)
	{
		AttachedMesh = Cast<UStaticMeshComponent>(GetOwner()->AddComponentByClass(UStaticMeshComponent::StaticClass(), true, FTransform::Identity, false));
		AttachedMesh->AttachToComponent(this, FAttachmentTransformRules::SnapToTargetIncludingScale);
		AttachedMesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
		AttachedMesh->SetGenerateOverlapEvents(false);
		AttachedMesh->CastShadow = false;

		auto GeneralData = UAssetManagerSubsystem::GetGeneralData(GetWorld());
		checkf(GeneralData, TEXT("Could not load GeneralData"));

		AttachedMesh->SetStaticMesh(GeneralData->BoxStaticMesh);

		if (CustomRopeMaterial)
		{
			AttachedMesh->SetMaterial(0, CustomRopeMaterial);
		}
		else
		{
			AttachedMesh->SetMaterial(0, GeneralData->RopeMaterial);
		}

		AttachedMesh->SetTranslucentSortPriority(108);

		SetComponentTickEnabled(true);
	}
}

void UJoltSpringConstraint::RecreateConstraint()
{
	Super::RecreateConstraint();

	auto Result = ConstraintSettings::sRestoreFromBinaryState(ConstraintSettingsRecorded);
	check(!Result.HasError());
	
	auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();

	if (!ConstraintData.bConstrainToImmovableWorld)
	{
		auto Body1 = JoltSubsystem->GetPhysicsSystem()->GetBodyLockInterfaceNoLock().TryGetBody(ConstraintData.BodyID1);
		auto Body2 = JoltSubsystem->GetPhysicsSystem()->GetBodyLockInterfaceNoLock().TryGetBody(ConstraintData.BodyID2);
		check(Body1);
		check(Body2);

		UpdateCollisionBetweenBodies(Body1, Body2);

		JoltConstraint = static_cast<DistanceConstraintSettings*>(Result.Get().GetPtr())->Create(*Body1, *Body2);
	}
	else
	{
		auto Body2 = JoltSubsystem->GetPhysicsSystem()->GetBodyLockInterfaceNoLock().TryGetBody(ConstraintData.BodyID2);
		check(Body2);

		JoltConstraint = static_cast<DistanceConstraintSettings*>(Result.Get().GetPtr())->Create(Body::sFixedToWorld, *Body2);
	}
	
	DistanceConstraintInstance = static_cast<DistanceConstraint*>(JoltConstraint);
	JoltSubsystem->AddConstraint(DistanceConstraintInstance);

	LOG(TEXT("[JoltSpringConstraint] Re-created between bodies %d and %d"), Body1->GetID().GetIndex(), Body2->GetID().GetIndex());

	if (bAttachStaticMesh && AttachedMesh)
	{
		AttachedMesh->SetHiddenInGame(false);
	}
}

void UJoltSpringConstraint::DestroyConstraint()
{
	Super::DestroyConstraint();

	if (bAttachStaticMesh && AttachedMesh)
	{
		AttachedMesh->SetHiddenInGame(true);
	}
}

void UJoltSpringConstraint::TickComponent(float DeltaTime, ELevelTick TickType,
                                          FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!DistanceConstraintInstance)
	{
		return;
	}

	auto Settings = DistanceConstraintInstance->GetDistanceConstraintSettings();
	FVector Start = Units::FromJoltPos(Settings->mPoint1);
	FVector End = Units::FromJoltPos(Settings->mPoint2);

	// 1. Compute direction vector
	FVector Direction = End - Start;
	FVector MidPoint = (Start + End) * 0.5f;
	MidPoint.Y = -50.f;
	FVector NormalizedDirection = Direction.GetSafeNormal();

	// 2. Set location at midpoint
	AttachedMesh->SetWorldLocation(MidPoint);

	// 3. Align mesh Z-axis to the direction vector
	FQuat Rotation = FRotationMatrix::MakeFromZ(NormalizedDirection).ToQuat();
	AttachedMesh->SetWorldRotation(Rotation);

	// 4. Stretch the cube along Z to match the distance
	AttachedMesh->SetWorldScale3D(FVector(1.5f, 2.5f, (Settings->mPoint2 - Settings->mPoint1).Length()));
}

#ifdef MY_DEBUG_RENDERING
FBoxSphereBounds UJoltSpringConstraint::CalcBounds(const FTransform& LocalToWorld) const
{
	return FBoxSphereBounds( FVector::ZeroVector, FVector(1000000.f), 1000000.f).TransformBy(LocalToWorld);
}

FPrimitiveSceneProxy* UJoltSpringConstraint::CreateSceneProxy()
{
	class FConstraintSceneProxy final : public FPrimitiveSceneProxy
	{
	public:
		SIZE_T GetTypeHash() const override
		{			
			static size_t UniquePointer;
			return reinterpret_cast<size_t>(&UniquePointer);
		}

		FConstraintSceneProxy(UJoltSpringConstraint* InComponent)
			:	FPrimitiveSceneProxy(InComponent)
			,	JoltSpringConstraint( InComponent )
		{
			bWillEverBeLit = false;
		}

		virtual bool IsDrawnInGame() const override
		{
			return true;
		}

		virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override
		{
			if (!JoltSpringConstraint.IsValid() || !JoltSpringConstraint->ShouldDrawPrimitive())
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
					if (JoltSpringConstraint->DistanceConstraintInstance)
					{
						auto Settings = JoltSpringConstraint->DistanceConstraintInstance->GetDistanceConstraintSettings();
						const FVector Position1 = Units::FromJoltPos(Settings->mPoint1);
						const FVector Position2 = Units::FromJoltPos(Settings->mPoint2);
						const float MinDistance = Units::FromJoltUnits(Settings->mMinDistance);
						const float MaxDistance = Units::FromJoltUnits(Settings->mMaxDistance);
						
						FVector Delta = Position2 - Position1;
						float len = Delta.Length();
						if (len < MinDistance)
						{
							FVector RealEndPosition = Position1 + (len > 0.0f? Delta * MinDistance / len : FVector(0, len, 0));
							PDI->DrawLine(Position1, Position2, FColor::Green, SDPG_MAX, 100.f);
							PDI->DrawLine(Position2, RealEndPosition, FColor::Yellow, SDPG_MAX, 100.f);
						}
						else if (len > MaxDistance)
						{
							FVector RealEndPosition = Position1 + (len > 0.0f? Delta * MaxDistance / len : FVector(0, len, 0));
							PDI->DrawLine(Position1, RealEndPosition, FColor::Green, SDPG_MAX, 100.f);
							PDI->DrawLine(RealEndPosition, Position2, FColor::Red, SDPG_MAX, 100.f);
						}
						else
							PDI->DrawLine(Position1, Position2, FColor::Green, SDPG_MAX, 100.f);

						// Draw constraint end points
						DrawWireSphere(PDI, Position1, DrawColor, 500.f, 8, SDPG_World, 30.f);
						DrawWireSphere(PDI, Position2, DrawColor, 500.f, 8, SDPG_World, 30.f);

						// Draw current length
						//DrawDebugString(JoltSpringConstraint->GetWorld(), 0.5f * (Position1 + Position2), FString::Printf(TEXT("%f"), len));
					}
					else
					{
						if (JoltSpringConstraint->GetNumberOfSplinePoints() == 2)
						{
							const FVector FirstPoint = JoltSpringConstraint->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::World);
							const FVector SecondPoint = JoltSpringConstraint->GetLocationAtSplinePoint(1, ESplineCoordinateSpace::World);
							DrawWireSphere(PDI, FirstPoint, DrawColor, 500.f, 8, SDPG_World, 30.f);
							DrawWireSphere(PDI, SecondPoint, DrawColor, 500.f, 8, SDPG_World, 30.f);
							
							FVector Direction = (SecondPoint - FirstPoint).GetSafeNormal();
							PDI->DrawLine(
								FirstPoint + Direction * Units::FromJoltUnits(JoltSpringConstraint->ConstraintSettings.MinDistance),
								FirstPoint + Direction * Units::FromJoltUnits(JoltSpringConstraint->ConstraintSettings.MaxDistance),
								FColor::Green,
								SDPG_World,
								100.f
							);
						}
						else
						{
							DrawWireSphere(PDI, LocalToWorld.GetOrigin(), DrawColor, 500.f, 8, SDPG_World, 30.f);
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
		TWeakObjectPtr<UJoltSpringConstraint> JoltSpringConstraint = nullptr;
	};

	return new FConstraintSceneProxy( this );
}
#endif
