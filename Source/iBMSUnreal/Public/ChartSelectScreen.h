// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Chart.h"
#include "ChartListEntryData.h"
#include "FJukebox.h"
#include "fmod.hpp"
#include "Tasks/Task.h"
#include "GameFramework/Actor.h"
#include "ChartSelectScreen.generated.h"

UCLASS()
class IBMSUNREAL_API AChartSelectScreen : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AChartSelectScreen();
private:
	FJukebox* jukebox;
	FCriticalSection jukeboxLock;
	std::atomic_bool bCancelled;
	std::atomic_bool bJukeboxCancelled;
	FMOD::System* FMODSystem;
	void LoadCharts();
	void SetChartMetas(const TArray<FChartMeta*>& ChartMetas);

	// On Search Box Text Changed
	UFUNCTION()
	void OnSearchBoxTextChanged(const FText& Text);

	// On Search Box Text Committed
	UFUNCTION()
	void OnSearchBoxTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);
	
	UPROPERTY()
	UChartListEntryData* CurrentEntryData;
protected:
	UPROPERTY(EditAnywhere, Category="Class Types")
	TSubclassOf<UUserWidget> WidgetClass;

	UPROPERTY(VisibleInstanceOnly, Category="Runtime")
	class UChartSelectUI* ChartSelectUI;

	UFUNCTION()
	void OnStartButtonClicked();
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	// EndPlay
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
