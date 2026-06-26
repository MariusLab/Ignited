// Fill out your copyright notice in the Description page of Project Settings.


#include "GameCameraLimits.h"


AGameCameraLimits::AGameCameraLimits()
{
	PrimaryActorTick.bCanEverTick = false;

	BoxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxComponent"));
	SetRootComponent(BoxComponent);
}
