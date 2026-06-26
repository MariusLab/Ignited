// Fill out your copyright notice in the Description page of Project Settings.


#include "JoltBoxComponent.h"

#include "JoltPhysicsTest/Data/AssetManagerSubsystem.h"
#include "JoltPhysicsTest/Data/GeneralData.h"
#include "JoltPhysicsTest/Debug/CustomLogger.h"
#include "JoltPhysicsTest/Jolt/JoltConverter.h"
#include "JoltPhysicsTest/Jolt/JoltSubsystem.h"


UJoltBoxComponent::UJoltBoxComponent()
{
	BoxExtent = FVector(100.f, 100.f, 100.f);
	BoxColor = FColor::Blue;
	LineThickness = 100.f;

	bHiddenInGame = false;
	bNeverDistanceCull = true;
	bUseEditorCompositing = true;

	StaticMeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComp"));
	StaticMeshComp->SetupAttachment(this);

#if WITH_EDITOR
	static ConstructorHelpers::FObjectFinder<UGeneralData> GeneralDataRef(TEXT("/Game/Data/DA_GeneralData.DA_GeneralData"));
	auto GeneralData = Cast<UGeneralData>(GeneralDataRef.Object);
	checkf(GeneralData, TEXT("Could not load GeneralData. EDITOR ONLY."));
		
	StaticMeshComp->SetStaticMesh(GeneralData->BoxStaticMesh);
	StaticMeshComp->SetMaterial(0, GeneralData->PlatformMaterial);
	StaticMeshComp->SetCustomPrimitiveDataFloat(0, static_cast<float>(GeneratedSpawnOrder));
	StaticMeshComp->CastShadow = false;
#endif
	
	StaticMeshComp->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	StaticMeshComp->SetGenerateOverlapEvents(false);
	StaticMeshComp->SetComponentTickEnabled(false);
}

#if WITH_EDITOR
void UJoltBoxComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
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

void UJoltBoxComponent::InitBody(const FJoltBodyData& JoltBodyData)
{
	Super::InitBody(JoltBodyData);

	PhysicsProperties = GetPhysicsProperties();
	PhysicsProperties.bAllowDynamicOrKinematic = bAllowDynamicOrKinematic;

#ifdef MY_DEBUG_RENDERING
	OverwriteDebugDraw();
#endif
	
	BoxExtent.Y = 500.f;
	
	if (bAttachStaticMesh)
	{
		auto GeneralData = UAssetManagerSubsystem::GetGeneralData(GetWorld());
		checkf(GeneralData, TEXT("Could not load GeneralData"));

		StaticMeshComp->SetStaticMesh(GeneralData->BoxStaticMesh);
		StaticMeshComp->SetVisibility(true);
		StaticMeshComp->SetRelativeScale3D(BoxExtent * 0.01f * 2.f);
		StaticMeshComp->SetTranslucentSortPriority(10);

		if (!AttachedMeshMaterial)
		{
			StaticMeshComp->SetMaterial(0, GeneralData->PlatformMaterial);
		}
		else
		{
			StaticMeshComp->SetMaterial(0, AttachedMeshMaterial);
		}
		StaticMeshComp->SetCustomPrimitiveDataFloat(0, static_cast<float>(GeneratedSpawnOrder));

		if (bAttachedStaticMeshCastShadow)
		{
			StaticMeshComp->SetCollisionProfileName("LineTraceOnly");
		}
		else
		{
			StaticMeshComp->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
		}
	}
	else
	{
		StaticMeshComp->SetVisibility(false);
	}
	
	//bDebugDraw = false;

	/*if (bDrawOutline)
	{
		const FVector ComponentLocation = GetComponentLocation();
		const FTransform SpawnTransform = {
			FRotator(GetComponentRotation().Pitch, GetComponentRotation().Yaw, -90.f),
			ComponentLocation,
			FVector::OneVector
		};
		
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.bDeferConstruction = true;
		auto* PathTracerActor = Cast<ASimplePathTracerActor>(GetWorld()->SpawnActor(UHelper::GetLocalPlayerController(GetWorld())->PlatformOutlineRendererClass, &SpawnTransform, SpawnParameters));

		// Y and Z coordinates are flipped, because we rotate the path tracer actor -90 roll. Otherwise it doesn't face the camera.
		// Top-left corner
		PathTracerActor->AddLinePoint(FVector(-BoxExtent.X, 0.f, BoxExtent.Z));
		// Top-right corner
		PathTracerActor->AddLinePoint(FVector(BoxExtent.X, 0.f, BoxExtent.Z));
		// Bottom-right corner
		PathTracerActor->AddLinePoint(FVector(BoxExtent.X, 0, -BoxExtent.Z));
		// Bottom-left corner
		PathTracerActor->AddLinePoint(FVector(-BoxExtent.X, 0, -BoxExtent.Z));

		PathTracerActor->FinishSpawning(SpawnTransform);

		PathTracerActor->AttachToComponent(this, FAttachmentTransformRules::KeepWorldTransform);
	}*/
	
	auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();
	JoltBodyID = JoltSubsystem->AddBox(JoltBodyData.BodyIndex, GetComponentLocation(), GetComponentQuat(), BoxExtent, PhysicsProperties);
	CenterOfMassLocation = JoltSubsystem->GetCenterOfMassPosition(JoltBodyID);
}

FPhysicsProperties UJoltBoxComponent::GetPhysicsProperties() const
{
	if (PhysicsPropertiesDataAsset)
	{
		return PhysicsPropertiesDataAsset->PhysicsProperties;
	}

	return PhysicsProperties;
}

#ifdef MY_DEBUG_RENDERING
void UJoltBoxComponent::OverwriteDebugDraw()
{
	if (PhysicsPropertiesDataAsset && PhysicsPropertiesDataAsset->bForceDebugDraw)
	{
		bDebugDraw = true;
	}
}
#endif

void UJoltBoxComponent::OnRender()
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
			if (!bAttachStaticMesh && StaticMeshComp)
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
			DrawDebugLine(GetWorld(), CenterOfMassLocation, CenterOfMassLocation + VelocityDirection * VelocityMagnitude, FColor::Yellow, false, 0.016f, SDPG_MAX, 10.f);
		}
	}
#endif
}

TArray<FVector2D> UJoltBoxComponent::GetAttachedMeshVertices() const
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
FBoxSphereBounds UJoltBoxComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	FBox LocalBox = FBox::BuildAABB(FVector::ZeroVector, BoxExtent);
	
	return FBoxSphereBounds(LocalBox.TransformBy(LocalToWorld));
}

FPrimitiveSceneProxy* UJoltBoxComponent::CreateSceneProxy()
{
	class FBoxSceneProxy final : public FPrimitiveSceneProxy
	{
	public:
		SIZE_T GetTypeHash() const override
		{			
			static size_t UniquePointer;
			return reinterpret_cast<size_t>(&UniquePointer);
		}

		FBoxSceneProxy(UJoltBoxComponent* InComponent)
			:	FPrimitiveSceneProxy(InComponent)
		{
			bWillEverBeLit = false;
			BoxColor = InComponent->BoxColor;
			LineThickness = InComponent->LineThickness;
			bShouldDrawPrimitive = InComponent->ShouldDrawPrimitive();
			BoxExtent = InComponent->BoxExtent;
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

					const FLinearColor DrawColor = GetViewSelectionColor(BoxColor, *View, IsSelected(), IsHovered(), false, IsIndividuallySelected() );

					FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);
					DrawOrientedWireBox(PDI, LocalToWorld.GetOrigin(), LocalToWorld.GetScaledAxis( EAxis::X ), LocalToWorld.GetScaledAxis( EAxis::Y ), LocalToWorld.GetScaledAxis( EAxis::Z ), BoxExtent, DrawColor, SDPG_MAX, LineThickness);
					DrawWireStar(PDI, LocalToWorld.GetOrigin(), 15.f, FColor::Red, SDPG_MAX);
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
		FColor BoxColor;
		float LineThickness;
		bool bShouldDrawPrimitive = false;
		FVector BoxExtent;
	};

	return new FBoxSceneProxy( this );
}
#endif

