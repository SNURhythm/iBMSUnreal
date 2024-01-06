// Fill out your copyright notice in the Description page of Project Settings.


#include "RhythmControl.h"

#include <thread>

#include "BMSGameInstance.h"
#include "BMSParser.h"
#include "RhythmHUD.h"
#include "PauseHUD.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "Tasks/Task.h"


void ARhythmControl::OnJudge(const FJudgeResult& JudgeResult) const
{
	State->LatestJudgement = JudgeResult.Judgement;
	State->JudgeCount[JudgeResult.Judgement]++;
	if(JudgeResult.IsComboBreak())
	{
		State->Combo = 0;
	} else if(JudgeResult.Judgement != Kpoor)
	{
		State->Combo++;
	}
	CurrentRhythmHUD->OnJudge(JudgeResult, State->Combo);
	// UE_LOG(LogTemp, Warning, TEXT("Judge: %s, Combo: %d, Diff: %lld"), *JudgeResult.ToString(), State->Combo, JudgeResult.Diff);
}

void ARhythmControl::CheckPassedTimeline(const long long Time)
{
	auto Measures = Chart->Measures;
	if(State == nullptr) return;
	int totalLoopCount = 0;
	for(int i = State->PassedMeasureCount; i < Measures.Num(); i++)
	{
		const bool IsFirstMeasure = i == State->PassedMeasureCount;
		const auto& Measure = Measures[i];
		for(int j = IsFirstMeasure ? State->PassedTimelineCount : 0; j < Measure->TimeLines.Num(); j++)
		{
			totalLoopCount++;
			const auto& Timeline = Measure->TimeLines[j];
			if(Timeline->Timing < Time - 200000)
			{
				if(IsFirstMeasure) State->PassedTimelineCount++;
				// make remaining notes POOR
				for (const auto& Note : Timeline->Notes)
				{
					if(Note == nullptr) continue;
					if(Note->IsPlayed) continue;
					if(Note->IsLandmineNote()) continue;
					if(Note->IsLongNote())
					{
						const auto& LongNote = static_cast<FBMSLongNote*>(Note);
						if(!LongNote->IsTail())
						{
							LongNote->MissPress(Time);
						}
					}
					const auto PoorResult = FJudgeResult(Poor, Time - Timeline->Timing);
					OnJudge(PoorResult);
				}
			} else if(Timeline->Timing <= Time)
			{
				// auto-release long notes
				for(const auto& Note : Timeline->Notes)
				{
					if(Note == nullptr) continue;
					if(Note->IsPlayed) continue;
					if(Note->IsLandmineNote())
					{
						// TODO: if lane is being pressed, detonate landmine
						continue;
					}
					if(Note->IsLongNote())
					{
						const auto& LongNote = static_cast<FBMSLongNote*>(Note);
						if(LongNote->IsTail())
						{
							if(!LongNote->IsHolding) continue;
							LongNote->Release(Time);
							const auto JudgeResult = State->Judge->JudgeNow(LongNote->Head, LongNote->Head->PlayedTime);
							OnJudge(JudgeResult);
							if(Options.AutoPlay)
							{
								Renderer->OnLaneReleased(Note->Lane, std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
							}
							continue;
						}
					}
					if(Options.AutoPlay) // NormalNote or LongNote's head
					{
						const FJudgeResult JudgeResult = PressNote(Note, Time);
						Renderer->OnLanePressed(Note->Lane, JudgeResult, Time);
						if(!Note->IsLongNote())
						{
							Renderer->OnLaneReleased(Note->Lane, std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
						}
						
					}
					
				}
			} else
			{
				i = Measures.Num();
				break;
			}
		}
		if(State->PassedTimelineCount == Measure->TimeLines.Num() && IsFirstMeasure)
		{
			State->PassedMeasureCount++;
			State->PassedTimelineCount = 0;
		}
	}
	// UE_LOG(LogTemp, Warning, TEXT("Loop count: %d"), totalLoopCount);
}

// Sets default values
ARhythmControl::ARhythmControl()
{

 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}
void ARhythmControl::OnMediaOpened(FString OpenedUrl)
{
	const auto Dim = MediaPlayer->GetVideoTrackDimensions(0, 0);
	FVector Origin, BoxExtent;
	ImagePlate->GetActorBounds(false, Origin, BoxExtent);
	const float BoxWidth = BoxExtent.X * 2;
	const float BoxHeight = BoxExtent.Z * 2;
	
	// fill the screen
	FVector2D ViewportSize;
	GetWorld()->GetGameViewport()->GetViewportSize(ViewportSize);
	const FVector LeftTopWorld = FVector(-BoxWidth / 2, 0, BoxHeight / 2) + ImagePlate->GetActorLocation();
	const FVector RightBottomWorld = FVector(BoxWidth / 2, 0, -BoxHeight / 2) + ImagePlate->GetActorLocation();
	FVector2D LeftTopProjected;
	UGameplayStatics::ProjectWorldToScreen(GetWorld()->GetFirstPlayerController(), LeftTopWorld, LeftTopProjected);
	FVector2D RightBottomProjected;
	UGameplayStatics::ProjectWorldToScreen(GetWorld()->GetFirstPlayerController(), RightBottomWorld, RightBottomProjected);
	const float ProjectedBoxWidth = RightBottomProjected.X - LeftTopProjected.X;
	const float ProjectedBoxHeight = RightBottomProjected.Y - LeftTopProjected.Y;
	float NewWidth;
	float NewHeight;
	if(1.0*Dim.X / Dim.Y > ViewportSize.X / ViewportSize.Y)
	{
		NewWidth = ViewportSize.X;
		NewHeight = NewWidth * Dim.Y / Dim.X;
	} else
	{
		NewHeight = ViewportSize.Y;
		NewWidth = NewHeight * Dim.X / Dim.Y;
		
	}
	const float WScale = NewWidth / ProjectedBoxWidth;
	const float HScale = NewHeight / ProjectedBoxHeight;
	const auto OriginalScale = ImagePlate->GetActorScale();
	ImagePlate->SetActorScale3D(FVector(OriginalScale.X * WScale, OriginalScale.Y * HScale, OriginalScale.Z));
	
}

FJudgeResult ARhythmControl::PressNote(FBMSNote* Note, long long PressedTime)
{
	if(Note->Wav != FBMSParser::NoWav && !Options.AutoKeysound) Jukebox->PlayKeySound(Note->Wav);
	const auto JudgeResult = State->Judge->JudgeNow(Note, PressedTime);
	if(JudgeResult.Judgement != None)
	{
		if(JudgeResult.IsNotePlayed())
		{
			// TODO: play keybomb
			if(Note->IsLongNote())
			{
				const auto& LongNote = static_cast<FBMSLongNote*>(Note);
				if(!LongNote->IsTail()) LongNote->Press(PressedTime);
				return JudgeResult;
			}
			Note->Press(PressedTime);
		}
		OnJudge(JudgeResult);
	}
	return JudgeResult;
}

void ARhythmControl::ReleaseNote(FBMSNote* Note, long long ReleasedTime)
{
	if(!Note->IsLongNote()) return;
	const auto& LongNote = static_cast<FBMSLongNote*>(Note);
	if(!LongNote->IsTail()) return;
	if(!LongNote->IsHolding) return;
	LongNote->Release(ReleasedTime);
	const auto JudgeResult = State->Judge->JudgeNow(LongNote, ReleasedTime);
	// if tail judgement is not good/great/pgreat, make it bad
	if(JudgeResult.Judgement == None || JudgeResult.Judgement == Kpoor || JudgeResult.Judgement == Poor)
	{
		OnJudge(FJudgeResult(Bad, JudgeResult.Diff));
		return;
	}
	// otherwise, follow the head's judgement
	const auto HeadJudgeResult = State->Judge->JudgeNow(LongNote->Head, LongNote->Head->PlayedTime);
	OnJudge(HeadJudgeResult);
}

int ARhythmControl::PressLane(int MainLane, int CompensateLane, double InputDelay)
{
	std::vector<int> AvailableCandidates;

	for(auto Lane: MainLane == CompensateLane ? std::initializer_list<int>{MainLane} : std::initializer_list<int>{MainLane, CompensateLane})
	{
		if(!IsLanePressed.Contains(Lane) || IsLanePressed[Lane])
		{
			UE_LOG(LogTemp, Warning, TEXT("Ignoring %d"), Lane);
			continue;
		}
		AvailableCandidates.push_back(Lane);
	}
	if(AvailableCandidates.empty()) return MainLane;


	if(IsGamePaused) return MainLane;
	
	if(State == nullptr)
	{
		IsLanePressed[MainLane] = true;
		Renderer->OnLanePressed(MainLane, FJudgeResult(None, 0), std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
		return MainLane;
	}
	if(!State->IsPlaying)
	{
		IsLanePressed[MainLane] = true;
		return MainLane;
	}
	

	const auto& Measures = Chart->Measures;
	const auto PressedTime = Jukebox->GetPositionMicro() - static_cast<long long>(InputDelay * 1000000);
	for(int i = State->PassedMeasureCount; i < Measures.Num(); i++)
	{
		const bool IsFirstMeasure = i == State->PassedMeasureCount;
		const auto& Measure = Measures[i];

		for(int j = IsFirstMeasure ? State->PassedTimelineCount : 0; j < Measure->TimeLines.Num(); j++)
		{
			const auto& Timeline = Measure->TimeLines[j];
			if(Timeline->Timing < PressedTime - 200000) continue;
			for(auto Lane: AvailableCandidates)
			{
				const auto& Note = Timeline->Notes[Lane];
				if(Note == nullptr) continue;
				if(Note->IsPlayed) continue;
				if(Note->IsLandmineNote()) continue;
				if(State->Judge->JudgeNow(Note, PressedTime).Judgement == None) continue;
				const FJudgeResult Judgement = PressNote(Note, PressedTime);
				IsLanePressed[Lane] = true;
				Renderer->OnLanePressed(Lane, Judgement, std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
				return Lane;
			}
		}
	}
	IsLanePressed[MainLane] = true;
	Renderer->OnLanePressed(MainLane, FJudgeResult(None, 0), std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
	return MainLane;
}

int ARhythmControl::PressLane(int Lane, double InputDelay)
{
	return PressLane(Lane, Lane, InputDelay);
}

void ARhythmControl::ReleaseLane(int Lane, double InputDelay)
{
	if(!IsLanePressed.Contains(Lane) || !IsLanePressed[Lane]) return;
	IsLanePressed[Lane] = false;
	Renderer->OnLaneReleased(Lane, std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
	const auto ReleasedTime = Jukebox->GetPositionMicro() - static_cast<long long>(InputDelay * 1000000);
	
	if(State == nullptr) return;
	if(!State->IsPlaying) return;
	
	const auto& Measures = Chart->Measures;

	for(int i = State->PassedMeasureCount; i < Measures.Num(); i++)
	{
		const bool IsFirstMeasure = i == State->PassedMeasureCount;
		const auto& Measure = Measures[i];
		for(int j = IsFirstMeasure ? State->PassedTimelineCount : 0; j < Measure->TimeLines.Num(); j++)
		{
			const auto& Timeline = Measure->TimeLines[j];
			if(Timeline->Timing < ReleasedTime - 200000) continue;
			const auto& Note = Timeline->Notes[Lane];
			if(Note == nullptr) continue;
			if(Note->IsPlayed) continue;
			ReleaseNote(Note, ReleasedTime);
			return;
		}
	}
}

UWorld* ARhythmControl::GetContextWorld()
{
	return GetWorld();
}

void ARhythmControl::ResumeGame()
{
	IsGamePaused = false;
	CurrentPauseHUD->SetVisibility(ESlateVisibility::Hidden);
	if(!State) return;
	if(!State->IsPlaying)
	{
		StartGame();
		return;
	}
	
	Jukebox->Unpause();
}

void ARhythmControl::ExitGame()
{
	// exit level
	UGameplayStatics::OpenLevel(GetWorld(), FName("ChartSelectScreen"));
}

void ARhythmControl::RestartGame()
{
	LastPauseMicro = -1;
	IsLoaded = false;
	IsLoadCancelled = true;
	Jukebox->Stop();
	MediaPlayer->Close();
	Renderer->Reset();
	CurrentRhythmHUD->Reset();
	if(!Chart)
	{
		LoadGame();
		return;
	}

	for(auto& Measure: Chart->Measures)
	{
		for(auto& TimeLine: Measure->TimeLines)
		{
			for(auto& Note: TimeLine->Notes)
			{
				if(Note == nullptr) continue;
				Note->Reset();
			}
		}
	}
	State = new FRhythmState(Chart, false);
	IsLoadCancelled = false;
	IsLoaded = true;
	StartGame();
}

void ARhythmControl::StartGame()
{
	IsGamePaused = false;
	Jukebox->Start(Options.StartPosition, Options.AutoKeysound);
	State->IsPlaying = true;
	if(IsInGameThread()) CurrentPauseHUD->SetVisibility(ESlateVisibility::Hidden);
}

void ARhythmControl::PauseGame()
{
	auto CurrentMicro = Jukebox->GetPositionMicro();
	if(LastPauseMicro != -1 && LastPauseMicro + 1000000 > CurrentMicro)
	{
		// ignore pause if it's too close to the last pause
		return;
	}
	LastPauseMicro = Jukebox->GetPositionMicro();
	IsGamePaused = true;
	Jukebox->Pause();
	if(IsInGameThread())
	{
		CurrentPauseHUD->SetVisibility(ESlateVisibility::Visible);
		CurrentPauseHUD->ResumeButton->SetFocus();
	}
}

// Called when the game starts or when spawned
void ARhythmControl::BeginPlay()
{
	Super::BeginPlay();
	MediaPlayer->OnMediaOpened.AddDynamic(this, &ARhythmControl::OnMediaOpened);
	// force garbage collection
	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	UE_LOG(LogTemp, Warning, TEXT("Rhythm BeginPlay"));
	// get BMSRenderer actor component
	GameInstance = Cast<UBMSGameInstance>(GetGameInstance());
	Renderer = Cast<UBMSRenderer>(GetComponentByClass(UBMSRenderer::StaticClass()));
	Jukebox = new FJukebox(GameInstance->GetFMODSystem());
	if(IsValid(RhythmHUDClass))
	{
		CurrentRhythmHUD = CreateWidget<URhythmHUD>(GetWorld(), RhythmHUDClass);
		CurrentRhythmHUD->AddToViewport();
		CurrentRhythmHUD->PauseButton->OnClicked.AddDynamic(this, &ARhythmControl::PauseGame);
	}
	if(IsValid(PauseHUDClass))
	{
		CurrentPauseHUD = CreateWidget<UPauseHUD>(GetWorld(), PauseHUDClass);
		CurrentPauseHUD->AddToViewport();
		CurrentPauseHUD->RestartButton->OnClicked.AddDynamic(this, &ARhythmControl::RestartGame);
		CurrentPauseHUD->ResumeButton->OnClicked.AddDynamic(this, &ARhythmControl::ResumeGame);
		CurrentPauseHUD->ExitButton->OnClicked.AddDynamic(this, &ARhythmControl::ExitGame);
		CurrentPauseHUD->SetVisibility(ESlateVisibility::Hidden);
	}
	// get player controller
	PlayerController = GetWorld()->GetFirstPlayerController();
	// Get the input component
	PlayerInputComponent = PlayerController->InputComponent;
	if (!PlayerInputComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("InputComponent is null"));
		return;
	}
	// on esc
	FInputKeyBinding Press(FInputChord(EKeys::Escape, false, false, false, false), EInputEvent::IE_Pressed);
	Press.bConsumeInput = true;
	Press.bExecuteWhenPaused = false;
	Press.KeyDelegate.GetDelegateWithKeyForManualSet().BindLambda([this](const FKey& Key)
	{
		PauseGame();
	});

	PlayerInputComponent->KeyBindings.Add(Press);
	LoadGame();

	MainLoopTask = UE::Tasks::Launch(UE_SOURCE_LOCATION, [&]()
	{
		while(!IsMainLoopCancelled)
		{
			auto Start = std::chrono::high_resolution_clock::now();

			if(State != nullptr && State->IsPlaying) CheckPassedTimeline(Jukebox->GetPositionMicro());

			auto End = std::chrono::high_resolution_clock::now();
			auto Elapsed = std::chrono::duration_cast<std::chrono::microseconds>(End - Start).count();

			// If the loop body took less than 125 microseconds, sleep for the remaining time
			if(Elapsed < 125) {
				std::this_thread::sleep_for(std::chrono::microseconds(125 - Elapsed));
			} else
			{
				// warn that game loop is taking too long
				UE_LOG(LogTemp, Warning, TEXT("Game loop took %lld microseconds"), Elapsed);
			}
		}
	});

	


	// Bind the binding to the input component
	// InputComponent->KeyBindings.Add(Binding);

	
}

void ARhythmControl::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	UE_LOG(LogTemp, Warning, TEXT("Rhythm EndPlay"));
	IsLoadCancelled = true;
	IsLoaded = false;
	IsMainLoopCancelled = true;
	// ReSharper disable once CppExpressionWithoutSideEffects
	LoadTask.Wait();
	if(InputHandler != nullptr)
	{
		InputHandler->StopListen();
		delete InputHandler;
	}
	// ReSharper disable once CppExpressionWithoutSideEffects
	MainLoopTask.Wait();
	if(Jukebox != nullptr)
	{
		Jukebox->Unload();
		delete Jukebox;
		Jukebox = nullptr;
	}
	if(State != nullptr)
	{
		delete State;
		State = nullptr;
	}
	if(Chart != nullptr)
	{
		delete Chart;
		Chart = nullptr;
	}

}

void ARhythmControl::LoadGame()
{
	UE_LOG(LogTemp, Warning, TEXT("Loading Game"));
	
	Options = GameInstance->GetStartOptions();
	Renderer->InitMeta(Options.ChartMeta);
	// init IsLanePressed
	for (const auto& Lane : Options.ChartMeta.GetTotalLaneIndices())
	{
		UE_LOG(LogTemp, Warning, TEXT("Lane: %d"), Lane);
		IsLanePressed.Add(Lane, false);
	}
	if(!Options.AutoPlay)
	{
		InputHandler = new FRhythmInputHandler(this, Options.ChartMeta);
		bool result1 = InputHandler->StartListenNative() || InputHandler->StartListenUnreal(PlayerInputComponent); // fallback to unreal input if native input failed
		bool result2 = InputHandler->StartListenUnrealTouch(PlayerController, Renderer->NoteArea, 15);
		if(!result1 && !result2)
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to start listen"));
			return;
		}
	}
	LoadTask = UE::Tasks::Launch(UE_SOURCE_LOCATION, [&]()
	{
		UE_LOG(LogTemp, Warning, TEXT("Loading Chart&Jukebox"));
		FBMSParser Parser;
		Parser.Parse(Options.ChartMeta.BmsPath, &Chart, false, false, IsLoadCancelled);
		if (IsLoadCancelled) return;
		Jukebox->LoadChart(Chart, IsLoadCancelled, MediaPlayer);
		if(Chart->Measures.IsEmpty())
		{
			// We don't need to check for empty timeline since all measures have at least one timeline
			UE_LOG(LogTemp, Warning, TEXT("Chart is empty"));
			return;
		}
		if (IsLoadCancelled) return;

		State = new FRhythmState(Chart, false);
		// init renderer in game thread task and wait for it
		if (IsLoadCancelled) return;
		
		Renderer->Init(Chart);

		UE_LOG(LogTemp, Warning, TEXT("Chart&Jukebox loaded"));
		if (!IsLoadCancelled)
		{
			IsLoaded = true;
			if(!IsGamePaused) StartGame();
		}
		
	});
}


// Called every frame
void ARhythmControl::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Renderer->Draw(Jukebox->GetPositionMicro());
	if(!IsLoaded) return;
	if(State == nullptr) return;
	Jukebox->OnGameTick();
}

