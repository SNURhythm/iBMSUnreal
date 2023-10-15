// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "fmod.hpp"
#include "BMSGameInstance.generated.h"


/**
 * 
 */
UCLASS()
class IBMSUNREAL_API UBMSGameInstance : public UGameInstance
{
	GENERATED_BODY()
	FMOD::System *fmodSystem;
	virtual void Init() override;
	
	void InitFMOD();
public:
	FMOD::System* GetFMODSystem();
};
