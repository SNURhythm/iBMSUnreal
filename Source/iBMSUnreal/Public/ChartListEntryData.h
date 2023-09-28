// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Chart.h"
#include "UObject/Object.h"
#include "ChartListEntryData.generated.h"

/**
 * 
 */
UCLASS()
class IBMSUNREAL_API UChartListEntryData : public UObject
{
	GENERATED_BODY()
public:
	FChartMeta* ChartMeta;
	// convert from FChartMeta to UChartListEntryData
	UChartListEntryData(FChartMeta* ChartMeta);
	UChartListEntryData();
	
};
