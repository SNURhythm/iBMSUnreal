#pragma once
#include "IInputHandler.h"

class IInputSource
{
public:
	virtual ~IInputSource() = default;
	virtual bool StartListen() = 0;
	virtual void StopListen() = 0;
	virtual void SetHandler(IInputHandler* Handler) = 0;
};
