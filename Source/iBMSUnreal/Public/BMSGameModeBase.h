// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Chart.h"
#include "ChartListEntryData.h"
#include "GameFramework/GameModeBase.h"
#include "fmod.hpp"
#include "Jukebox.h"
#include "Tasks/Task.h"
#include "BMSGameModeBase.generated.h"
/**
 * 
 */
UCLASS()
class IBMSUNREAL_API ABMSGameModeBase : public AGameModeBase
{
	GENERATED_BODY()
public:
	ABMSGameModeBase();

protected:
	UPROPERTY(EditAnywhere, Category="Class Types")
	TSubclassOf<UUserWidget> WidgetClass;

	UPROPERTY(VisibleInstanceOnly, Category="Runtime")
	class UChartSelectUI* ChartSelectUI;
	// InitGame
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	void LoadCharts();
	// Logout
	virtual void Logout(AController* Exiting) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;
private:
	Jukebox* jukebox;
	FCriticalSection jukeboxLock;
	std::atomic_bool bCancelled;
	std::atomic_bool bJukeboxCancelled;
	FMOD::System* FMODSystem;
	UPROPERTY()
	UChartListEntryData* CurrentEntryData;

	void SetChartMetas(const TArray<FChartMeta*>& ChartMetas);
	// On Search Box Text Changed
	UFUNCTION()
	void OnSearchBoxTextChanged(const FText& Text);

	// On Search Box Text Committed
	UFUNCTION()
	void OnSearchBoxTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);
	
};