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
	IInputSource* NativeInput = nullptr;
	IInputSource* UnrealInput = nullptr;
	IInputSource* UnrealTouchInput = nullptr;
	APlayerController* PlayerController = nullptr;
	AActor* Area = nullptr;
	float TouchDistance = 0;
	FChartMeta& ChartMeta;
	TMap<int, int> FingerToLane;
public:
	FRhythmInputHandler(IRhythmControl* Control, FChartMeta& Meta);
	~FRhythmInputHandler();
	virtual void OnKeyDown(int KeyCode, KeySource Source, int CharCode = 0) override;
	virtual void OnKeyUp(int KeyCode, KeySource Source, int CharCode = 0) override;
	virtual void OnFingerDown(int FingerIndex, FVector Location) override;
	virtual void OnFingerUp(int FingerIndex, FVector Location) override;
	int ClampLane(int Lane) const;
	void GetLaneFromScreenPosition(int* MainLane, int* CompensateLane, FVector2D ScreenPosition) const;
	bool StartListenNative();
	bool StartListenUnreal(UInputComponent* InputComponent);
	bool StartListenUnrealTouch(APlayerController* PController, AActor* NoteArea, float Distance);
	void StopListen();
};
