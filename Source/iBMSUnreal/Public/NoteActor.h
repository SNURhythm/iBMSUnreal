// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PaperSpriteActor.h"
#include "PaperSpriteComponent.h"
#include "GameFramework/Actor.h"
#include "NoteActor.generated.h"

UCLASS()
class IBMSUNREAL_API ANoteActor : public APaperSpriteActor
{
	GENERATED_BODY()
private:
	UPaperSprite* Sprite;
public:	
	// Sets default values for this actor's properties
	ANoteActor();
	void SetSprite(UPaperSprite* sprite);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
