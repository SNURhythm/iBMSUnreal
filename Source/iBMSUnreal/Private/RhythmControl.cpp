// Fill out your copyright notice in the Description page of Project Settings.


#include "RhythmControl.h"

#include "BMSGameInstance.h"
#include "BMSParser.h"
#include "CanvasItem.h"
#include "RhythmHUD.h"
#include "Engine/Canvas.h"
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
	CurrentHUD->OnJudge(JudgeResult, State->Combo);
	UE_LOG(LogTemp, Warning, TEXT("Judge: %s, Combo: %d, Diff: %lld"), *JudgeResult.ToString(), State->Combo, JudgeResult.Diff);
}

void ARhythmControl::CheckPassedTimeline(const long long Time)
{
	auto Measures = Chart->Measures;
	if(State == nullptr) return;
	for(int i = State->PassedMeasureCount; i < Measures.Num(); i++)
	{
		const bool IsFirstMeasure = i == State->PassedMeasureCount;
		const auto& Measure = Measures[i];
		for(int j = IsFirstMeasure ? State->PassedTimelineCount : 0; j < Measure->TimeLines.Num(); j++)
		{
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
							// TODO: Resume lane beam effect if play mode is Auto
							continue;
						}
					}
					if(Options.AutoPlay) // NormalNote or LongNote's head
					{
						PressNote(Note, Time);
						// TODO: Start lane beam
						// TODO: Resume lane beam immediately if note is long note head
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
}

// Sets default values
ARhythmControl::ARhythmControl()
{

 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

void ARhythmControl::PressNote(FBMSNote* Note, long long PressedTime)
{
	if(Note->Wav != FBMSParser::NoWav && !Options.AutoKeysound) Jukebox->PlayKeysound(Note->Wav);
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
				return;
			}
			Note->Press(PressedTime);
		}
		OnJudge(JudgeResult);
	}
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

void ARhythmControl::PressLane(int Lane, double InputDelay)
{
	if(IsLanePressed[Lane]) return;
	IsLanePressed[Lane] = true;
	if(State == nullptr) return;
	if(!State->IsPlaying) return;

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
			const auto& Note = Timeline->Notes[Lane];
			if(Note == nullptr) continue;
			if(Note->IsPlayed) continue;
			if(Note->IsLandmineNote()) continue;
			PressNote(Note, PressedTime);
			return;
		}
	}
	
}

void ARhythmControl::ReleaseLane(int Lane, double InputDelay)
{
	if(!IsLanePressed[Lane]) return;
	IsLanePressed[Lane] = false;
	if(State == nullptr) return;
	if(!State->IsPlaying) return;

	const auto ReleasedTime = Jukebox->GetPositionMicro() - static_cast<long long>(InputDelay * 1000000);
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
// Called when the game starts or when spawned
void ARhythmControl::BeginPlay()
{
	Super::BeginPlay();
	// force garbage collection
	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	UE_LOG(LogTemp, Warning, TEXT("Rhythm BeginPlay"));
	// get BMSRenderer actor component
	GameInstance = Cast<UBMSGameInstance>(GetGameInstance());
	Renderer = Cast<UBMSRenderer>(GetComponentByClass(UBMSRenderer::StaticClass()));
	Jukebox = new FJukebox(GameInstance->GetFMODSystem());
	if(IsValid(RhythmHUDClass))
	{
		CurrentHUD = CreateWidget<URhythmHUD>(GetWorld(), RhythmHUDClass);
		CurrentHUD->AddToViewport();
	}
	LoadGame();

	MainLoopTask = UE::Tasks::Launch(UE_SOURCE_LOCATION, [&]()
	{
		while(!IsMainLoopCancelled)
		{
			if(State == nullptr) continue;
			if(!State->IsPlaying) continue;
			CheckPassedTimeline(Jukebox->GetPositionMicro());
		}
	});
}

void ARhythmControl::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	UE_LOG(LogTemp, Warning, TEXT("Rhythm EndPlay"));
	RhythmInput->StopListen();
	IsLoadCancelled = true;
	IsLoaded = false;
	IsMainLoopCancelled = true;
	// ReSharper disable once CppExpressionWithoutSideEffects
	LoadTask.Wait();
	// ReSharper disable once CppExpressionWithoutSideEffects
	MainLoopTask.Wait();
	
	Jukebox->Unload();
	delete Jukebox;
	Jukebox = nullptr;
	delete State;
	State = nullptr;
	if(Chart)
	{
		delete Chart;
		Chart = nullptr;
	}

}

void ARhythmControl::LoadGame()
{
	UE_LOG(LogTemp, Warning, TEXT("Loading Game"));
	
	Options = GameInstance->GetStartOptions();
	
	LoadTask = UE::Tasks::Launch(UE_SOURCE_LOCATION, [&]()
	{
		UE_LOG(LogTemp, Warning, TEXT("Loading Chart&Jukebox"));
		FBMSParser Parser;
		Parser.Parse(Options.BmsPath, &Chart, false, false, IsLoadCancelled);
		if (IsLoadCancelled) return;
		Jukebox->LoadChart(Chart, IsLoadCancelled);
		// init IsLanePressed
		for (const auto& Lane : Chart->Meta->GetTotalLaneIndices())
		{
			IsLanePressed.Add(Lane, false);
		}
		State = new FRhythmState(Chart, false);
		Renderer->Init(Chart);
		RhythmInput = new FRhythmInput(this, *Chart->Meta);
		RhythmInput->StartListen();
		UE_LOG(LogTemp, Warning, TEXT("Chart&Jukebox loaded"));
		if (!IsLoadCancelled)
		{
			IsLoaded = true;
			Jukebox->Start(Options.StartPosition, Options.AutoKeysound);
		}
		State->IsPlaying = true;
	});
}


// Called every frame
void ARhythmControl::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if(!IsLoaded) return;
	if(State == nullptr) return;
	Renderer->Draw(Jukebox->GetPositionMicro());
	// draw rectangle on Screen
	
	
	
}

