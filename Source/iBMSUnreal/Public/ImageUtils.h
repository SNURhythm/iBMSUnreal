// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
namespace ImageUtils
{
	void LoadTexture2D(const FString ImageFileName, const TArray<uint8>& ImageBytes, bool& IsValid, int32 TargetWidth,
	                   int32 TargetHeight, UTexture2D*& OutTexture, bool UpdateResource = true);
}
