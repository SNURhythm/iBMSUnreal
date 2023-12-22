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
	UPROPERTY(EditAnywhere, Category="BMS Renderer")
	AActor* NoteArea;
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// NoteArea, where notes are rendered within its local space


	// Paper2D Sprite for rendering notes
	UPROPERTY(EditAnywhere, Category="BMS Renderer")
	class UPaperSprite* NoteSprite;
	FTimeLine* LastTimeLine;
	float NoteAreaHeight;
	float NoteAreaWidth;
	float JudgeLineZ;
	int KeyLaneCount;
	float NoteWidth;
	

public:
	void DestroyNote(const FBMSNote* Note);
	void RecycleInstance(EBMSObjectType Type, APaperSpriteActor* Instance) const;

	APaperSpriteActor* GetInstance(EBMSObjectType Type) const;
	void DestroyMeasureLine(const FMeasure* Measure) const;
	void DrawMeasureLine(FMeasure* Measure, double Offset);
	FLinearColor GetColorByLane(int Lane) const;
	void DrawLongNote(FBMSLongNote* Head, double StartOffset, double EndOffset, bool TailOnly = false);
	double LaneToLeft(int Lane) const;
	bool IsScratchLane(int Lane) const;
	static bool IsLeftScratchLane(int Lane);
	static bool IsRightScratchLane(int Lane);
	double OffsetToTop(double Offset) const;
	bool IsOverUpperBound(double Offset) const;
	bool IsUnderLowerBound(double Offset) const;
	void DrawNote(FBMSNote* Note, double Offset);
	void Draw(long long CurrentTime);
	void Init(FChart* ChartInit);
	void Reset();
};
