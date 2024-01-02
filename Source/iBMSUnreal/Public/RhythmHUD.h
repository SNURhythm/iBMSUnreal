// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Judge.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "RhythmHUD.generated.h"

class UButton;
/**
 * 
 */
UCLASS()
class IBMSUNREAL_API URhythmHUD : public UUserWidget
{
	GENERATED_BODY()

	UPROPERTY(meta = (BindWidget))
	UTextBlock* JudgementText;

	FString LastJudgeString;
	int LastCombo;

	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

public:
	UPROPERTY(meta = (BindWidget))
	UButton* PauseButton;
	void OnJudge(FJudgeResult JudgeResult, int Combo);
	void Reset();
};
