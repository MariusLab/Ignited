// Fill out your copyright notice in the Description page of Project Settings.


#include "MyGameInstance.h"

#include "Sound/SoundClass.h"
#include "GameFramework/GameUserSettings.h"
#include "JoltPhysicsTest/Data/AssetManagerSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetInternationalizationLibrary.h"

void UMyGameInstance::Init()
{
	Super::Init();

	MusicMaster->Properties.Volume = GetCurrentGameSave()->GameSettings.MusicVolume;
	SoundEffectsMaster->Properties.Volume = GetCurrentGameSave()->GameSettings.SoundsVolume;

	UKismetInternationalizationLibrary::SetCurrentCulture(GetCurrentGameSave()->GameSettings.Language);

	/*auto GameUserSettings = UGameUserSettings::GetGameUserSettings();
	if (GetCurrentGameSave()->GameSettings.bFullscreen)
	{
		GameUserSettings->SetFullscreenMode(EWindowMode::Fullscreen);
	}
	else
	{
		GameUserSettings->SetFullscreenMode(EWindowMode::Windowed);
	}

	GameUserSettings->bUseVSync = GetCurrentGameSave()->GameSettings.bVsync;
	GameUserSettings->SetScreenResolution({GetCurrentGameSave()->GameSettings.DisplayResolutionX, GetCurrentGameSave()->GameSettings.DisplayResolutionY});

	GameUserSettings->ApplySettings(false);*/
}

void UMyGameInstance::SetMusicVolume(float NewVolume)
{
	GetCurrentGameSave()->GameSettings.MusicVolume = NewVolume;
	MusicMaster->Properties.Volume = NewVolume;
}

void UMyGameInstance::SetSoundsVolume(float NewVolume)
{
	GetCurrentGameSave()->GameSettings.SoundsVolume = NewVolume;
	SoundEffectsMaster->Properties.Volume = NewVolume;
}

void UMyGameInstance::SetResolution(int ResX, int ResY)
{
	GetCurrentGameSave()->GameSettings.DisplayResolutionX = ResX;
	GetCurrentGameSave()->GameSettings.DisplayResolutionY = ResY;

	UGameUserSettings::GetGameUserSettings()->SetScreenResolution({ResX, ResY});
}

void UMyGameInstance::SetFullscreen(bool bFullscreen)
{
	GetCurrentGameSave()->GameSettings.bFullscreen = bFullscreen;
	if (bFullscreen)
	{
		UGameUserSettings::GetGameUserSettings()->SetFullscreenMode(EWindowMode::Fullscreen);
	}
	else
	{
		UGameUserSettings::GetGameUserSettings()->SetFullscreenMode(EWindowMode::Windowed);
	}

	UGameUserSettings::GetGameUserSettings()->ApplyNonResolutionSettings();
}

void UMyGameInstance::SetVSync(bool bVSync)
{
	GetCurrentGameSave()->GameSettings.bVsync = bVSync;
	UGameUserSettings::GetGameUserSettings()->bUseVSync = bVSync;
	
	UGameUserSettings::GetGameUserSettings()->ApplyNonResolutionSettings();
}

void UMyGameInstance::SetLanguage(FString NewLanguage)
{
	GetCurrentGameSave()->GameSettings.Language = NewLanguage;
	bool bResult = UKismetInternationalizationLibrary::SetCurrentCulture(GetCurrentGameSave()->GameSettings.Language);
	checkf(bResult, TEXT("Failed to set current culture with %s"), *NewLanguage);
}

bool UMyGameInstance::IsTutorialCompleted()
{
#if WITH_EDITOR
	if (UAssetManagerSubsystem::GetGeneralData(GetWorld())->bDebugTutorial)
	{
		return false;
	}
#endif
	return GetCurrentGameSave()->bTutorialCompleted;
}

FGameSettings UMyGameInstance::GetGameSettings()
{
	return GetCurrentGameSave()->GameSettings;
}

void UMyGameInstance::BP_SaveGame()
{
	SaveGame();
}

UMyGameSave* UMyGameInstance::GetCurrentGameSave()
{
	if (MyGameSave)
	{
		return MyGameSave;
	}

	MyGameSave = Cast<UMyGameSave>(UGameplayStatics::LoadGameFromSlot(TEXT("GameSave1"), 0));
	if (!MyGameSave)
	{
		MyGameSave = Cast<UMyGameSave>(UGameplayStatics::CreateSaveGameObject(UMyGameSave::StaticClass()));
	}

	return MyGameSave;
}

void UMyGameInstance::SaveGame()
{
	if (MyGameSave)
	{
		const bool bSaveGame = UGameplayStatics::SaveGameToSlot(MyGameSave, TEXT("GameSave1"), 0);
		check(bSaveGame);
	}

	UGameUserSettings::GetGameUserSettings()->SaveSettings();
}

void UMyGameInstance::BeginDestroy()
{
	Super::BeginDestroy();

	//SaveGame();
}
