// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerRenderer.h"

#include "MaterialDomain.h"
#include "Engine/AssetManager.h"
#include "Engine/PostProcessVolume.h"
#include "JoltPhysicsTest/Data/GeneralData.h"
#include "JoltPhysicsTest/Debug/CustomLogger.h"
#include "JoltPhysicsTest/Jolt/JoltConverter.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"


APlayerRenderer::APlayerRenderer()
{
	PrimaryActorTick.bCanEverTick = false;

	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootSceneComponent"));
	SetRootComponent(RootSceneComponent);

	UpperArmMeshL = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("UpperArmMeshL"));
	UpperArmMeshL->SetupAttachment(RootSceneComponent);

	LowerArmMeshL = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LowerArmMeshL"));
	LowerArmMeshL->SetupAttachment(RootSceneComponent);

	UpperArmMeshR = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("UpperArmMeshR"));
	UpperArmMeshR->SetupAttachment(RootSceneComponent);

	LowerArmMeshR = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LowerArmMeshR"));
	LowerArmMeshR->SetupAttachment(RootSceneComponent);

	StickMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StickMesh"));
	StickMesh->SetupAttachment(RootSceneComponent);

	/*DynamicMeshComponent = CreateDefaultSubobject<UDynamicMeshComponent>(TEXT("DynamicMeshComponent"));
	DynamicMeshComponent->SetMobility(EComponentMobility::Movable);
	DynamicMeshComponent->SetGenerateOverlapEvents(false);
	DynamicMeshComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	DynamicMeshComponent->CollisionType = ECollisionTraceFlag::CTF_UseDefault;

	DynamicMeshComponent->SetupAttachment(GetRootComponent());*/
}

void APlayerRenderer::SetBelongsToLocalPlayer(bool bLocal)
{
	bBelongsToLocalPlayer = bLocal;
}

void APlayerRenderer::BeginPlay()
{
	Super::BeginPlay();	

	UAssetManager& AssetManager = UAssetManager::Get();
	FPrimaryAssetId GeneralDataAssetId = FPrimaryAssetId(TEXT("GeneralData:DA_GeneralData"));
		
	auto GeneralData = Cast<UGeneralData>(AssetManager.GetPrimaryAssetObject(GeneralDataAssetId));
	checkf(GeneralData, TEXT("Could not load GeneralData"));
	
	auto PostProcessVolume = Cast<APostProcessVolume>(UGameplayStatics::GetActorOfClass(GetWorld(), APostProcessVolume::StaticClass()));
	checkf(PostProcessVolume, TEXT("PP volume not found"));
	PostProcessVolume->Settings = GeneralData->PostProcessSettings;
}

bool APlayerRenderer::BelongsToLocalPlayer()
{
	return bBelongsToLocalPlayer;
}

void APlayerRenderer::UpdateMeshLocationAndRotationGivenTwoPoints(UStaticMeshComponent* StaticMeshComponent,
                                                                  const FVector& StartLocation, const FVector& EndLocation)
{
	StaticMeshComponent->SetWorldRotation(UKismetMathLibrary::FindLookAtRotation(StartLocation, EndLocation));

	const FVector LineMidpoint = (EndLocation - StartLocation) / 2 + StartLocation;
	StaticMeshComponent->SetWorldLocation(LineMidpoint);
}

void APlayerRenderer::UpdateMeshLocationAndRotationGivenTwoPoints2D(UStaticMeshComponent* StaticMeshComponent,
																  const FVector& StartLocation, const FVector& EndLocation)
{
	FVector Direction = (EndLocation - StartLocation).GetSafeNormal();
	StaticMeshComponent->SetWorldRotation(VecDirectionToFRotator(Units::ToJoltCoord(Direction)));
	
	const FVector LineMidpoint = (EndLocation - StartLocation) / 2 + StartLocation;
	StaticMeshComponent->SetWorldLocation(LineMidpoint);
}

/*
void APlayerRenderer::UpdateBodyShape(const TArray<FVector>& Locations)
{
	check(Locations.Num() >= 3);

	constexpr int32 PointsPerSegment = 10;
	TArray<FVector> SmoothedPoints = GenerateSmoothCurvePoints(Locations, PointsPerSegment);

	FDynamicMesh3 DynamicMesh3;
	TArray<int32> VertexIDs;

	for (const auto& Pt : SmoothedPoints)
	{
		VertexIDs.Add(DynamicMesh3.AppendVertex(Pt));
	}

	// Fan triangulation — fine for convex or near-convex shapes
	for (int32 i = 1; i < VertexIDs.Num() - 1; ++i)
	{
		DynamicMesh3.AppendTriangle(VertexIDs[0], VertexIDs[i + 1], VertexIDs[i]);
	}

	DynamicMeshComponent->GetDynamicMesh()->SetMesh(MoveTemp(DynamicMesh3));
	DynamicMeshComponent->NotifyMeshModified();
}

TArray<FVector> APlayerRenderer::GenerateSmoothCurvePoints(const TArray<FVector>& ControlPoints, int32 PointsPerSegment)
{
	TArray<FVector> Result;
	int32 Num = ControlPoints.Num();

	for (int32 i = 0; i < Num; ++i)
	{
		const FVector& P0 = ControlPoints[(i - 1 + Num) % Num];
		const FVector& P1 = ControlPoints[i];
		const FVector& P2 = ControlPoints[(i + 1) % Num];
		const FVector& P3 = ControlPoints[(i + 2) % Num];

		for (int32 j = 0; j < PointsPerSegment; ++j)
		{
			float T = static_cast<float>(j) / PointsPerSegment;
			Result.Add(CatmullRom(P0, P1, P2, P3, T));
		}
	}

	return Result;
}

FVector APlayerRenderer::CatmullRom(const FVector& P0, const FVector& P1, const FVector& P2, const FVector& P3, float T)
{
	return 0.5f * (
		(2.0f * P1) +
		(-P0 + P2) * T +
		(2.0f * P0 - 5.0f * P1 + 4.0f * P2 - P3) * T * T +
		(-P0 + 3.0f * P1 - 3.0f * P2 + P3) * T * T * T
	);
}*/

