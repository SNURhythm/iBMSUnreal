// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FChart.h"
/**
 * 
 */
class IBMSUNREAL_API FBMSParser
{


public:
	FBMSParser();
	void Parse(FString& path, bool addReadyMeasure, bool metaOnly);
	~FBMSParser();
	FChart Chart;
private:
	// bpmTable
	double BpmTable[36 * 36];
	FString WavTable[36 * 36];
	FString BmpTable[36 * 36];
	double StopLengthTable[36 * 36];

	int Lnobj = -1;
	int Lntype = 1;
	int DecodeBase36(FString& Str);
	void ParseHeader(FString& Cmd, FString& Xx, FString& Value);

	static int Gcd(int A, int B);
	int ToWaveId(FString& Wav);
};
