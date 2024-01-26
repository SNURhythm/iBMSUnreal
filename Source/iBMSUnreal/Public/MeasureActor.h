// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PaperSpriteActor.h"
#include "GameFramework/Actor.h"
#include "MeasureActor.generated.h"

UCLASS()
class IBMSUNREAL_API AMeasureActor : public APaperSpriteActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AMeasureActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
