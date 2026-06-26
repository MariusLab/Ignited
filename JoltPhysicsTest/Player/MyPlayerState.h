// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "MyPlayerState.generated.h"

USTRUCT()
struct FPlayerSettings
{
	GENERATED_BODY()
public:
	// No longer used. This was a previous FLAWED method of controlling mouse sensitivity. Now we use GameSettings->MouseSensitivity
	UPROPERTY()
	int MouseSensitivity = 10;
};

inline FArchive& operator<<(FArchive& Ar, FPlayerSettings& Data)
{
	Ar << Data.MouseSensitivity;

	return Ar;
}

UCLASS()
class JOLTPHYSICSTEST_API AMyPlayerState : public APlayerState
{
	GENERATED_BODY()
public:
	AMyPlayerState();
protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(Reliable, NetMulticast)
	void MulticastSetPlayerIdAndGameSeed(const int& InPlayerId, const FString& GameSeedString);

	UFUNCTION(Reliable, NetMulticast)
	void MulticastSetPlayerSettings(const FPlayerSettings& InPlayerSettings);

	int GetMyPlayerId() const;
	bool IsPlayerIdSet() const;

	void UpdatePlayerSettings(const FPlayerSettings& InPlayerSettings);
	FPlayerSettings GetPlayerSettings() const;
private:
	int PlayerId = 0;
	bool bPlayerIdSet = false;

	FPlayerSettings PlayerSettings;
};
