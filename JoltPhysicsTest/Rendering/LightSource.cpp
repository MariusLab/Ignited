// Fill out your copyright notice in the Description page of Project Settings.


#include "LightSource.h"

#include "ProceduralMeshComponent.h"

ALightSource::ALightSource()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("RootSceneComponent")));
	
	ProceduralMeshComponent = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMeshComponent"));
	ProceduralMeshComponent->SetupAttachment(GetRootComponent());
}

void ALightSource::UpdateMesh(const TArray<FVector>& Locations)
{
	check(Locations.Num() >= 3);

	const FVector ActorLocation = GetActorLocation();

	// Mesh Data
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UV0;
	TArray<FColor> VertexColors;
	TArray<FProcMeshTangent> Tangents;

	// Add center vertex
	const FVector CenterLocal = FVector::ZeroVector;
	const int32 CenterIndex = Vertices.Add(CenterLocal);
	UV0.Add(FVector2D(0.5f, 0.5f));
	Normals.Add(FVector(0.f, 1.f, 0.f)); // Upward normal
	VertexColors.Add(FColor::White);
	Tangents.Add(FProcMeshTangent(1.f, 0.f, 0.f));

	// Add perimeter vertices
	for (const FVector& WorldLocation : Locations)
	{
		const FVector LocalLocation = WorldLocation - ActorLocation;
		Vertices.Add(LocalLocation);

		// Basic planar UV mapping
		const float UVScale = 512.f;
		UV0.Add(FVector2D(LocalLocation.X / UVScale + 0.5f, LocalLocation.Z / UVScale + 0.5f));
		Normals.Add(FVector(0.f, 1.f, 0.f)); // Upward normal
		VertexColors.Add(FColor::White);
		Tangents.Add(FProcMeshTangent(1.f, 0.f, 0.f));
	}

	// Fan triangulation (counter-clockwise winding for front face)
	for (int32 i = 1; i < Vertices.Num() - 1; i++)
	{
		Triangles.Add(CenterIndex);
		Triangles.Add(i + 1);
		Triangles.Add(i);
	}

	// Final triangle to close the fan
	Triangles.Add(CenterIndex);
	Triangles.Add(1);
	Triangles.Add(Vertices.Num() - 1);

	// Create mesh section
	ProceduralMeshComponent->ClearAllMeshSections();
	ProceduralMeshComponent->CreateMeshSection(0, Vertices, Triangles, Normals, UV0, VertexColors, Tangents, false);
}

void ALightSource::BeginPlay()
{
	Super::BeginPlay();

	ProceduralMeshComponent->SetMaterial(0, LightMaterial);
}

