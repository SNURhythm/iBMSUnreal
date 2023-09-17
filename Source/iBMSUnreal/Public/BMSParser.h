// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Chart.h"
/**
 * 
 */
class IBMSUNREAL_API BMSParser
{


public:
	BMSParser();
	void Parse(FString path, bool addReadyMeasure, bool metaOnly);
	~BMSParser();
	Chart chart;
private:
	// bpmTable
	double bpmTable[36 * 36];
	FString wavTable[36 * 36];
	FString bmpTable[36 * 36];
	double StopLengthTable[36 * 36];

	int lnobj = -1;
	int lntype = 1;
	int DecodeBase36(FString str);
	void ParseHeader(FString cmd, FString xx, FString value);

	int Gcd(int a, int b);
	int ToWaveId(FString wav);
};
