// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "IInputSource.h"

/**
 * 
 */
class IBMSUNREAL_API FUnrealInputSource : public IInputSource
{
	IInputHandler* InputHandler = nullptr;
	int32 Index = -1;

public:
	UInputComponent* InputComponent;

	explicit FUnrealInputSource(UInputComponent* Comp): InputComponent(Comp)
	{
	}

	virtual ~FUnrealInputSource() override;
	virtual bool StartListen() override;
	virtual void StopListen() override;
	virtual void SetHandler(IInputHandler* Handler) override;
};
