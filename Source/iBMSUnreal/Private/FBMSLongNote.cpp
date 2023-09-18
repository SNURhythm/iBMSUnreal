// Fill out your copyright notice in the Description page of Project Settings.


#include "FBMSLongNote.h"

FBMSLongNote::FBMSLongNote(int Wav) : FBMSNote(Wav)
{
}

void FBMSLongNote::Press(long Time)
{
	Play(Time);
	IsHolding = true;
	Tail->IsHolding = true;
}

void FBMSLongNote::Release(long Time)
{
	Play(Time);
	IsHolding = false;
	Head->IsHolding = false;
	ReleaseTime = Time;
}

void FBMSLongNote::MissPress(long Time)
{

}

void FBMSLongNote::Reset()
{
	FBMSNote::Reset();
	IsHolding = false;
	ReleaseTime = 0;
}


FBMSLongNote::~FBMSLongNote()
{
}

