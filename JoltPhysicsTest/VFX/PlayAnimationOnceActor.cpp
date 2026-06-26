// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayAnimationOnceActor.h"

#include "Kismet/KismetMathLibrary.h"


APlayAnimationOnceActor::APlayAnimationOnceActor()
{
	PrimaryActorTick.bCanEverTick = false;

	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootSceneComp"));
	SetRootComponent(RootSceneComponent);
	
	AnimationComponent = CreateDefaultSubobject<UAnimationComponent>(TEXT("AnimationComp"));
	AnimationComponent->SetupAttachment(RootSceneComponent);
}

void APlayAnimationOnceActor::BeginPlay()
{
	Super::BeginPlay();

	const float RandomScaleVariation = UKismetMathLibrary::RandomFloatInRange(-ScaleVariation, ScaleVariation);
	const FVector OriginalActorScale = GetActorScale();
	SetActorScale3D(FVector(
		OriginalActorScale.X + RandomScaleVariation,
		OriginalActorScale.Y + RandomScaleVariation,
		OriginalActorScale.Z + RandomScaleVariation
	));

	AnimationComponent->OnAnimationFinished.AddUniqueDynamic(this, &APlayAnimationOnceActor::OnAnimationFinished);
}

void APlayAnimationOnceActor::OnAnimationFinished()
{ 
	if (!bDestroyAfterAnimationFinishes)
	{
		return;
	}
	
	SetActorHiddenInGame(true);
	Destroy();
}

void APlayAnimationOnceActor::SetPlayerTeam(EPlayerTeam PlayerTeam)
{
	if (Team == PlayerTeam)
	{
		return;
	}
	
	Team = PlayerTeam;
	BP_OnTeamUpdated(PlayerTeam);
}

