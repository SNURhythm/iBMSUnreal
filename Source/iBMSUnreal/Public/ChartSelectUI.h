// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/Image.h"
#include "Components/ListView.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"

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
	UPROPERTY(meta = (BindWidget))
	UEditableTextBox* SearchBox;
	UPROPERTY(meta = (BindWidget))
	UImage* BackgroundImage;
	UPROPERTY(meta = (BindWidget))
	UImage* StageFileImage;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TitleText;
	UPROPERTY(meta = (BindWidget))
	UTextBlock* ArtistText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* GenreText;
	UPROPERTY(meta = (BindWidget))
	UTextBlock* BPMText;
	UPROPERTY(meta = (BindWidget))
	UTextBlock* TotalText;
	UPROPERTY(meta = (BindWidget))
	UTextBlock* NotesText;
	UPROPERTY(meta = (BindWidget))
	UTextBlock* JudgementText;

	UPROPERTY(meta = (BindWidget))
	UButton* StartButton;
	
	UPROPERTY(meta = (BindWidget))
	UBorder* OverlayInfoBox;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* OverlayInfoText;

	UPROPERTY(meta = (BindWidget))
	USizeBox* BackgroundSizeBox;
};
