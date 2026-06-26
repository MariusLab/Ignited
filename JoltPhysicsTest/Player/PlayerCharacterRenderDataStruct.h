#pragma once
#include "PlayerBodyVFX.h"
#include "JoltPhysicsTest/Data/GeneralData.h"
#include "JoltPhysicsTest/Data/LootItems.h"

#include "PlayerCharacterRenderDataStruct.generated.h"

USTRUCT(BlueprintType)
struct FPlayerCharacterRenderData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int PlayerId = 0;

	int HandBodyIndex = -1;

	UPROPERTY(BlueprintReadOnly)
	FVector CursorLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly)
	FVector2D MouseDelta = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly)
	FVector BodyLocation = FVector::ZeroVector;

	/** 0 - 360.f deg */
	UPROPERTY(BlueprintReadOnly)
	float HandRotationDeg = 0.f;

	UPROPERTY(BlueprintReadOnly)
	FVector LeftHandStartLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly)
	FVector LeftHandJointLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly)
	FVector LeftHandEndLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly)
	FVector RightHandStartLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly)
	FVector RightHandJointLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly)
	FVector RightHandEndLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly)
	FVector HeadStartLocation = FVector::ZeroVector;
	UPROPERTY(BlueprintReadOnly)
	FVector HeadEndLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly)
	FVector LeftThighStartLocation = FVector::ZeroVector;
	UPROPERTY(BlueprintReadOnly)
	FVector LeftThighEndLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly)
	FVector LeftCalfStartLocation = FVector::ZeroVector;
	UPROPERTY(BlueprintReadOnly)
	FVector LeftCalfEndLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly)
	FVector RightThighStartLocation = FVector::ZeroVector;
	UPROPERTY(BlueprintReadOnly)
	FVector RightThighEndLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly)
	FVector RightCalfStartLocation = FVector::ZeroVector;
	UPROPERTY(BlueprintReadOnly)
	FVector RightCalfEndLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly)
	FVector StickStartLocation = FVector::ZeroVector;
	UPROPERTY(BlueprintReadOnly)
	FVector StickEndLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly)
	UStaticMeshComponent* HandStaticMeshComponent = nullptr;

	UPROPERTY(BlueprintReadOnly)
	UMaterialInstanceDynamic* BodyDynamicMaterial = nullptr;

	class UAnimationComponent* HeadAnimationComp = nullptr;

	class UEyesAnimationComponent* EyesAnimationComp = nullptr;

	UPROPERTY(BlueprintReadOnly)
	UMaterialInstanceDynamic* HeadDynamicMaterial = nullptr;

	UPROPERTY(BlueprintReadOnly)
	UMaterialInstanceDynamic* EyesDynamicMaterial = nullptr;
	
	UPROPERTY(BlueprintReadOnly)
	UMaterialInstanceDynamic* HammerDynamicMaterial = nullptr;

	UPROPERTY(BlueprintReadOnly)
	UMaterialInstanceDynamic* StickDynamicMaterial = nullptr;

	FPlayerColorScheme ColorScheme;

	UPROPERTY(BlueprintReadOnly)
	int MaxBullets = 0;

	UPROPERTY(BlueprintReadOnly)
	TArray<ELootItem> PlayerItems;

	UPROPERTY()
	TWeakObjectPtr<APlayerBodyVFX> ActivePlayerBodyVFX = nullptr;

	UPROPERTY(BlueprintReadOnly)
	int SimulationFrameId = -1;
};