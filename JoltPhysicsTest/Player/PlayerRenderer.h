// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/DynamicMeshComponent.h"
#include "GameFramework/Actor.h"
#include "JoltPhysicsTest/Data/PlayerRenderFrameData.h"
#include "PlayerRenderer.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerRenderFrame, FPlayerRenderFrameData, PlayerRenderFrameData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRespawnCountdown, int, SecondsLeft);

UCLASS()
class JOLTPHYSICSTEST_API APlayerRenderer : public AActor
{
	GENERATED_BODY()

public:
	APlayerRenderer();

	UPROPERTY(BlueprintAssignable)
	FOnPlayerRenderFrame OnPlayerRenderFrame;

	UPROPERTY(BlueprintAssignable)
	FOnRespawnCountdown OnRespawnCountdown;

	UFUNCTION(BlueprintImplementableEvent)
	void OnSaySomething(const FText& InText);

	void SetBelongsToLocalPlayer(bool bLocal);
protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<USceneComponent> RootSceneComponent = nullptr;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UStaticMeshComponent> UpperArmMeshL = nullptr;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UStaticMeshComponent> LowerArmMeshL = nullptr;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UStaticMeshComponent> UpperArmMeshR = nullptr;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UStaticMeshComponent> LowerArmMeshR = nullptr;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
	TObjectPtr<UStaticMeshComponent> StickMesh = nullptr;

	UFUNCTION(BlueprintPure)
	bool BelongsToLocalPlayer();

	static void UpdateMeshLocationAndRotationGivenTwoPoints(UStaticMeshComponent* StaticMeshComponent, const FVector& StartLocation, const FVector& EndLocation);
	static void UpdateMeshLocationAndRotationGivenTwoPoints2D(UStaticMeshComponent* StaticMeshComponent, const FVector& StartLocation, const FVector& EndLocation);

	UPROPERTY(BlueprintReadOnly, Category = "PlayerRenderer")
	FString DefaultPlayerName;

	UFUNCTION(BlueprintImplementableEvent, Category = "PlayerRenderer", Meta = (DisplayName = "On Match Has Started"))
	void BP_OnMatchHasStarted(int PlayerId);


private:
	bool bBelongsToLocalPlayer = false;
	//void UpdateBodyShape(const TArray<FVector>& Locations);

private:
	//TArray<FVector> GenerateSmoothCurvePoints(const TArray<FVector>& ControlPoints, int32 PointsPerSegment);
	//FVector CatmullRom(const FVector& P0, const FVector& P1, const FVector& P2, const FVector& P3, float T);
};
