// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AnimationComponent.h"
#include "Engine/Texture2DArray.h"
#include "EyesAnimationComponent.generated.h"

enum EEyeState
{
	Idle,
	Blink,
	Mean
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class JOLTPHYSICSTEST_API UEyesAnimationComponent : public UAnimationComponent
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UTexture2DArray* IdleTextureArray = nullptr;
	
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UTexture2DArray* BlinkTextureArray = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UTexture2DArray* MeanEyesTextureArray = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Animation|Blink")
	int BlinkMaxFrames = 6;

	UPROPERTY(EditDefaultsOnly, Category = "Animation|Blink")
	int BlinkFramesPerSecond = 12;

	UPROPERTY(EditDefaultsOnly, Category = "Animation|Blink")
	int BlinkEveryXSeconds = 5;

	UPROPERTY(EditDefaultsOnly, Category = "Animation|MeanEyes")
	int MeanEyesMaxFrames = 6;

	UPROPERTY(EditDefaultsOnly, Category = "Animation|MeanEyes")
	int MeanEyesFramesPerSecond = 12;

	UPROPERTY(EditDefaultsOnly, Category = "Animation|Levitate")
	int LevitateLoopXFrames = 30;
	
	UPROPERTY(EditDefaultsOnly, Category = "Animation|Levitate")
	float LevitateHeight = 100.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Animation|Levitate")
	class UCurveFloat* LevitateCurve = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Animation|Pupils")
	float PupilsMaxUVsX = 0.004f;

	UPROPERTY(EditDefaultsOnly, Category = "Animation|Pupils")
	float PupilsMaxUVsY = 0.01f;
	
	virtual void Update(const int& SimulationFrameId) override;
	void UpdatePupils(const FVector& LookDirection);
	void TriggerMeanEyes(const int& SimulationFrameId);
private:
	EEyeState CurrentState = EEyeState::Idle;
	int StateChangedSimulationFrameId = -1;
	FVector OriginalRelativeLocation = FVector::ZeroVector;
	bool bFirstFrame = true;

	void LevitateEyes(const int& SimulationFrameId);
	void IdleUpdate(const int& SimulationFrameId);
	void BlinkUpdate(const int& SimulationFrameId);
	void MeanEyesUpdate(const int& SimulationFrameId);

	void TransitionToState(EEyeState NewState, const int& SimulationFrameId);
};
