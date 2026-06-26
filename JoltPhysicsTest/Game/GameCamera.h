// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraActor.h"
#include "GameCamera.generated.h"

UCLASS()
class JOLTPHYSICSTEST_API AGameCamera : public ACameraActor
{
	GENERATED_BODY()

public:
	AGameCamera();

	void InitGameCamera(FVector InOriginalLocation, class AGameBackground* GameBackground);

	bool IsCameraInitialized() const;

	UPROPERTY(EditDefaultsOnly, Category = "Shake")
	TObjectPtr<UCurveFloat> DirectionalShakeCurve;
	
	void Shake(float Strength = 1.f);
	void DirectionalShake(const FVector& InDirection, float Strength = 1.f);
	void Update(const FVector& InNewLocation, const float& InDeltaSeconds);
	
	virtual void Tick(float DeltaTime) override;

	FVector GetCameraLocation() const;
private:
	void ResetToDefaults();
	
	bool bCameraShake = false;
	bool bCameraShakeInitialized = false;
	float CameraShakeTimePassed = 0.f;
	float CameraShakeSeed1 = 0.f;
	float CameraShakeSeed2 = 0.f;
	float CameraShakeSeed3 = 0.f;
	FTransform OriginalCameraTransform;
	
	FVector CameraLocation = FVector::ZeroVector;
	FVector AdditiveCameraLocation = FVector::ZeroVector;

	// Use this to adjust the overall screen shake effects strength. Could be exposed to game options.
	UPROPERTY(EditDefaultsOnly, Category = "Shake", Meta = (AllowPrivateAccess = "true"))
	float ShakeStrengthMultiplier = 1.0f;
	
	// Don't change this one. It's used for storing data real time
	float InternalShakeStrength = 1.f;

	bool bDirectionalShake = false;
	FVector ShakeDirection;

	TArray<class UParallaxComponent*> ParallaxComponents;

	bool bCameraInitialized = false;

	float AccumulatedDelta = 0.f;
	// This acts as a safeguard to not accumulate too much delta and then explode with too many steps per tick
	// Currently it's at max 10 steps per tick, which already is insane. We might want to lower it even further.
	const float MaxAccumulatedDelta = 0.16f;

	float CurrentStepTime = 0.016f;
};
