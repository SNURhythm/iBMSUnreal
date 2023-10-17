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


void UBMSRenderer::DestroyNote(FBMSNote* Note)
{
	if(!State->NoteActors.Contains(Note)) return;
	RecycleInstance(EBMSObjectType::Note, State->NoteActors[Note]);
	State->NoteActors.Remove(Note);
}

void UBMSRenderer::RecycleInstance(EBMSObjectType Type, APaperSpriteActor* Instance)
{
	Instance->SetActorHiddenInGame(true);
	if(!State->ObjectPool.Contains(Type))
	{
		State->ObjectPool.Add(Type, new TQueue<APaperSpriteActor*>());
	}
	State->ObjectPool[Type]->Enqueue(Instance);
}

APaperSpriteActor* UBMSRenderer::GetInstance(EBMSObjectType Type)
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
	}
	Instance->SetActorHiddenInGame(false);
	if(Type == Note || Type == LongNoteHead || Type == LongNoteTail)
	{
		static_cast<ANoteActor*>(Instance)->SetSprite(NoteSprite);
		Instance->AttachToActor(NoteArea, FAttachmentTransformRules::KeepRelativeTransform);
		Instance->GetRootComponent()->SetWorldScale3D(FVector(NoteWidth, 0.1, 1));

	}
	return Instance;
}

void UBMSRenderer::DestroyMeasureLine(FMeasure* Measure)
{
	if(!State->MeasureActors.Contains(Measure)) return;
	RecycleInstance(EBMSObjectType::MeasureLine, State->MeasureActors[Measure]);
	State->MeasureActors.Remove(Measure);
}

void UBMSRenderer::DrawMeasureLine(FMeasure* Measure, double Offset)
{
	float top = OffsetToTop(Offset);
	if(State->MeasureActors.Contains(Measure))
	{
		APaperSpriteActor* MeasureActor = State->MeasureActors[Measure];
		MeasureActor->SetActorRelativeLocation(FVector(0, 0, top));
	} else
	{
		APaperSpriteActor* MeasureActor = GetInstance(EBMSObjectType::MeasureLine);
		MeasureActor->SetActorScale3D(FVector(NoteAreaWidth, 0.1, 0.1));
		MeasureActor->SetActorRelativeLocation(FVector(0, 0, top));
		State->MeasureActors.Add(Measure, MeasureActor);
	}
}

void UBMSRenderer::DrawNote(FBMSNote* Note, double Offset)
{
	if(Note->IsPlayed) return;
	float Left = LaneToLeft(Note->Lane);

	if(State->NoteActors.Contains(Note))
	{
		APaperSpriteActor* Actor = State->NoteActors[Note];
		Actor->SetActorRelativeLocation(FVector(Left, 0, OffsetToTop(Offset)));
	} else
	{
		APaperSpriteActor* Actor = GetInstance(EBMSObjectType::Note);
		Actor->SetActorRelativeLocation(FVector(Left, 0, OffsetToTop(Offset)));
		State->NoteActors.Add(Note, Actor);
	}
}

void UBMSRenderer::DrawLongNote(FBMSLongNote* Head, double StartOffset, double EndOffset, bool TailOnly)
{
	FBMSLongNote* Tail = Head->Tail;
	float Left = LaneToLeft(Head->Lane);
	float StartTop = OffsetToTop(StartOffset);
	float EndTop = OffsetToTop(EndOffset);
	if(Head->IsPlayed)
	{
		if(EndTop < JudgeLineZ)
		{
			DestroyNote(Tail);
			return;
		}
		StartTop = FMath::Max(JudgeLineZ, StartTop);
	}
	float Height = EndTop - StartTop;

	if(!TailOnly)
	{
		if(State->NoteActors.Contains(Head))
		{
			APaperSpriteActor* Actor = State->NoteActors[Head];
			Actor->SetActorRelativeLocation(FVector(Left, 0, StartTop));
		} else
		{
			APaperSpriteActor* Actor = GetInstance(EBMSObjectType::LongNoteHead);
			Actor->SetActorRelativeLocation(FVector(Left, 0, StartTop));
			State->NoteActors.Add(Head, Actor);
		}
	}
	float Alpha = 0.01f;
	if(Head->IsHolding)
	{
		Alpha = 1.0f;
	} else
	{
		Alpha = Head->IsPlayed? 0.2f : 0.5f;
	}

	if(State->NoteActors.Contains(Tail))
	{
		APaperSpriteActor* Actor = State->NoteActors[Tail];
		Actor->SetActorRelativeLocation(FVector(Left, 0, StartTop));
		// scale sprite
		FVector Scale = Actor->GetActorRelativeScale3D();
		Scale.Z = Height;
		Actor->SetActorRelativeScale3D(Scale);
		Actor->GetRenderComponent()->SetSpriteColor(FLinearColor(1, 1, 1, Alpha));
	} else
	{
		APaperSpriteActor* Actor = GetInstance(EBMSObjectType::LongNoteTail);
		FVector Scale = Actor->GetActorRelativeScale3D();
		Scale.Z = Height;
		Actor->SetActorRelativeScale3D(Scale);
		Actor->SetActorRelativeLocation(FVector(Left, 0, StartTop));
		State->NoteActors.Add(Tail, Actor);
	}
}

float UBMSRenderer::LaneToLeft(int Lane)
{
	return (static_cast<float>(Lane)/LaneCount) - 1;
}

float UBMSRenderer::OffsetToTop(double Offset)
{
	// TODO: implement
	return static_cast<float>(JudgeLineZ + Offset * 0.0000007);
}

bool UBMSRenderer::IsOverUpperBound(double Offset)
{
	return OffsetToTop(Offset) > 1;
}

bool UBMSRenderer::IsUnderLowerBound(double Offset)
{
	return OffsetToTop(Offset) < -1;
}

void UBMSRenderer::Draw(long long CurrentTime)
{
	if(!State) return;

	for(int i = State->PassedMeasureCount; i < Chart->Measures.Num(); i++)
	{
		bool IsFirstMeasure = i == State->PassedMeasureCount;
		auto& Measure = Chart->Measures[i];
		if(Measure->Timing > CurrentTime) break;

		for(int j = IsFirstMeasure ? State->PassedTimelineCount : 0; j < Measure->TimeLines.Num(); j++)
		{
			auto& TimeLine = Measure->TimeLines[j];
			if (TimeLine->Timing > CurrentTime) break;
			LastTimeLine = TimeLine;
		}
	}

	// draw notes
	double CurrentPos = (CurrentTime < LastTimeLine->Timing + LastTimeLine->GetStopDuration()) ? LastTimeLine->Pos :
						LastTimeLine->Pos + (CurrentTime - (LastTimeLine->Timing + LastTimeLine->GetStopDuration())) * LastTimeLine->Bpm / Chart->Meta->Bpm;

	for (int i = State->PassedMeasureCount; i < Chart->Measures.Num(); i++)
	{
		bool IsFirstMeasure = i == State->PassedMeasureCount;
		auto& Measure = Chart->Measures[i];

		for (int j = IsFirstMeasure ? State->PassedTimelineCount : 0; j < Measure->TimeLines.Num(); j++)
		{
			auto& TimeLine = Measure->TimeLines[j];
			double Offset = TimeLine->Pos - CurrentPos;
			if(IsOverUpperBound(Offset)) break;

			if(j==0) DrawMeasureLine(Measure, Measure->Pos - CurrentPos);

			bool ShouldDestroyTimeLine = IsUnderLowerBound(Offset);

			if(ShouldDestroyTimeLine && IsFirstMeasure)
			{
				State->PassedTimelineCount++;
			}

			for(auto& Note : TimeLine->Notes)
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
							UE_LOG(LogTemp, Warning, TEXT("Orphan Long Note: %d"), LongNote->Lane);
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

	for(auto& OrphanLongNote : State->OrphanLongNotes)
	{
		DrawLongNote(OrphanLongNote, OrphanLongNote->Timeline->Pos - CurrentPos, OrphanLongNote->Tail->Timeline->Pos - CurrentPos, true);
	}
	
}

void UBMSRenderer::Init(FChart* chart)
{
	this->Chart = chart;
	State = new FRendererState();
	LaneCount = chart->Meta->KeyMode; // main line count except for scratch
	NoteWidth = NoteAreaWidth / LaneCount;

	LastTimeLine = chart->Measures[0]->TimeLines[0];

	FTimeLine* _lastTimeLine = chart->Measures[0]->TimeLines[0];
	_lastTimeLine->Pos = 0.0;
	for(auto& measure: chart->Measures)
	{
		measure->Pos = _lastTimeLine->Pos + (measure->Timing - (_lastTimeLine->Timing + _lastTimeLine->GetStopDuration())) * _lastTimeLine->Bpm / chart->Meta->Bpm;
		for(auto& timeline: measure->TimeLines)
		{
			timeline->Pos = _lastTimeLine->Pos + (timeline->Timing - (_lastTimeLine->Timing + _lastTimeLine->GetStopDuration())) * _lastTimeLine->Bpm / chart->Meta->Bpm;
			_lastTimeLine = timeline;
		}
	}
}

void UBMSRenderer::Reset()
{
	State->Dispose();
	delete State;
	State = new FRendererState();
}


// float mouseX;
// float mouseY;
// UGameplayStatics::GetPlayerController(GetWorld(), 0)->GetMousePosition(mouseX, mouseY);
// UE_LOG(LogTemp, Warning, TEXT("Mouse Location: %f, %f"), mouseX, mouseY);
// FVector worldPosition;
// FVector worldDirection;
// UGameplayStatics::GetPlayerController(GetWorld(), 0)->DeprojectScreenPositionToWorld(mouseX, mouseY, worldPosition, worldDirection);
// worldPosition = worldPosition / GNearClippingPlane * 10;
// UE_LOG(LogTemp, Warning, TEXT("World Location: %f, %f, %f"), worldPosition.X, worldPosition.Y, worldPosition.Z);