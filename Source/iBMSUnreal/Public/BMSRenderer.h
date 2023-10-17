// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BMSNote.h"
#include "BMSLongNote.h"
#include "TimeLine.h"
#include "Chart.h"
#include "PaperSpriteActor.h"
#include "GameFramework/Actor.h"
#include "BMSRenderer.generated.h"


enum EBMSObjectType
{
	Note,
	LongNoteHead,
	LongNoteTail,
	MeasureLine
};

class FRendererState
{
public:
	TMap<FBMSNote*, APaperSpriteActor*> NoteActors;
	TMap<FMeasure*, APaperSpriteActor*> MeasureActors;
	// object pool per type
	TMap<EBMSObjectType, TQueue<APaperSpriteActor*>*> ObjectPool;
	int PassedTimelineCount = 0;
	int PassedMeasureCount = 0;
	TArray<FBMSLongNote*> OrphanLongNotes;
	void Dispose();
	
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class IBMSUNREAL_API UBMSRenderer : public UActorComponent
{
	GENERATED_BODY()
private:
	FChart* Chart;
	FRendererState* State;
	
public:	
	// Sets default values for this actor's properties
	UBMSRenderer();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// NoteArea, where notes are rendered within its local space
	UPROPERTY(EditAnywhere, Category="BMS Renderer")
	AActor* NoteArea;

	// Paper2D Sprite for rendering notes
	UPROPERTY(EditAnywhere, Category="BMS Renderer")
	class UPaperSprite* NoteSprite;
	FTimeLine* LastTimeLine;
	float NoteAreaHeight;
	float NoteAreaWidth;
	float JudgeLineZ;
	int LaneCount;
	float NoteWidth;
	

public:
	void DestroyNote(FBMSNote* Note);
	void RecycleInstance(EBMSObjectType Type, APaperSpriteActor* Instance);

	APaperSpriteActor* GetInstance(EBMSObjectType Type);
	void DestroyMeasureLine(FMeasure* Measure);
	void DrawMeasureLine(FMeasure* Measure, double Offset);
	void DrawLongNote(FBMSLongNote* Head, double StartOffset, double EndOffset, bool TailOnly = false);
	double LaneToLeft(int Lane);
	bool IsScratchLane(int Lane);
	double OffsetToTop(double Offset);
	bool IsOverUpperBound(double Offset);
	bool IsUnderLowerBound(double Offset);
	void DrawNote(FBMSNote* Note, double Offset);
	void Draw(long long CurrentTime);
	void Init(FChart* chart);
	void Reset();
};
