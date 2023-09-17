// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "TimeLine.h"
/**
 * 
 */
class IBMSUNREAL_API Measure {
public:
	double Scale = 1;
	long Timing = 0;
	double Pos = 0;
	UPROPERTY()
	TArray<TimeLine*> TimeLines;
	~Measure();
};