#include "RhythmInputHandler.h"

#include "InputNormalizer.h"
#include "NativeInputSource.h"
#include "UnrealInputSource.h"

FRhythmInputHandler::FRhythmInputHandler(IRhythmControl* Control, FChartMeta& ChartMeta): RhythmControl(Control)
{
	const TMap<int, TMap<FKey, int>> DefaultKeyMap = {
		{
			7,{
				// keys: SDF, SPACE, JKL
				{EKeys::S, 0}, {EKeys::D, 1}, {EKeys::F, 2}, {EKeys::SpaceBar, 3}, {EKeys::J, 4}, {EKeys::K, 5}, {EKeys::L, 6},
				// scratch: LShift, RShift
				{EKeys::LeftShift, 7},
				{EKeys::RightShift, 7}
			}
		},
		{
			5,{
				// keys: DF, SPACE, JK
				{EKeys::D, 0}, {EKeys::F, 1}, {EKeys::SpaceBar, 2}, {EKeys::J, 3}, {EKeys::K, 4},
				// scratch: LShift, RShift
				{EKeys::LeftShift, 7},
				{EKeys::RightShift, 7}
			}
		},
		{
			14,{
				// keys: ZSXDCFV and MK,L.;/
				{EKeys::Z, 0}, {EKeys::S, 1}, {EKeys::X, 2}, {EKeys::D, 3}, {EKeys::C, 4}, {EKeys::F, 5}, {EKeys::V, 6},
				{EKeys::M, 8}, {EKeys::K, 9}, {EKeys::Comma, 10}, {EKeys::L, 11}, {EKeys::Period, 12}, {EKeys::Semicolon, 13}, {EKeys::Slash, 14},
				// Lscratch: LShift
				{EKeys::LeftShift, 7},
				// Rscratch: RShift
				{EKeys::RightShift, 15}
			}
		},
		{
			10,{
				// keys: ZSXDC and ,l.;/
				{EKeys::Z, 0}, {EKeys::S, 1}, {EKeys::X, 2}, {EKeys::D, 3}, {EKeys::C, 4},
				{EKeys::Comma, 8}, {EKeys::L, 9}, {EKeys::Period, 10}, {EKeys::Semicolon, 11}, {EKeys::Slash, 12},
				// Lscratch: LShift
				{EKeys::LeftShift, 7},
				// Rscratch: RShift
				{EKeys::RightShift, 15}
			}
		}
	};
	KeyMap = DefaultKeyMap[ChartMeta.KeyMode];
}

void FRhythmInputHandler::OnKeyDown(int KeyCode, KeySource Source, int CharCode)
{
	FKey Normalized = InputNormalizer::Normalize(KeyCode, Source, CharCode);
	UE_LOG(LogTemp, Warning, TEXT("Rhythm Key down: %s"), *Normalized.ToString());
	if(KeyMap.Contains(Normalized)){
		RhythmControl->PressLane(KeyMap[Normalized]);
	}
}

void FRhythmInputHandler::OnKeyUp(int KeyCode, KeySource Source, int CharCode)
{
	FKey Normalized = InputNormalizer::Normalize(KeyCode, Source, CharCode);
	UE_LOG(LogTemp, Warning, TEXT("Rhythm Key up: %s"), *Normalized.ToString());
	if(KeyMap.Contains(Normalized)){
		RhythmControl->ReleaseLane(KeyMap[Normalized]);
	}
}
bool FRhythmInputHandler::StartListenNative()
{
	Input = new FNativeInputSource();
	Input->SetHandler(this);
	return Input->StartListen();
}

bool FRhythmInputHandler::StartListenUnreal(UInputComponent* InputComponent)
{
	Input = new FUnrealInputSource(InputComponent);
	Input->SetHandler(this);
	return Input->StartListen();
}

void FRhythmInputHandler::StopListen()
{
	if(Input != nullptr){
		Input->StopListen();
	}
}
