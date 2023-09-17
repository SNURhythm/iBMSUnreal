// Fill out your copyright notice in the Description page of Project Settings.


#include "ChartSelectScreen.h"
#include "BMSParser.h"
#include <Tasks/Task.h>
using namespace UE::Tasks;
enum DiffType {
    Deleted,
    Added
};
struct Diff {
    FString path;
    DiffType type;
};
void FindNew(TArray<Diff>& Diffs, const TSet<FString>& PrevPathSet, const FString& Directory, FThreadSafeBool& bCancelled)
{
    IFileManager& FileManager = IFileManager::Get();
    TArray<FString> DirectoriesToVisit;
    DirectoriesToVisit.Add(Directory);
    while (DirectoriesToVisit.Num() > 0 )
    {
        FString CurrentDirectory = DirectoriesToVisit.Pop();
        TArray<FString> Files;
        FileManager.IterateDirectory(*CurrentDirectory, [&](const TCHAR* FilenameOrDirectory, bool bIsDirectory) -> bool
            {
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
                        auto diff = Diff();
                        diff.path = FilePath;
                        diff.type = DiffType::Added;
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

    FThreadSafeBool bCancelled = false;
    FTask loadTask = Launch(UE_SOURCE_LOCATION, [&]() {
        UE_LOG(LogTemp, Warning, TEXT("ChartSelectScreen Start Task!!"));
        // find new charts
        TArray<Diff> Diffs;
        TSet<FString> PathSet;
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
                auto diff = Diff();
                diff.path = path;
                diff.type = DiffType::Deleted;
                Diffs.Add(diff);
            }
        }

        if (Diffs.Num() > 0) {
            bool bSupportMultithread = FPlatformProcess::SupportsMultithreading();

            ParallelFor(Diffs.Num(), [&](int32 i) {

                auto diff = Diffs[i];

                if (diff.type == DiffType::Added) {
                    try {
                        TArray <FString> lines;
                        lines.SetNum(100);
                        auto parser = new BMSParser();
                        parser->Parse(diff.path, false, false);
                        auto measureNum = parser->chart.Measures.Num();
                        /*UE_LOG(LogTemp, Warning, TEXT("measure num: %d"), measureNum);*/
                    }
                    catch (...) {
						UE_LOG(LogTemp, Warning, TEXT("exception!"));
					}
                }
                }, !bSupportMultithread);
        }


        });
	
}

// Called every frame
void AChartSelectScreen::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

