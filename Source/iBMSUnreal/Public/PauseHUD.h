// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PauseHUD.generated.h"

class UButton;
/**
 * 
 */
UCLASS()
class IBMSUNREAL_API UPauseHUD : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget))
	UButton* ResumeButton;
	UPROPERTY(meta = (BindWidget))
	UButton* RestartButton;
	UPROPERTY(meta = (BindWidget))
	UButton* ExitButton;
};
