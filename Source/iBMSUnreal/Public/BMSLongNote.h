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
	~FBMSLongNote();
	FBMSLongNote* Tail;
	FBMSLongNote* Head;
	bool IsHolding;
	bool IsTail();
	long ReleaseTime;
	
	FBMSLongNote(int Wav);

	virtual void Press(long Time) override;

	void Release(long Time);

	void MissPress(long Time);

	virtual void Reset() override;

	virtual bool IsLongNote() override { return true; }
};
