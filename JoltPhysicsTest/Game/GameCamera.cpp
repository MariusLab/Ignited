// Fill out your copyright notice in the Description page of Project Settings.


#include "GameCamera.h"

#include "GameBackground.h"
#include "JoltPhysicsTest/Components/ParallaxComponent.h"
#include "Kismet/KismetMathLibrary.h"

AGameCamera::AGameCamera()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
}

void AGameCamera::InitGameCamera(FVector InOriginalLocation, AGameBackground* GameBackground)
{
	SetActorLocation(InOriginalLocation);

	if (GameBackground)
	{
		ParallaxComponents.Empty();
		
		TArray<UActorComponent*> ActorComponents;
		GameBackground->GetComponents(ActorComponents);

		for (UActorComponent* ActorComponent : ActorComponents)
		{
			if (UParallaxComponent* ParallaxComponent = Cast<UParallaxComponent>(ActorComponent))
			{
				ParallaxComponent->Init(InOriginalLocation);
				ParallaxComponents.Add(ParallaxComponent);
			}
		}

		GameBackground->AttachToActor(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	}
}

bool AGameCamera::IsCameraInitialized() const
{
	return bCameraInitialized;
}

void AGameCamera::Shake(float Strength)
{
	if (bCameraShake || bDirectionalShake)
	{
		return;
	}
	
	ResetToDefaults();
	
	bCameraShake = true;
	InternalShakeStrength = Strength * ShakeStrengthMultiplier;
	
	SetActorTickEnabled(true);
}

void AGameCamera::DirectionalShake(const FVector& InDirection, float Strength)
{
	if (bCameraShake || bDirectionalShake)
	{
		return;
	}
	
	ResetToDefaults();
	
	bDirectionalShake = true;
	ShakeDirection = InDirection * -1.f;

	CameraShakeTimePassed = 0.f;
	OriginalCameraTransform = GetActorTransform();

	InternalShakeStrength = Strength * ShakeStrengthMultiplier;

	SetActorTickEnabled(true);
}

void AGameCamera::Update(const FVector& InNewLocation, const float& InDeltaSeconds)
{
	CameraLocation = InNewLocation;
	SetActorLocation(CameraLocation + AdditiveCameraLocation);
	
	for (UParallaxComponent* ParallaxComponent : ParallaxComponents)
	{
		ParallaxComponent->Update(InNewLocation + AdditiveCameraLocation, InDeltaSeconds);
	}
}

void AGameCamera::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	AccumulatedDelta += DeltaTime;
	while (AccumulatedDelta >= CurrentStepTime * 2.f)
	{
		AccumulatedDelta -= CurrentStepTime;
	}

	if (AccumulatedDelta >= CurrentStepTime)
	{
		AccumulatedDelta -= CurrentStepTime;
		
		if (bDirectionalShake)
		{
			// 18 frames at a fixed 60 fps
			constexpr float CameraShakeDuration = 0.288f;
			// 12 frames
			//constexpr float CameraShakeDuration = 0.192f;
			const float MaxShakeDistance = 250.f * InternalShakeStrength;

			const float CurveAlpha = FMath::Clamp(CameraShakeTimePassed / CameraShakeDuration, 0.f, 1.f);
			//LOG_TEMPORARY(TEXT("CurveAlpha: %f"), CurveAlpha);
			AdditiveCameraLocation = UKismetMathLibrary::VLerp(FVector::ZeroVector, ShakeDirection * MaxShakeDistance, DirectionalShakeCurve->GetFloatValue(CurveAlpha));
			//LOG_TEMPORARY(TEXT("AdditiveCameraLocation: %s"), *AdditiveCameraLocation.ToString());
			
			if (CameraShakeTimePassed > CameraShakeDuration)
			{
				ResetToDefaults();
			}

			CameraShakeTimePassed += CurrentStepTime;
		}
	
		if (bCameraShake)
		{
			constexpr float CameraShakeDuration = 0.160f;
			constexpr float CameraShakeSpeed = 100.f;
				
			constexpr float MaxShakeAngle = 2.f;
			const float MaxShakePos = 250.f * InternalShakeStrength;
				
			if (!bCameraShakeInitialized)
			{
				CameraShakeTimePassed = 0.f;
				OriginalCameraTransform = GetActorTransform();
				CameraShakeSeed1 = UKismetMathLibrary::RandomFloat() * 100.f;
				CameraShakeSeed2 = UKismetMathLibrary::RandomFloat() * 100.f;
				CameraShakeSeed3 = UKismetMathLibrary::RandomFloat() * 100.f;
				bCameraShakeInitialized = true;
			}
			else
			{
				CameraShakeTimePassed += CurrentStepTime;
			}

				
			const float NewAngle = MaxShakeAngle * UKismetMathLibrary::PerlinNoise1D(CameraShakeTimePassed * CameraShakeSpeed + CameraShakeSeed1);
			const FRotator NewRotation = OriginalCameraTransform.Rotator() + FRotator(0, 0, NewAngle);
			SetActorRotation(NewRotation);

			const float NewPosX = MaxShakePos * UKismetMathLibrary::PerlinNoise1D( CameraShakeTimePassed * CameraShakeSpeed + CameraShakeSeed2);
			const float NewPosZ = MaxShakePos * UKismetMathLibrary::PerlinNoise1D(CameraShakeTimePassed * CameraShakeSpeed + CameraShakeSeed3);
			const FVector NewLocation = OriginalCameraTransform.GetLocation() + FVector(NewPosX, 0.f, NewPosZ);
			SetActorLocation(NewLocation);
				
			if (CameraShakeTimePassed > CameraShakeDuration)
			{
				ResetToDefaults();
			}
		}
	}
}

FVector AGameCamera::GetCameraLocation() const
{
	return CameraLocation;
}

void AGameCamera::ResetToDefaults()
{
	if (bCameraShake)
	{
		bCameraShake = false;
		bCameraShakeInitialized = false;
		CameraShakeTimePassed = 0.f;
		AdditiveCameraLocation = FVector::ZeroVector;

		SetActorTransform(OriginalCameraTransform);

		SetActorTickEnabled(false);
	}

	if (bDirectionalShake)
	{
		bDirectionalShake = false;
		bCameraShakeInitialized = false;
		CameraShakeTimePassed = 0.f;
		AdditiveCameraLocation = FVector::ZeroVector;

		//SetActorTransform(OriginalCameraTransform);

		SetActorTickEnabled(false);
	}
}

