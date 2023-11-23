// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "IInputHandler.h"
#include "IInputSource.h"

/**
 * 
 */
class IBMSUNREAL_API FUnrealTouchInputSource: public IInputSource
{
	IInputHandler* InputHandler = nullptr;
	int32 Index = -1;

public:
	UInputComponent* InputComponent;

	explicit FUnrealTouchInputSource(UInputComponent* Comp): InputComponent(Comp) {}
	~FUnrealTouchInputSource();
	virtual bool StartListen() override;
	virtual void StopListen() override;
	virtual void SetHandler(IInputHandler* Handler) override;
};
