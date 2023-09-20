// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BMSNote.h"
/**
 * 
 */
class IBMSUNREAL_API FBMSLandmineNote: public FBMSNote
{
public:
	float Damage;
	FBMSLandmineNote(float Damage);
	~FBMSLandmineNote();
};
