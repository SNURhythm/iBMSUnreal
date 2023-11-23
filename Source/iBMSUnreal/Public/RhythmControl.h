// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BMSGameInstance.h"
#include "BMSRenderer.h"
#include "Jukebox.h"
#include "Judge.h"
#include "GameFramework/Actor.h"
#include "Tasks/Task.h"
#include "IRhythmControl.h"

#include "Input/RhythmInputHandler.h"
#include "RhythmControl.generated.h"


class FRhythmState
{
public:
	FJudge* Judge;
	bool IsPlaying = false;

	int PassedMeasureCount = 0;
	int PassedTimelineCount = 0;

	int Combo = 0;
	EJudgement LatestJudgement = None;
	// judge count. default 0
	TMap<EJudgement, int> JudgeCount;
	explicit FRhythmState(const FChart* Chart, bool AddReadyMeasure)
	{
		Judge = new FJudge(Chart->Meta->Rank);
		for(int i = 0; i < EJudgementCount; i++)
		{
			JudgeCount.Add(static_cast<EJudgement>(i), 0);
		}
	}

private:
	long long FirstTiming = 0;
	
};

UCLASS()
class IBMSUNREAL_API ARhythmControl : public AActor, public IRhythmControl
{
	GENERATED_BODY()
private:
	void LoadGame();
	UPROPERTY()
	UBMSGameInstance* GameInstance;
	StartOptions Options;
	UE::Tasks::TTask<void> LoadTask;
	UE::Tasks::TTask<void> MainLoopTask;
	TObjectPtr<UInputComponent> PlayerInputComponent;
	APlayerController* PlayerController = nullptr;
	void OnJudge(const FJudgeResult& JudgeResult) const;
	void CheckPassedTimeline(long long Time);
	FRhythmState* State = nullptr;
	TMap<int, bool> IsLanePressed;
	FRhythmInputHandler* InputHandler = nullptr;

	UPROPERTY(VisibleInstanceOnly, Category="Runtime")
	class URhythmHUD* CurrentHUD = nullptr;
public:	
	// Sets default values for this actor's properties
	ARhythmControl();
	std::atomic_bool IsLoaded;
	std::atomic_bool IsLoadCancelled;
	std::atomic_bool IsMainLoopCancelled;
	
	FJukebox* Jukebox;
	void PressNote(FBMSNote* Note, long long PressedTime);
	void ReleaseNote(FBMSNote* Note, long long ReleasedTime);
	virtual void PressLane(int Lane, double InputDelay = 0) override;
	virtual void ReleaseLane(int Lane, double InputDelay = 0) override;
	virtual UWorld* GetContextWorld() override;
	

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY()
	UBMSRenderer* Renderer;
	UPROPERTY(EditAnywhere, Category="Class Types")
	TSubclassOf<UUserWidget> RhythmHUDClass;
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	FChart* Chart = nullptr;

};
