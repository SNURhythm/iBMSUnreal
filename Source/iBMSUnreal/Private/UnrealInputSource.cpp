// Fill out your copyright notice in the Description page of Project Settings.


#include "UnrealInputSource.h"



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
	Press.KeyDelegate.GetDelegateWithKeyForManualSet().BindLambda([=](const FKey& Key)
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
	Release.KeyDelegate.GetDelegateWithKeyForManualSet().BindLambda([=](const FKey& Key)
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
