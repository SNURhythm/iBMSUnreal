// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
/**
 * 
 */
class TimeLine;
class IBMSUNREAL_API BMSNote {
public:
	int Lane = 0;
	int Wav = 0;

	bool IsPlayed = false;
	bool IsDead = false;
	long PlayedTime = 0;
	TimeLine* Timeline;

	// ∑π¿Œ
	// private Note nextNote;

	BMSNote(int wav);

	void Play(long time);

	void Press(long time);

	void Reset();
};