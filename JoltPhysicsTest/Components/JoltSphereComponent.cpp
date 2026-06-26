// Fill out your copyright notice in the Description page of Project Settings.


#include "JoltSphereComponent.h"

#include "JoltPhysicsTest/Data/AssetManagerSubsystem.h"
#include "JoltPhysicsTest/Data/GeneralData.h"
#include "JoltPhysicsTest/Debug/CustomLogger.h"
#include "JoltPhysicsTest/Jolt/JoltConverter.h"
#include "JoltPhysicsTest/Jolt/JoltSubsystem.h"


UJoltSphereComponent::UJoltSphereComponent()
{
	SphereRadius = 100.f;
	SphereColor = FColor::Blue;
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
		
	StaticMeshComp->SetStaticMesh(GeneralData->SphereStaticMesh);
	StaticMeshComp->SetWorldRotation(FRotator(0, 0, 90.f));
	StaticMeshComp->SetMaterial(0, GeneralData->PlatformMaterial);
	StaticMeshComp->SetCustomPrimitiveDataFloat(0, static_cast<float>(GeneratedSpawnOrder));
	StaticMeshComp->CastShadow = false;
#endif
	
	StaticMeshComp->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	StaticMeshComp->SetGenerateOverlapEvents(false);
	StaticMeshComp->SetComponentTickEnabled(false);
}

#if WITH_EDITOR
void UJoltSphereComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	
	if (!bAttachStaticMesh)
	{
		StaticMeshComp->SetVisibility(false);
	}

	if (bAttachStaticMesh && PhysicsProperties.Layer != EObjectLayer::NoCollision)
	{
		StaticMeshComp->SetVisibility(true);
		if (!bAttachStaticMesh)
		{
			StaticMeshComp->SetVisibility(false);
		}
	}

	const float Scale = GetSphereMeshScale();
	StaticMeshComp->SetRelativeScale3D(FVector(Scale, Scale, Scale));
}
#endif

void UJoltSphereComponent::InitBody(const FJoltBodyData& JoltBodyData)
{
	Super::InitBody(JoltBodyData);

	PhysicsProperties = GetPhysicsProperties();
	PhysicsProperties.bAllowDynamicOrKinematic = bAllowDynamicOrKinematic;
	
#ifdef MY_DEBUG_RENDERING
	OverwriteDebugDraw();
#endif
	
	auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();
	
	JoltBodyID = JoltSubsystem->AddSphere(JoltBodyData.BodyIndex, GetComponentLocation(), GetComponentQuat(), SphereRadius, PhysicsProperties);
	CenterOfMassLocation = JoltSubsystem->GetCenterOfMassPosition(JoltBodyID);

	if (bAttachStaticMesh)
	{
		auto GeneralData = UAssetManagerSubsystem::GetGeneralData(GetWorld());
		checkf(GeneralData, TEXT("Could not load GeneralData"));

		StaticMeshComp->SetStaticMesh(GeneralData->SphereStaticMesh);
		StaticMeshComp->SetWorldRotation(FRotator(0, 0, 90.f));
		StaticMeshComp->SetVisibility(true);
		const float Scale = GetSphereMeshScale();
		StaticMeshComp->SetRelativeScale3D(FVector(Scale, Scale, Scale));

		StaticMeshComp->SetMaterial(0, GeneralData->PlatformMaterial);
		StaticMeshComp->SetCustomPrimitiveDataFloat(0, static_cast<float>(GeneratedSpawnOrder));

		StaticMeshComp->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	}
	else
	{
		StaticMeshComp->SetVisibility(false);
	}
}

FPhysicsProperties UJoltSphereComponent::GetPhysicsProperties() const
{
	if (PhysicsPropertiesDataAsset)
	{
		return PhysicsPropertiesDataAsset->PhysicsProperties;
	}

	return PhysicsProperties;
}

#ifdef MY_DEBUG_RENDERING
void UJoltSphereComponent::OverwriteDebugDraw()
{
	if (PhysicsPropertiesDataAsset && PhysicsPropertiesDataAsset->bForceDebugDraw)
	{
		bDebugDraw = true;
	}
}
#endif

void UJoltSphereComponent::OnRender()
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

TArray<FVector2D> UJoltSphereComponent::GetAttachedMeshVertices() const
{
	if (!ShapeVertices.IsEmpty())
	{
		return LocalPositionsToWorld(ShapeVertices);
	}
	
	if (!bAttachStaticMesh)
	{
		return {};
	}

	check(StaticMeshComp);

	float BoxExtentX = 1000.f;
	float BoxExtentZ = 1000.f;

	return {
		LocalPosToWorld(-BoxExtentX, -BoxExtentZ),
		LocalPosToWorld(BoxExtentX, -BoxExtentZ),
		LocalPosToWorld(-BoxExtentX, BoxExtentZ),
		LocalPosToWorld(BoxExtentX, BoxExtentZ)
	};
}


#ifdef MY_DEBUG_RENDERING
FBoxSphereBounds UJoltSphereComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	return FBoxSphereBounds( FVector::ZeroVector, FVector(SphereRadius), SphereRadius ).TransformBy(LocalToWorld);
}

FPrimitiveSceneProxy* UJoltSphereComponent::CreateSceneProxy()
{
	class FSphereSceneProxy final : public FPrimitiveSceneProxy
	{
	public:
		SIZE_T GetTypeHash() const override
		{			
			static size_t UniquePointer;
			return reinterpret_cast<size_t>(&UniquePointer);
		}

		FSphereSceneProxy(UJoltSphereComponent* InComponent)
			:	FPrimitiveSceneProxy(InComponent)
			,   SphereRadius( InComponent->SphereRadius )
			,	SphereColor( InComponent->SphereColor )
			,	LineThickness( InComponent->LineThickness )
			,	JoltSphereComponent( InComponent )
		{
			bWillEverBeLit = false;
		}

		virtual bool IsDrawnInGame() const override
		{
			return true;
		}

		virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override
		{
			if (JoltSphereComponent.IsStale() || !JoltSphereComponent->ShouldDrawPrimitive())
			{
				return;
			}
			
			const FMatrix& LocalToWorld = GetLocalToWorld();
			
			for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
			{
				if (VisibilityMap & (1 << ViewIndex))
				{
					const FSceneView* View = Views[ViewIndex];

					const FLinearColor DrawColor = GetViewSelectionColor(SphereColor, *View, IsSelected(), IsHovered(), false, IsIndividuallySelected() );

					FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);
					DrawWireSphere(PDI, FTransform(LocalToWorld.Rotator(), LocalToWorld.GetOrigin(), FVector::OneVector), DrawColor, SphereRadius, 16, SDPG_World, LineThickness);
					DrawWireStar(PDI, JoltSphereComponent->CenterOfMassLocation, 15.f, FColor::Red, SDPG_World);
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
		const float SphereRadius;
		const FColor SphereColor;
		const float LineThickness;
		TWeakObjectPtr<UJoltSphereComponent> JoltSphereComponent = nullptr;
	};

	return new FSphereSceneProxy( this );
}
#endif

float UJoltSphereComponent::GetSphereMeshScale() const
{
	return SphereRadius * 0.01f;
}
