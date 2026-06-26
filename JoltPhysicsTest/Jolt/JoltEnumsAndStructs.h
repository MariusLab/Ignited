#pragma once

#include "JoltPhysicsTest/Debug/CustomLogger.h"
#include "JoltEnumsAndStructs.generated.h"

// Layer that objects can be in, determines which other objects it can collide with
// Typically you at least want to have 1 layer for moving bodies and 1 layer for static bodies, but you can have more
// layers if you want. E.g. you could have a layer for high detail collision (which is not used by the physics simulation
// but only if you do collision testing).
namespace Layers
{
	static constexpr ObjectLayer NON_MOVING = 0;
	static constexpr ObjectLayer MOVING = 1;
	static constexpr ObjectLayer NO_COLLISION = 2;
	static constexpr ObjectLayer PLAYER = 3;
	static constexpr ObjectLayer PLAYER2 = 4;
	static constexpr ObjectLayer PICKUP = 5;
	static constexpr ObjectLayer PROJECTILE = 6;
	static constexpr ObjectLayer OUTOFBOUNDS = 7;
	static constexpr ObjectLayer PLAYERTAKEDAMAGESENSOR = 8;
	static constexpr ObjectLayer FLAGBASE = 9;
	static constexpr ObjectLayer FLAG = 10;
	static constexpr ObjectLayer LOOTBOX = 11;
	static constexpr ObjectLayer CROWN = 12;
	static constexpr ObjectLayer PLAYERSENSOR = 13;
	static constexpr ObjectLayer BOOMERANG = 14;
	static constexpr ObjectLayer BLADE = 15;
	static constexpr ObjectLayer SWORD = 16;
	static constexpr ObjectLayer BLACKHOLE = 17;
	static constexpr ObjectLayer DEBRIS = 18;
	static constexpr ObjectLayer PORTAL = 19;
	static constexpr ObjectLayer NUM_LAYERS = 20;
};

UENUM(BlueprintType)
enum class EObjectLayer : uint8
{
	NonMoving = Layers::NON_MOVING,	
	Moving = Layers::MOVING,
	NoCollision = Layers::NO_COLLISION,
	Player = Layers::PLAYER,
	Player2 = Layers::PLAYER2,
	Pickup = Layers::PICKUP,
	Projectile = Layers::PROJECTILE,
	OutOfBounds = Layers::OUTOFBOUNDS,
	PlayerTakeDamageSensor = Layers::PLAYERTAKEDAMAGESENSOR,
	FlagBase = Layers::FLAGBASE,
	Flag = Layers::FLAG,
	Lootbox = Layers::LOOTBOX,
	Crown = Layers::CROWN,
	PlayerSensor = Layers::PLAYERSENSOR,
	Boomerang = Layers::BOOMERANG,
	Blade = Layers::BLADE,
	Sword = Layers::SWORD,
	Blackhole = Layers::BLACKHOLE,
	Debris = Layers::DEBRIS,
	Portal = Layers::PORTAL
};

UENUM(BlueprintType)
enum class EObjectMotion : uint8
{
	Static = EMotionType::Static,
	Dynamic = EMotionType::Dynamic,
	Kinematic = EMotionType::Kinematic,
};

UENUM(BlueprintType)
enum class EObjectMotionQuality : uint8
{
	Discrete = EMotionQuality::Discrete,
	LinearCast = EMotionQuality::LinearCast,
};

USTRUCT(BlueprintType)
struct FPhysicsProperties
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere)
	EObjectMotion Motion = EObjectMotion::Dynamic;

	UPROPERTY(EditAnywhere)
	EObjectLayer Layer = EObjectLayer::Moving;

	/** Discrete is cheaper, but thing, fast moving objects might tunnel through geometry. Linear cast is more accurate, but more expensive. */
	UPROPERTY(EditAnywhere)
	EObjectMotionQuality MotionQuality = EObjectMotionQuality::Discrete;

	/** By default Jolt will calculate this automatically for you */
	UPROPERTY(EditAnywhere, meta = (editcondition = "bOverrideMass", ClampMin = "0.001", UIMin = "0.001", DisplayName = "Mass (kg)"))
	float Mass = 0.f;

	UPROPERTY(EditAnywhere)
	float Friction = 0.2f;												///< Friction of the body (dimensionless number, usually between 0 and 1, 0 = no friction, 1 = friction force equals force that presses the two bodies together). Note that bodies can have negative friction but the combined friction (see PhysicsSystem::SetCombineFriction) should never go below zero.

	UPROPERTY(EditAnywhere)
	float Restitution = 0.0f;											///< Restitution of body (dimensionless number, usually between 0 and 1, 0 = completely inelastic collision response, 1 = completely elastic collision response). Note that bodies can have negative restitution but the combined restitution (see PhysicsSystem::SetCombineRestitution) should never go below zero.

	UPROPERTY(EditAnywhere)
	float LinearDamping = 0.05f;											///< Linear damping: dv/dt = -c * v. c must be between 0 and 1 but is usually close to 0.

	UPROPERTY(EditAnywhere)
	float AngularDamping = 0.05f;										///< Angular damping: dw/dt = -c * w. c must be between 0 and 1 but is usually close to 0.

	UPROPERTY(EditAnywhere)
	float MaxLinearVelocity = 500.0f;									///< Maximum linear velocity that this body can reach (m/s)

	UPROPERTY(EditAnywhere)
	float MaxAngularVelocity = 0.25f * JPH_PI * 60.0f;					///< Maximum angular velocity that this body can reach (rad/s)

	UPROPERTY(EditAnywhere)
	float GravityFactor = 30.f;
	
	UPROPERTY(meta = (InlineEditConditionToggle))
	uint8 bOverrideMass : 1;

	/** If true, body will not be able to rotate at all. */
	UPROPERTY(EditAnywhere)
	bool bDisableRotation = false;
	
	/** If true, body will function as sensor. Collisions will be reported, but not resolved. */
	UPROPERTY(EditAnywhere)
	bool bIsSensor = false;

	bool bAllowDynamicOrKinematic = false;
};
