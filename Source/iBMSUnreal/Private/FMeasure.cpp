// Fill out your copyright notice in the Description page of Project Settings.


#include "FMeasure.h"

FMeasure::~FMeasure()
{
	for (const auto* Timeline : TimeLines)
	{
		delete Timeline;
	}
}