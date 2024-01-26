// Fill out your copyright notice in the Description page of Project Settings.


#include "Input/UnrealTouchInputSource.h"


FUnrealTouchInputSource::~FUnrealTouchInputSource()
{
}

bool FUnrealTouchInputSource::StartListen()
{
	// touch
	FInputTouchBinding TouchPress(IE_Pressed);
	FInputTouchBinding TouchRelease(IE_Released);
	// touch move
	FInputTouchBinding TouchMove(IE_Repeat);
	TouchPress.bConsumeInput = true;
	TouchPress.bExecuteWhenPaused = false;

	TouchPress.TouchDelegate.GetDelegateForManualSet().BindLambda(
		[this](ETouchIndex::Type FingerIndex, FVector Location)
		{
			UE_LOG(LogTemp, Warning, TEXT("Touch pressed from unreal basic input: %d, %s"), FingerIndex,
			       *Location.ToString());
			// Your code here
			if (InputHandler != nullptr)
			{
				InputHandler->OnFingerDown(FingerIndex, Location);
			}
		});
	TouchRelease.bConsumeInput = true;
	TouchRelease.bExecuteWhenPaused = false;

	TouchRelease.TouchDelegate.GetDelegateForManualSet().BindLambda(
		[this](ETouchIndex::Type FingerIndex, FVector Location)
		{
			UE_LOG(LogTemp, Warning, TEXT("Touch released from unreal basic input: %d, %s"), FingerIndex,
			       *Location.ToString());
			// Your code here
			if (InputHandler != nullptr)
			{
				InputHandler->OnFingerUp(FingerIndex, Location);
			}
		});

	TouchMove.bConsumeInput = true;
	TouchMove.bExecuteWhenPaused = false;

	TouchMove.TouchDelegate.GetDelegateForManualSet().BindLambda([this](ETouchIndex::Type FingerIndex, FVector Location)
	{
		UE_LOG(LogTemp, Warning, TEXT("Touch moved from unreal basic input: %d, %s"), FingerIndex,
		       *Location.ToString());
		// Your code here
	});

	Index = InputComponent->TouchBindings.Add(TouchPress);
	InputComponent->TouchBindings.Add(TouchRelease);
	InputComponent->TouchBindings.Add(TouchMove);

	return true;
}

void FUnrealTouchInputSource::StopListen()
{
	if (Index >= 0)
	{
		InputComponent->TouchBindings.RemoveAt(Index);
		InputComponent->TouchBindings.RemoveAt(Index);
		InputComponent->TouchBindings.RemoveAt(Index);
	}
}

void FUnrealTouchInputSource::SetHandler(IInputHandler* Handler)
{
	InputHandler = Handler;
}
