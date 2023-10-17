// Fill out your copyright notice in the Description page of Project Settings.


#include "MeasureActor.h"

#include "PaperSpriteComponent.h"

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
	GetRenderComponent()->SetMobility(EComponentMobility::Movable);
	GetRenderComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetRenderComponent()->SetComponentTickEnabled(false);
	SetActorEnableCollision(false);
}

// Called every frame
void AMeasureActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

