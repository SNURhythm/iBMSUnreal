// Fill out your copyright notice in the Description page of Project Settings.


#include "FChart.h"

FChart::FChart(): Meta()
{
}

FChart::~FChart()
{
	for (auto& measure : Measures)
	{
		delete measure;
	}
}
