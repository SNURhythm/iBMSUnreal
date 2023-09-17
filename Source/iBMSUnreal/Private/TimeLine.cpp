// Fill out your copyright notice in the Description page of Project Settings.


#include "TimeLine.h"

TimeLine::TimeLine(int lanes) {
	UE_LOG(LogTemp, Warning, TEXT("TimeLine::TimeLine %d"), lanes);
	Notes.SetNumUninitialized(lanes);
	InvisibleNotes.SetNumUninitialized(lanes);
	LandmineNotes.SetNumUninitialized(lanes);
}

TimeLine* TimeLine::SetNote(int lane, BMSNote* note)
{

	Notes[lane] = note;
	note->Lane = lane;
	note->Timeline = this;
	return this;
}

TimeLine* TimeLine::SetInvisibleNote(int lane, BMSNote* note)
{
	InvisibleNotes[lane] = note;
	note->Lane = lane;
	note->Timeline = this;
	return this;
}

TimeLine* TimeLine::SetLandmineNote(int lane, BMSNote* note)
{
	LandmineNotes[lane] = note;
	note->Lane = lane;
	note->Timeline = this;
	return this;
}

TimeLine* TimeLine::AddBackgroundNote(BMSNote* note)
{
	BackgroundNotes.Add(note);
	note->Timeline = this;
	return this;
}

inline double TimeLine::GetStopDuration()
{
	return 1250000.0 * StopLength / Bpm; // 1250000 = 240 * 1000 * 1000 / 192
}

TimeLine::~TimeLine()
{

}
