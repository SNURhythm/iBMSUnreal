// Fill out your copyright notice in the Description page of Project Settings.


#include "BMSGameModeBase.h"


#include "BMSParser.h"
#include <Tasks/Task.h>

using namespace UE::Tasks;
enum EDiffType {
    Deleted,
    Added
};
struct FDiff {
    FString path;
    EDiffType type;
};
static void FindNew(TArray<FDiff>& Diffs, const TSet<FString>& PrevPathSet, const FString& Directory, std::atomic_bool& bCancelled)
{
    
    IFileManager& FileManager = IFileManager::Get();
    TArray<FString> DirectoriesToVisit;
    DirectoriesToVisit.Add(Directory);
    while (DirectoriesToVisit.Num() > 0)
    {

        if (bCancelled) break;
        FString CurrentDirectory = DirectoriesToVisit.Pop();
        TArray<FString> Files;
        FileManager.IterateDirectory(*CurrentDirectory, [&](const TCHAR* FilenameOrDirectory, bool bIsDirectory) -> bool
            {
                if (bCancelled) return false;
                if (bIsDirectory)
                {

                    DirectoriesToVisit.Add(FilenameOrDirectory);
                }
                else
                {
                    FString FilePath = FString(FilenameOrDirectory);
                    FString FileExtension = FPaths::GetExtension(FilePath);

                    if (FileExtension != TEXT("bms") && FileExtension != TEXT("bme") && FileExtension != TEXT("bml"))
                    {
                        return true;
                    }

                    if (!PrevPathSet.Contains(FilePath))
                    {
                        auto diff = FDiff();
                        diff.path = FilePath;
                        diff.type = EDiffType::Added;
                        Diffs.Add(diff);
                    }
                }

                return true;
            });
    }
}

ABMSGameModeBase::ABMSGameModeBase()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ABMSGameModeBase::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
    auto result = FMOD::System_Create(&FMODSystem);
    if (result != FMOD_OK)
	{
		UE_LOG(LogTemp, Warning, TEXT("FMOD error! (%d)\n"), result);
		return;
	}
    FMODSystem->setSoftwareChannels(4092);
    FMODSystem->setSoftwareFormat(48000, FMOD_SPEAKERMODE_DEFAULT, 0);
    FMODSystem->setDSPBufferSize(256, 4);
    FMODSystem->init(4092, FMOD_INIT_NORMAL, 0);


	UE_LOG(LogTemp, Warning, TEXT("InitGame"));
    FTask loadTask = Launch(UE_SOURCE_LOCATION, [&]() {
        FMOD::Sound* SuccessSound;
        FString SoundPathRel = FPaths::Combine(FPaths::ProjectContentDir(), "Sounds/success.wav");
        FString SoundPath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*SoundPathRel);
        UE_LOG(LogTemp, Warning, TEXT("BMSGameModeBase SoundPath: %s"), *SoundPath);
        // /private/var/containers/Bundle/Application/GUID/iBMSUnreal.app/Documents
        FString DocumentsPathRel = FPaths::RootDir();
        FString DocumentsPath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*DocumentsPathRel);
        UE_LOG(LogTemp, Warning, TEXT("BMSGameModeBase DocumentsPath: %s"), *DocumentsPath);

        FMODSystem->createSound(TCHAR_TO_ANSI(*SoundPath), FMOD_DEFAULT, 0, &SuccessSound);
        UE_LOG(LogTemp, Warning, TEXT("BMSGameModeBase Start Task!!"));
        // find new charts
        TArray<FDiff> Diffs;
        TSet<FString> PathSet;
        std::atomic_int SuccessCount;
        // TODO: Initialize prevPathSet by db
        IFileManager& FileManager = IFileManager::Get();
        // use iOS Document Directory
#if PLATFORM_IOS
        // mkdir "BMS"
        FString Directory = FPaths::Combine(FPaths::RootDir(), "BMS/");
        FileManager.MakeDirectory(*Directory);
#else
        // use Project/BMS. Note that this would not work on packaged build, so we need to make it configurable
        FString Directory = FPaths::Combine(FPaths::ProjectDir(), "BMS/");
#endif
        UE_LOG(LogTemp, Warning, TEXT("BMSGameModeBase Directory: %s"), *Directory);
        UE_LOG(LogTemp, Warning, TEXT("BMSGameModeBase FindNew"));
        // print bCancelled
        UE_LOG(LogTemp, Warning, TEXT("BMSGameModeBase isCancelled: %d"), (bool)bCancelled);
        FindNew(Diffs, PathSet, Directory, bCancelled);
        UE_LOG(LogTemp, Warning, TEXT("BMSGameModeBase FindNew Done: %d"), Diffs.Num());
        // find deleted charts
        for (auto& path : PathSet) {
            if (bCancelled) break;
            if (!FileManager.FileExists(*path)) {
                auto diff = FDiff();
                diff.path = path;
                diff.type = EDiffType::Deleted;
                Diffs.Add(diff);
            }
        }

        if (Diffs.Num() > 0) {
            bool bSupportMultithread = FPlatformProcess::SupportsMultithreading();
            const int taskNum = FPlatformMisc::NumberOfWorkerThreadsToSpawn() - 1;
            UE_LOG(LogTemp, Warning, TEXT("BMSGameModeBase taskNum: %d"), taskNum);
            const int taskSize = Diffs.Num() / taskNum;
            ParallelFor(taskNum, [&](int32 idx) {
                if (bCancelled) return;
                int start = idx * taskSize;
                if (start >= Diffs.Num()) return;
                int end = (idx + 1) * taskSize;
                if (idx == taskNum - 1) end = Diffs.Num();
                if (end > Diffs.Num()) end = Diffs.Num();
                for (int i = start; i < end; i++) {
                    if (bCancelled) return;
                    auto& diff = Diffs[i];

                    if (diff.type == EDiffType::Added)
                    {
                        auto parser = new FBMSParser();
                        FChart* chart;
                        parser->Parse(diff.path, &chart, false, false);
                        SuccessCount++;
                        if (SuccessCount % 100 == 0)
                            UE_LOG(LogTemp, Warning, TEXT("success count: %d"), (int)SuccessCount);
                        //UE_LOG(LogTemp,Warning,TEXT("TITLE: %s"), *chart->Meta.Title);
                        delete chart;
                        delete parser;
                    }
                }

                }, !bSupportMultithread);
        }
        auto result = FMODSystem->playSound(SuccessSound, 0, false, 0);
        UE_LOG(LogTemp, Warning, TEXT("FMODSystem->playSound: %d"), result);
        UE_LOG(LogTemp, Warning, TEXT("BMSGameModeBase End Task"));
        UE_LOG(LogTemp, Warning, TEXT("success count: %d"), (int)SuccessCount);


        });
}

void ABMSGameModeBase::Logout(AController* Exiting)
{
	Super::Logout(Exiting);
	UE_LOG(LogTemp, Warning, TEXT("Logout"));
	bCancelled = true;
    FMODSystem->release();
    FMODSystem = nullptr;
}

void ABMSGameModeBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
    if (FMODSystem==nullptr) return;
    FMODSystem->update();
}