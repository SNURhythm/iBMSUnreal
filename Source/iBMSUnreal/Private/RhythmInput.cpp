// Fill out your copyright notice in the Description page of Project Settings.
// get winuser.h raw input for windows

#include "RhythmInput.h"


#include "Chart.h"


FRhythmInput::FRhythmInput(IRhythmControl* RhythmControlInit, const FChartMeta& Meta):RhythmControl(RhythmControlInit){
	// TODO: load keymap from config
	const TMap<int, TMap<int, int>> DefaultKeyMap = {
		{
			7,{
				// keys: SDF, SPACE, JKL
				{0x53, 0}, {0x44, 1}, {0x46, 2}, {0x20, 3}, {0x4A, 4}, {0x4B, 5}, {0x4C, 6},
				// scratch: LShift, RShift
				{0xA0, 7},
				{0xA1, 7}
			}
		},
		{
			5,{
				// keys: DF, SPACE, JK
				{0x44, 0}, {0x46, 1}, {0x20, 2}, {0x4A, 3}, {0x4B, 4}, {0x4C, 5},
				// scratch: LShift, RShift
				{0xA0, 7},
				{0xA1, 7}
			}
		},
		{
		14,{
				// keys: ZSXDCFV and MK,L.;/
				{0x5A, 0}, {0x53, 1}, {0x58, 2}, {0x44, 3}, {0x43, 4}, {0x46, 5}, {0x56, 6},
				{0x4D, 8}, {0x4B, 9}, {0xBC, 10}, {0x4C, 11}, {0xBE, 12}, {0xBA, 13}, {0xBF, 14},
				// Lscratch: LShift
				{0xA0, 7},
				// Rscratch: RShift
				{0xA1, 15}
			}
		},
		{
			10,{
				// keys: ZSXDC and ,l.;/
				{0x5A, 0}, {0x53, 1}, {0x58, 2}, {0x44, 3}, {0x43, 4},
				{0xBC, 8}, {0x4C, 9}, {0xBE, 10}, {0xBA, 11}, {0xBF, 12},
				// Lscratch: LShift
				{0xA0, 7},
				// Rscratch: RShift
				{0xA1, 15}
			}
		}
	};
	KeyMap = DefaultKeyMap[Meta.KeyMode];
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
	UE_LOG(LogTemp, Warning, TEXT("Start listen key input"));
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
#endif
	});
}

void FRhythmInput::StopListen()
{
	UE_LOG(LogTemp, Warning, TEXT("Stop listen key input"));
	if(CurrentHwnd)
	{
		PostMessageW(CurrentHwnd, WM_CLOSE, 0, 0);
		UE_LOG(LogTemp, Warning, TEXT("Hwnd closed"));
	}
	IsListening = false;

	// ReSharper disable once CppExpressionWithoutSideEffects
	ListenTask.Wait();
}

void FRhythmInput::OnKeyDown(int KeyCode)
{
	UE_LOG(LogTemp, Warning, TEXT("Key %d pressed"), KeyCode);
	if(KeyMap.Contains(KeyCode)){
		UE_LOG(LogTemp, Warning, TEXT("Key %d mapped to %d"), KeyCode, KeyMap[KeyCode]);
		RhythmControl->PressLane(KeyMap[KeyCode]);
	}
}

void FRhythmInput::OnKeyUp(int KeyCode)
{
	UE_LOG(LogTemp, Warning, TEXT("Key %d released"), KeyCode);
	if(KeyMap.Contains(KeyCode)){
		UE_LOG(LogTemp, Warning, TEXT("Key %d mapped to %d"), KeyCode, KeyMap[KeyCode]);
		RhythmControl->ReleaseLane(KeyMap[KeyCode]);
	}
}
