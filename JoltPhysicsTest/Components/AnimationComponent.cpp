// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimationComponent.h"

#include "JoltPhysicsTest/Game/GameLoopSubsystem.h"

void UAnimationComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!bUpdateAnimationManually)
	{
		auto GameLoopSubsystem = GetWorld()->GetSubsystem<UGameLoopSubsystem>();
		check(GameLoopSubsystem);
		GameLoopSubsystem->OnRenderFrame.AddUniqueDynamic(this, &UAnimationComponent::OnRender);
		StartedOnSimulationFrameId = GameLoopSubsystem->GetSteppingFrameId();
	}
}

void UAnimationComponent::OnRender()
{
	Update(GetWorld()->GetSubsystem<UGameLoopSubsystem>()->GetCurrentFrameId());
}

void UAnimationComponent::SetNewMaterial(UMaterialInterface* NewMaterial)
{
	if (IsValid(MaterialInstanceDynamic))
	{
		MaterialInstanceDynamic = nullptr;
	}

	SetMaterial(0, NewMaterial);
	MaterialInstanceDynamic = CreateDynamicMaterialInstance(0);
}

void UAnimationComponent::Update(const int& SimulationFrameId)
{
	const int FramesPassed = SimulationFrameId - StartedOnSimulationFrameId;
	
	const int ChangeFrameEveryXFrames = 60 / FramesPerSecond;
	const int NewFrameNum = (FramesPassed / ChangeFrameEveryXFrames) % MaxFrames;

	if (NewFrameNum < FrameNum)
	{
		OnAnimationFinished.Broadcast();
	}
	else
	{
		OnAnimationFrameRendered.Broadcast(NewFrameNum);
	}

	FrameNum = NewFrameNum;

	GetDynamicMaterial()->SetScalarParameterValue("FrameNum", FrameNum);
}

UMaterialInstanceDynamic* UAnimationComponent::GetDynamicMaterial()
{
	if (IsValid(MaterialInstanceDynamic))
	{
		return MaterialInstanceDynamic;
	}

	MaterialInstanceDynamic = CreateDynamicMaterialInstance(0);

	return MaterialInstanceDynamic;
}
