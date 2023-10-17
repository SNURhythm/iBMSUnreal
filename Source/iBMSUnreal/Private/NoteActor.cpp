// Fill out your copyright notice in the Description page of Project Settings.


#include "NoteActor.h"

// Sets default values
ANoteActor::ANoteActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	// init root component
	// RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RenderComponent"));
	// RootComponent->SetMobility(EComponentMobility::Movable);
}

void ANoteActor::SetSprite(UPaperSprite* sprite)
{
	GetRenderComponent()->SetMobility(EComponentMobility::Movable);
	GetRenderComponent()->SetSprite(sprite);
	GetRenderComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
}

// Called when the game starts or when spawned
void ANoteActor::BeginPlay()
{
	Super::BeginPlay();
	
	
	
}

// Called every frame
void ANoteActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

