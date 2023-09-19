// Fill out your copyright notice in the Description page of Project Settings.


#include "ChartSelectScreen.h"
#include "FBMSParser.h"
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
    while (DirectoriesToVisit.Num() > 0 )
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
// Sets default values
AChartSelectScreen::AChartSelectScreen()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AChartSelectScreen::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogTemp, Warning, TEXT("ChartSelectScreen BeginPlay()!!"));

    
    FTask loadTask = Launch(UE_SOURCE_LOCATION, [&]() {
        UE_LOG(LogTemp, Warning, TEXT("ChartSelectScreen Start Task!!"));
        // find new charts
        TArray<FDiff> Diffs;
        TSet<FString> PathSet;
        std::atomic_int SuccessCount;
        // TODO: Initialize prevPathSet by db
        IFileManager& FileManager = IFileManager::Get();
        FString Directory = FString("C:/Users/XF/AppData/LocalLow/SNURhythm/iBMS/");
        UE_LOG(LogTemp, Warning, TEXT("ChartSelectScreen FindNew!!"));
        // print bCancelled
        UE_LOG(LogTemp, Warning, TEXT("ChartSelectScreen isCancelled!! %d"), (bool)bCancelled);
        FindNew(Diffs, PathSet, Directory, bCancelled);
        UE_LOG(LogTemp, Warning, TEXT("ChartSelectScreen FindNew Done!! %d"), Diffs.Num());
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
            const int taskNum = FPlatformMisc::NumberOfWorkerThreadsToSpawn()-1;
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
                        parser->Parse(diff.path, false, true);
                        SuccessCount++;
                        if (SuccessCount % 100 == 0)
                            UE_LOG(LogTemp, Warning, TEXT("success count: %d"), (int)SuccessCount);
                        delete parser->Chart;
                        delete parser;
                    }
                }
            
                }, !bSupportMultithread);
        }
        UE_LOG(LogTemp, Warning, TEXT("ChartSelectScreen End Task!!"));
        UE_LOG(LogTemp, Warning, TEXT("success count: %d"), (int)SuccessCount);


        });
	
}

// Called every frame
void AChartSelectScreen::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AChartSelectScreen::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	UE_LOG(LogTemp, Warning, TEXT("ChartSelectScreen EndPlay()!!"));
    bCancelled = true;
}

