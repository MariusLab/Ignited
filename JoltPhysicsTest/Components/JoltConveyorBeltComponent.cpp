// Fill out your copyright notice in the Description page of Project Settings.


#include "JoltConveyorBeltComponent.h"

#include "JoltPhysicsTest/Data/AssetManagerSubsystem.h"
#include "JoltPhysicsTest/Jolt/JoltConverter.h"
#include "JoltPhysicsTest/JoltBodies/ConveyorBelt.h"


UJoltConveyorBeltComponent::UJoltConveyorBeltComponent()
{
	BoxExtent = FVector(100.f, 100.f, 100.f);
	BoxColor = FColor::Blue;
	LineThickness = 100.f;

	bHiddenInGame = false;
	bNeverDistanceCull = true;
	bUseEditorCompositing = true;

	StaticMeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComp"));
	StaticMeshComp->SetupAttachment(this);

	StaticMeshComp->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	StaticMeshComp->SetGenerateOverlapEvents(false);
	StaticMeshComp->SetComponentTickEnabled(false);
}

#if WITH_EDITOR
void UJoltConveyorBeltComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	
	if (!bAttachStaticMesh)
	{
		StaticMeshComp->SetVisibility(false);
	}

	if (bAttachStaticMesh && PhysicsProperties.Layer != EObjectLayer::NoCollision)
	{
		StaticMeshComp->SetVisibility(true);
	}

	StaticMeshComp->SetRelativeScale3D(BoxExtent * 0.01f * 2.f);
}
#endif

void UJoltConveyorBeltComponent::InitBody(const FJoltBodyData& JoltBodyData)
{
	Super::InitBody(JoltBodyData);

	PhysicsProperties = GetPhysicsProperties();
#ifdef MY_DEBUG_RENDERING
	OverwriteDebugDraw();
#endif
	
	if (bAttachStaticMesh)
	{
		auto GeneralData = UAssetManagerSubsystem::GetGeneralData(GetWorld());
		checkf(GeneralData, TEXT("Could not load GeneralData"));

		StaticMeshComp->SetStaticMesh(GeneralData->BoxStaticMesh);
		StaticMeshComp->SetVisibility(true);
		StaticMeshComp->bReceivesDecals = 1;
		StaticMeshComp->SetRelativeScale3D(BoxExtent * 0.01f * 2.f);

		if (!AttachedMeshMaterial)
		{
			StaticMeshComp->SetMaterial(0, GeneralData->PlatformMaterial);
		}
		else
		{
			StaticMeshComp->SetMaterial(0, AttachedMeshMaterial);
		}
		StaticMeshComp->SetCustomPrimitiveDataFloat(0, static_cast<float>(GeneratedSpawnOrder));
		StaticMeshComp->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);

		const FVector ComponentForwardVector = GetForwardVector();
		ConveyorVelocityVector = ComponentForwardVector * ConveyorVelocity;
		StaticMeshComp->CreateDynamicMaterialInstance(0)->SetVectorParameterValue(TEXT("ConveyorVelocity"), ConveyorVelocityVector);
	}
	else
	{
		StaticMeshComp->SetVisibility(false);
	}
	
	auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();
	JoltBodyID = JoltSubsystem->AddBox(JoltBodyData.BodyIndex, GetComponentLocation(), GetComponentQuat(), BoxExtent, PhysicsProperties);
	CenterOfMassLocation = JoltSubsystem->GetCenterOfMassPosition(JoltBodyID);
}

void UJoltConveyorBeltComponent::ReverseConveyorVelocity()
{
	ConveyorVelocityVector = -ConveyorVelocityVector;
	if (bAttachStaticMesh)
	{
		StaticMeshComp->CreateDynamicMaterialInstance(0)->SetVectorParameterValue(TEXT("ConveyorVelocity"), ConveyorVelocityVector);
	}
}

FPhysicsProperties UJoltConveyorBeltComponent::GetPhysicsProperties() const
{
	if (PhysicsPropertiesDataAsset)
	{
		return PhysicsPropertiesDataAsset->PhysicsProperties;
	}

	return PhysicsProperties;
}

#ifdef MY_DEBUG_RENDERING
void UJoltConveyorBeltComponent::OverwriteDebugDraw()
{
	if (PhysicsPropertiesDataAsset && PhysicsPropertiesDataAsset->bForceDebugDraw)
	{
		bDebugDraw = true;
	}
}
#endif

void UJoltConveyorBeltComponent::OnRender()
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
			if (!bAttachStaticMesh)
			{
				StaticMeshComp->SetVisibility(false);
			}
		}
	}

	// We purposely run the code above before calling the parent, because the parent overwrites bRemovedFromSimulationLastRenderFrameValue
	Super::OnRender();

#ifdef MY_DEBUG_RENDERING
	if (bDebugDraw)
	{
		auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();
		DrawDebugString(GetWorld(), CenterOfMassLocation, FString::Printf(TEXT("ID: %d"), JoltBodyID.GetIndex()), 0, FColor::White, 0.016f);

		if (PhysicsProperties.Motion == EObjectMotion::Dynamic)
		{
			const auto VelocityMagnitude = Units::FromJoltPos(JoltSubsystem->GetBodyInterface()->GetLinearVelocity(JoltBodyID)).Size();
			const auto VelocityDirection = Units::FromJoltPos(JoltSubsystem->GetBodyInterface()->GetLinearVelocity(JoltBodyID)).GetSafeNormal();
			DrawDebugLine(GetWorld(), CenterOfMassLocation, CenterOfMassLocation + VelocityDirection * VelocityMagnitude, FColor::Yellow, false, 0.016f, SDPG_World, 10.f);
		}
	}
#endif
}

TArray<FVector2D> UJoltConveyorBeltComponent::GetAttachedMeshVertices() const
{
	if (!ShapeVertices.IsEmpty())
	{
		return LocalPositionsToWorld(ShapeVertices);
	}
	
	if (!bAttachStaticMesh || !bAttachedStaticMeshCastShadow)
	{
		return {};
	}

	check(StaticMeshComp);

	float BoxExtentX = BoxExtent.X;
	float BoxExtentZ = BoxExtent.Z;

	return {
		LocalPosToWorld(-BoxExtentX, -BoxExtentZ),
		LocalPosToWorld(BoxExtentX, -BoxExtentZ),
		LocalPosToWorld(-BoxExtentX, BoxExtentZ),
		LocalPosToWorld(BoxExtentX, BoxExtentZ)
	};
}

#ifdef MY_DEBUG_RENDERING
FBoxSphereBounds UJoltConveyorBeltComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	FBox LocalBox = FBox::BuildAABB(FVector::ZeroVector, BoxExtent);
	
	return FBoxSphereBounds(LocalBox.TransformBy(LocalToWorld));
}

FPrimitiveSceneProxy* UJoltConveyorBeltComponent::CreateSceneProxy()
{
	class FBoxSceneProxy final : public FPrimitiveSceneProxy
	{
	public:
		SIZE_T GetTypeHash() const override
		{			
			static size_t UniquePointer;
			return reinterpret_cast<size_t>(&UniquePointer);
		}

		FBoxSceneProxy(UJoltConveyorBeltComponent* InComponent)
			:	FPrimitiveSceneProxy(InComponent)
			,	BoxColor( InComponent->BoxColor )
			,	LineThickness( InComponent->LineThickness )
			,	JoltConveyorBeltComponent( InComponent )
		{
			bWillEverBeLit = false;
		}

		virtual bool IsDrawnInGame() const override
		{
			return true;
		}

		virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override
		{
			if (JoltConveyorBeltComponent.IsStale() || !JoltConveyorBeltComponent->ShouldDrawPrimitive())
			{
				return;
			}
			
			const FMatrix& LocalToWorld = GetLocalToWorld();
			
			for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
			{
				if (VisibilityMap & (1 << ViewIndex))
				{
					const FSceneView* View = Views[ViewIndex];

					const FLinearColor DrawColor = GetViewSelectionColor(BoxColor, *View, IsSelected(), IsHovered(), false, IsIndividuallySelected() );

					FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);
					DrawOrientedWireBox(PDI, LocalToWorld.GetOrigin(), LocalToWorld.GetScaledAxis( EAxis::X ), LocalToWorld.GetScaledAxis( EAxis::Y ), LocalToWorld.GetScaledAxis( EAxis::Z ), JoltConveyorBeltComponent->BoxExtent, DrawColor, SDPG_World, LineThickness);
					DrawWireStar(PDI, JoltConveyorBeltComponent->CenterOfMassLocation, 15.f, FColor::Red, SDPG_World);

					const auto ConveyorVel = JoltConveyorBeltComponent->GetForwardVector() * JoltConveyorBeltComponent->ConveyorVelocity;
					const FTransform Transform = FTransform(ConveyorVel.GetSafeNormal().Rotation(), LocalToWorld.GetOrigin() + FVector(0, 0, 200.f));
					DrawDirectionalArrow(PDI, Transform.ToMatrixNoScale(), FColor::Green, ConveyorVel.Length() * 4.f, 160.f, SDPG_World, 40.f);
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
		const FColor BoxColor;
		const float LineThickness;
		TWeakObjectPtr<UJoltConveyorBeltComponent> JoltConveyorBeltComponent = nullptr;
	};

	return new FBoxSceneProxy( this );
}
#endif

UClass* UJoltConveyorBeltComponent::GetJoltBodyClass() const
{
	return AConveyorBelt::StaticClass();
}



