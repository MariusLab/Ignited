#pragma once

#include "PlayerMovementConfigData.generated.h"

USTRUCT()
struct FPlayerMovementHandConfig
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly)
	float HandMass =  65.f;

	UPROPERTY(EditDefaultsOnly)
	float HandFriction = 100.f;

	UPROPERTY(EditDefaultsOnly)
	float HandLinearDamping = 10.f;
};

USTRUCT()
struct FPlayerMovementShoulderConfig
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly)
	float ShoulderMass =  3.f;
	
	UPROPERTY(EditDefaultsOnly)
	float ShoulderFriction = 0.f;

	UPROPERTY(EditDefaultsOnly)
	float ShoulderLinearDamping = 0.f;
};

USTRUCT()
struct FPlayerMovementBodyConfig
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly)
	float BodyMass =  50.f;

	UPROPERTY(EditDefaultsOnly)
	float BodyFriction = 2.f;

	UPROPERTY(EditDefaultsOnly)
	float BodyLinearDamping = 1.f;
};

USTRUCT()
struct FPlayerMovementSliderConfig
{
	GENERATED_BODY()
	
	// How far away can the hammer move from the player center of mass
	UPROPERTY(EditDefaultsOnly)
	float SliderDistanceMax = 25.f;

	UPROPERTY(EditDefaultsOnly)
	float SliderMotorForceLimit = 1000000000.f;

	UPROPERTY(EditDefaultsOnly)
	float SliderMotorDamping = 1.f;

	UPROPERTY(EditDefaultsOnly)
	float SliderMotorFrequency = 20.f;
};

USTRUCT()
struct FPlayerMovementHingeConfig
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly)
	float HingeMotorTorqueLimit = 10000000.f;

	UPROPERTY(EditDefaultsOnly)
	float HingeMotorDamping = 1.f;

	UPROPERTY(EditDefaultsOnly)
	float HingeMotorFrequency = 15.f;
};

USTRUCT()
struct FPlayerMovementConfig
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	FString Title;
	
	UPROPERTY(EditDefaultsOnly)
	float Gravity = 35.f;

	UPROPERTY(EditDefaultsOnly)
	float MaxLinearVelocity = 1500.f;

	UPROPERTY(EditDefaultsOnly)
	FPlayerMovementHandConfig HandConfig;

	UPROPERTY(EditDefaultsOnly)
	FPlayerMovementBodyConfig BodyConfig;

	UPROPERTY(EditDefaultsOnly)
	FPlayerMovementShoulderConfig ShoulderConfig;
	
	UPROPERTY(EditDefaultsOnly)
	FPlayerMovementSliderConfig SliderConfig;

	UPROPERTY(EditDefaultsOnly)
	FPlayerMovementHingeConfig HingeConfig;
};

UCLASS()
class JOLTPHYSICSTEST_API UPlayerMovementConfigData : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:

	UPROPERTY(EditDefaultsOnly)
	int ConfigIndexToUse = 0;
	
	UPROPERTY(EditDefaultsOnly)
	TArray<FPlayerMovementConfig> AvailableConfigurations;

	inline const FPlayerMovementConfig& Get()
	{
		checkf(AvailableConfigurations.IsValidIndex(ConfigIndexToUse), TEXT("Invalid index requested for movement config"));

		return AvailableConfigurations[ConfigIndexToUse];
	}
	
	inline const FPlayerMovementHandConfig& GetHandConfig()
	{
		checkf(AvailableConfigurations.IsValidIndex(ConfigIndexToUse), TEXT("Invalid index requested for movement config"));

		return AvailableConfigurations[ConfigIndexToUse].HandConfig;
	}

	inline const FPlayerMovementBodyConfig& GetBodyConfig()
	{
		checkf(AvailableConfigurations.IsValidIndex(ConfigIndexToUse), TEXT("Invalid index requested for movement config"));

		return AvailableConfigurations[ConfigIndexToUse].BodyConfig;
	}
	
	inline const FPlayerMovementShoulderConfig& GetShoulderConfig()
	{
		checkf(AvailableConfigurations.IsValidIndex(ConfigIndexToUse), TEXT("Invalid index requested for movement config"));

		return AvailableConfigurations[ConfigIndexToUse].ShoulderConfig;
	}
	
	inline const FPlayerMovementSliderConfig& GetSliderConfig()
	{
		checkf(AvailableConfigurations.IsValidIndex(ConfigIndexToUse), TEXT("Invalid index requested for movement config"));

		return AvailableConfigurations[ConfigIndexToUse].SliderConfig;
	}
	
	inline const FPlayerMovementHingeConfig& GetHingeConfig()
	{
		checkf(AvailableConfigurations.IsValidIndex(ConfigIndexToUse), TEXT("Invalid index requested for movement config"));

		return AvailableConfigurations[ConfigIndexToUse].HingeConfig;
	}
};