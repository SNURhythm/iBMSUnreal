// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BMSNote.h"
/**
 * 
 */
class IBMSUNREAL_API BMSLandmineNote: public BMSNote
{
public:
	float damage;
	BMSLandmineNote(float damage);
	~BMSLandmineNote();
};
