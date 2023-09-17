// Fill out your copyright notice in the Description page of Project Settings.


#include "BMSLongNote.h"

BMSLongNote::BMSLongNote(int wav) : BMSNote(wav)
{
}

void BMSLongNote::Press(long time)
{
	Play(time);
	IsHolding = true;
	Tail->IsHolding = true;
}

void BMSLongNote::Release(long time)
{
	Play(time);
	IsHolding = false;
	Head->IsHolding = false;
	ReleaseTime = time;
}

void BMSLongNote::MissPress(long time)
{

}

void BMSLongNote::Reset()
{
	BMSNote::Reset();
	IsHolding = false;
	ReleaseTime = 0;
}


BMSLongNote::~BMSLongNote()
{
}

