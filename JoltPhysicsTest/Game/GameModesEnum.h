#pragma once

UENUM(BlueprintType)
enum class EGameMode : uint8
{
	None,
	Deathmatch,
	RopedTogether,
	CaptureTheFlag,
	Lobby,
	PullingRope,
	KingOfTheCrown,
	Tutorial,
	TimeAttack
};