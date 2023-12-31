// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
/**
 * 
 */
class FTimeLine;
class IBMSUNREAL_API FBMSNote {
public:
	int Lane = 0;
	int Wav = 0;

	bool IsPlayed = false;
	bool IsDead = false;
	long long PlayedTime = 0;
	FTimeLine* Timeline;
	
	// private Note nextNote;

	FBMSNote(int Wav);

	void Play(long long Time);

	virtual void Press(long long Time);

	virtual void Reset();
	virtual ~FBMSNote();

	virtual bool IsLongNote() { return false; }
	virtual bool IsLandmineNote() { return false; }
};