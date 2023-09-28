// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Chart.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Blueprint/UserWidget.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

#include "ChartListEntry.generated.h"

/**
 * 
 */
UCLASS()
class IBMSUNREAL_API UChartListEntry : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget))
	UTextBlock* TitleText;
	UPROPERTY(meta = (BindWidget))
	UTextBlock* ArtistText;
	UPROPERTY(meta = (BindWidget))
	UTextBlock* KeyModeText;
	UPROPERTY(meta = (BindWidget))
	UBorder* Border;

	UPROPERTY()
	TObjectPtr<UChartMeta> EntryData;
	
	// OnEntryClicked is a delegate that will be bound to the OnClicked event of the button
	DECLARE_DELEGATE_OneParam(FOnEntryClicked, TObjectPtr<UChartMeta>);
	FOnEntryClicked OnEntryClicked;

	UFUNCTION()
	void OnButtonClicked();
	virtual void NativeConstruct() override;
	virtual void NativeOnListItemObjectSet(UObject* InObject) override;
	
	
};
