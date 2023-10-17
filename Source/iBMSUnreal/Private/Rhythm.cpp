// Fill out your copyright notice in the Description page of Project Settings.


#include "Rhythm.h"

#include "BMSGameInstance.h"
#include "BMSParser.h"
#include "Tasks/Task.h"


// Sets default values
ARhythm::ARhythm()
{

 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ARhythm::BeginPlay()
{
	Super::BeginPlay();
	// force garbage collection
	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	UE_LOG(LogTemp, Warning, TEXT("Rhythm BeginPlay"));
	// get BMSRenderer actor component
	GameInstance = Cast<UBMSGameInstance>(GetGameInstance());
	Renderer = Cast<UBMSRenderer>(GetComponentByClass(UBMSRenderer::StaticClass()));
	Jukebox = new FJukebox(GameInstance->GetFMODSystem());
	LoadGame();
}

void ARhythm::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	UE_LOG(LogTemp, Warning, TEXT("Rhythm EndPlay"));
	IsLoadCancelled = true;
	IsLoaded = false;
	Jukebox->Unload();
	delete Jukebox;
	if(Chart)
	{
		delete Chart;
	}

}

void ARhythm::LoadGame()
{
	UE_LOG(LogTemp, Warning, TEXT("Loading Game"));
	auto Options = GameInstance->GetStartOptions();
	
	UE::Tasks::FTask LoadTask = UE::Tasks::Launch(UE_SOURCE_LOCATION, [&, Options]()
	{
		UE_LOG(LogTemp, Warning, TEXT("Loading Chart&Jukebox"));
		FBMSParser Parser;
		Parser.Parse(Options.BmsPath, &Chart, false, false, IsLoadCancelled);
		if(IsLoadCancelled) return;
		Jukebox->LoadChart(Chart, IsLoadCancelled);
		Renderer->Init(Chart);
		UE_LOG(LogTemp, Warning, TEXT("Chart&Jukebox loaded"));
		if(!IsLoadCancelled)
		{
			IsLoaded = true;
			Jukebox->Start(Options.StartPosition, true);
		}
	});
}


// Called every frame
void ARhythm::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if(!IsLoaded) return;
	Renderer->Draw(Jukebox->GetPosition());
	GameInstance->GetFMODSystem()->update();

}

