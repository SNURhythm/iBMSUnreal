// Fill out your copyright notice in the Description page of Project Settings.


#include "Input/UnrealInputSource.h"



FUnrealInputSource::~FUnrealInputSource()
{
}

bool FUnrealInputSource::StartListen()
{
	// detect any key, pressed or released
	FInputKeyBinding Press(FInputChord(EKeys::AnyKey, false, false, false, false), EInputEvent::IE_Pressed);
	FInputKeyBinding Release(FInputChord(EKeys::AnyKey, false, false, false, false), EInputEvent::IE_Released);
	// Key is any FKey value, commonly found in the EKeys struct, including EKeys::AnyKey
	// You can also skip FInputChord altogether and just use the FKey value instead
	// EInputEvent::IE_Pressed can be swapped out to IE_Released if you want the release event instead
	Press.bConsumeInput = true;
	Press.bExecuteWhenPaused = false;
	Press.KeyDelegate.GetDelegateWithKeyForManualSet().BindLambda([this](const FKey& Key)
	{
		// print keycode
		// get keycode integer
		const uint32 *KeyCode, *CharCode;
		FInputKeyManager::Get().GetCodesFromKey(Key, KeyCode, CharCode);
		const uint32 KeyCodeValue = KeyCode ? *KeyCode : 0;
		const uint32 CharCodeValue = CharCode ? *CharCode : 0;
		UE_LOG(LogTemp, Warning, TEXT("Key pressed from unreal basic input: %s, %d, %d"), *Key.ToString(), KeyCodeValue, CharCodeValue);
		// Your code here
		if(InputHandler != nullptr) InputHandler->OnKeyDown(KeyCodeValue, KeySource::UnrealKey, CharCodeValue);
	});
	Release.bConsumeInput = true;
	Release.bExecuteWhenPaused = false;
	Release.KeyDelegate.GetDelegateWithKeyForManualSet().BindLambda([this](const FKey& Key)
	{
		// print keycode
		// get keycode integer
		const uint32 *KeyCode, *CharCode;
		FInputKeyManager::Get().GetCodesFromKey(Key, KeyCode, CharCode);
		const uint32 KeyCodeValue = KeyCode ? *KeyCode : 0;
		const uint32 CharCodeValue = CharCode ? *CharCode : 0;
		UE_LOG(LogTemp, Warning, TEXT("Key released from unreal basic input: %s, %d, %d"), *Key.ToString(), KeyCodeValue, CharCodeValue);
		// Your code here
		if(InputHandler != nullptr) InputHandler->OnKeyUp(KeyCodeValue, KeySource::UnrealKey, CharCodeValue);
	});
	Index = InputComponent->KeyBindings.Add(Press);
	InputComponent->KeyBindings.Add(Release);
	// touch
	FInputTouchBinding TouchPress(EInputEvent::IE_Pressed);
	FInputTouchBinding TouchRelease(EInputEvent::IE_Released);
	// touch move
	FInputTouchBinding TouchMove(EInputEvent::IE_Repeat);
	TouchPress.bConsumeInput = true;
	TouchPress.bExecuteWhenPaused = false;

	TouchPress.TouchDelegate.GetDelegateForManualSet().BindLambda([this](ETouchIndex::Type FingerIndex, FVector Location)
	{
		UE_LOG(LogTemp, Warning, TEXT("Touch pressed from unreal basic input: %d, %s"), FingerIndex, *Location.ToString());
		// Your code here
		if(InputHandler != nullptr) InputHandler->OnFingerDown(FingerIndex, Location);
	});
	TouchRelease.bConsumeInput = true;
	TouchRelease.bExecuteWhenPaused = false;

	TouchRelease.TouchDelegate.GetDelegateForManualSet().BindLambda([this](ETouchIndex::Type FingerIndex, FVector Location)
	{
		UE_LOG(LogTemp, Warning, TEXT("Touch released from unreal basic input: %d, %s"), FingerIndex, *Location.ToString());
		// Your code here
		if(InputHandler != nullptr) InputHandler->OnFingerUp(FingerIndex, Location);
	});

	TouchMove.bConsumeInput = true;
	TouchMove.bExecuteWhenPaused = false;

	TouchMove.TouchDelegate.GetDelegateForManualSet().BindLambda([this](ETouchIndex::Type FingerIndex, FVector Location)
	{
		UE_LOG(LogTemp, Warning, TEXT("Touch moved from unreal basic input: %d, %s"), FingerIndex, *Location.ToString());
		// Your code here
	});

	InputComponent->TouchBindings.Add(TouchPress);
	InputComponent->TouchBindings.Add(TouchRelease);
	InputComponent->TouchBindings.Add(TouchMove);
	
	return true;
}

void FUnrealInputSource::StopListen()
{
	if(Index >= 0)
	{
		InputComponent->KeyBindings.RemoveAt(Index);
		InputComponent->KeyBindings.RemoveAt(Index);
	}
}

void FUnrealInputSource::SetHandler(IInputHandler* Handler)
{
	InputHandler = Handler;
}
