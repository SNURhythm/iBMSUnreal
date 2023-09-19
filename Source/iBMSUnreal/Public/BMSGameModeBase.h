// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BMSGameModeBase.generated.h"

/**
 * 
 */
UCLASS()
class IBMSUNREAL_API ABMSGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

protected:
	// InitGame
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	// Logout
	virtual void Logout(AController* Exiting) override;
private:
	std::atomic_bool bCancelled;
	
};
