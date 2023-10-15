// Fill out your copyright notice in the Description page of Project Settings.


#include "BMSGameModeBase.h"


ABMSGameModeBase::ABMSGameModeBase()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ABMSGameModeBase::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
}


void ABMSGameModeBase::BeginPlay()
{
	Super::BeginPlay();
}


void ABMSGameModeBase::Logout(AController* Exiting)
{
	Super::Logout(Exiting);
	UE_LOG(LogTemp, Warning, TEXT("Logout"));

}

void ABMSGameModeBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

}