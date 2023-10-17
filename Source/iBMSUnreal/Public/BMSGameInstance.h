// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "fmod.hpp"
#include "BMSGameInstance.generated.h"


/**
 * 
 */

struct StartOptions
{
	FString BmsPath;
	unsigned long long StartPosition = 0;
	bool AutoKeysound;
};
UCLASS()
class IBMSUNREAL_API UBMSGameInstance : public UGameInstance
{
	GENERATED_BODY()
	FMOD::System *fmodSystem;
	virtual void Init() override;
	virtual void BeginDestroy() override;
	void InitFMOD();
	StartOptions startOptions;
public:
	inline FMOD::System* GetFMODSystem();
	void SetStartOptions(StartOptions options);
	StartOptions GetStartOptions();
};