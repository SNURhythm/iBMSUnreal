// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Chart.h"
#include "GameFramework/Actor.h"
#include "BMSRenderer.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class IBMSUNREAL_API UBMSRenderer : public UActorComponent
{
	GENERATED_BODY()
private:
	FChart* Chart;
	
public:	
	// Sets default values for this actor's properties
	UBMSRenderer();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// A judgeline actor in level for 0-position reference
	UPROPERTY(EditAnywhere, Category="BMS Renderer")
	AActor* Judgeline;

	// NoteArea, where notes are rendered within its local space
	UPROPERTY(EditAnywhere, Category="BMS Renderer")
	AActor* NoteArea;

	// Paper2D Sprite for rendering notes
	UPROPERTY(EditAnywhere, Category="BMS Renderer")
	class UPaperSprite* NoteSprite;

	

public:	
	void Draw(unsigned long long CurrentTime);
	void Init(FChart* chart);
};
