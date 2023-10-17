// Fill out your copyright notice in the Description page of Project Settings.


#include "ChartSelectScreen.h"

#include "BMSGameInstance.h"
#include "BMSParser.h"
#include "ChartDBHelper.h"
#include "ChartSelectUI.h"
#include "ChartListEntry.h"

#include "DesktopPlatformModule.h"
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

// Sets default values
AChartSelectScreen::AChartSelectScreen()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	CurrentEntryData = nullptr;
}
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

void AChartSelectScreen::LoadCharts()
{
		auto dbHelper = ChartDBHelper::GetInstance();
		auto db = dbHelper.Connect();
		dbHelper.CreateChartMetaTable(db);
	    dbHelper.CreateEntriesTable(db);
	    auto entries = dbHelper.SelectAllEntries(db);
	// user home dir
	FString homeDir = FPlatformProcess::UserHomeDir();
	UE_LOG(LogTemp, Warning, TEXT("BMSGameModeBase UserHomeDir: %s"), *homeDir);
	    if(entries.IsEmpty())
	    {
#if PLATFORM_DESKTOP
	    	FString defaultPath;
#if PLATFORM_MAC
	    	defaultPath = "~";
#else
	    	defaultPath = FPlatformProcess::UserDir();
#endif

	    	// prompt user to select folder
			UE_LOG(LogTemp, Warning, TEXT("BMSGameModeBase Select Folder"));
			FString EntryPath;
			bool bFolderSelected = FDesktopPlatformModule::Get()->OpenDirectoryDialog(nullptr, TEXT("Select BMS Folder"), defaultPath, EntryPath);
			if(bFolderSelected)
			{
				dbHelper.InsertEntry(db, EntryPath);

			}

#else
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
	    	dbHelper.InsertEntry(db, Directory);
#endif
	    	entries = dbHelper.SelectAllEntries(db);
	    }
	    FTask LoadTask = Launch(UE_SOURCE_LOCATION, [&, db, entries]()
	    {
		    FMOD::Sound* SuccessSound;
			const FString SoundPathRel = FPaths::Combine(FPaths::ProjectContentDir(), "Sounds/success.wav");
			const FString SoundPath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*SoundPathRel);
			UE_LOG(LogTemp, Warning, TEXT("BMSGameModeBase SoundPath: %s"), *SoundPath);

			
			FMODSystem->createSound(TCHAR_TO_ANSI(*SoundPath), FMOD_DEFAULT, 0, &SuccessSound);
			UE_LOG(LogTemp, Warning, TEXT("BMSGameModeBase Start Task!!"));
			
			auto chartMetas = dbHelper.SelectAllChartMeta(db);
			chartMetas.Sort([](const FChartMeta& A, const FChartMeta& B) {
				return A.Title < B.Title;
			});
			auto ChartList = ChartSelectUI->ChartList;
			AsyncTask(ENamedThreads::GameThread, [chartMetas, ChartList, this]()
			{
				if (bCancelled) return;
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
	    	
			UE_LOG(LogTemp, Warning, TEXT("BMSGameModeBase FindNew"));
			// print bCancelled
			UE_LOG(LogTemp, Warning, TEXT("BMSGameModeBase isCancelled: %d"), (bool)bCancelled);
			for(auto& entry : entries)
			{
				if(bCancelled) break;
				FindNew(Diffs, PathSet, entry, bCancelled);
			}
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
							FBMSParser Parser;
							FChart* Chart;
							Parser.Parse(diff.path, &Chart, false, true, bCancelled);
							if(bCancelled) return;
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

							dbHelper.InsertChartMeta(db, *Chart->Meta);
							// close db
							delete Chart;
						}
					}

					}, !bSupportMultithreading);
				dbHelper.CommitTransaction(db);
			}
	    	if (bCancelled) return;
			chartMetas = dbHelper.SelectAllChartMeta(db);
			chartMetas.Sort([](const FChartMeta& A, const FChartMeta& B) {
				return A.Title < B.Title;
			});
			AsyncTask(ENamedThreads::GameThread, [chartMetas, ChartList, this]()
			{
				if (bCancelled) return;
				if (IsValid(ChartList))
					SetChartMetas(chartMetas);
			});

	    	UE_LOG(LogTemp, Warning, TEXT("BMSGameModeBase End Task"));
			UE_LOG(LogTemp, Warning, TEXT("success count: %d"), (int)SuccessCount);
	    	if (bCancelled) return;
			const auto Result = FMODSystem->playSound(SuccessSound, nullptr, false, nullptr);
			UE_LOG(LogTemp, Warning, TEXT("FMODSystem->playSound: %d"), Result);
			
	    });
}

void AChartSelectScreen::OnStartButtonClicked()
{
	if(!CurrentEntryData) return;
	auto chartMeta = CurrentEntryData->ChartMeta;
	UE_LOG(LogTemp, Warning, TEXT("ChartMeta: %s"), *chartMeta->BmsPath);
	auto gameInstance = Cast<UBMSGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
	StartOptions options;
	options.BmsPath = chartMeta->BmsPath;
	options.AutoKeysound = false;
	gameInstance->SetStartOptions(options);
	// load level
	UGameplayStatics::OpenLevel(GetWorld(), "RhythmPlay");
}

// Called when the game starts or when spawned
void AChartSelectScreen::BeginPlay()
{
	Super::BeginPlay();
	
	UE_LOG(LogTemp, Warning, TEXT("ChartSelectScreen BeginPlay()!!"));
	UBMSGameInstance* GameInstance = Cast<UBMSGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
	FMODSystem = GameInstance->GetFMODSystem();
	jukebox = new FJukebox(FMODSystem);
	UE_LOG(LogTemp, Warning, TEXT("InitGame"));
    
	if(IsValid(WidgetClass))
	{
		ChartSelectUI = CreateWidget<UChartSelectUI>(GetWorld(), WidgetClass);
		if(IsValid(ChartSelectUI))
		{
			ChartSelectUI->AddToViewport();
			ChartSelectUI->StartButton->OnClicked.AddDynamic(this, &AChartSelectScreen::OnStartButtonClicked);
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
				FString BmsPath = chartMeta->BmsPath;
				// full-parse chart
				FTask LoadTask = Launch(UE_SOURCE_LOCATION, [&, BmsPath]()
				{
					jukeboxLock.Lock();
					FBMSParser Parser;
					FChart* Chart;
					bJukeboxCancelled = false;
					Parser.Parse(BmsPath, &Chart, false, false, bJukeboxCancelled);
					jukebox->LoadChart(Chart, bJukeboxCancelled);
					if (bJukeboxCancelled)
					{
						delete Chart;
						jukeboxLock.Unlock();
						return;
					}
					jukebox->Start(0, true);
					delete Chart;
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
			ChartSelectUI->SearchBox->OnTextCommitted.AddDynamic(this, &AChartSelectScreen::OnSearchBoxTextCommitted);
			ChartSelectUI->SearchBox->OnTextChanged.AddDynamic(this, &AChartSelectScreen::OnSearchBoxTextChanged);
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

void AChartSelectScreen::SetChartMetas(const TArray<FChartMeta*>& ChartMetas)
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

void AChartSelectScreen::OnSearchBoxTextChanged(const FText& Text)
{

}

void AChartSelectScreen::OnSearchBoxTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	auto dbHelper = ChartDBHelper::GetInstance();
	auto db = dbHelper.Connect();
	auto str = Text.ToString();
	auto chartMetas = dbHelper.SearchChartMeta(db, str);
	chartMetas.Sort([](const FChartMeta& A, const FChartMeta& B) {
		return A.Title < B.Title;
	});
	SetChartMetas(chartMetas);
}

// Called every frame
void AChartSelectScreen::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (FMODSystem==nullptr) return;
	FMODSystem->update();

}

void AChartSelectScreen::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	UE_LOG(LogTemp, Warning, TEXT("ChartSelectScreen EndPlay()!!"));
	bCancelled = true;
	bJukeboxCancelled = true;
	jukeboxLock.Lock();
	jukebox->Unload();
	delete jukebox;
	jukeboxLock.Unlock();
}

