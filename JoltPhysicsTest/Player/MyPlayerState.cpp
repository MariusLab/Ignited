// Fill out your copyright notice in the Description page of Project Settings.


#include "MyPlayerState.h"

#include "MyPlayerController.h"
#include "JoltPhysicsTest/Debug/CustomLogger.h"
#include "JoltPhysicsTest/FunctionLibrary/Helper.h"

AMyPlayerState::AMyPlayerState()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void AMyPlayerState::BeginPlay()
{
	Super::BeginPlay();
}

void AMyPlayerState::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (GetNetMode() == NM_Client)
	{
		auto LocalPlayerController = UHelper::GetLocalPlayerController(GetWorld());
		if (!LocalPlayerController)
		{
			return;
		}
		
		LocalPlayerController->GetPlayerNetworkData().PlayerStatesReplicated++;
		LOG(TEXT("PlayerState replicated for client %d.\nIs supported for networking: %d"), GetMyPlayerId(), IsSupportedForNetworking());
		
		LocalPlayerController->ServerUpdatePlayerNetworkData(LocalPlayerController->GetPlayerNetworkData());
		
		SetActorTickEnabled(false);
	}
	else
	{
		SetActorTickEnabled(false);
	}
}

void AMyPlayerState::MulticastSetPlayerIdAndGameSeed_Implementation(const int& InPlayerId, const FString& GameSeedString)
{
	PlayerId = InPlayerId;
	bPlayerIdSet = true;
	LOG(TEXT("CharacterId %d set for PlayerState client %d."), PlayerId, GetMyPlayerId());

	if (GetNetMode() == NM_Client)
	{
		auto LocalPlayerController = UHelper::GetLocalPlayerController(GetWorld());
		LocalPlayerController->GetPlayerNetworkData().CharacterIdsReplicated++;
		LocalPlayerController->GetPlayerNetworkData().GameSeed = GameSeedString;

		LOG(TEXT("Sending RPC to server to updated amount of character ids replicated to %d."), LocalPlayerController->GetPlayerNetworkData().CharacterIdsReplicated);
		LocalPlayerController->ServerUpdatePlayerNetworkData(LocalPlayerController->GetPlayerNetworkData());

		auto JoltSubsystem = GetWorld()->GetSubsystem<UJoltSubsystem>();
		if (!JoltSubsystem->IsRandomNumberGeneratorInitialized())
		{
			const uint64_t GameSeed = FCString::Strtoui64(*GameSeedString, nullptr, 10);
			JoltSubsystem->InitRandomNumberGenerator(GameSeed);
		}
	}
}

void AMyPlayerState::MulticastSetPlayerSettings_Implementation(const FPlayerSettings& InPlayerSettings)
{
	UpdatePlayerSettings(InPlayerSettings);

	if (GetNetMode() == NM_Client)
	{
		auto LocalPlayerController = UHelper::GetLocalPlayerController(GetWorld());
		// We don't care about getting our own settings replicated back to us, but multicast will include us, so we skip
		if (LocalPlayerController->GetMyPlayerState() == this)
		{
			return;
		}
		
		LocalPlayerController->GetPlayerNetworkData().PlayersSettingsReplicated++;
		LocalPlayerController->ServerUpdatePlayerNetworkData(LocalPlayerController->GetPlayerNetworkData());

		FPlayerSettings LocalPlayerSettings = FPlayerSettings(LocalPlayerController->GetMouseSensitivity());
		LocalPlayerController->ServerUpdatePlayerSettings(LocalPlayerSettings);
	}
}

int AMyPlayerState::GetMyPlayerId() const
{
	return PlayerId;
}

bool AMyPlayerState::IsPlayerIdSet() const
{
	return bPlayerIdSet;
}

void AMyPlayerState::UpdatePlayerSettings(const FPlayerSettings& InPlayerSettings)
{
	PlayerSettings = InPlayerSettings;
}

FPlayerSettings AMyPlayerState::GetPlayerSettings() const
{
	return PlayerSettings;
}
