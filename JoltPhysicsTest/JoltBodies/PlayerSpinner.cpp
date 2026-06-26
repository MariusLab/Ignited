// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerSpinner.h"

#include "JoltPhysicsTest/Data/AssetManagerSubsystem.h"
#include "JoltPhysicsTest/Data/LootItems.h"
#include "JoltPhysicsTest/FunctionLibrary/Helper.h"
#include "JoltPhysicsTest/Game/SpawnerSubsystem.h"
#include "JoltPhysicsTest/GameInputs/InputsSubsystem.h"
#include "JoltPhysicsTest/Jolt/JoltConverter.h"
#include "JoltPhysicsTest/Jolt/JoltSubsystem.h"
#include "JoltPhysicsTest/Player/MyPlayerController.h"


void APlayerSpinner::InitializeBody(const FJoltBodyData& JoltBodyData)
{
	Super::InitializeBody(JoltBodyData);

	UGeneralData* GeneralData = UAssetManagerSubsystem::GetGeneralData(GetWorld());
	StepsToCompleteSpinner = GeneralData->AbilitySpinnerTakesFramesNum;
	SpinnerStopsOnItemIndex = GeneralData->SpinnerStopsOnItemIndex;
}

void APlayerSpinner::StepSimulation()
{
	Super::StepSimulation();

	auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();
	const Vec3 PlayerPos = JoltSubsystem->GetBodyInterface()->GetCenterOfMassPosition(BodyID(BelongsToPlayerId));
	JoltSubsystem->GetBodyInterface()->SetPosition(GetBodyID(), PlayerPos, EActivation::DontActivate);

	auto LocalPlayerController = UHelper::GetLocalPlayerController(GetWorld());
	if (LocalPlayerController->IsPlayerDead(BelongsToPlayerId))
	{
		GetWorld()->GetSubsystem<USpawnerSubsystem>()->DespawnJoltBody(GetBodyID());
		return;
	}
	
	StepCountSinceOpened++;

	if (StepCountSinceOpened == 1)
	{
		GenerateSpinnerItemsList();
	}

	// We only actually trigger the opening when we're outside the rollback window, so that we don't have to waste time making the UI rollback compliant
	if (!bConfirmedOpen && StepCountSinceOpened > GetWorld()->GetSubsystem<UInputsSubsystem>()->GameInputs->GetRollbackWindow())
	{
		ConfirmedOpenLootbox();
	}

	checkf(StepsToCompleteSpinner > 0, TEXT("Spinner wasn't provided duration"));

	if (StepCountSinceOpened == (StepsToCompleteSpinner - 35))
	{
		LocalPlayerController->SpawnGrantAbilityVFX(Units::FromJoltPos(PlayerPos), BelongsToPlayerId);
	}

	if (StepCountSinceOpened == StepsToCompleteSpinner)
	{
		LocalPlayerController->GrantPlayerItem(BelongsToPlayerId, SelectedRewardItem);
		LocalPlayerController->GetPlayerCharacter(BelongsToPlayerId).CreatedBodyIndexes.Remove(GetBodyID().GetIndex());
		GetWorld()->GetSubsystem<USpawnerSubsystem>()->DespawnJoltBody(GetBodyID());

		LOG_NETWORKING(TEXT("[Frame %d] Grant item to player %d : %s"), GetWorld()->GetSubsystem<UGameLoopSubsystem>()->GetSteppingFrameId(), BelongsToPlayerId, *UEnum::GetValueAsString(SelectedRewardItem));
	}
}

void APlayerSpinner::Save(StateRecorderImpl& InStream)
{
	Super::Save(InStream);

	InStream.Write(StepCountSinceOpened);
}

void APlayerSpinner::Load(StateRecorderImpl& InStream)
{
	Super::Load(InStream);

	InStream.Read(StepCountSinceOpened);
}

void APlayerSpinner::SetPossibleOptions(const TArray<ELootItem>& InPossibleOptions)
{
	PossibleOptions = InPossibleOptions;
}

void APlayerSpinner::ConfirmedOpenLootbox()
{
	check(!bConfirmedOpen);

	bConfirmedOpen = true;

	TArray<FLootItem> LootItems;
	//LOG_NETWORKING(TEXT("Confirmed loot items list"));
	for (auto SpinnerItem : SpinnerItemsList)
	{
		//LOG_NETWORKING(TEXT("%s"), *UEnum::GetValueAsString(SpinnerItem));
		LootItems.Add(UAssetManagerSubsystem::GetLootItemRow(GetWorld(), SpinnerItem));
	}
	
	OnStartSpinning(LootItems, GetSpinDurationInSeconds());
}

void APlayerSpinner::GenerateSpinnerItemsList()
{
	//LOG_TEMPORARY(TEXT("Generate spinner items list"));
	SpinnerItemsList.Empty();
	
	auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();

	const int FinalRewardRandomIndex = JoltSubsystem->RandomNumberGenerator.NextIntInRange(0, PossibleOptions.Num() - 1);
	//LOG_TEMPORARY(TEXT("Final reward random index: %d; %s"), FinalRewardRandomIndex, *UEnum::GetValueAsString(PossibleOptions[FinalRewardRandomIndex]));
	SelectedRewardItem = PossibleOptions[FinalRewardRandomIndex];
	
	for (int i = 0; i < 40; i++)
	{
		const int RandomItemIndex = JoltSubsystem->RandomNumberGenerator.NextIntInRange(0, PossibleOptions.Num() - 1);
		auto RandomItem = PossibleOptions[RandomItemIndex];

		if (i == SpinnerStopsOnItemIndex)
		{
			RandomItem = SelectedRewardItem;
		}
		
		SpinnerItemsList.Add(RandomItem);
	}
}

float APlayerSpinner::GetSpinDurationInSeconds() const
{
	return static_cast<float>(StepsToCompleteSpinner) / 60.f;
}
