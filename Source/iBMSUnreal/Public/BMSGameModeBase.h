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
	// InitGame
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	// Logout
	virtual void Logout(AController* Exiting) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;
};
