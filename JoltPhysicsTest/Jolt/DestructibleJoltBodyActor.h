// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JoltBodyActor.h"
#include "DestructibleJoltBodyActor.generated.h"

UCLASS()
class JOLTPHYSICSTEST_API ADestructibleJoltBodyActor : public AJoltBodyActor
{
	GENERATED_BODY()

public:
	void DestroyAndSeparateIntoParts(const Vec3& Impulse, const Vec3& ImpactPoint);
	void RollbackDestroy();
	bool IsDestroyedIntoSeparateParts() const { return bDestroyedIntoSeparateParts; }

	virtual void Save(StateRecorderImpl& InStream) override;
	virtual void Load(StateRecorderImpl& InStream) override;

private:
	bool bDestroyedIntoSeparateParts = false;
};
