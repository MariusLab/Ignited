// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Sound/SoundCue.h"
#include "Subsystems/WorldSubsystem.h"
#include "SoundSubsystem.generated.h"

UENUM(BlueprintType)
enum class ESoundEffectCategory : uint8
{
	None,
	Music,
	VoiceLine,
	Sound
};

USTRUCT(BlueprintType)
struct FSoundEffect : public FTableRowBase
{
	GENERATED_BODY()
public:
	FSoundEffect() {}
	
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
	ESoundEffectCategory Category = ESoundEffectCategory::None;
	
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
	TObjectPtr<USoundCue> SoundCue = nullptr;

	// Same sound will not be allowed to repeat more often than this
	UPROPERTY(EditDefaultsOnly)
	int MinFramesToWaitBeforePlayingSoundAgain = 3;

	UPROPERTY(EditDefaultsOnly, Meta = (EditCondition = "Category = ESoundEffectCategory::VoiceLine"))
	FText VoiceLineText = FText::GetEmpty();
};

UCLASS()
class JOLTPHYSICSTEST_API USoundSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void PostInitialize() override;


	UFUNCTION(BlueprintCallable, Category = "SoundSubsystem")
	void PlaySound(FString Name);

	UFUNCTION(BlueprintCallable, Category = "SoundSubsystem")
	void PlaySoundFromStartTime(FString Name, float StartTime);

	FString ChooseRandomVoiceLineDeterministic(TArray<FString> Names);

	UFUNCTION(BlueprintCallable, Category = "SoundSubsystem")
	void PlayVoiceLine(FString Name, class APlayerRenderer* InPlayerRenderer);
private:
	UPROPERTY()
	TObjectPtr<UDataTable> SoundEffectsTable = nullptr;

	// The frame that this sound effect was last played on
	TMap<FSoundEffect*, int> SoundEffectLastPlayerOnFrameId;
};
