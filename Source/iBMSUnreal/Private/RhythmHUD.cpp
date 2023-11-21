// Fill out your copyright notice in the Description page of Project Settings.


#include "RhythmHUD.h"


int32 URhythmHUD::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId,
	const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	FString JudgeComboString = LastJudgeString;
	if(LastCombo > 0)
	{
		JudgeComboString += FString::Printf(TEXT(" %d"), LastCombo);
	}
	JudgementText->SetText(FText::FromString(JudgeComboString));
	return Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle,
	                          bParentEnabled);
}

void URhythmHUD::OnJudge(const FJudgeResult JudgeResult, const int Combo)
{
	LastJudgeString = JudgeResult.ToString();
	LastCombo = Combo;
}
