// Fill out your copyright notice in the Description page of Project Settings.


#include "Jukebox.h"

#include "BMSParser.h"
#include <Tasks/Task.h>

#include "FileMediaSource.h"
#include "transcode.h"
#include "iBMSUnreal/Public/Utils.h"
#include "ChartDBHelper.h"

FMOD_RESULT FJukebox::ReadWav(const FString& Path, FMOD::Sound** Sound, const std::atomic_bool& Cancelled) const
{
	TArray<uint8> Bytes;
	// ignore extension, and try .mp3, .ogg, .wav, .flac
	const FString WithoutExt = FPaths::Combine(FPaths::GetPath(Path), FPaths::GetBaseFilename(Path));
	bool Found = false;
	for(const auto ExtraDot : {"", "."}) // some kusofumens have filename..wav (ura_63..wav in http://www.comeup.info/bofoon2007/automation.zip)
	{
		for(const auto Ext : {".flac", ".wav", ".ogg", ".mp3" })
		{
			if(Cancelled) return FMOD_ERR_FILE_NOTFOUND;
		
			FString NewPath = WithoutExt + ExtraDot + Ext;
			if(FPaths::FileExists(NewPath))
			{
				FFileHelper::LoadFileToArray(Bytes, *NewPath);
				Found = true;
				break;
			}
		
		}
	}
	if(!Found)
	{
		UE_LOG(LogTemp, Error, TEXT("File not found: %s"), *Path);
		return FMOD_ERR_FILE_NOTFOUND;
	}
	FMOD_CREATESOUNDEXINFO exinfo = {};
	exinfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
	exinfo.length = Bytes.Num();
	if(Cancelled) return FMOD_ERR_FILE_NOTFOUND;
	return System->createSound(reinterpret_cast<const char*>(Bytes.GetData()),  FMOD_LOOP_OFF | FMOD_DEFAULT | FMOD_2D | FMOD_3D_WORLDRELATIVE | FMOD_3D_INVERSEROLLOFF | FMOD_OPENMEMORY | FMOD_ACCURATETIME | FMOD_MPEGSEARCH | FMOD_IGNORETAGS | FMOD_LOWMEM | FMOD_OPENMEMORY | FMOD_CREATESAMPLE | FMOD_ACCURATETIME, &exinfo, Sound);
}

unsigned long long FJukebox::MsToDspClocks(const double Ms) const
{	
	return Ms * static_cast<unsigned long long>(SampleRate) / 1000;
}

void FJukebox::ScheduleSound(unsigned long long DspClock, FMOD::Sound* Sound)
{
	int Channels;
	int RealChannels;
	System->getChannelsPlaying(&Channels, &RealChannels);
	if(RealChannels >= MaxBgRealChannels)
	{
		SoundQueueLock.Lock();
		// enqueue
		SoundQueue.Enqueue(TPair<unsigned long long, FMOD::Sound*>(DspClock, Sound));
		SoundQueueLock.Unlock();
		return;
	}
	FMOD::Channel* channel;
	PlaySound(Sound, ChannelGroup, true, &channel);
	channel->setDelay(DspClock, 0, true);
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
	ChannelGroup->release();
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
			const auto EstimatedTime = GetPositionMicro() - CurrentBGAStart;
			const auto ActualTime = MediaPlayer->GetTime().GetTotalMicroseconds();
			const auto DriftMicro = ActualTime - EstimatedTime;
			const auto AbsDriftMicro = FMath::Abs(DriftMicro);

			if (AbsDriftMicro > 40 * 1000 || (IsCorrectingBGADrift && AbsDriftMicro > 5 * 1000))
			{
				// UE_LOG(LogTemp, Warning, TEXT("BGA drift: %f"), DriftMicro);
				// if slower, set playback rate faster
				// if faster, set playback rate slower
				// adjustment is a linear function of drift which makes player to catch up estimated time quickly

				const auto RateAdjustment = AbsDriftMicro / 1000.0 / 1000.0 * 5.0;
				// UE_LOG(LogTemp, Warning, TEXT("Rate adjustment: %f"), rateAdjustment);
				auto Rate = DriftMicro > 0 ? 1.0 - RateAdjustment : 1.0 + RateAdjustment;
				// UE_LOG(LogTemp, Warning, TEXT("Rate: %f"), rate);
				TArray<FFloatRange> SupportedRates;
				MediaPlayer->GetSupportedRates(SupportedRates, true);
				bool IsGood = false;
				for (auto& Range : SupportedRates)
				{
					if (Range.Contains(Rate))
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

					auto Closest = SupportedRates[0].GetLowerBoundValue();
					for (auto& Range : SupportedRates)
					{
						const auto Bound = Rate > 1.0 ? Range.GetUpperBoundValue() : Range.GetLowerBoundValue();
						if (Bound == 0.0f) continue;
						if (FMath::Abs(Bound - Rate) < FMath::Abs(Closest - Rate))
						{
							Closest = Bound;
						}
					}
					Rate = Closest;
				}
				// UE_LOG(LogTemp, Warning, TEXT("Closest Available Rate: %f"), Rate);
				MediaPlayer->SetRate(Rate);
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
		
		if (!BGAStartQueue.Peek(pair))
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
	constexpr int32 WavNum = 36*36;
	const int TaskSize = WavNum / TaskNum;
	FString ChartFolder = Chart->Meta->Folder;
	FCriticalSection SoundTableLock;
	ParallelFor(TaskNum, [&](const int32 Index){
		if(bCancelled) return;
		const int32 Start = Index * TaskSize;
		if(Start >= WavNum) return;
		int32 End = (Index+1) * TaskSize;
		if(Index == TaskNum - 1)
		{
			End = WavNum;
		}
		if(End > WavNum)
		{
			End = WavNum;
		}
		for(int32 j=Start; j<End; j++)
		{
			if(bCancelled) return;
			if(!Chart->WavTable.Contains(j)) continue;
			auto& Wav = Chart->WavTable[j];
			
			FMOD::Sound* Sound;
			const FMOD_RESULT Result = ReadWav(FPaths::Combine(ChartFolder, Wav), &Sound, bCancelled);
			if(Result != FMOD_OK)
			{
				UE_LOG(LogTemp, Error, TEXT("Failed to load wav: %s, error code: %d"), *Wav, Result);
				
				continue;
			}
			Sound->setLoopCount(0);

			SoundTableLock.Lock();
			SoundTable.Add(j, Sound);
			SoundTableLock.Unlock();
		}
	}, !bSupportMultiThreading);
	
	if(OptionalPlayer == nullptr) return;
	
	MediaPlayer = OptionalPlayer;
	MediaPlayer->PlayOnOpen = true;

	for(auto& Bmp: Chart->BmpTable)
	{
		if(bCancelled) return;
		FString Name = Bmp.Value;
		FString Path = FPaths::Combine(ChartFolder, Name);
		FString WithoutExt = FPaths::Combine(FPaths::GetPath(Path), FPaths::GetBaseFilename(Path));
		for(FString ExtraDot : {"", "."})
		{
			if(bCancelled) return;
			for(FString Ext : {".mp4", ".avi", ".wmv", ".mkv", ".mpg", ".mpeg", ".mov", ".flv", ".webm"})
			{
				if(bCancelled) return;
				// transcode into temp file
				FString NewPath = WithoutExt + ExtraDot + Ext;
				if(!FPaths::FileExists(NewPath)) continue;
				FString Original = NewPath;
				FString TempPath = Original;
				if(Ext != ".mp4")
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
						const int Result = transcode(TCHAR_TO_UTF8(*Original), TCHAR_TO_UTF8(*TempPath), &bCancelled);
						if(Result<0||bCancelled)
						{
							// remove temp file
							IFileManager::Get().Delete(*TempPath);
							break;
						}
						UE_LOG(LogTemp, Warning, TEXT("Transcoding done: %d"), Result);
					}
				}
				if(bCancelled) return;
				const FString FileName = FPaths::GetBaseFilename(TempPath);
				const FName ObjectName = MakeUniqueObjectName(GetTransientPackage(), UFileMediaSource::StaticClass(), FName(*FileName));

				auto MediaSource = NewObject<UFileMediaSource>(GetTransientPackage(), ObjectName, RF_Transactional | RF_Transient);
				MediaSource->SetFilePath(TempPath);
				BGASourceMap.Add(Bmp.Key, MediaSource);
				UE_LOG(LogTemp, Warning, TEXT("Added BGA %d"), Bmp.Key);
				break;
			}
		}
	}	
}

void FJukebox::Start(std::atomic_bool& bCancelled, const long long PosMicro, const bool AutoKeySound)
{
	Stop();
	if(bCancelled) return;
	if(!Chart)
	{
		UE_LOG(LogTemp, Error, TEXT("Chart is not loaded"));
		return;
	}
	// schedule all sounds
	ChannelGroup->setPaused(true);
	System->getSoftwareFormat(&SampleRate, nullptr, nullptr);

	ChannelGroup->getDSPClock(&StartDspClock, nullptr);
	
	for(auto& Measure: Chart->Measures)
	{
		for(const auto& Timeline: Measure->TimeLines)
		{
			if(bCancelled) return;
			if(Timeline->BgaBase != -1)
			{
				UE_LOG(LogTemp, Warning, TEXT("BGA %d at %lld"), Timeline->BgaBase, Timeline->Timing);
				BGAStartQueue.Enqueue({Timeline->BgaBase, Timeline->Timing});
			}
			const auto Timing = Timeline->Timing - PosMicro;
			if(Timing < 0) continue;
			const auto DspClock = StartDspClock + MsToDspClocks(static_cast<double>(Timing)/1000.0);
			for(const auto& Note: Timeline->BackgroundNotes)
			{
				if(!Note) continue;
				const auto Wav = Note->Wav;
				if(Wav == FBMSParser::NoWav) continue;
				const auto Sound = SoundTable.FindRef(Wav);
				if(!Sound) continue;
				ScheduleSound(DspClock, Sound);
			}
			if(AutoKeySound)
			{
				for(const auto& Note: Timeline->Notes)
				{
					if(!Note) continue;
					const auto Wav = Note->Wav;
					if(Wav == FBMSParser::NoWav) continue;
					const auto Sound = SoundTable.FindRef(Wav);
					if(!Sound) continue;
					ScheduleSound(DspClock, Sound);
				}
			}
		}
	}
	if(bCancelled) return;
	Unpause();
}

void FJukebox::Unpause()
{

	ChannelGroup->setPaused(false);
	
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
			
			int Channels;
			int RealChannels;
			System->getChannelsPlaying(&Channels, &RealChannels);
			const int AvailableChannels = MaxBgRealChannels - RealChannels;
			for (int i = 0; i < AvailableChannels; i++)
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

				FMOD::Channel* Channel;
				PlaySound(pair.Value, ChannelGroup, true, &Channel);
				Channel->setDelay(pair.Key, 0, true);

				Channel->setPaused(false);
			}
			System->update();
			// sleep for 1ms
			FPlatformProcess::Sleep(0.001);
		}
	});
}

void FJukebox::PlaySound(FMOD::Sound* Sound, FMOD::ChannelGroup* Group, const bool Paused, FMOD::Channel** Channel)
{
	// THIS IS BUGGY RIGHT NOW
	// if(const auto PreviousChannel = AudioChannelMap.FindRef(sound))
	// {
	// 	UE_LOG(LogTemp, Warning, TEXT("Sound %p already playing, stop it"), sound);
	// 	PreviousChannel->stop();
	// }
	System->playSound(Sound, Group, Paused, Channel);
	AudioChannelMapLock.Lock();
	AudioChannelMap.Add(Sound, *Channel);
	AudioChannelMapLock.Unlock();
}

void FJukebox::Pause() const
{
	ChannelGroup->setPaused(true);
	if(IsValid(MediaPlayer))
	{
		MediaPlayer->Pause();
	}
}

bool FJukebox::IsPaused() const
{
	bool IsPaused;
	ChannelGroup->getPaused(&IsPaused);
	return IsPaused;
}

void FJukebox::Stop()
{
	ChannelGroup->setPaused(true);
	ChannelGroup->stop();
	// ReSharper disable once CppExpressionWithoutSideEffects
	MainLoopTask.Wait();

	SoundQueueLock.Lock();
	SoundQueue.Empty();
	SoundQueueLock.Unlock();
}

void FJukebox::PlayKeySound(const int Wav)
{
	FMOD::Sound* Sound = SoundTable.FindRef(Wav);
	if(!Sound) return;
	UE::Tasks::Launch(UE_SOURCE_LOCATION, [this, Sound](){
		FMOD::Channel* Channel;
		if(!ChannelGroup) return;
		PlaySound(Sound, ChannelGroup, false, &Channel);
	});
}

void FJukebox::Unload()
{
	Stop();
	for(const auto& Pair: SoundTable)
	{
		if(Pair.Value) Pair.Value->release();
	}
	SoundTable.Empty();
	

	MediaPlayerLock.Lock();
	MediaPlayer = nullptr;
	MediaPlayerLock.Unlock();

	BGALock.Lock();
	BGAStartQueue.Empty();
	for(const auto& Pair: BGASourceMap)
	{
		Pair.Value->RemoveFromRoot();
		Pair.Value->ConditionalBeginDestroy();
	}
	BGASourceMap.Empty();
	BGALock.Unlock();
	AudioChannelMapLock.Lock();
	for(const auto& Pair: AudioChannelMap)
	{
		Pair.Value->stop();
	}
	AudioChannelMapLock.Unlock();

}


long long FJukebox::GetPositionMicro() const
{
	if(!ChannelGroup) return 0;
	if(SampleRate == -1) return 0;
	unsigned long long DspClock;
	ChannelGroup->getDSPClock(&DspClock, nullptr);

	const auto Result = (static_cast<double>(DspClock) / SampleRate * 1000000 - static_cast<double>(StartDspClock) / SampleRate * 1000000);
	return Result;
}
