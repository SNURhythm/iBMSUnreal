// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
enum KeySource
{
	VirtualKey,
	ScanCode,
	UnrealKey
};
namespace InputNormalizer
{
	FKey Normalize(int KeyCode, KeySource Source, int CharCode = 0);
}