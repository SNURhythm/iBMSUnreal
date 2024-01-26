#include "Input/RhythmInputHandler.h"

#include "Input/InputNormalizer.h"
#include "Input/NativeInputSource.h"
#include "Input/UnrealInputSource.h"
#include "Input/UnrealTouchInputSource.h"

FRhythmInputHandler::FRhythmInputHandler(IRhythmControl* Control, FChartMeta& Meta): RhythmControl(Control),
	ChartMeta(Meta)
{
	const TMap<int, TMap<FKey, int>> DefaultKeyMap = {
		{
			7, {
				// keys: SDF, SPACE, JKL
				{EKeys::S, 0}, {EKeys::D, 1}, {EKeys::F, 2}, {EKeys::SpaceBar, 3}, {EKeys::J, 4}, {EKeys::K, 5},
				{EKeys::L, 6},
				// scratch: LShift, RShift
				{EKeys::LeftShift, 7},
				{EKeys::RightShift, 7}
			}
		},
		{
			5, {
				// keys: DF, SPACE, JK
				{EKeys::D, 0}, {EKeys::F, 1}, {EKeys::SpaceBar, 2}, {EKeys::J, 3}, {EKeys::K, 4},
				// scratch: LShift, RShift
				{EKeys::LeftShift, 7},
				{EKeys::RightShift, 7}
			}
		},
		{
			14, {
				// keys: ZSXDCFV and MK,L.;/
				{EKeys::Z, 0}, {EKeys::S, 1}, {EKeys::X, 2}, {EKeys::D, 3}, {EKeys::C, 4}, {EKeys::F, 5}, {EKeys::V, 6},
				{EKeys::M, 8}, {EKeys::K, 9}, {EKeys::Comma, 10}, {EKeys::L, 11}, {EKeys::Period, 12},
				{EKeys::Semicolon, 13}, {EKeys::Slash, 14},
				// Lscratch: LShift
				{EKeys::LeftShift, 7},
				// Rscratch: RShift
				{EKeys::RightShift, 15}
			}
		},
		{
			10, {
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
	KeyMap = DefaultKeyMap[Meta.KeyMode];
}

FRhythmInputHandler::~FRhythmInputHandler()
{
	StopListen();
}

void FRhythmInputHandler::OnKeyDown(int KeyCode, KeySource Source, int CharCode)
{
	FKey Normalized = InputNormalizer::Normalize(KeyCode, Source, CharCode);
	UE_LOG(LogTemp, Warning, TEXT("Rhythm Key down: %s"), *Normalized.ToString());
	if (KeyMap.Contains(Normalized))
	{
		RhythmControl->PressLane(KeyMap[Normalized]);
	}
}

void FRhythmInputHandler::OnKeyUp(int KeyCode, KeySource Source, int CharCode)
{
	const FKey Normalized = InputNormalizer::Normalize(KeyCode, Source, CharCode);
	UE_LOG(LogTemp, Warning, TEXT("Rhythm Key up: %s"), *Normalized.ToString());
	if (KeyMap.Contains(Normalized))
	{
		RhythmControl->ReleaseLane(KeyMap[Normalized]);
	}
}

void FRhythmInputHandler::OnFingerDown(int FingerIndex, FVector Location)
{
	UE_LOG(LogTemp, Warning, TEXT("Finger Down Location: %f, %f"), Location.X, Location.Y);
	int MainLane, CompensateLane = -1;

	GetLaneFromScreenPosition(&MainLane, &CompensateLane, FVector2D(Location.X, Location.Y));
	UE_LOG(LogTemp, Warning, TEXT("MainLane: %d, CompensateLane: %d"), MainLane, CompensateLane);
	auto EffectiveLane = CompensateLane != -1
		                     ? RhythmControl->PressLane(MainLane, CompensateLane)
		                     : RhythmControl->PressLane(MainLane);
	UE_LOG(LogTemp, Warning, TEXT("EffectiveLane: %d"), EffectiveLane);
	FingerToLane[FingerIndex] = EffectiveLane;
}

void FRhythmInputHandler::OnFingerUp(int FingerIndex, FVector Location)
{
	int Lane = FingerToLane[FingerIndex];
	if (Lane < 0)
	{
		return;
	}
	RhythmControl->ReleaseLane(Lane);
	FingerToLane[FingerIndex] = -1;
}

int FRhythmInputHandler::ClampLane(int Lane) const
{
	int LaneCount = ChartMeta.KeyMode;
	if (Lane < 0)
	{
		return 7; // left scratch
	}
	if (Lane >= LaneCount)
	{
		return ChartMeta.IsDP ? 15 : 7; // right scratch
	}
	if (Lane >= 7 && ChartMeta.KeyMode == 14)
	{
		// 14Keys: 7 is scratch, so we should map 7~13 to 8~14
		Lane += 1;
	}
	if (Lane >= 5 && ChartMeta.KeyMode == 10)
	{
		// 10Keys: 5,6 is empty and 7 is scratch, so we should map 5~9 to 8~12
		Lane += 3;
	}
	return Lane;
}


void FRhythmInputHandler::GetLaneFromScreenPosition(int* MainLane, int* CompensateLane, FVector2D ScreenPosition) const
{
	FVector worldPosition;
	FVector worldDirection;
	PlayerController->DeprojectScreenPositionToWorld(ScreenPosition.X, ScreenPosition.Y, worldPosition, worldDirection);
	worldPosition = worldPosition / GNearClippingPlane * TouchDistance;
	worldPosition.X = worldPosition.X + Area->GetActorLocation().X; // -leftmost~rightmost => 0~width
	UE_LOG(LogTemp, Warning, TEXT("World Location: %f, %f, %f"), worldPosition.X, worldPosition.Y, worldPosition.Z);
	const int LaneCount = ChartMeta.KeyMode;
	const float LaneWidth = Area->GetActorScale().X / LaneCount;
	UE_LOG(LogTemp, Warning, TEXT("Lane Width: %f"), LaneWidth);
	int Lane = static_cast<int>(FMath::FloorToInt((worldPosition.X) / LaneWidth));
	int LeftCompensateLane = static_cast<int>(FMath::FloorToInt((worldPosition.X - LaneWidth / 3) / LaneWidth));
	int RightCompensateLane = static_cast<int>(FMath::FloorToInt((worldPosition.X + LaneWidth / 3) / LaneWidth));
	UE_LOG(LogTemp, Warning, TEXT("RawLane: %d"), Lane);
	Lane = ClampLane(Lane);
	LeftCompensateLane = ClampLane(LeftCompensateLane);
	RightCompensateLane = ClampLane(RightCompensateLane);

	*MainLane = Lane;
	if (LeftCompensateLane != Lane)
	{
		*CompensateLane = LeftCompensateLane;
	}
	if (RightCompensateLane != Lane)
	{
		*CompensateLane = RightCompensateLane;
	}
}

bool FRhythmInputHandler::StartListenNative()
{
	if (NativeInput != nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Input already started"));
		return false;
	}
	NativeInput = new FNativeInputSource();
	NativeInput->SetHandler(this);
	return NativeInput->StartListen();
}

bool FRhythmInputHandler::StartListenUnreal(UInputComponent* InputComponent)
{
	if (UnrealInput != nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Input already started"));
		return false;
	}
	UnrealInput = new FUnrealInputSource(InputComponent);
	UnrealInput->SetHandler(this);
	return UnrealInput->StartListen();
}

bool FRhythmInputHandler::StartListenUnrealTouch(APlayerController* PController, AActor* NoteArea, float Distance)
{
	// init finger to lane map
	for (int i = 0; i < 10; i++)
	{
		FingerToLane.Add(i, -1);
	}
	PlayerController = PController;
	Area = NoteArea;
	TouchDistance = Distance;
	if (UnrealTouchInput != nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Input already started"));
		return false;
	}
	UnrealTouchInput = new FUnrealTouchInputSource(PController->InputComponent);
	UnrealTouchInput->SetHandler(this);
	return UnrealTouchInput->StartListen();
}

void FRhythmInputHandler::StopListen()
{
	for (auto& Input : {&NativeInput, &UnrealInput, &UnrealTouchInput})
	{
		if (*Input != nullptr)
		{
			(*Input)->StopListen();
			delete (*Input);
			(*Input) = nullptr;
		}
	}
}
