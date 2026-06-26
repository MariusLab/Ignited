// Fill out your copyright notice in the Description page of Project Settings.


#include "OptimizedStaticMeshComponent.h"

UOptimizedStaticMeshComponent::UOptimizedStaticMeshComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	
	bCanEverAffectNavigation = false;
	bMultiBodyOverlap = false;
	bApplyImpulseOnDamage = false;
	bReplicatePhysicsToAutonomousProxy = false;
	BodyInstance.bEnableGravity = false;
	BodyInstance.bAutoWeld = false;
	BodyInstance.bUpdateMassWhenScaleChanges = false;
	BodyInstance.bSimulatePhysics = false;
	BodyInstance.bEnableGravity = false;
	BodyInstance.SetCollisionProfileNameDeferred("NoCollision");
	
	bCastStaticShadow = false;
	bCastCinematicShadow = false;
	bCastHiddenShadow = false;
	bCastContactShadow = false;
	CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;

	CastShadow = false;
	bAffectDynamicIndirectLighting = false;
	bAffectDistanceFieldLighting = false;
	bCastDynamicShadow = false;

	bVisibleInReflectionCaptures = false;
	bVisibleInRealTimeSkyCaptures = false;
	bVisibleInRayTracing = false;
	bVisibleInSceneCaptureOnly = false;
}
