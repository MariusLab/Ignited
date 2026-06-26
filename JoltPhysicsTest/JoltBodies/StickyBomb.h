#pragma once

#include "CoreMinimal.h"
#include "JoltPhysicsTest/Jolt/JoltBodyActor.h"
#include "JoltPhysicsTest/Jolt/JoltConstraintActor.h"
#include "StickyBomb.generated.h"

UCLASS()
class JOLTPHYSICSTEST_API AStickyBomb : public AJoltBodyActor
{
	GENERATED_BODY()

public:
	// How many bombs to spawn in the circle burst
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	int NumBombs = 8;

	// Impulse applied to each bomb when launched
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float LaunchImpulse = 3000.f;

	// Radius of the spawn circle around the player (Jolt units)
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float SpawnRadius = 3.f;

	// Frames after sticking before the bomb explodes (180 = 3s at 60fps)
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	int FramesUntilExplosion = 180;

	// Radius of the explosion kill check (Jolt units)
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float ExplosionRadius = 12.f;

	// Point constraint used to pin the bomb to whatever it sticks to
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	TSubclassOf<AJoltConstraintActor> StickConstraintClass;

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	TObjectPtr<class UNiagaraSystem> ExplodeVFX = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	TObjectPtr<class UNiagaraSystem> ReleaseVFX = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Config", Meta = (DevelopmentOnly))
	bool bDrawDebug = false;

	UFUNCTION(BlueprintImplementableEvent)
	void BP_SecondsLeftUntilExplode(int32 SecondsLeft);

	virtual void InitializeBody(const FJoltBodyData& JoltBodyData) override;
	virtual void StepSimulation() override;
	virtual void Save(StateRecorderImpl& InStream) override;
	virtual void Load(StateRecorderImpl& InStream) override;

	void Stick(const BodyID& InTargetBodyID, const Vec3& InContactWorldPos);
	bool IsStuck() const { return bStuck; }

private:
	bool bStuck = false;
	int StepsSinceStuck = 0;
	int StuckConstraintId = -1;
};
