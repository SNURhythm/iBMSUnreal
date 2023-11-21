// Fill out your copyright notice in the Description page of Project Settings.
// get winuser.h raw input for windows

#include "RhythmInput.h"

#include "Chart.h"

TMap<int, int> FRhythmInput::DefaultKeyMap = {
	// keys: SDF, SPACE, JKL
	{0x53, 0}, {0x44, 1}, {0x46, 2}, {0x20, 3}, {0x4A, 4}, {0x4B, 5}, {0x4C, 6},
	// scratch: LS, RS
	{0x10, 7}
};
FRhythmInput::FRhythmInput(IRhythmControl* RhythmControlInit, const FChartMeta& Meta):RhythmControl(RhythmControlInit){
	// TODO: load keymap from config
	KeyMap = DefaultKeyMap;
}

FRhythmInput::~FRhythmInput()
{
}
#if PLATFORM_WINDOWS
LRESULT FRhythmInput::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){
	if(msg == WM_INPUT)
	{
		// get raw input
		UINT dwSize = 40;
		std::vector<uint8> lpb(dwSize);
		GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb.data(), &dwSize, sizeof(RAWINPUTHEADER));
		const int rawInput = lpb[30];
		const bool IsKeyDown = (lpb[26] & 0x01) != 1;
		if(rawInput != 0){
			UE_LOG(LogTemp, Warning, TEXT("Raw input: %d, %d"), rawInput, IsKeyDown);
			if(IsKeyDown){
				OnKeyDown(rawInput);
			} else {
				OnKeyUp(rawInput);
			}
		}
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
};
#endif
void FRhythmInput::StartListen()
{
	IsListening = true;
	ListenTask = UE::Tasks::Launch (UE_SOURCE_LOCATION, [this]()
	{
#if PLATFORM_WINDOWS
		WNDCLASSEXW wx = {};
		wx.cbSize = sizeof(WNDCLASSEXW);
		// proc to handle messages
		wx.lpfnWndProc = [](HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) -> LRESULT
		{
			FRhythmInput* pThis;
			if(msg == WM_NCCREATE){
				pThis = static_cast<FRhythmInput*>(reinterpret_cast<CREATESTRUCT*>(lParam)->lpCreateParams);
				SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
			} else {
				pThis = reinterpret_cast<FRhythmInput*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
			}
			if(pThis == nullptr) return DefWindowProc(hwnd, msg, wParam, lParam);
			if(!pThis->IsListening) return DefWindowProc(hwnd, msg, wParam, lParam);
			pThis->WndProc(hwnd, msg, wParam, lParam);
			return DefWindowProc(hwnd, msg, wParam, lParam);
		};
		
		wx.hInstance = GetModuleHandle(nullptr);
		wx.lpszClassName = TEXT("Dummy");
		RegisterClassEx(&wx);
		HWND hwnd = CreateWindowEx(WS_EX_OVERLAPPEDWINDOW, wx.lpszClassName, TEXT(""), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, HWND_MESSAGE, nullptr, wx.hInstance, this);
		RAWINPUTDEVICE rid;
		rid.usUsagePage = 0x01;
		rid.usUsage = 0x06;
		rid.dwFlags = RIDEV_INPUTSINK;
		rid.hwndTarget = hwnd;
		RegisterRawInputDevices(&rid, 1, sizeof(rid));
		MSG msg;
		
		while(IsListening && GetMessageW(&msg, hwnd, WM_NCCREATE, WM_INPUT))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		SendMessageW(hwnd, WM_CLOSE, 0, 0);
#endif
	});
}

void FRhythmInput::StopListen()
{
	IsListening = false;
}

void FRhythmInput::OnKeyDown(int KeyCode)
{
	UE_LOG(LogTemp, Warning, TEXT("Key %d pressed"), KeyCode);
	if(DefaultKeyMap.Contains(KeyCode)){
		RhythmControl->PressLane(DefaultKeyMap[KeyCode]);
	}
}

void FRhythmInput::OnKeyUp(int KeyCode)
{
	UE_LOG(LogTemp, Warning, TEXT("Key %d released"), KeyCode);
	if(DefaultKeyMap.Contains(KeyCode)){
		RhythmControl->ReleaseLane(DefaultKeyMap[KeyCode]);
	}
}
