// Fill out your copyright notice in the Description page of Project Settings.


#include "Chart.h"

FChart::FChart()
{
}

FChart::~FChart()
{
	for (const auto& measure : Measures)
	{
		delete measure;
	}
	
	Measures.Empty();
	if(Meta)
		delete Meta;
}
