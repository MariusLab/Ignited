#pragma once

#include "LootItems.h"
#include "JoltPhysicsTest/Player/PlayerTeamsEnum.h"
#include "PlayerRenderFrameData.generated.h"

USTRUCT(BlueprintType)
struct FPlayerRenderFrameData
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadOnly)
	ELootItem EquippedItem = ELootItem::None;
	
	UPROPERTY(BlueprintReadOnly)
	int BulletsLeft = 0;

	UPROPERTY(BlueprintReadOnly)
	int MaxBullets = 0;

	UPROPERTY(BlueprintReadOnly)
	float BulletReloadPerc = 0.f;
	
	UPROPERTY(BlueprintReadOnly)
	float DashReloadPerc = 0.f;

	UPROPERTY(BlueprintReadOnly)
	bool bHasCrown = false;

	UPROPERTY(BlueprintReadOnly)
	int CrownHeldFramesNum = 0;

	UPROPERTY(BlueprintReadOnly)
	int InvulnerabilityFramesNum = 0;

	UPROPERTY(BlueprintReadOnly)
	float ShootChargePerc = 0.f;

	UPROPERTY(BlueprintReadOnly)
	EPlayerTeam CarryingFlagOfTeam = EPlayerTeam::None;
};