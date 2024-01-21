// Fill out your copyright notice in the Description page of Project Settings.


#include "RhythmHUD.h"

#include "RhythmControl.h"




void URhythmHUD::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if(LastRenderRequestState == LastRenderedState)
	{
		return;
	}
	LastRenderedState = LastRenderRequestState;
	FString JudgeComboString = LastJudgeString;
	if(LastCombo > 0)
	{
		JudgeComboString += FString::Printf(TEXT(" %d"), LastCombo);
	}
	JudgementText->SetText(FText::FromString(JudgeComboString));
	ExScoreText->SetText(FText::FromString(FString::Printf(TEXT("%d"), LastExScore)));
}

void URhythmHUD::OnJudge(const FRhythmState* State)
{
	LastJudgeString = State->LatestJudgeResult.ToString();
	LastCombo = State->Combo;
	LastExScore = State->JudgeCount[PGreat] * 2 + State->JudgeCount[Great];
	LastJudgeCount = State->JudgeCount;
	LastRenderRequestState++;
}

void URhythmHUD::Reset()
{
	LastJudgeString = TEXT("");
	LastCombo = 0;
	LastExScore = 0;
	LastJudgeCount.Empty();
	LastRenderRequestState++;
}
