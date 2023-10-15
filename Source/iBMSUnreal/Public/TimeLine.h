// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BMSNote.h"
#include "BMSLandmineNote.h"
/**
 * 
 */

class IBMSUNREAL_API FTimeLine {

public:
	TArray<FBMSNote*> BackgroundNotes;
	TArray<FBMSNote*> InvisibleNotes;
	TArray<FBMSNote*> Notes;
	TArray<FBMSLandmineNote*> LandmineNotes;
	double Bpm = 0;
	bool BpmChange = false;
	bool BpmChangeApplied = false;
	int BgaBase = -1;
	int BgaLayer = -1;
	int BgaPoor = -1;

	double StopLength = 0;
	double Scroll = 1;

	long long Timing = 0;
	double Pos = 0;

	explicit FTimeLine(int lanes, bool metaOnly);

	FTimeLine* SetNote(int lane, FBMSNote* note);

	FTimeLine* SetInvisibleNote(int lane, FBMSNote* note);

	FTimeLine* SetLandmineNote(int lane, FBMSLandmineNote* note);

	FTimeLine* AddBackgroundNote(FBMSNote* note);

	double GetStopDuration();

	~FTimeLine();
};
