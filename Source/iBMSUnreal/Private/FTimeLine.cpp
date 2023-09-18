// Fill out your copyright notice in the Description page of Project Settings.


#include "FTimeLine.h"

FTimeLine::FTimeLine(int lanes, bool metaOnly) {
	if (metaOnly) return;
	Notes.Init(nullptr, lanes);
	InvisibleNotes.Init(nullptr, lanes);
	LandmineNotes.Init(nullptr, lanes);
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

FTimeLine* FTimeLine::SetLandmineNote(int lane, FBMSLandmineNote* note)
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
	for (const auto& note : Notes) {
		if (note != nullptr) {
			delete note;
		}
	}
	Notes.Empty();
	for (const auto& note : InvisibleNotes) {
		if (note != nullptr) {
			delete note;
		}
	}
	InvisibleNotes.Empty();
	for (const auto& note : LandmineNotes) {
		if (note != nullptr) {
			delete note;
		}
	}
	LandmineNotes.Empty();
	for (const auto& note : BackgroundNotes) {
		if (note != nullptr) {
			delete note;
		}
	}
	BackgroundNotes.Empty();
}
