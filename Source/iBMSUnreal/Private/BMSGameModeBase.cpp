// Fill out your copyright notice in the Description page of Project Settings.


#include "BMSGameModeBase.h"


#include "BMSParser.h"

#include "BMSGameInstance.h"
#include "ChartDBHelper.h"
#include "ChartListEntry.h"
#include "ChartSelectUI.h"
#include "Blueprint/UserWidget.h"
#include "iBMSUnreal/Public/ImageUtils.h"
#include "Kismet/GameplayStatics.h"

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
                        diff.path = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*FilePath);
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

	CurrentEntryData = nullptr;

}

void ABMSGameModeBase::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	UBMSGameInstance* GameInstance = Cast<UBMSGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
	FMODSystem = GameInstance->GetFMODSystem();
	jukebox = new Jukebox(FMODSystem);
	UE_LOG(LogTemp, Warning, TEXT("InitGame"));

}

void ABMSGameModeBase::LoadCharts()
{
	    FTask LoadTask = Launch(UE_SOURCE_LOCATION, [&]()
	    {
		    FMOD::Sound* SuccessSound;
			const FString SoundPathRel = FPaths::Combine(FPaths::ProjectContentDir(), "Sounds/success.wav");
			const FString SoundPath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*SoundPathRel);
			UE_LOG(LogTemp, Warning, TEXT("BMSGameModeBase SoundPath: %s"), *SoundPath);
			// /private/var/containers/Bundle/Application/GUID/iBMSUnreal.app/Documents
			const FString DocumentsPathRel = FPaths::RootDir();
			const FString DocumentsPath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*DocumentsPathRel);
			UE_LOG(LogTemp, Warning, TEXT("BMSGameModeBase DocumentsPath: %s"), *DocumentsPath);
			
			FMODSystem->createSound(TCHAR_TO_ANSI(*SoundPath), FMOD_DEFAULT, 0, &SuccessSound);
			UE_LOG(LogTemp, Warning, TEXT("BMSGameModeBase Start Task!!"));
			auto dbHelper = ChartDBHelper::GetInstance();
			auto db = dbHelper.Connect();
			dbHelper.CreateTable(db);
	    	auto chartMetas = dbHelper.SelectAll(db);
	    	chartMetas.Sort([](const FChartMeta& A, const FChartMeta& B) {
				return A.Title < B.Title;
			});
	    	auto ChartList = ChartSelectUI->ChartList;
			AsyncTask(ENamedThreads::GameThread, [chartMetas, ChartList, this]()
			{
				if (IsValid(ChartList))
					SetChartMetas(chartMetas);
			});
	    	
			// find new charts
			TArray<FDiff> Diffs;
			TSet<FString> PathSet;
			std::atomic_int SuccessCount;
			// TODO: Initialize prevPathSet by db
			for (auto& meta : chartMetas) {
				if (bCancelled) break;
				auto& path = meta->BmsPath;
				PathSet.Add(path);
			}
			IFileManager& FileManager = IFileManager::Get();
			// use iOS Document Directory
	#if PLATFORM_IOS
			// mkdir "BMS"
			FString DirectoryRel = FPaths::Combine(FPaths::RootDir(), "BMS/");
			FileManager.MakeDirectory(*DirectoryRel);
	#else
			// use Project/BMS. Note that this would not work on packaged build, so we need to make it configurable
			FString DirectoryRel = FPaths::Combine(FPaths::ProjectDir(), "BMS/");
	#endif
			FString Directory = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*DirectoryRel);
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

			if (Diffs.Num() > 0)
			{
				const bool bSupportMultithreading = FPlatformProcess::SupportsMultithreading();
				const int TaskNum = FMath::Max(FPlatformMisc::NumberOfWorkerThreadsToSpawn()/2, 1);
				UE_LOG(LogTemp, Warning, TEXT("BMSGameModeBase taskNum: %d"), TaskNum);
				const int TaskSize = Diffs.Num() / TaskNum;
				dbHelper.BeginTransaction(db);
				std::atomic_bool isCommitting;
				ParallelFor(TaskNum, [&](int32 idx) {
					if (bCancelled) return;
					const int Start = idx * TaskSize;
					if (Start >= Diffs.Num()) return;
					int end = (idx + 1) * TaskSize;
					if (idx == TaskNum - 1) end = Diffs.Num();
					if (end > Diffs.Num()) end = Diffs.Num();
					for (int i = Start; i < end; i++) {
						if (bCancelled) return;
						auto& diff = Diffs[i];

						if (diff.type == EDiffType::Added)
						{
							const auto Parser = new FBMSParser();
							FChart* Chart;
							Parser->Parse(diff.path, &Chart, false, true);
							++SuccessCount;
							if (SuccessCount % 100 == 0) {
								UE_LOG(LogTemp, Warning, TEXT("success count: %d"), (int)SuccessCount);
								if (isCommitting) continue;
								isCommitting = true;
								dbHelper.CommitTransaction(db);
								dbHelper.BeginTransaction(db);
								isCommitting = false;

							}
                            
							// UE_LOG(LogTemp,Warning,TEXT("TITLE: %s"), *Chart->Meta->Title);

							dbHelper.Insert(db, *Chart->Meta);
							// close db
							delete Chart;
							delete Parser;
						}
					}

					}, !bSupportMultithreading);
				dbHelper.CommitTransaction(db);
			}
	    	chartMetas = dbHelper.SelectAll(db);
	    	chartMetas.Sort([](const FChartMeta& A, const FChartMeta& B) {
				return A.Title < B.Title;
			});
	    	AsyncTask(ENamedThreads::GameThread, [chartMetas, ChartList, this]()
			{
				if (IsValid(ChartList))
					SetChartMetas(chartMetas);
			});
	    	
			const auto Result = FMODSystem->playSound(SuccessSound, nullptr, false, nullptr);
			UE_LOG(LogTemp, Warning, TEXT("FMODSystem->playSound: %d"), Result);
			UE_LOG(LogTemp, Warning, TEXT("BMSGameModeBase End Task"));
			UE_LOG(LogTemp, Warning, TEXT("success count: %d"), (int)SuccessCount);
	    });
}
void ABMSGameModeBase::BeginPlay()
{


	if(IsValid(WidgetClass))
	{
	    ChartSelectUI = CreateWidget<UChartSelectUI>(GetWorld(), WidgetClass);
	    if(IsValid(ChartSelectUI))
	    {
	        ChartSelectUI->AddToViewport();
	    	// bind event to list
	    	ChartSelectUI->ChartList->OnItemSelectionChanged().AddLambda([&](UObject* Item)
	    	{
	    		auto EntryData = Cast<UChartListEntryData>(Item);
	    		if(!IsValid(EntryData)) return;
				auto chartMeta = EntryData->ChartMeta;
	    		UE_LOG(LogTemp, Warning, TEXT("ChartMeta: %s"), *chartMeta->BmsPath);
				ChartSelectUI->TitleText->SetText(FText::FromString(chartMeta->Title));
	    		ChartSelectUI->ArtistText->SetText(FText::FromString(chartMeta->Artist));

	    		ChartSelectUI->GenreText->SetText(FText::FromString(chartMeta->Genre));
	    		if(chartMeta->MaxBpm == chartMeta->MinBpm)
	    		{
	    			ChartSelectUI->BPMText->SetText(FText::FromString(FString::Printf(TEXT("%.1f"), chartMeta->Bpm)));
	    		} else
	    		{
	    			//min~max
	    			ChartSelectUI->BPMText->SetText(FText::FromString(FString::Printf(TEXT("%.1f~%.1f"), chartMeta->MinBpm, chartMeta->MaxBpm)));
	    		}
	    		ChartSelectUI->TotalText->SetText(FText::FromString(FString::Printf(TEXT("%.2lf"), chartMeta->Total)));
	    		ChartSelectUI->NotesText->SetText(FText::FromString(FString::Printf(TEXT("%d"), chartMeta->TotalNotes)));
	    		ChartSelectUI->JudgementText->SetText(FText::FromString(FString::Printf(TEXT("%d"), chartMeta->Rank)));
				bJukeboxCancelled = true;
	    		// full-parse chart
	    		FTask LoadTask = Launch(UE_SOURCE_LOCATION, [&]()
	    		{
	    			
	    			const auto Parser = new FBMSParser();
					FChart* Chart;
					Parser->Parse(chartMeta->BmsPath, &Chart, false, false);
	    			jukeboxLock.Lock();
	    			bJukeboxCancelled = false;
	    			jukebox->LoadChart(Chart, bJukeboxCancelled);
	    			if (bJukeboxCancelled)
	    			{
	    				jukeboxLock.Unlock();
	    				return;
	    			}
	    			jukebox->Start(0, true);
	    			jukeboxLock.Unlock();
	    		});
	    		
	    		
	    		
	    		// make background of item CACACA of 20% opacity
	    		auto entry = ChartSelectUI->ChartList->GetEntryWidgetFromItem(Item);
	    		auto chartListEntry = Cast<UChartListEntry>(entry);
	    		if (IsValid(chartListEntry))
	    		{
	    			chartListEntry->Border->SetBrushColor(FLinearColor(0.79f, 0.79f, 0.79f, 0.2f));
	    		}
	    		// restore previous selection
	    		if(CurrentEntryData)
	    		{
	    			auto prevEntry = ChartSelectUI->ChartList->GetEntryWidgetFromItem(CurrentEntryData);
	    			auto prevChartListEntry = Cast<UChartListEntry>(prevEntry);
	    			if (IsValid(prevChartListEntry))
	    			{
	    				prevChartListEntry->Border->SetBrushColor(FLinearColor::Transparent);
	    			}
	    		}
	    		CurrentEntryData = EntryData;
	    		UTexture2D* BackgroundImage = nullptr;
	    		bool IsValid = false;
	    		
	    		auto path = chartMeta->StageFile;
	    		if(path.IsEmpty()) {
	    			path = chartMeta->BackBmp;
	    		}
	    		if(path.IsEmpty()) {
	    			path = chartMeta->Preview;
	    		}
	    		if(path.IsEmpty()) {
	    			// set to blank, black
	    			ChartSelectUI->BackgroundImage->SetBrushFromTexture(nullptr);
	    			ChartSelectUI->BackgroundImage->SetBrushTintColor(FLinearColor::Black);
	    			ChartSelectUI->StageFileImage->SetBrushFromTexture(nullptr);
	    			ChartSelectUI->StageFileImage->SetBrushTintColor(FLinearColor::Black);
	    			return;
	    		}
	    		path = FPaths::Combine(chartMeta->Folder, path);
	    		ImageUtils::LoadTexture2D(path, IsValid, -1, -1, BackgroundImage);
	    		ChartSelectUI->BackgroundImage->SetBrushTintColor(FLinearColor(0.5f, 0.5f, 0.5f, 0.5f));
	    		ChartSelectUI->BackgroundImage->SetBrushFromTexture(BackgroundImage);
	    		ChartSelectUI->StageFileImage->SetBrushTintColor(FLinearColor::White);
	    		ChartSelectUI->StageFileImage->SetBrushFromTexture(BackgroundImage);
	    		
	    	});
	    	// on item bound
	    	ChartSelectUI->ChartList->OnEntryWidgetGenerated().AddLambda([&](UUserWidget& Widget)
	    	{
	    		// get item from widget
	    		auto chartListEntry = Cast<UChartListEntry>(&Widget);
	    		auto chartMeta = chartListEntry->EntryData;
	    		if (IsValid(chartMeta))
	    		{
	    			// if item is selected, set background color
	    			if (chartMeta == CurrentEntryData)
	    			{
	    				chartListEntry->Border->SetBrushColor(FLinearColor(0.79f, 0.79f, 0.79f, 0.2f));
	    			} else
	    			{
	    				chartListEntry->Border->SetBrushColor(FLinearColor::Transparent);
	    			}
	    		}
	    	
	    	});
	    	// on search
	    	ChartSelectUI->SearchBox->OnTextCommitted.AddDynamic(this, &ABMSGameModeBase::OnSearchBoxTextCommitted);
	    	ChartSelectUI->SearchBox->OnTextChanged.AddDynamic(this, &ABMSGameModeBase::OnSearchBoxTextChanged);
	    } else
	    {
		    UE_LOG(LogTemp, Warning, TEXT("ChartSelectUI is not valid"));
	    }
	} else
	{
	    UE_LOG(LogTemp, Warning, TEXT("WidgetClass is not valid"));
	}
	LoadCharts();
}

void ABMSGameModeBase::SetChartMetas(const TArray<FChartMeta*>& ChartMetas)
{
	// convert to UChartListEntryData
	TArray<UChartListEntryData*> EntryDatas;
	for (auto& meta : ChartMetas)
	{
		auto entryData = NewObject<UChartListEntryData>();
		entryData->ChartMeta = meta;
		EntryDatas.Add(entryData);
	}
	ChartSelectUI->ChartList->SetListItems(EntryDatas);
}

void ABMSGameModeBase::OnSearchBoxTextChanged(const FText& Text)
{

}

void ABMSGameModeBase::OnSearchBoxTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	auto dbHelper = ChartDBHelper::GetInstance();
	auto db = dbHelper.Connect();
	auto str = Text.ToString();
	auto chartMetas = dbHelper.Search(db, str);
	chartMetas.Sort([](const FChartMeta& A, const FChartMeta& B) {
		return A.Title < B.Title;
	});
	SetChartMetas(chartMetas);
}


void ABMSGameModeBase::Logout(AController* Exiting)
{
	Super::Logout(Exiting);
	UE_LOG(LogTemp, Warning, TEXT("Logout"));
	bCancelled = true;
	bJukeboxCancelled = true;
	jukeboxLock.Lock();
	jukebox->Unload();
	delete jukebox;
	jukeboxLock.Unlock();
    FMODSystem->release();
    FMODSystem = nullptr;
}

void ABMSGameModeBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
    if (FMODSystem==nullptr) return;
    FMODSystem->update();
}