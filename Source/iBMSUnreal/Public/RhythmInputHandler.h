#pragma once
#include "Chart.h"
#include "IInputSource.h"
#include "IInputHandler.h"
#include "InputNormalizer.h"
#include "IRhythmControl.h"

class FRhythmInputHandler: public IInputHandler
{
private:
	TMap<FKey, int> KeyMap;
	IRhythmControl* RhythmControl;
	IInputSource* Input = nullptr;
public:
	FRhythmInputHandler(IRhythmControl* Control, FChartMeta& ChartMeta);
	virtual void OnKeyDown(int KeyCode, KeySource Source, int CharCode = 0) override;
	virtual void OnKeyUp(int KeyCode, KeySource Source, int CharCode = 0) override;
	bool StartListenNative();
	bool StartListenUnreal(UInputComponent* InputComponent);
	void StopListen();
};
