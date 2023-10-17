// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BMSRenderer.h"
#include "FJukebox.h"
#include "GameFramework/Actor.h"
#include "Rhythm.generated.h"



UCLASS()
class IBMSUNREAL_API ARhythm : public AActor
{
	GENERATED_BODY()
private:
	void LoadGame();
public:	
	// Sets default values for this actor's properties
	ARhythm();
	std::atomic_bool IsLoaded;
	std::atomic_bool IsLoadCancelled;
	FJukebox* Jukebox;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY()
	UBMSRenderer* Renderer;
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	FChart* Chart;

};
