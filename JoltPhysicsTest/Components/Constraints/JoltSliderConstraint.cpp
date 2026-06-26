// Fill out your copyright notice in the Description page of Project Settings.


#include "JoltSliderConstraint.h"

#include "JoltPhysicsTest/Jolt/JoltConverter.h"
#include "JoltPhysicsTest/Jolt/JoltSubsystem.h"

void UJoltSliderConstraint::StepSimulation()
{
	Super::StepSimulation();

	if (bSwitchDirectionAutomatically)
	{
#ifdef MY_DEBUG_RENDERING
		if (bDebugDraw)
		{
			DrawDebugString(GetWorld(), GetComponentLocation() + FVector(0, 10000.f, 0), FString::Printf(TEXT("Frames moved: %d"), FramesMovedInSameDirection), 0, FColor::White, 0.016f, false, 1.f);
		}
#endif
		
		if (SliderConstraintInstance && SliderConstraintInstance->GetMotorState() == EMotorState::Position && FramesMovedInSameDirection == SwitchDirectionEveryXFrames)
		{
			float PreviousTargetPosition = SliderConstraintInstance->GetTargetPosition();
			SliderConstraintInstance->SetTargetPosition(NextTargetPosition);
			NextTargetPosition = PreviousTargetPosition;
			FramesMovedInSameDirection = 0;
		}

		FramesMovedInSameDirection++;
	}
}

void UJoltSliderConstraint::InitConstraintComponent()
{
	auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();

	SliderConstraintSettings Settings;
	Settings.mAutoDetectPoint = true;
	Settings.SetSliderAxis(Units::ToJoltCoord(GetComponentQuat().GetUpVector()));
	
	Settings.mConstraintPriority = ConstraintData.ConstraintPriority;
	
	Settings.mLimitsMin = ConstraintSettings.LimitsMin;
	Settings.mLimitsMax = ConstraintSettings.LimitsMax;

	Settings.mMaxFrictionForce = ConstraintSettings.MaxFrictionForce;

	Settings.mLimitsSpringSettings = ConstraintSettings.CustomSpringSettings.ToJoltSpringSettings();
	Settings.mMotorSettings = ConstraintSettings.CustomMotorSettings.ToJoltMotorSettings();

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

	SliderConstraintInstance = static_cast<SliderConstraint*>(JoltConstraint);

	if (ConstraintSettings.Motor == EMotor::Position)
	{
		SliderConstraintInstance->SetMotorState(EMotorState::Position);
		SliderConstraintInstance->SetTargetPosition(ConstraintSettings.TargetPosition);
	}

	if (ConstraintSettings.Motor == EMotor::Velocity)
	{
		SliderConstraintInstance->SetMotorState(EMotorState::Velocity);
		SliderConstraintInstance->SetTargetVelocity(ConstraintSettings.TargetVelocity);
	}
	
	JoltSubsystem->AddConstraint(JoltConstraint);
}

void UJoltSliderConstraint::RecreateConstraint()
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

		JoltConstraint = static_cast<SliderConstraintSettings*>(Result.Get().GetPtr())->Create(*Body1, *Body2);
	}
	else
	{
		auto Body2 = JoltSubsystem->GetPhysicsSystem()->GetBodyLockInterfaceNoLock().TryGetBody(ConstraintData.BodyID2);
		check(Body2);
	
		JoltConstraint = static_cast<SliderConstraintSettings*>(Result.Get().GetPtr())->Create(Body::sFixedToWorld, *Body2);
	}
	
	JoltSubsystem->AddConstraint(JoltConstraint);

	SliderConstraintInstance = static_cast<SliderConstraint*>(JoltConstraint);
}

void UJoltSliderConstraint::SaveConstraint(StateRecorderImpl& InStream)
{
	Super::SaveConstraint(InStream);

	InStream.Write(FramesMovedInSameDirection);
	InStream.Write(NextTargetPosition);
}

void UJoltSliderConstraint::LoadConstraint(StateRecorderImpl& InStream)
{
	Super::LoadConstraint(InStream);

	InStream.Read(FramesMovedInSameDirection);
	InStream.Read(NextTargetPosition);
}

#ifdef MY_DEBUG_RENDERING
FPrimitiveSceneProxy* UJoltSliderConstraint::CreateSceneProxy()
{
	class FConstraintSceneProxy final : public FPrimitiveSceneProxy
	{
	public:
		SIZE_T GetTypeHash() const override
		{			
			static size_t UniquePointer;
			return reinterpret_cast<size_t>(&UniquePointer);
		}

		FConstraintSceneProxy(UJoltSliderConstraint* InComponent)
			:	FPrimitiveSceneProxy(InComponent)
			,	JoltSliderConstraint( InComponent )
		{
			bWillEverBeLit = false;
		}

		virtual bool IsDrawnInGame() const override
		{
			return true;
		}

		virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override
		{
			if (!JoltSliderConstraint.IsValid() || !JoltSliderConstraint->ShouldDrawPrimitive())
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

					if (JoltSliderConstraint->SliderConstraintInstance)
					{
						auto SliderConstraintInstance = JoltSliderConstraint->SliderConstraintInstance;
						constexpr float DrawConstraintSize = 500.f;
						const RMat44 Transform1 = SliderConstraintInstance->GetBody1()->GetCenterOfMassTransform();
						const RMat44 Transform2 = SliderConstraintInstance->GetBody2()->GetCenterOfMassTransform();

						// Transform the local positions into world space
						Vec3 SliderAxis = Transform1.Multiply3x3(SliderConstraintInstance->GetLocalSpaceSliderAxis1());
						RVec3 Position1 = Transform1 * SliderConstraintInstance->GetLocalSpacePosition1();
						RVec3 Position2 = Transform2 * SliderConstraintInstance->GetLocalSpacePosition2();

						DrawWireSphere(PDI, Units::FromJoltPos(Position1), FColor::Red, DrawConstraintSize, 16, SDPG_World, 30.f);
						DrawWireSphere(PDI, Units::FromJoltPos(Position2), FColor::Green, DrawConstraintSize, 16, SDPG_World, 30.f);
						PDI->DrawLine(Units::FromJoltPos(Position1), Units::FromJoltPos(Position2), FColor::Green, SDPG_MAX, 100.f);

						switch (SliderConstraintInstance->GetMotorState())
						{
							case EMotorState::Position:
								DrawWireSphere(PDI, Units::FromJoltPos(Position1 + SliderConstraintInstance->GetTargetPosition() * SliderAxis), FColor::Red, DrawConstraintSize, 16, SDPG_World, 30.f);
								break;

							case EMotorState::Velocity:
								{
									Vec3 CurrentVelocity = (SliderConstraintInstance->GetBody2()->GetLinearVelocity() - SliderConstraintInstance->GetBody2()->GetLinearVelocity()).Dot(SliderAxis) * SliderAxis;
									PDI->DrawLine(Units::FromJoltPos(Position2), Units::FromJoltPos(Position2 + CurrentVelocity), FColor::Blue, SDPG_MAX, 100.f);
									PDI->DrawLine(Units::FromJoltPos(Position2 + CurrentVelocity), Units::FromJoltPos(Position2 + SliderConstraintInstance->GetTargetVelocity() * SliderAxis), FColor::Red, SDPG_MAX, 100.f);
									break;
								}

							case EMotorState::Off:
								break;
						}
					}
					else
					{
						const auto UpVector = JoltSliderConstraint->GetComponentQuat().GetUpVector();
						PDI->DrawLine(LocalToWorld.GetOrigin(), LocalToWorld.GetOrigin() + UpVector * Units::FromJoltUnits(JoltSliderConstraint->ConstraintSettings.LimitsMax), FColor::Green, SDPG_MAX, 100.f);
						PDI->DrawLine(LocalToWorld.GetOrigin() + UpVector * Units::FromJoltUnits(JoltSliderConstraint->ConstraintSettings.LimitsMin), LocalToWorld.GetOrigin(), FColor::Green, SDPG_MAX, 100.f);
						DrawWireSphere(PDI, LocalToWorld.GetOrigin(), DrawColor, 500.f, 16, SDPG_World, 30.f);

						if (JoltSliderConstraint->ConstraintSettings.Motor == EMotor::Position)
						{
							//Target pos
							DrawWireSphere(PDI, LocalToWorld.GetOrigin() + JoltSliderConstraint->GetComponentQuat().GetUpVector() * Units::FromJoltUnits(JoltSliderConstraint->ConstraintSettings.TargetPosition), FColor::Red, 500.f, 16, SDPG_World, 30.f);
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
		TWeakObjectPtr<UJoltSliderConstraint> JoltSliderConstraint = nullptr;
	};

	return new FConstraintSceneProxy( this );
}
#endif


