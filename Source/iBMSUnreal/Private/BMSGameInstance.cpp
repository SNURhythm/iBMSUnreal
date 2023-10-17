// Fill out your copyright notice in the Description page of Project Settings.


#include "BMSGameInstance.h"

void UBMSGameInstance::Init()
{
	Super::Init();
	InitFMOD();
}

void UBMSGameInstance::BeginDestroy()
{
	Super::BeginDestroy();
	UE_LOG(LogTemp, Warning, TEXT("Releasing fmod system"));
	fmodSystem->release();
	delete fmodSystem;
}

void UBMSGameInstance::InitFMOD()
{
	auto result = FMOD::System_Create(&fmodSystem);
	if (result != FMOD_OK)
	{
		UE_LOG(LogTemp, Warning, TEXT("FMOD error! (%d)\n"), result);
		return;
	}
	fmodSystem->setSoftwareChannels(4092);
	fmodSystem->setSoftwareFormat(48000, FMOD_SPEAKERMODE_DEFAULT, 0);
	fmodSystem->setDSPBufferSize(256, 4);
	fmodSystem->init(4092, FMOD_INIT_NORMAL, 0);
}

FMOD::System* UBMSGameInstance::GetFMODSystem()
{
	return fmodSystem;
}

void UBMSGameInstance::SetStartOptions(StartOptions options)
{
	startOptions = options;
}

StartOptions UBMSGameInstance::GetStartOptions()
{
	return startOptions;
}
