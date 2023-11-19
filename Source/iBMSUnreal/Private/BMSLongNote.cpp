// Fill out your copyright notice in the Description page of Project Settings.


#include "BMSLongNote.h"

bool FBMSLongNote::IsTail()
{
	return Tail == nullptr;
}

FBMSLongNote::FBMSLongNote(int Wav) : FBMSNote(Wav)
{
	Tail = nullptr;
}

void FBMSLongNote::Press(long long Time)
{
	Play(Time);
	IsHolding = true;
	Tail->IsHolding = true;
}

void FBMSLongNote::Release(long long Time)
{
	Play(Time);
	IsHolding = false;
	Head->IsHolding = false;
	ReleaseTime = Time;
}

void FBMSLongNote::MissPress(long long Time)
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
	Head = nullptr;
	Tail = nullptr;
}

