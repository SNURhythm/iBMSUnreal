// Fill out your copyright notice in the Description page of Project Settings.
// get winuser.h raw input for windows

#include "Input/NativeInputSource.h"

#include "Chart.h"
// include macos appkit
FNativeInputSource::FNativeInputSource(){
}

FNativeInputSource::~FNativeInputSource()
{
}
#if PLATFORM_WINDOWS
LRESULT FNativeInputSource::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){
	if(msg == WM_INPUT)
	{
		// get raw input
		UINT dwSize = 40;
		std::vector<uint8> lpb(dwSize);
		GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb.data(), &dwSize, sizeof(RAWINPUTHEADER));
		int rawInput = lpb[30];
		const bool IsKeyDown = (lpb[26] & 0x01) != 1;
		if(rawInput != 0){
			UE_LOG(LogTemp, Warning, TEXT("Raw input: %d, %d"), rawInput, IsKeyDown);
			if(!IsListening) return DefWindowProc(hwnd, msg, wParam, lParam);
			if (rawInput == 0x10)
			{
				// LShift
				switch(lpb[24])
				{
				case 42:
					rawInput = 0xA0; // LShift
					break;
				case 54:
					rawInput = 0xA1; // RShift
					break;
				default:
					break;
				}
			}
			if(IsKeyDown){
				OnKeyDown(rawInput, KeySource::VirtualKey);
			} else {
				OnKeyUp(rawInput, KeySource::VirtualKey);
			}
		}
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
};

#elif PLATFORM_MAC

CFMutableDictionaryRef myCreateDeviceMatchingDictionary(UInt32 usagePage,
		UInt32 usage) {
	CFMutableDictionaryRef ret = CFDictionaryCreateMutable(kCFAllocatorDefault,
			0, &kCFTypeDictionaryKeyCallBacks,
			&kCFTypeDictionaryValueCallBacks);
	if (!ret)
		return NULL;

	CFNumberRef pageNumberRef = CFNumberCreate(kCFAllocatorDefault,
			kCFNumberIntType, &usagePage );
	if (!pageNumberRef) {
		CFRelease(ret);
		return NULL;
	}

	CFDictionarySetValue(ret, CFSTR(kIOHIDDeviceUsagePageKey), pageNumberRef);
	CFRelease(pageNumberRef);

	CFNumberRef usageNumberRef = CFNumberCreate(kCFAllocatorDefault,
			kCFNumberIntType, &usage);
	if (!usageNumberRef) {
		CFRelease(ret);
		return NULL;
	}

	CFDictionarySetValue(ret, CFSTR(kIOHIDDeviceUsageKey), usageNumberRef);
	CFRelease(usageNumberRef);

	return ret;
}
#endif
bool FNativeInputSource::StartListen()
{
	UE_LOG(LogTemp, Warning, TEXT("Start listen key input"));
	IsListening = true;
#if PLATFORM_MAC
	IOHIDManagerRef hidManager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
	if (!hidManager) {
		UE_LOG(LogTemp, Warning, TEXT("failed to create hid manager"));
		return false;
	}
	CFMutableDictionaryRef keyboard =
		myCreateDeviceMatchingDictionary(0x01, 6);
	CFMutableDictionaryRef keypad =
		myCreateDeviceMatchingDictionary(0x01, 7);

	CFMutableDictionaryRef matchesList[] = {
		keyboard,
		keypad,
	};
	CFArrayRef matches = CFArrayCreate(kCFAllocatorDefault,
			(const void **)matchesList, 2, NULL);
	IOHIDManagerSetDeviceMatchingMultiple(hidManager, matches);
	IOHIDManagerRegisterInputValueCallback(hidManager, [](void* context, IOReturn result, void* sender, IOHIDValueRef value) {
		FNativeInputSource* pThis = reinterpret_cast<FNativeInputSource*>(context);
		if(!pThis->IsListening) {
			UE_LOG(LogTemp, Warning, TEXT("Stop listen key input - macOS"));
			CFRunLoopStop(CFRunLoopGetCurrent());
			return;
		}
		if(result != kIOReturnSuccess) return;
		IOHIDElementRef elem = IOHIDValueGetElement(value);
		if (IOHIDElementGetUsagePage(elem) != 0x07)
			return;
		const int KeyCode = IOHIDElementGetUsage(elem);
		if(KeyCode<4 || KeyCode>231) return;
		const bool IsKeyDown = IOHIDValueGetIntegerValue(value);
		if(IsKeyDown){
			pThis->OnKeyDown(KeyCode, KeySource::ScanCode);
		} else {
			pThis->OnKeyUp(KeyCode, KeySource::ScanCode);
		}
	}, this);

	UE_LOG(LogTemp, Warning, TEXT("Start listen key input - macOS"));

	ListenTask = UE::Tasks::Launch (UE_SOURCE_LOCATION, [this, hidManager]()
	{
		CurrentCFRunLoop = CFRunLoopGetCurrent();
		IOHIDManagerScheduleWithRunLoop(hidManager, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
		IOHIDManagerOpen(hidManager, kIOHIDOptionsTypeNone);
		UE_LOG(LogTemp, Warning, TEXT("Start listen key input - macOS 2"));
		CFRunLoopRun();
		UE_LOG(LogTemp, Warning, TEXT("Start listen key input - macOS 3"));
	});
#elif PLATFORM_WINDOWS
	ListenTask = UE::Tasks::Launch (UE_SOURCE_LOCATION, [this]()
	{
		WNDCLASSEXW wx = {};
		wx.cbSize = sizeof(WNDCLASSEXW);
		// proc to handle messages
		wx.lpfnWndProc = [](HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) -> LRESULT
		{
			FNativeInputSource* pThis;
			if(msg == WM_NCCREATE){
				pThis = static_cast<FNativeInputSource*>(reinterpret_cast<CREATESTRUCT*>(lParam)->lpCreateParams);
				SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
			} else {
				pThis = reinterpret_cast<FNativeInputSource*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
			}
			if(pThis == nullptr) return DefWindowProc(hwnd, msg, wParam, lParam);
			if(!pThis->IsListening) return DefWindowProc(hwnd, msg, wParam, lParam);
			pThis->WndProc(hwnd, msg, wParam, lParam);
			return DefWindowProc(hwnd, msg, wParam, lParam);
		};
		
		wx.hInstance = GetModuleHandle(nullptr);
		wx.lpszClassName = TEXT("Dummy");
		RegisterClassEx(&wx);
		CurrentHwnd = CreateWindowEx(WS_EX_OVERLAPPEDWINDOW, wx.lpszClassName, TEXT(""), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, HWND_MESSAGE, nullptr, wx.hInstance, this);
		
		RAWINPUTDEVICE rid;
		rid.usUsagePage = 0x01;
		rid.usUsage = 0x06;
		rid.dwFlags = RIDEV_INPUTSINK;
		rid.hwndTarget = CurrentHwnd;
		RegisterRawInputDevices(&rid, 1, sizeof(rid));
		MSG msg;
		
		while(IsListening && GetMessageW(&msg, CurrentHwnd, WM_CLOSE, WM_INPUT))
		{
			if(!IsListening) break;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		SendMessageW(CurrentHwnd, WM_CLOSE, 0, 0);
	});
#endif

	return true;
}

void FNativeInputSource::StopListen()
{
	UE_LOG(LogTemp, Warning, TEXT("Stop listen key input"));
	IsListening = false;
#if PLATFORM_WINDOWS
	if(CurrentHwnd)
	{
		PostMessageW(CurrentHwnd, WM_CLOSE, 0, 0);
		UE_LOG(LogTemp, Warning, TEXT("Hwnd closed"));
	}


#elif PLATFORM_MAC
	CFRunLoopStop(CurrentCFRunLoop);
#endif
	// ReSharper disable once CppExpressionWithoutSideEffects
	ListenTask.Wait();
}

void FNativeInputSource::SetHandler(IInputHandler* Handler)
{
	InputHandler = Handler;
}

void FNativeInputSource::OnKeyDown(int KeyCode, KeySource Source)
{
	UE_LOG(LogTemp, Warning, TEXT("Native: Key %d pressed"), KeyCode);
	if(InputHandler != nullptr) InputHandler->OnKeyDown(KeyCode, Source);
}

void FNativeInputSource::OnKeyUp(int KeyCode, KeySource Source)
{
	UE_LOG(LogTemp, Warning, TEXT("Native: Key %d released"), KeyCode);
	if(InputHandler != nullptr) InputHandler->OnKeyUp(KeyCode, Source);
}
