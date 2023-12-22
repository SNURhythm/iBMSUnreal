// Fill out your copyright notice in the Description page of Project Settings.


#include "ChartSelectScreen.h"
#include "MacNatives.h"
#include "BMSGameInstance.h"
#include "BMSParser.h"
#include "ChartDBHelper.h"
#include "ChartSelectUI.h"
#include "ChartListEntry.h"
#include "iOSNatives.h"
#include "Judge.h"
#include "MaterialDomain.h"
#include "MediaPlayer.h"
#include "MediaPlayer.h"
#include "MediaTexture.h"
#include "StreamMediaSource.h"
#include "tinyfiledialogs.h"
#include "transcode.h"
#include "Blueprint/UserWidget.h"
#include "iBMSUnreal/Public/ImageUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialExpressionTextureBase.h"
#include "Materials/MaterialExpressionTextureSample.h"


using namespace UE::Tasks;

enum EDiffType
{
	Deleted,
	Added
};

struct FDiff
{
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

static void FindNew(TArray<FDiff>& Diffs, const TSet<FString>& PrevPathSet, const FString& Directory,
                    std::atomic_bool& bCancelled)
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
				FString FullPath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*FilePath);
				if (!PrevPathSet.Contains(ChartDBHelper::ToRelativePath(FullPath)))
				{
					auto diff = FDiff();
					diff.path = FullPath;
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
	TArray<FString> entries;
	// user home dir
	FString homeDir = FPlatformProcess::UserHomeDir();
	UE_LOG(LogTemp, Warning, TEXT("BMSGameModeBase UserHomeDir: %s"), *homeDir);
#if PLATFORM_DESKTOP
	entries = dbHelper.SelectAllEntries(db);
	// check if platform is desktop
	if (entries.IsEmpty())
	{
		FString Directory;

		FString defaultPath;
#if PLATFORM_MAC
		defaultPath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*homeDir);
		// NSOpenPanel in C++
		std::string *stdstring = new std::string();
		bool result = MacOSSelectFolder("Select BMS Folder", TCHAR_TO_ANSI(*defaultPath), stdstring);
		if(result) Directory = FString(stdstring->c_str());
#else // not mac
		defaultPath = FPlatformProcess::UserDir();
		// prompt user to select folder
		Directory = tinyfd_selectFolderDialog("Select BMS Folder", TCHAR_TO_ANSI(*defaultPath));
		UE_LOG(LogTemp, Warning, TEXT("BMSGameModeBase Directory: %s"), *Directory);
		// normalize path

#endif
		Directory = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*Directory);
		if (!Directory.IsEmpty())
		{
			// insert entry
			dbHelper.InsertEntry(db, Directory);
		}
		entries = dbHelper.SelectAllEntries(db);


	}

#else // not desktop
	// use iOS Document Directory
#if PLATFORM_IOS
	// mkdir "BMS"
	FString DirectoryRel = FPaths::Combine(GetIOSDocumentsPath(), "BMS/");
	FString DirectoryAbs = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*DirectoryRel);
	UE_LOG(LogTemp, Warning, TEXT("BMSGameModeBase DirectoryAbs: %s"), *DirectoryAbs);
	IFileManager::Get().MakeDirectory(*DirectoryRel);
#else
	// use Project/BMS. Note that this would not work on packaged build, so we need to make it configurable
	FString DirectoryRel = FPaths::Combine(FPaths::ProjectDir(), "BMS/");
#endif
	entries.Add(DirectoryRel);
#endif
	FTask LoadTask = Launch(UE_SOURCE_LOCATION, [&, db, entries]()
	{
		IsScanning = true;
		FMOD::Sound* SuccessSound;
		const FString SoundPathRel = FPaths::Combine(FPaths::ProjectContentDir(), "Sounds/success.wav");
		const FString SoundPath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*SoundPathRel);
		UE_LOG(LogTemp, Warning, TEXT("BMSGameModeBase SoundPath: %s"), *SoundPath);


		FMODSystem->createSound(TCHAR_TO_ANSI(*SoundPath), FMOD_DEFAULT, 0, &SuccessSound);
		UE_LOG(LogTemp, Warning, TEXT("BMSGameModeBase Start Task!!"));

		auto chartMetas = dbHelper.SelectAllChartMeta(db);
		chartMetas.Sort([](const FChartMeta& A, const FChartMeta& B)
		{
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
		SuccessNewChartCount = 0;
		// TODO: Initialize prevPathSet by db
		for (auto& meta : chartMetas)
		{
			if (bCancelled) break;
			auto& path = meta->BmsPath;
			PathSet.Add(path);
		}
		IFileManager& FileManager = IFileManager::Get();

		UE_LOG(LogTemp, Warning, TEXT("BMSGameModeBase FindNew"));
		// print bCancelled
		UE_LOG(LogTemp, Warning, TEXT("BMSGameModeBase isCancelled: %d"), (bool)bCancelled);
		for (auto& entry : entries)
		{
			if (bCancelled) break;
			FindNew(Diffs, PathSet, entry, bCancelled);
		}
		UE_LOG(LogTemp, Warning, TEXT("BMSGameModeBase FindNew Done: %d"), Diffs.Num());
		TotalNewCharts = Diffs.Num();
		// find deleted charts
		for (auto& path : PathSet)
		{
			if (bCancelled) break;
			if (!FileManager.FileExists(*ChartDBHelper::ToAbsolutePath(path)))
			{
				auto diff = FDiff();
				diff.path = path;
				diff.type = EDiffType::Deleted;
				Diffs.Add(diff);
			}
		}

		if (Diffs.Num() > 0)
		{
			const bool bSupportMultithreading = FPlatformProcess::SupportsMultithreading();
			const int TaskNum = FMath::Max(FPlatformMisc::NumberOfWorkerThreadsToSpawn() / 2, 1);
			UE_LOG(LogTemp, Warning, TEXT("BMSGameModeBase taskNum: %d"), TaskNum);
			const int TaskSize = Diffs.Num() / TaskNum;
			dbHelper.BeginTransaction(db);
			std::atomic_bool isCommitting;
			ParallelFor(TaskNum, [&](int32 idx)
			{
				if (bCancelled) return;
				const int Start = idx * TaskSize;
				if (Start >= Diffs.Num()) return;
				int end = (idx + 1) * TaskSize;
				if (idx == TaskNum - 1) end = Diffs.Num();
				if (end > Diffs.Num()) end = Diffs.Num();
				for (int i = Start; i < end; i++)
				{
					if (bCancelled) return;
					auto& diff = Diffs[i];

					if (diff.type == EDiffType::Added)
					{
						FBMSParser Parser;
						FChart* Chart;
						Parser.Parse(diff.path, &Chart, false, true, bCancelled);
						if (bCancelled) return;
						++SuccessNewChartCount;
						if (SuccessNewChartCount % 100 == 0)
						{
							UE_LOG(LogTemp, Warning, TEXT("success count: %d"), (int)SuccessNewChartCount);
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
					} else
					{
						dbHelper.DeleteChartMeta(db, diff.path);
					}
				}
			}, !bSupportMultithreading);
			dbHelper.CommitTransaction(db);
		}
		if (bCancelled) goto scan_end;

		AsyncTask(ENamedThreads::GameThread, [ChartList, this]()
		{

			if (bCancelled) return;
			// refresh search result
			auto dbHelper = ChartDBHelper::GetInstance();
			const auto db = dbHelper.Connect();
			const auto SearchText = ChartSelectUI->SearchBox->GetText();
			FString SearchString = SearchText.ToString();
			auto chartMetas = SearchString.IsEmpty() ? dbHelper.SelectAllChartMeta(db) :
				dbHelper.SearchChartMeta(db, SearchString);
			chartMetas.Sort([](const FChartMeta& A, const FChartMeta& B)
			{
				return A.Title < B.Title;
			});
			if (bCancelled) return;
			if (IsValid(ChartList))
				SetChartMetas(chartMetas);
			
		});
		
		UE_LOG(LogTemp, Warning, TEXT("BMSGameModeBase End Task"));
		UE_LOG(LogTemp, Warning, TEXT("success count: %d"), (int)SuccessNewChartCount);
		if (bCancelled) goto scan_end;
		UE_LOG(LogTemp, Warning, TEXT("FMODSystem->playSound: %d"), FMODSystem->playSound(SuccessSound, nullptr, false, nullptr));

scan_end:
		IsScanning = false;
	});
}

void AChartSelectScreen::OnStartButtonClicked()
{
	if (!CurrentEntryData) return;
	auto chartMeta = CurrentEntryData->ChartMeta;
	UE_LOG(LogTemp, Warning, TEXT("ChartMeta: %s"), *chartMeta->BmsPath);
	auto gameInstance = Cast<UBMSGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
	StartOptions options;
	options.BmsPath = chartMeta->BmsPath;
	options.AutoKeysound = false;
	options.AutoPlay = false;
	gameInstance->SetStartOptions(options);
	// load level
	UGameplayStatics::OpenLevel(GetWorld(), "RhythmPlay");
}

void AChartSelectScreen::OnPlaybackResumed()
{
	ChartSelectUI->BackgroundImage->SetBrushFromMaterial(VideoMaterial);
}

// Called when the game starts or when spawned
void AChartSelectScreen::BeginPlay()
{
	Super::BeginPlay();
	MediaPlayer->OnMediaOpened.AddDynamic(this, &AChartSelectScreen::OnMediaOpened);
	MediaPlayer->OnPlaybackResumed.AddDynamic(this, &AChartSelectScreen::OnPlaybackResumed);
	// int ret = transcode("/Users/xf/iBMS/take003/bga_take.mpg", "/Users/xf/iBMS/take003/bga_take-ffmpeg.mp4");
	// UE_LOG(LogTemp, Warning, TEXT("transcode: %d"), ret);
	UE_LOG(LogTemp, Warning, TEXT("ChartSelectScreen BeginPlay()!!"));
	UBMSGameInstance* GameInstance = Cast<UBMSGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
	FMODSystem = GameInstance->GetFMODSystem();
	jukebox = new FJukebox(FMODSystem);
	UE_LOG(LogTemp, Warning, TEXT("InitGame"));

	if (IsValid(WidgetClass))
	{
		ChartSelectUI = CreateWidget<UChartSelectUI>(GetWorld(), WidgetClass);
		if (IsValid(ChartSelectUI))
		{
			ChartSelectUI->AddToViewport();
			ChartSelectUI->StartButton->OnClicked.AddDynamic(this, &AChartSelectScreen::OnStartButtonClicked);
			// bind event to list
			ChartSelectUI->ChartList->OnItemSelectionChanged().AddLambda([&](UObject* Item)
			{
				auto EntryData = Cast<UChartListEntryData>(Item);
				if (!IsValid(EntryData)) return;
				auto chartMeta = EntryData->ChartMeta;
				UE_LOG(LogTemp, Warning, TEXT("ChartMeta: %s"), *chartMeta->BmsPath);
				ChartSelectUI->TitleText->SetText(FText::FromString(chartMeta->Title));
				ChartSelectUI->ArtistText->SetText(FText::FromString(chartMeta->Artist));

				ChartSelectUI->GenreText->SetText(FText::FromString(chartMeta->Genre));
				if (chartMeta->MaxBpm == chartMeta->MinBpm)
				{
					ChartSelectUI->BPMText->SetText(FText::FromString(FString::Printf(TEXT("%.1f"), chartMeta->Bpm)));
				}
				else
				{
					//min~max
					ChartSelectUI->BPMText->SetText(
						FText::FromString(FString::Printf(TEXT("%.1f~%.1f"), chartMeta->MinBpm, chartMeta->MaxBpm)));
				}
				ChartSelectUI->TotalText->SetText(FText::FromString(FString::Printf(TEXT("%.2lf"), chartMeta->Total)));
				ChartSelectUI->NotesText->
				               SetText(FText::FromString(FString::Printf(TEXT("%d"), chartMeta->TotalNotes)));
				ChartSelectUI->JudgementText->SetText(
					FText::FromString(FString::Printf(TEXT("%s"), *FJudge::GetRankDescription(chartMeta->Rank))));
				bJukeboxCancelled = true;
				if (JukeboxTask.IsValid())
				{
					JukeboxTask.Wait();
				}
				bJukeboxCancelled = false;
				FString BmsPath = chartMeta->BmsPath;
				
				bool IsImageValid = false;
				UTexture2D* BackgroundImage = nullptr;
				auto ImagePath = chartMeta->StageFile;
				if (ImagePath.IsEmpty())
				{
					ImagePath = chartMeta->BackBmp;
				}
				if (ImagePath.IsEmpty())
				{
					ImagePath = chartMeta->Preview;
				}
				if (ImagePath.IsEmpty())
				{
					// set to blank, black
					ChartSelectUI->BackgroundImage->SetBrushFromTexture(nullptr);
					ChartSelectUI->BackgroundImage->SetBrushTintColor(FLinearColor::Black);
					ChartSelectUI->StageFileImage->SetBrushFromTexture(nullptr);
					ChartSelectUI->StageFileImage->SetBrushTintColor(FLinearColor::Black);
				} else
				{
					ImagePath = FPaths::Combine(chartMeta->Folder, ImagePath);
					ImageUtils::LoadTexture2D(ImagePath, IsImageValid, -1, -1, BackgroundImage);
					if(IsImageValid)
					{
						ChartSelectUI->StageFileImage->SetBrushTintColor(FLinearColor::White);
						ChartSelectUI->StageFileImage->SetBrushFromTexture(BackgroundImage);
					}
				}
				MediaPlayer->Close();
				
				// full-parse chart
				JukeboxTask = Launch(UE_SOURCE_LOCATION, [&, BmsPath, ImagePath]()
				{
					
					FBMSParser Parser;
					FChart* Chart;
					UE_LOG(LogTemp, Warning, TEXT("Full-parsing %s"), *BmsPath);
					Parser.Parse(BmsPath, &Chart, false, false, bJukeboxCancelled);
					UE_LOG(LogTemp, Warning, TEXT("Full-parsing done"));
					auto BmpTable = Chart->BmpTable;
					auto Folder = Chart->Meta->Folder;

					
					BGATask = Launch(UE_SOURCE_LOCATION, [&, BmpTable, Folder, ImagePath]()
					{
						UE_LOG(LogTemp, Warning, TEXT("BGATask"));
						for(auto& bmp: BmpTable)
						{
							// find first mpg/mp4/avi/...
							FString Name = bmp.Value;
							FString Extension = FPaths::GetExtension(Name);
							if(Extension == "mpg" || Extension == "mp4" || Extension == "avi")
							{
								// transcode into temp file
								FString Original = FPaths::Combine(Folder, Name);
								FString Hash = FMD5::HashBytes((uint8*)TCHAR_TO_UTF8(*Original), Original.Len());
								FString TempPath = FPaths::Combine(FPaths::ProjectSavedDir(), "Temp", Hash + ".mp4");
								TempPath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForWrite(*TempPath);
								if(!FPaths::FileExists(TempPath))
								{
									UE_LOG(LogTemp, Warning, TEXT("Transcoding %s to %s"), *Original, *TempPath);
									int result = transcode(TCHAR_TO_UTF8(*Original), TCHAR_TO_UTF8(*TempPath), &bJukeboxCancelled);
									if(result<0||bJukeboxCancelled)
									{
										// remove temp file
										IFileManager::Get().Delete(*TempPath);
										return;
									}
									UE_LOG(LogTemp, Warning, TEXT("Transcoding done: %d"), result);
								}
								UE_LOG(LogTemp, Warning, TEXT("geturl: %s"), *MediaPlayer->GetUrl());
								
								AsyncTask(ENamedThreads::GameThread, [&, TempPath]()
								{
									ChartSelectUI->BackgroundImage->SetBrushTintColor(FLinearColor(0.5f, 0.5f, 0.5f, 0.5f));
									ChartSelectUI->BackgroundImage->SetBrushFromMaterial(VideoMaterial);
									if(!MediaPlayer->GetUrl().EndsWith(TempPath))
									{
										// MediaPlayer->Close();
										MediaPlayer->OpenFile(TempPath);
									}
								});
								return;
							}
						}
						AsyncTask(ENamedThreads::GameThread, [&, ImagePath]()
						{
							if(!ImagePath.IsEmpty())
							{
								UTexture2D* Image = nullptr;
								bool IsValid = false;
								ImageUtils::LoadTexture2D(ImagePath, IsValid, -1, -1, Image);
								if(IsValid)
								{
									ChartSelectUI->BackgroundImage->SetBrushTintColor(FLinearColor(0.5f, 0.5f, 0.5f, 0.5f));
									ChartSelectUI->BackgroundImage->SetBrushFromTexture(Image);
								}
							}
						});
					});
					UE_LOG(LogTemp, Warning, TEXT("Loading sound"));
					jukebox->LoadChart(Chart, bJukeboxCancelled);
					UE_LOG(LogTemp, Warning, TEXT("Loading sound done"));
					if (bJukeboxCancelled)
					{
						delete Chart;
						return;
					}
					jukebox->Start(0, true);
					delete Chart;
				});


				// make background of item CACACA of 20% opacity
				auto entry = ChartSelectUI->ChartList->GetEntryWidgetFromItem(Item);
				auto chartListEntry = Cast<UChartListEntry>(entry);
				if (IsValid(chartListEntry))
				{
					chartListEntry->Border->SetBrushColor(FLinearColor(0.79f, 0.79f, 0.79f, 0.2f));
				}
				// restore previous selection
				if (CurrentEntryData)
				{
					auto prevEntry = ChartSelectUI->ChartList->GetEntryWidgetFromItem(CurrentEntryData);
					auto prevChartListEntry = Cast<UChartListEntry>(prevEntry);
					if (IsValid(prevChartListEntry))
					{
						prevChartListEntry->Border->SetBrushColor(FLinearColor::Transparent);
					}
				}
				CurrentEntryData = EntryData;
				
				MediaPlayer->PlayOnOpen = true;
				// set video to media texture
				
				// bool result = MediaPlayer->Play();
				// UE_LOG(LogTemp, Warning, TEXT("MediaPlayer->Play(): %d"), result);
				


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
					}
					else
					{
						chartListEntry->Border->SetBrushColor(FLinearColor::Transparent);
					}
				}
			});
			// on search
			ChartSelectUI->SearchBox->OnTextCommitted.AddDynamic(this, &AChartSelectScreen::OnSearchBoxTextCommitted);
			ChartSelectUI->SearchBox->OnTextChanged.AddDynamic(this, &AChartSelectScreen::OnSearchBoxTextChanged);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("ChartSelectUI is not valid"));
		}
	}
	else
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
	chartMetas.Sort([](const FChartMeta& A, const FChartMeta& B)
	{
		return A.Title < B.Title;
	});
	SetChartMetas(chartMetas);
}

// Called every frame
void AChartSelectScreen::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if(IsScanning)
	{
		const int CurrentNewCharts = SuccessNewChartCount;
		const FString Text = TotalNewCharts == 0 ? TEXT("Checking for new files") : FString::Printf(TEXT("Scanning %d/%d"), CurrentNewCharts, TotalNewCharts);
		ChartSelectUI->OverlayInfoText->SetText(FText::FromString(Text));
	} else
	{
		ChartSelectUI->OverlayInfoText->SetText(FText::FromString(""));
	}
	if(ChartSelectUI->OverlayInfoText->GetText().IsEmpty())
	{
		ChartSelectUI->OverlayInfoBox->SetVisibility(ESlateVisibility::Collapsed);
	} else {
		ChartSelectUI->OverlayInfoBox->SetVisibility(ESlateVisibility::Visible);
	}
	if (FMODSystem == nullptr) return;
	FMODSystem->update();
}

void AChartSelectScreen::OnMediaOpened(FString OpenedUrl)
{
	bool isPlayed = MediaPlayer->Play();
	UE_LOG(LogTemp, Warning, TEXT("MediaPlayer->Play(): %d"), isPlayed);
	
}

void AChartSelectScreen::OnMediaOpenFailed(FString FailedUrl)
{
	UE_LOG(LogTemp, Warning, TEXT("MediaPlayer->OnMediaOpenFailed"));
}

void AChartSelectScreen::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	UE_LOG(LogTemp, Warning, TEXT("ChartSelectScreen EndPlay()!!"));
	bCancelled = true;
	bJukeboxCancelled = true;
	if (JukeboxTask.IsValid())
	{
		JukeboxTask.Wait();
	}
	jukebox->Unload();
	delete jukebox;
	if (BGATask.IsValid())
	{
		BGATask.Wait();
	}
}
