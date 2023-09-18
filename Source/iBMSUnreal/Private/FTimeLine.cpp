// Fill out your copyright notice in the Description page of Project Settings.


#include "FTimeLine.h"

FTimeLine::FTimeLine(int lanes) {
	UE_LOG(LogTemp, Warning, TEXT("TimeLine::TimeLine %d"), lanes);
	Notes.SetNumUninitialized(lanes);
	InvisibleNotes.SetNumUninitialized(lanes);
	LandmineNotes.SetNumUninitialized(lanes);
}

FTimeLine* FTimeLine::SetNote(int lane, FBMSNote* note)
{

	Notes[lane] = note;
	note->Lane = lane;
	note->Timeline = this;
	return this;
}

FTimeLine* FTimeLine::SetInvisibleNote(int lane, FBMSNote* note)
{
	InvisibleNotes[lane] = note;
	note->Lane = lane;
	note->Timeline = this;
	return this;
}

FTimeLine* FTimeLine::SetLandmineNote(int lane, FBMSNote* note)
{
	LandmineNotes[lane] = note;
	note->Lane = lane;
	note->Timeline = this;
	return this;
}

FTimeLine* FTimeLine::AddBackgroundNote(FBMSNote* note)
{
	BackgroundNotes.Add(note);
	note->Timeline = this;
	return this;
}

double FTimeLine::GetStopDuration()
{
	return 1250000.0 * StopLength / Bpm; // 1250000 = 240 * 1000 * 1000 / 192
}

FTimeLine::~FTimeLine()
{

}
