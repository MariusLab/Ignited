// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JoltPhysicsTest/Jolt/JoltBodyActor.h"
#include "OrbitalSphere.generated.h"

UCLASS()
class JOLTPHYSICSTEST_API AOrbitalSphere : public AJoltBodyActor
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	int SphereCount = 6;

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float OrbitRadius = 20.f;

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float RotationSpeedDegreesPerFrame = 2.f;

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	int PulseDurationFrames = 60;

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	int PulsePauseFrames = 60;

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float PulseAmplitude = 8.f;

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	int AbilityDurationFramesNum = 300;

	int OrbitIndex = 0;

	virtual void InitializeBody(const FJoltBodyData& JoltBodyData) override;
};
