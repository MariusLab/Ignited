// Fill out your copyright notice in the Description page of Project Settings.


#include "EyesAnimationComponent.h"

void UEyesAnimationComponent::Update(const int& SimulationFrameId)
{
	if (bFirstFrame)
	{
		OriginalRelativeLocation = GetRelativeLocation();
		bFirstFrame = false;
	}
	
	LevitateEyes(SimulationFrameId);
	
	if (CurrentState == EEyeState::Idle)
	{
		IdleUpdate(SimulationFrameId);
	}

	if (CurrentState == EEyeState::Mean)
	{
		MeanEyesUpdate(SimulationFrameId);
	}
	
	if (CurrentState == EEyeState::Blink)
	{
		BlinkUpdate(SimulationFrameId);
	}

	if (CurrentState == EEyeState::Idle)
	{
		Super::Update(SimulationFrameId);
	}
}

void UEyesAnimationComponent::UpdatePupils(const FVector& LookDirection)
{
	FVector UVsOffset = FVector(LookDirection.X, LookDirection.Z, 0.f) * FVector(PupilsMaxUVsX, PupilsMaxUVsY, 0.f);
	GetDynamicMaterial()->SetVectorParameterValue("PupilsUVOffset", UVsOffset);
}

void UEyesAnimationComponent::TriggerMeanEyes(const int& SimulationFrameId)
{
	TransitionToState(EEyeState::Mean, SimulationFrameId);
}

void UEyesAnimationComponent::LevitateEyes(const int& SimulationFrameId)
{
	const float CurveTime = static_cast<float>(SimulationFrameId % LevitateLoopXFrames) / static_cast<float>(LevitateLoopXFrames);
	const float LevitateAlpha = LevitateCurve->GetFloatValue(CurveTime);
	SetRelativeLocation(OriginalRelativeLocation + FVector(0, 0, FMath::Lerp(0.f, LevitateHeight, LevitateAlpha)));
}

void UEyesAnimationComponent::IdleUpdate(const int& SimulationFrameId)
{
	const int BlinkEveryXFrames = BlinkEveryXSeconds * 60;
	if (SimulationFrameId > 10 && SimulationFrameId % BlinkEveryXFrames <= 1)
	{
		TransitionToState(EEyeState::Blink, SimulationFrameId);
	}
}

void UEyesAnimationComponent::BlinkUpdate(const int& SimulationFrameId)
{
	const int SimulationFramesNumSinceStateChange = SimulationFrameId - StateChangedSimulationFrameId;
		
	const int ChangeFrameEveryXFrames = 60 / BlinkFramesPerSecond;
	const int NewFrameNum = (SimulationFramesNumSinceStateChange / ChangeFrameEveryXFrames) % BlinkMaxFrames;

	if (NewFrameNum < FrameNum)
	{
		TransitionToState(EEyeState::Idle, SimulationFrameId);

		return;
	}

	FrameNum = NewFrameNum;
		
	GetDynamicMaterial()->SetScalarParameterValue("FrameNum", FrameNum);
}

void UEyesAnimationComponent::MeanEyesUpdate(const int& SimulationFrameId)
{
	const int SimulationFramesNumSinceStateChange = SimulationFrameId - StateChangedSimulationFrameId;
		
	const int ChangeFrameEveryXFrames = 60 / MeanEyesFramesPerSecond;
	const int NewFrameNum = (SimulationFramesNumSinceStateChange / ChangeFrameEveryXFrames) % MeanEyesMaxFrames;

	if (NewFrameNum < FrameNum)
	{
		TransitionToState(EEyeState::Idle, SimulationFrameId);

		return;
	}

	FrameNum = NewFrameNum;
		
	GetDynamicMaterial()->SetScalarParameterValue("FrameNum", FrameNum);
}

void UEyesAnimationComponent::TransitionToState(EEyeState NewState, const int& SimulationFrameId)
{
	if (CurrentState == NewState)
	{
		return;
	}

	if (NewState == EEyeState::Idle)
	{
		GetDynamicMaterial()->SetTextureParameterValue("TextureArray", IdleTextureArray);
	}

	if (NewState == EEyeState::Blink)
	{
		GetDynamicMaterial()->SetTextureParameterValue("TextureArray", BlinkTextureArray);
	}

	if (NewState == EEyeState::Mean)
	{
		GetDynamicMaterial()->SetTextureParameterValue("TextureArray", MeanEyesTextureArray);
	}

	FrameNum = 0;
	CurrentState = NewState;
	StateChangedSimulationFrameId = SimulationFrameId;
}
