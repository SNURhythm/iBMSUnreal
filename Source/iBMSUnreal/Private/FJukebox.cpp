// Fill out your copyright notice in the Description page of Project Settings.


#include "FJukebox.h"

#include "BMSParser.h"
#include <Tasks/Task.h>

FMOD_RESULT FJukebox::ReadWav(const FString& Path, FMOD::Sound** Sound)
{
	TArray<uint8> bytes;
	// ignore extension, and try .mp3, .ogg, .wav, .flac
	FString withoutExt = FPaths::Combine(FPaths::GetPath(Path), FPaths::GetBaseFilename(Path));
	bool found = false;
	for(auto ext : {".mp3", ".ogg", ".wav", ".flac"})
	{
		FString newPath = withoutExt + ext;
		if(FPaths::FileExists(newPath))
		{
			FFileHelper::LoadFileToArray(bytes, *newPath);
			found = true;
			break;
		}
	}
	if(!found)
	{
		UE_LOG(LogTemp, Error, TEXT("File not found: %s"), *Path);
		return FMOD_ERR_FILE_NOTFOUND;
	}
	FMOD_CREATESOUNDEXINFO exinfo = {};
	exinfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
	exinfo.length = bytes.Num();
	return System->createSound(reinterpret_cast<const char*>(bytes.GetData()), FMOD_OPENMEMORY | FMOD_CREATESAMPLE | FMOD_ACCURATETIME, &exinfo, Sound);
}

unsigned long long FJukebox::MsToDSPClocks(double Ms)
{
	int samplerate;
	
	System->getSoftwareFormat(&samplerate, nullptr, nullptr);
	return Ms * static_cast<unsigned long long>(samplerate) / 1000;
}

void FJukebox::ScheduleSound(unsigned long long startDspClock, FMOD::Sound* Sound)
{
	int channels;
	int realChannels;
	System->getChannelsPlaying(&channels, &realChannels);
	if(realChannels >= MaxBgRealChannels)
	{
		SoundQueueLock.Lock();
		// enqueue
		SoundQueue.Enqueue(TPair<unsigned long long, FMOD::Sound*>(startDspClock, Sound));
		SoundQueueLock.Unlock();
		return;
	}
	FMOD::Channel* channel;
	System->playSound(Sound, ChannelGroup, true, &channel);
	channel->setDelay(startDspClock, 0, false);
	channel->setPaused(false);
}

FJukebox::FJukebox(FMOD::System* System)
{
	this->System = System;
	int MaxRealChannels;
	System->getSoftwareChannels(&MaxRealChannels);
	MaxBgRealChannels = MaxRealChannels / 4 * 3;
	System->createChannelGroup("Jukebox", &ChannelGroup);
	ChannelGroup->setPaused(true);
	Chart = nullptr;
	StartDspClock = 0;
}

FJukebox::~FJukebox()
{
	Unload();
}

void FJukebox::LoadChart(const FChart* chart, std::atomic_bool& bCancelled)
{
	Unload();
	Chart = chart;
	const bool bSupportMultiThreading = FPlatformProcess::SupportsMultithreading();
	const int TaskNum = FMath::Max(FPlatformMisc::NumberOfWorkerThreadsToSpawn()/2,1);
	const int32 WavNum = 36*36;
	const int TaskSize = WavNum / TaskNum;
	FString ChartFolder = Chart->Meta->Folder;
	FCriticalSection SoundTableLock;
	ParallelFor(TaskNum, [&](int32 i){
		if(bCancelled) return;
		int32 start = i * TaskSize;
		if(start >= WavNum) return;
		int32 end = (i+1) * TaskSize;
		if(i == TaskNum - 1)
		{
			end = WavNum;
		}
		if(end > WavNum)
		{
			end = WavNum;
		}
		for(int32 j=start; j<end; j++)
		{
			if(bCancelled) return;
			if(!Chart->WavTable.Contains(j)) continue;
			auto& wav = Chart->WavTable[j];
			
			FMOD::Sound* sound;
			FMOD_RESULT result = ReadWav(FPaths::Combine(ChartFolder, wav), &sound);
			sound->setLoopCount(0);
			if(result != FMOD_OK)
			{
				UE_LOG(LogTemp, Error, TEXT("Failed to load wav: %s, error code: %d"), *wav, result);
				continue;
			}

			SoundTableLock.Lock();
			SoundTable.Add(j, sound);
			SoundTableLock.Unlock();
		}
	}, !bSupportMultiThreading);

}

void FJukebox::Start(long long PosMicro, bool autoKeysound)
{
	Stop();
	if(!Chart)
	{
		UE_LOG(LogTemp, Error, TEXT("Chart is not loaded"));
		return;
	}
	// schedule all sounds
	ChannelGroup->setPaused(true);


	ChannelGroup->getDSPClock(&StartDspClock, nullptr);
	
	for(auto& measure: Chart->Measures)
	{
		for(auto& timeline: measure->TimeLines)
		{
			auto timing = timeline->Timing - PosMicro;
			if(timing < 0) continue;
			auto dspClock = StartDspClock + MsToDSPClocks(static_cast<double>(timing)/1000.0);
			for(auto& note: timeline->BackgroundNotes)
			{
				if(!note) continue;
				auto id = note->Wav;
				if(id == FBMSParser::NoWav) continue;
				auto sound = SoundTable.FindRef(id);
				if(!sound) continue;
				ScheduleSound(dspClock, sound);
			}
			if(autoKeysound)
			{
				for(auto& note: timeline->Notes)
				{
					if(!note) continue;
					auto id = note->Wav;
					if(id == FBMSParser::NoWav) continue;
					auto sound = SoundTable.FindRef(id);
					if(!sound) continue;
					ScheduleSound(dspClock, sound);
				}
			}
		}
	}
	Unpause();
}

void FJukebox::Unpause()
{
	ChannelGroup->setPaused(false);
	// start Thread to pop SoundQueue
	UE::Tasks::FTask task = UE::Tasks::Launch(UE_SOURCE_LOCATION, [this](){
		while(true)
		{
			bool isPaused;
			ChannelGroup->getPaused(&isPaused);
			if(isPaused) break;
			int channels;
			int realChannels;
			System->getChannelsPlaying(&channels, &realChannels);
			int availableChannels = MaxBgRealChannels - realChannels;
			for(int i=0;i<availableChannels; i++)
			{
				
				ChannelGroup->getPaused(&isPaused);
				if(isPaused) {
					break;
				}
				TPair<unsigned long long, FMOD::Sound*> pair;
				SoundQueueLock.Lock();
				if(!SoundQueue.Dequeue(pair))
				{
					SoundQueueLock.Unlock();
					break;
				}
				
				FMOD::Channel* channel;
				System->playSound(pair.Value, ChannelGroup, true, &channel);
				channel->setDelay(pair.Key, 0, false);
				SoundQueueLock.Unlock();
				channel->setPaused(false);
				
			}
		}
	});
}

void FJukebox::Pause()
{
	ChannelGroup->setPaused(true);
}

void FJukebox::Stop()
{
	ChannelGroup->setPaused(true);
	ChannelGroup->stop();

	SoundQueueLock.Lock();
	SoundQueue.Empty();
	SoundQueueLock.Unlock();
}

void FJukebox::PlayKeysound(int id)
{
	FMOD::Sound* sound = SoundTable.FindRef(id);
	if(!sound) return;
	FMOD::Channel* channel;
	System->playSound(sound, ChannelGroup, false, &channel);
}

void FJukebox::Unload()
{
	Stop();
	for(auto& pair: SoundTable)
	{
		pair.Value->release();
	}
	SoundTable.Empty();
}

unsigned long long FJukebox::GetPosition()
{
	if(!ChannelGroup) return 0;
	unsigned long long dspClock;
	int samplerate;
	ChannelGroup->getDSPClock(&dspClock, nullptr);
	System->getSoftwareFormat(&samplerate, nullptr, nullptr);
	return (static_cast<double>(dspClock) /samplerate * 1000000 - static_cast<double>(StartDspClock) / samplerate * 1000000);
}
