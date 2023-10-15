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
	void Parse(FString path, FChart** Chart, bool addReadyMeasure, bool metaOnly);
	~FBMSParser();
	static int NoWav;
	static int MetronomeWav;
private:
	// bpmTable
	TMap<int, double> BpmTable;
	TMap<int, double> StopLengthTable;
	
	int Lnobj = -1;
	int Lntype = 1;
	int DecodeBase36(const FString& Str);
	void ParseHeader(FChart* Chart, const FString& Cmd, const FString& Xx, FString Value);

	static int Gcd(int A, int B);
	static bool CheckResourceIdRange(int Id);
	int ToWaveId(FChart* Chart, const FString& Wav);
};
