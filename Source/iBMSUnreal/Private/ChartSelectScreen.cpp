// Fill out your copyright notice in the Description page of Project Settings.


#include "ChartSelectScreen.h"
// Sets default values
AChartSelectScreen::AChartSelectScreen()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AChartSelectScreen::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogTemp, Warning, TEXT("ChartSelectScreen BeginPlay()!!"));

    
	
}

// Called every frame
void AChartSelectScreen::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AChartSelectScreen::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	UE_LOG(LogTemp, Warning, TEXT("ChartSelectScreen EndPlay()!!"));
}

