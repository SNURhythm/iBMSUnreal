// Fill out your copyright notice in the Description page of Project Settings.


#include "Jukebox.h"

#include "BMSParser.h"
#include <Tasks/Task.h>

#include "FileMediaSource.h"
#include "transcode.h"
#include "iBMSUnreal/Public/Utils.h"
#include "ChartDBHelper.h"

FMOD_RESULT FJukebox::ReadWav(const FString& Path, FMOD::Sound** Sound, std::atomic_bool& bCancelled)
{
	TArray<uint8> bytes;
	// ignore extension, and try .mp3, .ogg, .wav, .flac
	FString withoutExt = FPaths::Combine(FPaths::GetPath(Path), FPaths::GetBaseFilename(Path));
	bool found = false;
	for(auto ext : {".mp3", ".ogg", ".wav", ".flac"})
	{
		if(bCancelled) return FMOD_ERR_FILE_NOTFOUND;
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
	return System->createSound(reinterpret_cast<const char*>(bytes.GetData()),  FMOD_LOOP_OFF | FMOD_DEFAULT | FMOD_2D | FMOD_3D_WORLDRELATIVE | FMOD_3D_INVERSEROLLOFF | FMOD_OPENMEMORY | FMOD_ACCURATETIME | FMOD_MPEGSEARCH | FMOD_IGNORETAGS | FMOD_LOWMEM | FMOD_OPENMEMORY | FMOD_CREATESAMPLE | FMOD_ACCURATETIME, &exinfo, Sound);
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
	PlaySound(Sound, ChannelGroup, true, &channel);
	channel->setDelay(startDspClock, 0, true);
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
void FJukebox::OnGameTick()
{
	bool IsPaused;
	if (IsValid(MediaPlayer))
	{
		if (CurrentBGAStart >= 0 && MediaPlayer->IsPlaying())
		{
			// check drift
			auto estimatedTime = GetPositionMicro() - CurrentBGAStart;
			auto actualTime = MediaPlayer->GetTime().GetTotalMicroseconds();
			auto driftMicro = actualTime - estimatedTime;
			auto absDriftMicro = FMath::Abs(driftMicro);

			if (absDriftMicro > 40 * 1000 || (IsCorrectingBGADrift && absDriftMicro > 5 * 1000))
			{
				UE_LOG(LogTemp, Warning, TEXT("BGA drift: %f"), driftMicro);
				// if slower, set playback rate faster
				// if faster, set playback rate slower
				// adjustment is a linear function of drift which makes player to catch up estimated time quickly

				auto rateAdjustment = absDriftMicro / 1000.0 / 1000.0 * 5.0;
				UE_LOG(LogTemp, Warning, TEXT("Rate adjustment: %f"), rateAdjustment);
				auto rate = driftMicro > 0 ? 1.0 - rateAdjustment : 1.0 + rateAdjustment;
				UE_LOG(LogTemp, Warning, TEXT("Rate: %f"), rate);
				TArray<FFloatRange> SupportedRates;
				MediaPlayer->GetSupportedRates(SupportedRates, true);
				bool IsGood = false;
				for (auto& range : SupportedRates)
				{
					if (range.Contains(rate))
					{
						IsGood = true;
						break;
					}
				}
				if (!IsGood)
				{
					// find closest rate
					// if rate > 1.0, find closest rate > 1.0
					// if rate < 1.0, find closest rate < 1.0

					// iterate through all lower/upper bounds

					auto closest = SupportedRates[0].GetLowerBoundValue();
					for (auto& range : SupportedRates)
					{
						auto bound = rate > 1.0 ? range.GetUpperBoundValue() : range.GetLowerBoundValue();
						if (bound == 0.0f) continue;
						if (FMath::Abs(bound - rate) < FMath::Abs(closest - rate))
						{
							closest = bound;
						}
					}
					rate = closest;
				}
				UE_LOG(LogTemp, Warning, TEXT("Closest Available Rate: %f"), rate);
				MediaPlayer->SetRate(rate);
				IsCorrectingBGADrift = true;
			}
			else
			{
				if (IsCorrectingBGADrift)
				{
					IsCorrectingBGADrift = false;
					MediaPlayer->SetRate(1.0f);
				}
			}
		}
		ChannelGroup->getPaused(&IsPaused);
		if (IsPaused) return;
		BGALock.Lock();
		TPair<int, long long> pair;

		bool Valid = BGAStartQueue.Peek(pair);
		if (!Valid)
			goto Skip;
		// UE_LOG(LogTemp, Warning, TEXT("BGA %d at %lld vs current time %lld"), pair.Key, pair.Value, GetPositionMicro());
		if (GetPositionMicro() >= pair.Value)
		{
			if (!BGASourceMap.Contains(pair.Key))
			{
				UE_LOG(LogTemp, Warning, TEXT("BGA %d is not loaded"), pair.Key);
				BGAStartQueue.Dequeue(pair);
				goto Skip;
			}
			const auto Source = BGASourceMap[pair.Key];
			if (!Source)
			{
				UE_LOG(LogTemp, Warning, TEXT("BGA %d is null"), pair.Key);
				BGAStartQueue.Dequeue(pair);
				goto Skip;
			}
			UE_LOG(LogTemp, Warning, TEXT("Start BGA %d"), pair.Key);
			MediaPlayerLock.Lock();
			ChannelGroup->getPaused(&IsPaused);
			if (IsPaused)
			{
				MediaPlayerLock.Unlock();
				return;
			}
			if (IsValid(MediaPlayer) && IsValid(Source))
			{
				MediaPlayer->OpenSource(Source);
			}
			MediaPlayerLock.Unlock();
			ChannelGroup->getPaused(&IsPaused);
			if (IsPaused)
			{
				BGALock.Unlock();
				return;
			}
			CurrentBGAStart = pair.Value;

			BGAStartQueue.Dequeue(pair);
		}

		Skip:
			BGALock.Unlock();
	}
	
}
void FJukebox::LoadChart(const FChart* chart, std::atomic_bool& bCancelled, UMediaPlayer* OptionalPlayer)
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
			FMOD_RESULT result = ReadWav(FPaths::Combine(ChartFolder, wav), &sound, bCancelled);
			if(bCancelled) return;
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
	if(OptionalPlayer == nullptr) return;
	
	MediaPlayer = OptionalPlayer;
	MediaPlayer->PlayOnOpen = true;

	for(auto& bmp: Chart->BmpTable)
	{
		// find first mpg/mp4/avi/...
		FString Name = bmp.Value;
		FString Path = FPaths::Combine(ChartFolder, Name);
		FString withoutExt = FPaths::Combine(FPaths::GetPath(Path), FPaths::GetBaseFilename(Path));
		for(FString ext : {".mp4", ".avi", ".mpg", ".mpeg", ".wmv", ".mov", ".flv", ".mkv", ".webm"})
		{
			// transcode into temp file
			FString NewPath = withoutExt + ext;
			if(!FPaths::FileExists(NewPath)) continue;
			FString Original = NewPath;
			FString TempPath = Original;
			if(ext != ".mp4")
			{
				FString OriginalRel = ChartDBHelper::ToRelativePath(Original);
				FString Hash = FMD5::HashBytes((uint8*)TCHAR_TO_UTF8(*OriginalRel), OriginalRel.Len());
				FString Directory = FUtils::GetDocumentsPath("Temp");
				IFileManager::Get().MakeDirectory(*Directory, true);
				TempPath = FPaths::Combine(Directory, Hash + ".mp4");
				TempPath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForWrite(*TempPath);
				if(!FPaths::FileExists(TempPath))
				{
					UE_LOG(LogTemp, Warning, TEXT("Transcoding %s to %s"), *Original, *TempPath);
					int result = transcode(TCHAR_TO_UTF8(*Original), TCHAR_TO_UTF8(*TempPath), &bCancelled);
					if(result<0||bCancelled)
					{
						// remove temp file
						IFileManager::Get().Delete(*TempPath);
						break;
					}
					UE_LOG(LogTemp, Warning, TEXT("Transcoding done: %d"), result);
				}
			}
			const FString FileName = FPaths::GetBaseFilename(TempPath);
			const FName ObjectName = MakeUniqueObjectName(GetTransientPackage(), UFileMediaSource::StaticClass(), FName(*FileName));

			auto MediaSource = NewObject<UFileMediaSource>(GetTransientPackage(), ObjectName, RF_Transactional | RF_Transient);
			MediaSource->SetFilePath(TempPath);
			BGASourceMap.Add(bmp.Key, MediaSource);
			UE_LOG(LogTemp, Warning, TEXT("Added BGA %d"), bmp.Key);
			break;
		}
	}	
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
			if(timeline->BgaBase != -1)
			{
				UE_LOG(LogTemp, Warning, TEXT("BGA %d at %lld"), timeline->BgaBase, timeline->Timing);
				BGAStartQueue.Enqueue({timeline->BgaBase, timeline->Timing});
			}
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
	System->getSoftwareFormat(&SampleRate, nullptr, nullptr);
	if(IsValid(MediaPlayer))
	{
		MediaPlayer->Play();
	}
	// start Thread to pop SoundQueue
	IsCorrectingBGADrift = false;
	
	MainLoopTask = UE::Tasks::Launch(UE_SOURCE_LOCATION, [this]()
	{
		while (true)
		{
			// UE_LOG(LogTemp, Warning, TEXT("Main loop, current position: %lld"), GetPositionMicro());
			bool isPaused;
			ChannelGroup->getPaused(&isPaused);
			if (isPaused) break;
			
			int channels;
			int realChannels;
			System->getChannelsPlaying(&channels, &realChannels);
			int availableChannels = MaxBgRealChannels - realChannels;
			for (int i = 0; i < availableChannels; i++)
			{
				ChannelGroup->getPaused(&isPaused);
				if (isPaused)
				{
					break;
				}
				TPair<unsigned long long, FMOD::Sound*> pair;
				SoundQueueLock.Lock();
				if (!SoundQueue.Dequeue(pair))
				{
					SoundQueueLock.Unlock();
					break;
				}
				SoundQueueLock.Unlock();

				FMOD::Channel* channel;
				PlaySound(pair.Value, ChannelGroup, true, &channel);
				channel->setDelay(pair.Key, 0, true);

				channel->setPaused(false);
			}
			System->update();
			// sleep for 1ms
			FPlatformProcess::Sleep(0.001);
		}
	});
}

void FJukebox::PlaySound(FMOD::Sound* sound, FMOD::ChannelGroup* group, bool paused, FMOD::Channel** channel)
{
	// THIS IS BUGGY RIGHT NOW
	// if(const auto PreviousChannel = AudioChannelMap.FindRef(sound))
	// {
	// 	UE_LOG(LogTemp, Warning, TEXT("Sound %p already playing, stop it"), sound);
	// 	PreviousChannel->stop();
	// }
	System->playSound(sound, group, paused, channel);
	AudioChannelMapLock.Lock();
	AudioChannelMap.Add(sound, *channel);
	AudioChannelMapLock.Unlock();
}

void FJukebox::Pause()
{
	ChannelGroup->setPaused(true);
	if(IsValid(MediaPlayer))
	{
		MediaPlayer->Pause();
	}
}

bool FJukebox::IsPaused()
{
	bool isPaused;
	ChannelGroup->getPaused(&isPaused);
	return isPaused;
}

void FJukebox::Stop()
{
	ChannelGroup->setPaused(true);
	ChannelGroup->stop();
	MainLoopTask.Wait();

	SoundQueueLock.Lock();
	SoundQueue.Empty();
	SoundQueueLock.Unlock();
}

void FJukebox::PlayKeysound(int id)
{
	FMOD::Sound* sound = SoundTable.FindRef(id);
	if(!sound) return;
	UE::Tasks::Launch(UE_SOURCE_LOCATION, [this, sound](){
		FMOD::Channel* channel;
		if(!ChannelGroup) return;
		PlaySound(sound, ChannelGroup, false, &channel);
	});
}

void FJukebox::Unload()
{
	Stop();
	for(auto& pair: SoundTable)
	{
		pair.Value->release();
	}
	SoundTable.Empty();
	

	MediaPlayerLock.Lock();
	MediaPlayer = nullptr;
	MediaPlayerLock.Unlock();

	BGALock.Lock();
	BGAStartQueue.Empty();
	BGASourceMap.Empty();
	BGALock.Unlock();

}


long long FJukebox::GetPositionMicro() const
{
	if(!ChannelGroup) return 0;
	if(SampleRate == -1) return 0;
	unsigned long long dspClock;
	ChannelGroup->getDSPClock(&dspClock, nullptr);
	
	auto result = (static_cast<double>(dspClock) / SampleRate * 1000000 - static_cast<double>(StartDspClock) / SampleRate * 1000000);
	return result;
}
