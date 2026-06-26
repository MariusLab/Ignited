// Fill out your copyright notice in the Description page of Project Settings.


#include "SoundSubsystem.h"

#include "JoltPhysicsTest/Data/AssetManagerSubsystem.h"
#include "JoltPhysicsTest/Game/GameLoopSubsystem.h"
#include "JoltPhysicsTest/Jolt/JoltSubsystem.h"
#include "JoltPhysicsTest/Player/PlayerRenderer.h"
#include "Kismet/GameplayStatics.h"

bool USoundSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (!Super::ShouldCreateSubsystem(Outer))
	{
		return false;
	}

	const TArray AllowedNetModes{NM_Client, NM_ListenServer, NM_Standalone};

	const auto* World = Cast<UWorld>(Outer);
	if (World == nullptr)
	{
		return false;
	}

	const auto* GameInstance = Cast<UGameInstance>(World->GetGameInstance());
	if (GameInstance == nullptr)
	{
		return false;
	}

	if (World->WorldType != EWorldType::Game && World->WorldType != EWorldType::PIE)
	{
		return false;
	}

	return AllowedNetModes.Contains(World->GetNetMode());
}

void USoundSubsystem::PostInitialize()
{
	Super::PostInitialize();

	SoundEffectsTable = UAssetManagerSubsystem::GetGeneralData(GetWorld())->SoundEffectsDataTable;
	checkf(SoundEffectsTable, TEXT("Could not load SoundEffects table"));
}

void USoundSubsystem::PlaySound(FString Name)
{
	const FString ContextString = TEXT("USoundSubsystem::PlaySound");
	
	const auto Row = SoundEffectsTable->FindRow<FSoundEffect>(*Name, ContextString);
	checkf(Row, TEXT("Row not found for play sound"));

	const int SteppingFrameId = GetWorld()->GetSubsystem<UGameLoopSubsystem>()->GetSteppingFrameId();
	if (Row->MinFramesToWaitBeforePlayingSoundAgain > 0 && SoundEffectLastPlayerOnFrameId.Contains(Row))
	{
		if (SteppingFrameId - SoundEffectLastPlayerOnFrameId[Row] <= Row->MinFramesToWaitBeforePlayingSoundAgain)
		{
			return;
		}

		SoundEffectLastPlayerOnFrameId[Row] = SteppingFrameId;
	}
	else
	{
		SoundEffectLastPlayerOnFrameId.Add(Row, SteppingFrameId);
	}
	
	UGameplayStatics::PlaySound2D(GetWorld(), Row->SoundCue);
}

void USoundSubsystem::PlaySoundFromStartTime(FString Name, float StartTime)
{
	const FString ContextString = TEXT("USoundSubsystem::PlaySound");
	
	const auto Row = SoundEffectsTable->FindRow<FSoundEffect>(*Name, ContextString);
	checkf(Row, TEXT("Row not found for play sound"));
	
	UGameplayStatics::PlaySound2D(GetWorld(), Row->SoundCue, 1.f, 1.f, StartTime);
}

FString USoundSubsystem::ChooseRandomVoiceLineDeterministic(TArray<FString> Names)
{
	const int RandomIndex = GetWorld()->GetSubsystem<UJoltSubsystem>()->RandomNumberGenerator.NextIntInRange(0, Names.Num() - 1);
	
	return Names[RandomIndex];
}

void USoundSubsystem::PlayVoiceLine(FString Name, APlayerRenderer* InPlayerRenderer)
{
	const FString ContextString = TEXT("USoundSubsystem::PlayVoiceLine");
	const auto Row = SoundEffectsTable->FindRow<FSoundEffect>(*Name, ContextString);
	checkf(Row, TEXT("Row not found for play voice line"));
	
	PlaySound(Name);

	if (InPlayerRenderer)
	{
		InPlayerRenderer->OnSaySomething(Row->VoiceLineText);
	}
}
