// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ChartSelectScreen.generated.h"

UCLASS()
class IBMSUNREAL_API AChartSelectScreen : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AChartSelectScreen();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	// EndPlay
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
