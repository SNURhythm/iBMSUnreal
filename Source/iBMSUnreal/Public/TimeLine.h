// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BMSNote.h"

/**
 * 
 */
class IBMSUNREAL_API TimeLine {

public:
	UPROPERTY()
	TArray<BMSNote*> BackgroundNotes;
	UPROPERTY()
	BMSNote *InvisibleNotes[18];
	UPROPERTY()
	BMSNote *Notes[18];
	double Bpm = 0;
	bool BpmChange = false;
	bool BpmChangeApplied = false;
	int BgaBase = -1;
	int BgaLayer = -1;
	int BgaPoor = -1;
	UPROPERTY()
	TArray<BMSNote*> LandmineNotes;
	double StopLength = 0;
	double Scroll = 1;

	long Timing = 0;
	double Pos = 0;

	TimeLine(int lanes);

	TimeLine* SetNote(int lane, BMSNote* note);

	TimeLine* SetInvisibleNote(int lane, BMSNote* note);

	TimeLine* SetLandmineNote(int lane, BMSNote* note);

	TimeLine* AddBackgroundNote(BMSNote* note);

	double GetStopDuration();

	~TimeLine();
};
