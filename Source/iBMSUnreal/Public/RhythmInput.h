// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "CoreMinimal.h"
#include <Tasks/Task.h>
#include "IRhythmControl.h"
#if PLATFORM_WINDOWS

#include "Windows/AllowWindowsPlatformTypes.h"

#include "Windows.h"
#endif

class FChartMeta;

/**
 * 
 */
class IBMSUNREAL_API FRhythmInput
{

public:
	static TMap<int, int> DefaultKeyMap;
	FRhythmInput(IRhythmControl* RhythmControlInit, const FChartMeta& Meta);
	~FRhythmInput();
	
	void StartListen();
	void StopListen();

private:
	void OnKeyDown(int KeyCode);
	void OnKeyUp(int KeyCode);
#if PLATFORM_WINDOWS
	LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif
	IRhythmControl* RhythmControl;
	TMap<int, int> KeyMap;
	UE::Tasks::FTask ListenTask;
	std::atomic_bool IsListening;
};
