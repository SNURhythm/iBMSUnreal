// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Chart.h"
#include "fmod.hpp"

/**
 * 
 */
class IBMSUNREAL_API Jukebox
{
private:
	TMap<int, FMOD::Sound*> SoundTable;
	FMOD::System* System;
	int MaxBgRealChannels;
	FMOD::ChannelGroup* ChannelGroup;
	FMOD_RESULT ReadWav(const FString& Path, FMOD::Sound** Sound);
	unsigned long long MsToDSPClocks(double Ms);
	TQueue<TPair<unsigned long long, FMOD::Sound*>> SoundQueue;
	void ScheduleSound(unsigned long long startDspClock, FMOD::Sound* Sound);
	const FChart* Chart;
	FCriticalSection SoundQueueLock;

public:
	Jukebox(FMOD::System* System);
	~Jukebox();		
	void LoadChart(const FChart* chart, std::atomic_bool& bCancelled);
	void Start(long PosMicro = 0, bool autoKeysound = false);
	void Unpause();
	void Pause();
	void Stop();

	void PlayKeysound(int id);
	void Unload();
};
