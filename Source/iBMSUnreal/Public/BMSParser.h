// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Chart.h"
/**
 * 
 */
class IBMSUNREAL_API FBMSParser
{


public:
	FBMSParser();
	void Parse(const FString& path, FChart** chart, bool addReadyMeasure, bool metaOnly);
	~FBMSParser();
private:
	// bpmTable
	double BpmTable[36 * 36];
	double StopLengthTable[36 * 36];

	int Lnobj = -1;
	int Lntype = 1;
	int DecodeBase36(const FString& Str);
	void ParseHeader(FChart*& Chart, const FString& Cmd, const FString& Xx, const FString& Value);

	static int Gcd(int A, int B);
	int ToWaveId(FChart*& Chart, FString& Wav);
};
