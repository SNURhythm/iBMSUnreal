// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "CoreMinimal.h"
#include <Tasks/Task.h>
#include "IRhythmControl.h"

#if PLATFORM_WINDOWS

#include "Windows/AllowWindowsPlatformTypes.h"

#include "Windows.h"
#elif PLATFORM_MAC
#include <CoreGraphics/CoreGraphics.h>
#include <CoreFoundation/CFRunLoop.h>
#include <IOKit/hid/IOHIDManager.h>
#endif

class FChartMeta;

/**
 * 
 */
class IBMSUNREAL_API FRhythmInput
{

public:
	FRhythmInput(IRhythmControl* RhythmControlInit, const FChartMeta& Meta);
	~FRhythmInput();
	
	bool StartListen();
	void StopListen();

private:
	void OnKeyDown(int KeyCode);
	void OnKeyUp(int KeyCode);
#if PLATFORM_WINDOWS
	LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	HWND CurrentHwnd;
#elif PLATFORM_MAC
	CFRunLoopRef CurrentCFRunLoop;

	static CGEventRef EventTapCallBack(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon);
#endif
	IRhythmControl* RhythmControl;
	TMap<int, int> KeyMap;
	UE::Tasks::FTask ListenTask;
	std::atomic_bool IsListening;
};
