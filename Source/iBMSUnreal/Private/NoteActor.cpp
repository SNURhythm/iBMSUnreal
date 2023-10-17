// Fill out your copyright notice in the Description page of Project Settings.


#include "NoteActor.h"

// Sets default values
ANoteActor::ANoteActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	// init root component
	// RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RenderComponent"));
	// RootComponent->SetMobility(EComponentMobility::Movable);
}

// Called when the game starts or when spawned
void ANoteActor::BeginPlay()
{
	Super::BeginPlay();
	GetRenderComponent()->SetMobility(EComponentMobility::Movable);
	GetRenderComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	
	
}

// Called every frame
void ANoteActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

