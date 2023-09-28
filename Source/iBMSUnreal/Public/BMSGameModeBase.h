// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Chart.h"
#include "GameFramework/GameModeBase.h"
#include "fmod.hpp"
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
	std::atomic_bool bCancelled;
	FMOD::System* FMODSystem;
	UPROPERTY()
	TObjectPtr<UChartMeta> CurrentChartMeta;

	// On Search Box Text Changed
	UFUNCTION()
	void OnSearchBoxTextChanged(const FText& Text);

	// On Search Box Text Committed
	UFUNCTION()
	void OnSearchBoxTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);
	
};