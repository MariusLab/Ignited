#pragma once

#include "Engine/DataTable.h"
#include "JoltPhysicsTest/Jolt/JoltBodyActor.h"
#include "LootItems.generated.h"

UENUM(BlueprintType)
enum class ELootItem : uint8
{
	None,
	AdditionalProjectile,
	BiggerProjectile,
	IncreaseProjectileSpeed,
	ProjectileBounce,
	AdditionalProjectileOnImpact,
	MoreAmmo,
	FasterAmmoReload,
	FasterShootSpeed,
	ProjectileShotgun,
	MachineGun,
	Boomerang,
	Turret,
	HomingMissile,
	MineSweeper,
	Dash,
	Sword,
	GravityBomb,
	StickyBomb,
	OrbitalSpheres
};

UENUM()
enum class ELootItemCategory : uint8
{
	None,
	Weapon
};

USTRUCT(BlueprintType)
struct FLootItem : public FTableRowBase
{
	GENERATED_BODY()
public:
	FLootItem() {}

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
	ELootItemCategory Category = ELootItemCategory::None;
	
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
	ELootItem Type = ELootItem::None;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
	FText Name;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
	FText Description;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
	TObjectPtr<UTexture> IconTexture = nullptr;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
	TSoftClassPtr<AJoltBodyActor> ItemJoltBodyClass;

	template<typename T>
	T* GetItemCDO()
	{
		return ItemJoltBodyClass.LoadSynchronous()->GetDefaultObject<T>();
	}

	UClass* GetItemClass()
	{
		return ItemJoltBodyClass.LoadSynchronous();
	}

	UPROPERTY(EditDefaultsOnly, Category = "Ability")
	int FrameInterval = 5;

	UPROPERTY(EditDefaultsOnly, Category = "Ability")
	int TotalFrameDuration = 60;

	UPROPERTY(EditDefaultsOnly, Category = "Ability")
	int DegreesOffset = 15;
};