// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "RhythmControl.h"
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
	
	UPROPERTY(meta = (BindWidget))
	UTextBlock* ExScoreText;

	FString LastJudgeString;
	int LastCombo = 0;
	int LastExScore = 0;
	int LastRenderRequestState = 0;
	int LastRenderedState = 0;
	TMap<EJudgement, int> LastJudgeCount;

	virtual void NativeTick(const FGeometry & MyGeometry, float InDeltaTime) override;
public:
	UPROPERTY(meta = (BindWidget))
	UButton* PauseButton;
	void OnJudge(const FRhythmState* State);
	void Reset();
};
