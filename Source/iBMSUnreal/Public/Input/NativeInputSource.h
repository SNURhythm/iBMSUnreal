// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "CoreMinimal.h"
#include <Tasks/Task.h>

#include "IInputSource.h"
#include "IInputHandler.h"
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
class IBMSUNREAL_API FNativeInputSource : public IInputSource
{
public:
	FNativeInputSource();
	virtual ~FNativeInputSource() override;

	virtual bool StartListen() override;
	virtual void StopListen() override;
	virtual void SetHandler(IInputHandler* Handler) override;

private:
	void OnKeyDown(int KeyCode, KeySource Source);
	void OnKeyUp(int KeyCode, KeySource Source);
#if PLATFORM_WINDOWS
	LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	HWND CurrentHwnd;
#elif PLATFORM_MAC
	CFRunLoopRef CurrentCFRunLoop;
	IOHIDManagerRef hidManager = nullptr;

	static CGEventRef EventTapCallBack(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon);
#endif
	IInputHandler* InputHandler = nullptr;
	TMap<int, int> KeyMap;
	UE::Tasks::FTask ListenTask;
	std::atomic_bool IsListening;
};
