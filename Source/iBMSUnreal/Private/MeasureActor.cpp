// Fill out your copyright notice in the Description page of Project Settings.


#include "MeasureActor.h"

// Sets default values
AMeasureActor::AMeasureActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

// Called when the game starts or when spawned
void AMeasureActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AMeasureActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

