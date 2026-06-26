// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerSensor.h"


void APlayerSensor::PlayerEnter()
{
	bPlayerInsideSensor = true;
	StepsInsideSensor = 0;
	BP_OnPlayerEnter();
}

void APlayerSensor::PlayerLeave()
{
	bPlayerInsideSensor = false;
	StepsInsideSensor = 0;
	BP_OnPlayerLeave();
}

void APlayerSensor::StepSimulation()
{
	Super::StepSimulation();

	if (bPlayerInsideSensor)
	{
		StepsInsideSensor++;
	}
	
	BP_OnStepSimulation();
}
