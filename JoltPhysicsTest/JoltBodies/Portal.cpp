// Fill out your copyright notice in the Description page of Project Settings.


#include "Portal.h"
#include "Components/SphereComponent.h"
#include "JoltPhysicsTest/Jolt/JoltConverter.h"
#include "JoltPhysicsTest/Jolt/JoltSubsystem.h"

APortal::APortal()
{
	BodyName = EBodyName::Portal;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

#if WITH_EDITORONLY_DATA
	DestinationMarker = CreateEditorOnlyDefaultSubobject<USphereComponent>(TEXT("DestinationMarker"));
	if (DestinationMarker)
	{
		DestinationMarker->SetupAttachment(RootScene);
		DestinationMarker->InitSphereRadius(50.f);
		DestinationMarker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		DestinationMarker->ShapeColor = FColor::Cyan;
		DestinationMarker->bIsEditorOnly = true;
	}
#endif
}

bool APortal::CanTeleportBody(int32 InBodyIndex, int32 CurrentFrame) const
{
	const int32* LastFrame = BodyLastTeleportFrame.Find(InBodyIndex);
	return !LastFrame || CurrentFrame - *LastFrame >= CooldownFrames;
}

void APortal::RecordTeleport(int32 InBodyIndex, int32 CurrentFrame)
{
	BodyLastTeleportFrame.Add(InBodyIndex, CurrentFrame);
}

void APortal::Save(StateRecorderImpl& InStream)
{
	TArray<int32> Keys;
	BodyLastTeleportFrame.GetKeys(Keys);
	Keys.Sort();

	InStream.Write(Keys.Num());
	for (int32 Key : Keys)
	{
		InStream.Write(Key);
		InStream.Write(BodyLastTeleportFrame[Key]);
	}
}

void APortal::Load(StateRecorderImpl& InStream)
{
	BodyLastTeleportFrame.Empty();
	int32 Num = 0;
	InStream.Read(Num);
	for (int32 i = 0; i < Num; i++)
	{
		int32 Key = 0;
		int32 Value = 0;
		InStream.Read(Key);
		InStream.Read(Value);
		BodyLastTeleportFrame.Add(Key, Value);
	}
}

void APortal::BodyEntered(const BodyID& InBodyID)
{
	auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();
	auto BodyInterface = JoltSubsystem->GetBodyInterface();

	const Vec3 NewPos = Units::ToJoltPos(GetActorLocation() + TeleportDestination);
	BodyInterface->SetPosition(InBodyID, NewPos, EActivation::Activate);

	if (bReverseVelocityOnTeleport)
	{
		BodyInterface->SetLinearVelocity(InBodyID, -BodyInterface->GetLinearVelocity(InBodyID));
	}
}

void APortal::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

#if WITH_EDITORONLY_DATA
	if (DestinationMarker)
	{
		DestinationMarker->SetRelativeLocation(TeleportDestination);
	}
#endif
}
