// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Measure.h"


/**
 * 
 */


class IBMSUNREAL_API FChartMeta {

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
	long long PlayLength = 0; // Timing of the last playable note, in microseconds
	long long TotalLength = 0; // Timing of the last timeline(including background note, bga change note, invisible note, ...), in microseconds
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
	bool IsDP = false;
	int TotalNotes;
	int TotalLongNotes;
	int TotalScratchNotes;
	int TotalBackSpinNotes;
	int LnMode = 0; // 0: user decides, 1: LN, 2: CN, 3: HCN

	int GetKeyLaneCount() const { return KeyMode; }
	int GetScratchLaneCount() const { return IsDP? 2 : 1; }
	int GetTotalLaneCount() const { return KeyMode + GetScratchLaneCount(); }
	TArray<int> GetKeyLaneIndices() const
	{
		if(KeyMode == 5)
		{
			if(IsDP) return { 0, 1, 2, 3, 4, 8, 9, 10, 11, 12};
			return { 0, 1, 2, 3, 4 };
		}
		if(KeyMode == 7)
		{
			if(IsDP) return { 0, 1, 2, 3, 4, 5, 6, 8, 9, 10, 11, 12, 13, 14};
			return { 0, 1, 2, 3, 4, 5, 6 };
		}
		return {};
	}

	TArray<int> GetScratchLaneIndices() const
	{
		if(IsDP) return { 7, 15 };
		return { 5 };
	}

	TArray<int> GetTotalLaneIndices() const
	{
		TArray<int> Result;
		Result.Append(GetKeyLaneIndices());
		Result.Append(GetScratchLaneIndices());
		return Result;
	}

};


class IBMSUNREAL_API FChart
{
public:
	FChart();
	~FChart();
	FChartMeta* Meta;
	TArray<FMeasure*> Measures;
	TMap<int, FString> WavTable;
	TMap<int, FString> BmpTable;
};
