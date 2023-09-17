// Fill out your copyright notice in the Description page of Project Settings.


#include "BMSNote.h"

BMSNote::BMSNote(int wav)
{
	Wav = wav;
}

void BMSNote::Play(long time)
{
	IsPlayed = true;
	PlayedTime = time;
}

void BMSNote::Press(long time)
{
	Play(time);
}

void BMSNote::Reset()
{
	IsPlayed = false;
	IsDead = false;
	PlayedTime = 0;
}
