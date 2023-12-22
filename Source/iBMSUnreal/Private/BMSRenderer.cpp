// Fill out your copyright notice in the Description page of Project Settings.


#include "BMSRenderer.h"
#include "NoteActor.h"
#include "MeasureActor.h"
#include "PaperSpriteActor.h"
#include "PaperSpriteComponent.h"
#include "Kismet/GameplayStatics.h"

void FRendererState::Dispose()
{
	for(auto& item: NoteActors)
	{
		item.Value->Destroy();
	}

	for(auto& item: MeasureActors)
	{
		item.Value->Destroy();
	}
	for(auto& item: ObjectPool)
	{
		while(!item.Value->IsEmpty())
		{
			APaperSpriteActor* Instance;
			item.Value->Dequeue(Instance);
			if(Instance->IsHidden())
			{
				Instance->Destroy();
			}
		}
		delete item.Value;
	}
}

// Sets default values
UBMSRenderer::UBMSRenderer()
{
	State = nullptr;
}

// Called when the game starts or when spawned
void UBMSRenderer::BeginPlay()
{
	Super::BeginPlay();
	SetComponentTickEnabled(false);
	// spawn note sprite for test
	// auto note = GetWorld()->SpawnActor<APaperSpriteActor>();
	// note->GetRenderComponent()->Mobility = EComponentMobility::Movable;
	// note->GetRenderComponent()->SetSprite(NoteSprite);
	// // translucent unlit material
	// note->GetRenderComponent()->SetMaterial(0, LoadObject<UMaterialInterface>(NULL, TEXT("/Game/Materials/TranslucentUnlitSpriteMaterial")));
	// // attach to note area
	// note->AttachToActor(NoteArea, FAttachmentTransformRules::KeepRelativeTransform);
	// // set position
	// note->SetActorRelativeLocation(FVector(0, 0, 0));
	// // set scale
	// note->SetActorScale3D(FVector(1, 1, 1));
	const FVector NoteAreaSize = NoteArea->GetComponentsBoundingBox().GetSize();
	NoteAreaHeight = NoteAreaSize.Y;
	NoteAreaWidth = NoteAreaSize.X;
	// relative to NoteArea
	JudgeLineZ = 0;
	
	
}


void UBMSRenderer::DestroyNote(const FBMSNote* Note)
{
	if(!State->NoteActors.Contains(Note)) return;
	RecycleInstance(EBMSObjectType::Note, State->NoteActors[Note]);
	State->NoteActors.Remove(Note);
}

void UBMSRenderer::RecycleInstance(const EBMSObjectType Type, APaperSpriteActor* Instance) const
{
	Instance->SetActorHiddenInGame(true);
	if(!State->ObjectPool.Contains(Type))
	{
		State->ObjectPool.Add(Type, new TQueue<APaperSpriteActor*>());
	}
	State->ObjectPool[Type]->Enqueue(Instance);
}

APaperSpriteActor* UBMSRenderer::GetInstance(const EBMSObjectType Type) const
{
	APaperSpriteActor* Instance;
	if(State->ObjectPool.Contains(Type) && !State->ObjectPool[Type]->IsEmpty())
	{
		State->ObjectPool[Type]->Dequeue(Instance);
	} else {
		// Spawn inside NoteArea
		TSubclassOf<APaperSpriteActor> TypeClass;
		switch(Type)
		{
			case EBMSObjectType::Note:
				TypeClass = ANoteActor::StaticClass();
				break;
			case EBMSObjectType::LongNoteHead:
				TypeClass = ANoteActor::StaticClass();
				break;
			case EBMSObjectType::LongNoteTail:
				TypeClass = ANoteActor::StaticClass();
				break;
			case EBMSObjectType::MeasureLine:
				TypeClass = AMeasureActor::StaticClass();
				break;
		}
		Instance = GetWorld()->SpawnActor<APaperSpriteActor>(TypeClass);
		Instance->GetRenderComponent()->SetMaterial(0, LoadObject<UMaterialInterface>(nullptr, TEXT("/Paper2D/TranslucentUnlitSpriteMaterial")));
		Instance->AttachToActor(NoteArea, FAttachmentTransformRules::KeepRelativeTransform);
		switch(Type)
		{
		case EBMSObjectType::Note:
		case EBMSObjectType::LongNoteHead:
		case EBMSObjectType::LongNoteTail:
			{
				Instance->GetRenderComponent()->SetSprite(NoteSprite);
				Instance->GetRenderComponent()->TranslucencySortPriority = 2;
				break;
			}
		case EBMSObjectType::MeasureLine:
			{
				Instance->GetRenderComponent()->SetSprite(NoteSprite);
				Instance->GetRootComponent()->SetWorldScale3D(FVector(1, 0, 0.2));
				// gray
				Instance->GetRenderComponent()->SetSpriteColor(FLinearColor(0.02, 0.02, 0.02, 1));
				Instance->GetRenderComponent()->TranslucencySortPriority = 1;
				break;
			}
		}
	}
	Instance->SetActorHiddenInGame(false);
	if(Type == Note || Type == LongNoteHead || Type == LongNoteTail)
	{
		Instance->GetRenderComponent()->SetSpriteColor(FLinearColor(1, 1, 1, 1));
		Instance->GetRootComponent()->SetWorldScale3D(FVector(1, 0, 1));
		FVector Scale = Instance->GetActorRelativeScale3D();
		Scale.X = NoteWidth;
		Instance->SetActorRelativeScale3D(Scale);
		Instance->SetActorRelativeRotation(FRotator(0, 0, 0));
	}
	if(Type==MeasureLine)
	{

	}
	return Instance;
}

void UBMSRenderer::DestroyMeasureLine(const FMeasure* Measure) const
{
	if(!State->MeasureActors.Contains(Measure)) return;
	RecycleInstance(EBMSObjectType::MeasureLine, State->MeasureActors[Measure]);
	State->MeasureActors.Remove(Measure);
}

void UBMSRenderer::DrawMeasureLine(FMeasure* Measure, const double Offset)
{
	const double Top = OffsetToTop(Offset);
	if(State->MeasureActors.Contains(Measure))
	{
		APaperSpriteActor* MeasureActor = State->MeasureActors[Measure];
		MeasureActor->SetActorRelativeLocation(FVector(0, 0, Top));
	} else
	{
		APaperSpriteActor* MeasureActor = GetInstance(EBMSObjectType::MeasureLine);
		FVector Scale = MeasureActor->GetActorRelativeScale3D();
		Scale.X = 1;
		MeasureActor->SetActorRelativeScale3D(Scale);
		MeasureActor->SetActorRelativeLocation(FVector(0, 0, Top));
		State->MeasureActors.Add(Measure, MeasureActor);
	}
}

void UBMSRenderer::DrawNote(FBMSNote* Note, const double Offset)
{
	if(Note->IsPlayed) return;
	const double Left = LaneToLeft(Note->Lane);

	APaperSpriteActor* Actor;
	if(State->NoteActors.Contains(Note))
	{
		Actor = State->NoteActors[Note];
		Actor->SetActorRelativeLocation(FVector(Left, 0, OffsetToTop(Offset)));
	} else
	{
		Actor = GetInstance(EBMSObjectType::Note);
		Actor->SetActorRelativeLocation(FVector(Left, 0, OffsetToTop(Offset)));
		Actor->GetRenderComponent()->SetSpriteColor(GetColorByLane(Note->Lane));
		State->NoteActors.Add(Note, Actor);
	}

	if(IsScratchLane(Note->Lane))
	{
		// tilt 90 degrees on Z
		Actor->SetActorRelativeRotation(FRotator(0, 270, 0));
	}

	
}
FLinearColor UBMSRenderer::GetColorByLane(int Lane) const
{
	if(IsScratchLane(Lane)) return FLinearColor(1, 0, 0, 1);
	// even: white, odd: blue
	if(Lane % 2 == 0) return FLinearColor(1, 1, 1, 1);
	return FLinearColor(0, 0, 1, 1);
}
void UBMSRenderer::DrawLongNote(FBMSLongNote* Head, const double StartOffset, const double EndOffset, const bool TailOnly)
{
	FBMSLongNote* Tail = Head->Tail;
	const double Left = LaneToLeft(Head->Lane);
	double StartTop = OffsetToTop(StartOffset);
	const double EndTop = FMath::Min(1, OffsetToTop(EndOffset));
	if(Head->IsPlayed)
	{
		if(EndTop < JudgeLineZ)
		{
			DestroyNote(Tail);
			return;
		}
		StartTop = FMath::Max(JudgeLineZ, StartTop);
	}
	const double Height = EndTop - StartTop;
	const FLinearColor Color = GetColorByLane(Head->Lane);

	if(!TailOnly)
	{
		APaperSpriteActor* HeadActor;
		if(State->NoteActors.Contains(Head))
		{
			HeadActor = State->NoteActors[Head];
			HeadActor->SetActorRelativeLocation(FVector(Left, 0, StartTop));
		} else
		{
			HeadActor = GetInstance(EBMSObjectType::LongNoteHead);
			HeadActor->SetActorRelativeLocation(FVector(Left, 0, StartTop));
			HeadActor->GetRenderComponent()->SetSpriteColor(Color);
			if(IsScratchLane(Head->Lane))
			{
				// tilt 90 degrees on Z
				HeadActor->SetActorRelativeRotation(FRotator(0, 270, 0));
			}
			State->NoteActors.Add(Head, HeadActor);
		}
	}
	float Alpha = 0.01f;
	if(Head->IsHolding)
	{
		Alpha = 1.0f;
	} else
	{
		Alpha = Head->IsPlayed? 0.2f : 0.3f;
	}

	FLinearColor TailColor = FLinearColor(Color.R, Color.G, Color.B, Alpha);
	APaperSpriteActor* TailActor;
	if(State->NoteActors.Contains(Tail))
	{
		TailActor = State->NoteActors[Tail];
		TailActor->SetActorRelativeLocation(FVector(Left, 0, StartTop));
		// scale sprite
		FVector Scale = TailActor->GetActorRelativeScale3D();
		Scale.Z = Height;
		TailActor->SetActorRelativeScale3D(Scale);
		TailActor->GetRenderComponent()->SetSpriteColor(TailColor);
	} else
	{
		TailActor = GetInstance(EBMSObjectType::LongNoteTail);
		FVector Scale = TailActor->GetActorRelativeScale3D();
		Scale.Z = Height;
		TailActor->SetActorRelativeScale3D(Scale);
		TailActor->SetActorRelativeLocation(FVector(Left, 0, StartTop));
		TailActor->GetRenderComponent()->SetSpriteColor(TailColor);
		State->NoteActors.Add(Tail, TailActor);
	}
	if(IsScratchLane(Tail->Lane))
	{
		// tilt 90 degrees on Z
		TailActor->SetActorRelativeRotation(FRotator(0, 270, 0));
	}
}

double UBMSRenderer::LaneToLeft(int Lane) const
{
	if(IsLeftScratchLane(Lane)) return -1;
	if(IsRightScratchLane(Lane)) return 0; // Right Scratch
	if(Lane >= 8) Lane -= KeyLaneCount == 14 ? 1 : 3; // DP or 2P
	return (static_cast<double>(Lane+1)/KeyLaneCount) - 1;
}

bool UBMSRenderer::IsScratchLane(int Lane) const
{
	return IsLeftScratchLane(Lane) || IsRightScratchLane(Lane);
}

bool UBMSRenderer::IsLeftScratchLane(const int Lane)
{
	return Lane == 7;
}

bool UBMSRenderer::IsRightScratchLane(const int Lane)
{
	return Lane == 15;
}

double UBMSRenderer::OffsetToTop(const double Offset) const
{
	// TODO: implement
	return JudgeLineZ + Offset * 0.0000014;
}

bool UBMSRenderer::IsOverUpperBound(const double Offset) const
{
	return OffsetToTop(Offset) > 1;
}

bool UBMSRenderer::IsUnderLowerBound(const double Offset) const
{
	return OffsetToTop(Offset) < -0.1;
}

void UBMSRenderer::Draw(const long long CurrentTime)
{
	if(!State) return;

	for(int i = State->PassedMeasureCount; i < Chart->Measures.Num(); i++)
	{
		const bool IsFirstMeasure = i == State->PassedMeasureCount;
		const auto& Measure = Chart->Measures[i];
		if(Measure->Timing > CurrentTime) break;

		for(int j = IsFirstMeasure ? State->PassedTimelineCount : 0; j < Measure->TimeLines.Num(); j++)
		{
			const auto& TimeLine = Measure->TimeLines[j];
			if (TimeLine->Timing > CurrentTime) break;
			LastTimeLine = TimeLine;
		}
	}

	// draw notes
	const double CurrentPos = (CurrentTime < LastTimeLine->Timing + LastTimeLine->GetStopDuration()) ? LastTimeLine->Pos :
		                          LastTimeLine->Pos + (CurrentTime - (LastTimeLine->Timing + LastTimeLine->GetStopDuration())) * LastTimeLine->Bpm / Chart->Meta->Bpm;

	for (int i = State->PassedMeasureCount; i < Chart->Measures.Num(); i++)
	{
		const bool IsFirstMeasure = i == State->PassedMeasureCount;
		const auto& Measure = Chart->Measures[i];

		for (int j = IsFirstMeasure ? State->PassedTimelineCount : 0; j < Measure->TimeLines.Num(); j++)
		{
			const auto& TimeLine = Measure->TimeLines[j];
			const double Offset = TimeLine->Pos - CurrentPos;
			if(IsOverUpperBound(Offset)) break;

			if(j==0) DrawMeasureLine(Measure, Measure->Pos - CurrentPos);

			const bool ShouldDestroyTimeLine = IsUnderLowerBound(Offset);

			if(ShouldDestroyTimeLine && IsFirstMeasure)
			{
				State->PassedTimelineCount++;
			}

			for(const auto& Note : TimeLine->Notes)
			{
				if(!Note) continue;
				if(Note->IsDead) continue;
				if(ShouldDestroyTimeLine || Note->IsPlayed)
				{
					bool DontDestroy = false;
					if(Note->IsLongNote())
					{
						FBMSLongNote* LongNote = static_cast<FBMSLongNote*>(Note);
						if(LongNote->IsTail())
						{
							if(ShouldDestroyTimeLine)
							{
								State->OrphanLongNotes.Remove(LongNote->Head);
							} else
							{
								DontDestroy = true;
							}
						} else
						{
							State->OrphanLongNotes.Add(LongNote);
						}
					}

					if(!DontDestroy)
					{
						Note->IsDead = true;
						DestroyNote(Note);
					}
					continue;
				}
				if(Note->IsLongNote())
				{
					FBMSLongNote* LongNote = static_cast<FBMSLongNote*>(Note);
					if(!LongNote->IsTail()) DrawLongNote(LongNote, Offset, LongNote->Tail->Timeline->Pos - CurrentPos);
				} else
				{
					DrawNote(Note, Offset);
				}
			}
		}
		if(State->PassedTimelineCount == Measure->TimeLines.Num() && IsFirstMeasure)
		{
			State->PassedTimelineCount = 0;
			State->PassedMeasureCount++;
			DestroyMeasureLine(Measure);
		}
	}

	for(const auto& OrphanLongNote : State->OrphanLongNotes)
	{
		DrawLongNote(OrphanLongNote, OrphanLongNote->Timeline->Pos - CurrentPos, OrphanLongNote->Tail->Timeline->Pos - CurrentPos, true);
	}
	
}

void UBMSRenderer::Init(FChart* ChartInit)
{
	this->Chart = ChartInit;
	State = new FRendererState();
	KeyLaneCount = ChartInit->Meta->GetKeyLaneCount(); // main line count except for scratch
	NoteWidth = 1.0f / KeyLaneCount;

	LastTimeLine = ChartInit->Measures[0]->TimeLines[0];

	FTimeLine* _lastTimeLine = ChartInit->Measures[0]->TimeLines[0];
	_lastTimeLine->Pos = 0.0;
	for(const auto& Measure: ChartInit->Measures)
	{
		Measure->Pos = _lastTimeLine->Pos + (Measure->Timing - (_lastTimeLine->Timing + _lastTimeLine->GetStopDuration())) * _lastTimeLine->Bpm / ChartInit->Meta->Bpm;
		for(const auto& Timeline: Measure->TimeLines)
		{
			Timeline->Pos = _lastTimeLine->Pos + (Timeline->Timing - (_lastTimeLine->Timing + _lastTimeLine->GetStopDuration())) * _lastTimeLine->Bpm / ChartInit->Meta->Bpm;
			_lastTimeLine = Timeline;
		}
	}
}

void UBMSRenderer::Reset()
{
	State->Dispose();
	delete State;
	State = new FRendererState();
}


