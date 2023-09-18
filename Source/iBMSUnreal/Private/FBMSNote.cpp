// Fill out your copyright notice in the Description page of Project Settings.


#include "FBMSNote.h"

FBMSNote::FBMSNote(int wav)
{
	Wav = wav;
}

void FBMSNote::Play(long Time)
{
	IsPlayed = true;
	PlayedTime = Time;
}

void FBMSNote::Press(long Time)
{
	Play(Time);
}

void FBMSNote::Reset()
{
	IsPlayed = false;
	IsDead = false;
	PlayedTime = 0;
}

FBMSNote::~FBMSNote()
{
	Timeline = nullptr;
}