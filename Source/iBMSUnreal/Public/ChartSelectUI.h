// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ListView.h"

#include "ChartSelectUI.generated.h"


/**
 * 
 */
UCLASS()
class IBMSUNREAL_API UChartSelectUI : public UUserWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget))
	UListView* ChartList;
};
