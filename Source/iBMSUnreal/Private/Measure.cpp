// Fill out your copyright notice in the Description page of Project Settings.


#include "Measure.h"

Measure::~Measure()
{
	for (auto* timeline : TimeLines)
	{
		delete timeline;
	}
}