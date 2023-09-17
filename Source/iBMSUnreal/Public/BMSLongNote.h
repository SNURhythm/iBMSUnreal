// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BMSNote.h"
/**
 * 
 */

class IBMSUNREAL_API BMSLongNote : public BMSNote
{
public:
	BMSLongNote();
	~BMSLongNote();
	BMSLongNote* Tail;
	BMSLongNote* Head;
	bool IsHolding;
	bool IsTail;
	long ReleaseTime;
	
	BMSLongNote(int wav);

	void Press(long time);

	void Release(long time);

	void MissPress(long time);

	void Reset();


};
