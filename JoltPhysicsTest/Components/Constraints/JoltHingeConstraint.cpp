// Fill out your copyright notice in the Description page of Project Settings.


#include "JoltHingeConstraint.h"

#include "JoltPhysicsTest/Jolt/JoltConverter.h"
#include "JoltPhysicsTest/Jolt/JoltSubsystem.h"

void UJoltHingeConstraint::InitConstraintComponent()
{
	auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();

	if (ConstraintSettingsData)
	{
		ConstraintSettings = ConstraintSettingsData->HingeConstraintSettings;
	}

	HingeConstraintSettings Settings;
	Settings.mConstraintPriority = ConstraintData.ConstraintPriority;
	Settings.mPoint1 = Settings.mPoint2 = Units::ToJoltPos(GetComponentLocation());
	//HingeConstraintSettings.mPoint2 = JoltSubsystem->GetBodyInterface()->GetPosition(ConstraintData.BodyID2);
	
	Settings.mHingeAxis1 = Settings.mHingeAxis2 = WorldAxisToJoltAxis(ConstraintSettings.HingeAxis);
	Settings.mNormalAxis1 = Settings.mNormalAxis2 = WorldAxisToJoltAxis(ConstraintSettings.NormalAxis);
	
	Settings.mLimitsMin = DegreesToRadians(ConstraintSettings.MinAngle);
	Settings.mLimitsMax = DegreesToRadians(ConstraintSettings.MaxAngle);

	Settings.mMaxFrictionTorque = ConstraintSettings.MaxFrictionTorque;

	Settings.mLimitsSpringSettings = ConstraintSettings.CustomSpringSettings.ToJoltSpringSettings();
	Settings.mMotorSettings = ConstraintSettings.CustomMotorSettings.ToJoltMotorSettings();

	if (!ConstraintData.bConstrainToImmovableWorld)
	{
		auto Body1 = JoltSubsystem->GetPhysicsSystem()->GetBodyLockInterface().TryGetBody(ConstraintData.BodyID1);
		auto Body2 = JoltSubsystem->GetPhysicsSystem()->GetBodyLockInterface().TryGetBody(ConstraintData.BodyID2);

		UpdateCollisionBetweenBodies(Body1, Body2);

		JoltConstraint = Settings.Create(*Body1, *Body2);
	}
	else
	{
		// Constrain to "the unmovable world". To avoid having to create dummy invisible static bodies to constrain to. Much more convenient.
		auto Body2 = JoltSubsystem->GetPhysicsSystem()->GetBodyLockInterface().TryGetBody(ConstraintData.BodyID2);
		JoltConstraint = Settings.Create(Body::sFixedToWorld, *Body2);
	}

	HingeConstraintInstance = static_cast<HingeConstraint*>(JoltConstraint);

	if (ConstraintSettings.Motor == EMotor::Position)
	{
		HingeConstraintInstance->SetMotorState(EMotorState::Position);
		HingeConstraintInstance->SetTargetAngle(DegreesToRadians(ConstraintSettings.TargetAngle));
	}

	if (ConstraintSettings.Motor == EMotor::Velocity)
	{
		HingeConstraintInstance->SetMotorState(EMotorState::Velocity);
		HingeConstraintInstance->SetTargetAngularVelocity(DegreesToRadians(ConstraintSettings.TargetVelocity));
	}
	
	JoltSubsystem->AddConstraint(JoltConstraint);
}

void UJoltHingeConstraint::RecreateConstraint()
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

		JoltConstraint = static_cast<HingeConstraintSettings*>(Result.Get().GetPtr())->Create(*Body1, *Body2);
	}
	else
	{
		// Constrain to "the unmovable world". To avoid having to create dummy invisible static bodies to constrain to. Much more convenient.
		auto Body2 = JoltSubsystem->GetPhysicsSystem()->GetBodyLockInterface().TryGetBody(ConstraintData.BodyID2);
		JoltConstraint = static_cast<HingeConstraintSettings*>(Result.Get().GetPtr())->Create(Body::sFixedToWorld, *Body2);
	}

	JoltSubsystem->AddConstraint(JoltConstraint);

	HingeConstraintInstance = static_cast<HingeConstraint*>(JoltConstraint);
}

#ifdef MY_DEBUG_RENDERING
FPrimitiveSceneProxy* UJoltHingeConstraint::CreateSceneProxy()
{
	class FConstraintSceneProxy final : public FPrimitiveSceneProxy
	{
	public:
		SIZE_T GetTypeHash() const override
		{			
			static size_t UniquePointer;
			return reinterpret_cast<size_t>(&UniquePointer);
		}

		FConstraintSceneProxy(UJoltHingeConstraint* InComponent)
			:	FPrimitiveSceneProxy(InComponent)
			,	JoltHingeConstraint( InComponent )
		{
			bWillEverBeLit = false;
		}

		virtual bool IsDrawnInGame() const override
		{
			return true;
		}

		virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override
		{
			if (!JoltHingeConstraint.IsValid() || !JoltHingeConstraint->ShouldDrawPrimitive())
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

					if (JoltHingeConstraint->HingeConstraintInstance)
					{
						auto HingeConstraintInstance = JoltHingeConstraint->HingeConstraintInstance;
						constexpr float DrawConstraintSize = 5.f;
						const RMat44 Transform1 = HingeConstraintInstance->GetBody1()->GetCenterOfMassTransform();
						const RMat44 Transform2 = HingeConstraintInstance->GetBody2()->GetCenterOfMassTransform();

						// Draw constraint
						const RVec3 ConstraintPos1 = Transform1 * HingeConstraintInstance->GetLocalSpacePoint1();
						DrawWireSphere(PDI, Units::FromJoltPos(ConstraintPos1), FColor::Red, DrawConstraintSize, 16, SDPG_World, 10.f);
						RVec3 LineEndPos = Transform1 * (HingeConstraintInstance->GetLocalSpacePoint1() + DrawConstraintSize * HingeConstraintInstance->GetLocalSpaceHingeAxis1());
						PDI->DrawLine(Units::FromJoltPos(ConstraintPos1), Units::FromJoltPos(LineEndPos), FColor::Red, SDPG_MAX, 10.f);

						const RVec3 ConstraintPos2 = Transform2 * HingeConstraintInstance->GetLocalSpacePoint2();
						DrawWireSphere(PDI, Units::FromJoltPos(ConstraintPos1), FColor::Green, DrawConstraintSize, 16, SDPG_World, 10.f);
						
						LineEndPos = Transform2 * (HingeConstraintInstance->GetLocalSpacePoint2() + DrawConstraintSize * HingeConstraintInstance->GetLocalSpaceHingeAxis2());
						PDI->DrawLine(Units::FromJoltPos(ConstraintPos2), Units::FromJoltPos(LineEndPos), FColor::Green, SDPG_MAX, 10.f);

						LineEndPos = Transform2 * (HingeConstraintInstance->GetLocalSpacePoint2() + DrawConstraintSize * HingeConstraintInstance->GetLocalSpaceNormalAxis2());
						PDI->DrawLine(Units::FromJoltPos(ConstraintPos2), Units::FromJoltPos(LineEndPos), FColor::White, SDPG_MAX, 10.f);
					}
					else
					{
						FVector DrawLocation = LocalToWorld.GetOrigin();
						DrawLocation.Y += 2000.f;
						DrawWireSphere(PDI, DrawLocation, DrawColor, 30.f, 16, SDPG_World, 10.f);
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
		TWeakObjectPtr<UJoltHingeConstraint> JoltHingeConstraint = nullptr;
	};

	return new FConstraintSceneProxy( this );
}
#endif