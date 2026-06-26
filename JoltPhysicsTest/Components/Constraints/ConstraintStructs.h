#pragma once

#include "JoltPhysicsTest/Data/SpringSettingsData.h"
#include "JoltPhysicsTest/Debug/CustomLogger.h"
#include "ConstraintStructs.generated.h"

USTRUCT(BlueprintType)
struct FSpringSettings
{
	GENERATED_BODY()

	/** How quickly the body returns to rest. 0 = oscillates for a long time, 1 = almost immediate; */
	UPROPERTY(EditAnywhere, Meta = (EditCondition = "SettingsDataAsset == nullptr"))
	float Damping = 1.f;

	/** Frequency = 0, means the spring is disabled. Can only specify Frequency OR Stiffness. But not both. */
	UPROPERTY(EditAnywhere, Meta = (EditCondition = "SettingsDataAsset == nullptr"))
	float Frequency = 0.f;

	/** Can only specify Frequency OR Stiffness. But not both */
	UPROPERTY(EditAnywhere, Meta = (EditCondition = "SettingsDataAsset == nullptr"))
	float Stiffness = 0.f;

	UPROPERTY(EditAnywhere)
	TObjectPtr<USpringSettingsData> SettingsDataAsset = nullptr;

	SpringSettings ToJoltSpringSettings() const
	{
		SpringSettings JoltSpringSettings;
		if (SettingsDataAsset)
		{
			JoltSpringSettings.mDamping = SettingsDataAsset->Damping;
			check(SettingsDataAsset->Frequency == 0.f || SettingsDataAsset->Stiffness == 0.f);

			if (SettingsDataAsset->Frequency > 0.f)
			{
				JoltSpringSettings.mFrequency = SettingsDataAsset->Frequency;
				JoltSpringSettings.mMode = ESpringMode::FrequencyAndDamping;
			}
			else
			{
				JoltSpringSettings.mStiffness = SettingsDataAsset->Stiffness;
				JoltSpringSettings.mMode = ESpringMode::StiffnessAndDamping;
			}

			return JoltSpringSettings;
		}
		
		JoltSpringSettings.mDamping = Damping;
		check(Frequency == 0.f || Stiffness == 0.f);

		if (Frequency > 0.f)
		{
			JoltSpringSettings.mFrequency = Frequency;
			JoltSpringSettings.mMode = ESpringMode::FrequencyAndDamping;
		}
		else
		{
			JoltSpringSettings.mStiffness = Stiffness;
			JoltSpringSettings.mMode = ESpringMode::StiffnessAndDamping;
		}

		return JoltSpringSettings;
	}
};

UENUM(BlueprintType)
enum class EMotor
{
	Off,
	Position,
	Velocity
};

USTRUCT(BlueprintType)
struct FMotorSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, meta=(EditCondition="!bConfigureMinMaxLimitsSeparately"))
	float ForceLimit = 3000000.f;

	UPROPERTY(EditAnywhere, meta=(EditCondition="!bConfigureMinMaxLimitsSeparately"))
	float TorqueLimit = 3000000.f;

	/** Allows to configure min and max force limits separately. Which allows a motor to have uneven push/pull forces. */
	UPROPERTY(EditAnywhere)
	bool bConfigureMinMaxLimitsSeparately = false;
	
	/** Minimum force to apply in case of a linear constraint (N). Usually this is -mMaxForceLimit unless you want a motor that can e.g. push but not pull. Not used when motor is an angular motor. */
	UPROPERTY(EditAnywhere, meta=(EditCondition="bConfigureMinMaxLimitsSeparately"))
	float MinForceLimit = 0.f;

	/** Maximum force to apply in case of a linear constraint (N). Not used when motor is an angular motor. */
	UPROPERTY(EditAnywhere, meta=(EditCondition="bConfigureMinMaxLimitsSeparately"))
	float MaxForceLimit = 0.f;

	/** Minimum torque to apply in case of a angular constraint (N m). Usually this is -mMaxTorqueLimit unless you want a motor that can e.g. push but not pull. Not used when motor is a position motor. */
	UPROPERTY(EditAnywhere, meta=(EditCondition="bConfigureMinMaxLimitsSeparately"))
	float MinTorqueLimit = 0.f;

	/** Maximum torque to apply in case of a angular constraint (N m). Not used when motor is a position motor. */
	UPROPERTY(EditAnywhere, meta=(EditCondition="bConfigureMinMaxLimitsSeparately"))
	float MaxTorqueLimit = 0.f;

	/** Settings for the spring that is used to drive to the position target (not used when motor is a velocity motor). */
	UPROPERTY(EditAnywhere, Category = "Spring")
	FSpringSettings MotorSpringSettings;

	MotorSettings ToJoltMotorSettings() const
	{
		MotorSettings JoltMotorSettings;
		JoltMotorSettings.mSpringSettings = MotorSpringSettings.ToJoltSpringSettings();

		if (bConfigureMinMaxLimitsSeparately)
		{
			JoltMotorSettings.mMinForceLimit = MinForceLimit;
			JoltMotorSettings.mMaxForceLimit = MaxForceLimit;
		
			JoltMotorSettings.mMinTorqueLimit = MinTorqueLimit;
			JoltMotorSettings.mMaxTorqueLimit = MaxTorqueLimit;
		}
		else
		{
			JoltMotorSettings.mMinForceLimit = -ForceLimit;
			JoltMotorSettings.mMaxForceLimit = ForceLimit;
		
			JoltMotorSettings.mMinTorqueLimit = -TorqueLimit;
			JoltMotorSettings.mMaxTorqueLimit = TorqueLimit;
		}
			
		return JoltMotorSettings;
	}
};

USTRUCT(BlueprintType)
struct FDistanceConstraintSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	float MaxDistance = 0.f;
	
	UPROPERTY(EditAnywhere)
	float MinDistance = 0.f;

	/** When enabled, this makes the limits soft. When the constraint exceeds the limits, a spring force will pull it back. */
	UPROPERTY(EditAnywhere, Category = "Spring")
	FSpringSettings CustomSpringSettings;
};

UENUM(BlueprintType)
enum class EWorldAxis
{
	X,
	Y,
	Z
};

static Vec3 WorldAxisToJoltAxis(EWorldAxis InWorldAxis)
{
	switch (InWorldAxis)
	{
		case EWorldAxis::X:
			return Vec3::sAxisX();
		case EWorldAxis::Y:
			return Vec3::sAxisZ();
		case EWorldAxis::Z:
			return Vec3::sAxisY();
		default:
			checkNoEntry();
			return Vec3::sAxisY();
	}
}

USTRUCT(BlueprintType)
struct FHingeConstraintSettings
{
	GENERATED_BODY()

	/** The axis for the hinge to rotate around */
	UPROPERTY(EditAnywhere)
	EWorldAxis HingeAxis = EWorldAxis::Y;

	/** Should be perpendicular to Hinge axis */
	UPROPERTY(EditAnywhere)
	EWorldAxis NormalAxis = EWorldAxis::Z;
	
	/** Min allowed angle in degrees (-180 - 0) */
	UPROPERTY(EditAnywhere)
	float MinAngle = -180.f;

	/** Max allowed angle in degrees (0 - 180) */
	UPROPERTY(EditAnywhere)
	float MaxAngle = 180.f;

	/** Maximum amount of torque (N m) to apply as friction when the constraint is not powered by a motor */
	UPROPERTY(EditAnywhere, meta=(EditCondition="Motor == EMotor::Off"))
	float MaxFrictionTorque = 0.f;

	/** When enabled, this makes the limits soft. When the constraint exceeds the limits, a spring force will pull it back. */
	UPROPERTY(EditAnywhere, Category = "Spring")
	FSpringSettings CustomSpringSettings;

	UPROPERTY(EditAnywhere)
	EMotor Motor = EMotor::Off;

	/** Motor will drive to target angle. Degrees (-180 - 180) */
	UPROPERTY(EditAnywhere, meta=(EditCondition="Motor == EMotor::Position"))
	float TargetAngle = 0.f;

	/** Motor will drive to target velocity. deg/s */
	UPROPERTY(EditAnywhere, meta=(EditCondition="Motor == EMotor::Velocity"))
	float TargetVelocity = 0.f;

	/** In case the constraint is powered, this determines the motor settings around the hinge axis. **/
	UPROPERTY(EditAnywhere, Category = "Motor", meta=(EditCondition="Motor != EMotor::Off"))
	FMotorSettings CustomMotorSettings;
};

USTRUCT(BlueprintType)
struct FSliderConstraintSettings
{
	GENERATED_BODY()

	/** When the bodies move so that mPoint1 coincides with mPoint2 the slider position is defined to be 0, movement will be limited between [mLimitsMin, mLimitsMax] where mLimitsMin e [-inf, 0] and mLimitsMax e [0, inf] */
	UPROPERTY(EditAnywhere)
	float LimitsMin = 0.f;

	/** When the bodies move so that mPoint1 coincides with mPoint2 the slider position is defined to be 0, movement will be limited between [mLimitsMin, mLimitsMax] where mLimitsMin e [-inf, 0] and mLimitsMax e [0, inf] */
	UPROPERTY(EditAnywhere)
	float LimitsMax = 10.f;

	/** Maximum amount of friction force to apply (N) when not driven by a motor. */
	UPROPERTY(EditAnywhere, meta=(EditCondition="Motor == EMotor::Off"))
	float MaxFrictionForce = 0.f;

	/** When enabled, this makes the limits soft. When the constraint exceeds the limits, a spring force will pull it back. */
	UPROPERTY(EditAnywhere, Category = "Spring")
	FSpringSettings CustomSpringSettings;

	UPROPERTY(EditAnywhere, Category = "Motor")
	EMotor Motor = EMotor::Off;

	/** Motor will drive to target position. */
	UPROPERTY(EditAnywhere, Category = "Motor", meta=(EditCondition="Motor == EMotor::Position"))
	float TargetPosition = 0.f;

	/** Motor will apply force to reach target velocity. */
	UPROPERTY(EditAnywhere, Category = "Motor", meta=(EditCondition="Motor == EMotor::Velocity"))
	float TargetVelocity = 0.f;

	/** In case the constraint is powered, this determines the motor settings around the hinge axis. **/
	UPROPERTY(EditAnywhere, Category = "Motor", meta=(EditCondition="Motor != EMotor::Off"))
	FMotorSettings CustomMotorSettings;
};