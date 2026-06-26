#pragma once

UENUM(BlueprintType)
enum class EPlayerBodyState : uint8
{
	Idle             UMETA(DisplayName = "Idle"),
	Walk             UMETA(DisplayName = "Walk"),
	Knockback        UMETA(DisplayName = "Knockback"),
	KnockbackExplode UMETA(DisplayName = "Knockback Explode"),
};
