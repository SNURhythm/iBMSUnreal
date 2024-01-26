// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BMSNote.h"
/**
 * 
 */

class IBMSUNREAL_API FBMSLongNote : public FBMSNote
{
public:
	FBMSLongNote();
	virtual ~FBMSLongNote() override;
	FBMSLongNote* Tail;
	FBMSLongNote* Head;
	bool IsHolding = false;
	bool IsTail();
	long long ReleaseTime;

	FBMSLongNote(int Wav);

	virtual void Press(long long Time) override;

	void Release(long long Time);

	void MissPress(long long Time);

	virtual void Reset() override;

	virtual bool IsLongNote() override { return true; }
};
