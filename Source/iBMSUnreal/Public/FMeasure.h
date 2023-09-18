// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "FTimeLine.h"
/**
 * 
 */
class IBMSUNREAL_API FMeasure {
public:
	double Scale = 1;
	long Timing = 0;
	double Pos = 0;
	TArray<FTimeLine*> TimeLines;
	~FMeasure();
};