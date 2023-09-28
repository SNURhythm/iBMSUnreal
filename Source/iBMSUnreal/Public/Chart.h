// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Measure.h"
#include "UObject/Object.h"
#include "Chart.generated.h"
/**
 * 
 */

UCLASS()
class IBMSUNREAL_API UChartMeta: public UObject {
	GENERATED_BODY()
	public:
	FString SHA256;
	FString MD5;
	FString BmsPath;
	FString Folder;
	FString Artist = "";
	FString SubArtist = "";
	double Bpm;
	FString Genre = "";
	FString Title = "";
	FString SubTitle = "";
	int Rank = 3;
	double Total = 100;
	long PlayLength = 0; // Timing of the last playable note, in microseconds
	long TotalLength = 0; // Timing of the last timeline(including background note, bga change note, invisible note, ...), in microseconds
	FString Banner;
	FString StageFile;
	FString BackBmp;
	FString Preview;
	bool BgaPoorDefault = false;
	int Difficulty;
	double PlayLevel = 3;
	double MinBpm;
	double MaxBpm;
	int Player = 1;
	int KeyMode = 5;
	int TotalNotes;
	int TotalLongNotes;
	int TotalScratchNotes;
	int TotalBackSpinNotes;
	int LnMode = 0; // 0: user decides, 1: LN, 2: CN, 3: HCN

};


class IBMSUNREAL_API FChart
{
public:
	FChart();
	~FChart();
	TObjectPtr<UChartMeta> Meta;
	TArray<FMeasure*> Measures;
	FString WavTable[36 * 36];
	FString BmpTable[36 * 36];
};
