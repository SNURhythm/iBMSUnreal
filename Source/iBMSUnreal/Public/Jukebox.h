// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Chart.h"
#include "fmod.hpp"
#include "MediaPlayer.h"
#include "MediaPlaylist.h"


/**
 * 
 */
class IBMSUNREAL_API FJukebox
{
private:
	TMap<int, FMOD::Sound*> SoundTable;
	FMOD::System* System;
	int MaxBgRealChannels;
	FMOD::ChannelGroup* ChannelGroup;
	TMap<FMOD::Sound*, FMOD::Channel*> AudioChannelMap;
	long long CurrentBGAStart = -1;
	FMOD_RESULT ReadWav(const FString& Path, FMOD::Sound** Sound, std::atomic_bool& bCancelled);
	unsigned long long MsToDSPClocks(double Ms);
	void PlaySound(FMOD::Sound* sound, FMOD::ChannelGroup* group, bool paused, FMOD::Channel** channel);
	TQueue<TPair<unsigned long long, FMOD::Sound*>> SoundQueue;
	void ScheduleSound(unsigned long long startDspClock, FMOD::Sound* Sound);
	const FChart* Chart;
	FCriticalSection SoundQueueLock;
	unsigned long long StartDspClock;
	UPROPERTY();
	FCriticalSection BGALock;
	
	TMap<int, long long> BGAStartMap;
	UMediaPlayer* MediaPlayer = nullptr;
	bool IsCorrectingBGADrift = false;
public:
	TMap<int, UMediaSource*> BGASourceMap;
	FJukebox(FMOD::System* System);
	~FJukebox();
	UFUNCTION()
	void OnMediaOpened(FString OpenedUrl);
	void LoadChart(const FChart* chart, std::atomic_bool& bCancelled, UMediaPlayer* OptionalPlayer = nullptr);
	void Start(long long PosMicro = 0, bool autoKeysound = false);
	void Unpause();

	void Pause();
	void Stop();

	void PlayKeysound(int id);
	void Unload();

	long long GetPositionMicro() const;
};
