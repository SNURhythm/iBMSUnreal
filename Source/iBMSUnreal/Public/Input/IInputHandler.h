#pragma once
#include "InputNormalizer.h"

class IInputHandler
{
public:
	virtual ~IInputHandler() = default;
	virtual void OnKeyDown(int KeyCode, KeySource Source, int CharCode = 0) = 0;
	virtual void OnKeyUp(int KeyCode, KeySource Source, int CharCode = 0) = 0;
	virtual void OnFingerDown(int FingerIndex, FVector Location) = 0;
	virtual void OnFingerUp(int FingerIndex, FVector Location) = 0;
};
