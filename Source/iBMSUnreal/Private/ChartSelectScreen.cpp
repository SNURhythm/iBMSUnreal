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
#include "MediaPlayer.h"
#include "iBMSUnreal/Public/Utils.h"
#include "miniz.h"
#if PLATFORM_ANDROID
#include "AndroidNatives.h"
#include "Android/AndroidJNI.h"
#include "Android/AndroidApplication.h"
#include "Android/AndroidPlatform.h"
#endif
#include "tinyfiledialogs.h"
#include "Blueprint/UserWidget.h"
#include "iBMSUnreal/Public/ImageUtils.h"
#include "Kismet/GameplayStatics.h"



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
	UE_LOG(LogTemp, Warning, TEXT("ChartSelectScreen UserHomeDir: %s"), *homeDir);
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
		while (Directory.IsEmpty())
		{
			Directory = tinyfd_selectFolderDialog("Select BMS Folder", nullptr);
		}
		UE_LOG(LogTemp, Warning, TEXT("ChartSelectScreen Directory: %s"), *Directory);
		// normalize path

#endif

		if (!Directory.IsEmpty())
		{
			Directory = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*Directory);
		
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
	UE_LOG(LogTemp, Warning, TEXT("ChartSelectScreen DirectoryAbs: %s"), *DirectoryAbs);
	IFileManager::Get().MakeDirectory(*DirectoryRel);
#elif PLATFORM_ANDROID
	// external storage (/Android/data/[package name]/files)
	FString DirectoryRel = FUtils::GetDocumentsPath("BMS/");
	UE_LOG(LogTemp, Warning, TEXT("ChartSelectScreen DirectoryRel: %s"), *DirectoryRel);
	FString Imported = FUtils::GetDocumentsPath("imported/");
	// read all zips in imported & extract to BMS/zipname
	// mkdir BMS
	IFileManager::Get().MakeDirectory(*DirectoryRel);
	if (IFileManager::Get().DirectoryExists(*Imported))
	{
		IFileManager::Get().IterateDirectory(*Imported, [&](const TCHAR* FilenameOrDirectory, bool bIsDirectory) -> bool
		{
			if (bIsDirectory) return true;
			FString FilePath = FString(FilenameOrDirectory);
			FString FileExtension = FPaths::GetExtension(FilePath);
			if (FileExtension != TEXT("zip")) return true;
			FString FullPath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*FilePath);
			// extract to BMS
			FString Extracted = FPaths::Combine(DirectoryRel, FPaths::GetBaseFilename(FilePath));
			FString ExtractedAbs = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*Extracted);
			IFileManager::Get().MakeDirectory(*ExtractedAbs);
			// extract
			mz_zip_archive zip_archive;
			memset(&zip_archive, 0, sizeof(zip_archive));
			mz_bool status;
			status = mz_zip_reader_init_file(&zip_archive, TCHAR_TO_UTF8(*FullPath), 0);
			if (!status)
			{
				UE_LOG(LogTemp, Warning, TEXT("ChartSelectScreen mz_zip_reader_init_file failed"));
				return true;
			}
			int num_files = mz_zip_reader_get_num_files(&zip_archive);
			for (int i = 0; i < num_files; i++)
			{
				if (bCancelled) break;
				mz_zip_archive_file_stat file_stat;
				if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat))
				{
					UE_LOG(LogTemp, Warning, TEXT("ChartSelectScreen mz_zip_reader_file_stat failed"));
					continue;
				}
				// mkdir
				if (file_stat.m_is_directory)
				{
					FString Directory = FPaths::Combine(Extracted, file_stat.m_filename);
					IFileManager::Get().MakeDirectory(*Directory);
					continue;
				}
				// extract
				FString ExtractedPath = FPaths::Combine(Extracted, file_stat.m_filename);
				UE_LOG(LogTemp, Warning, TEXT("ChartSelectScreen ExtractedPath: %s"), *ExtractedPath);
				if (!mz_zip_reader_extract_to_file(&zip_archive, i, TCHAR_TO_UTF8(*ExtractedPath), 0))
				{
					UE_LOG(LogTemp, Warning, TEXT("ChartSelectScreen mz_zip_reader_extract_to_file failed"));
					continue;
				}
			}
			mz_zip_reader_end(&zip_archive);
			// delete zip
			IFileManager::Get().Delete(*FullPath);
			return true;
		});
	}

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
		UE_LOG(LogTemp, Warning, TEXT("ChartSelectScreen SoundPath: %s"), *SoundPath);


		FMODSystem->createSound(TCHAR_TO_ANSI(*SoundPath), FMOD_DEFAULT, 0, &SuccessSound);
		UE_LOG(LogTemp, Warning, TEXT("ChartSelectScreen Start Task!!"));

		auto chartMetas = dbHelper.SelectAllChartMeta(db);
		chartMetas.Sort([](const FChartMeta& A, const FChartMeta& B)
		{
			return A.Title < B.Title;
		});
		
		AsyncTask(ENamedThreads::GameThread, [chartMetas, this]()
		{
			if (bCancelled) return;
			SetChartMetas(chartMetas);
			auto prevStartOption = Cast<UBMSGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()))->GetStartOptions();
			if (prevStartOption.BmsPath.IsEmpty()) return;
			// get index
			int index = chartMetas.IndexOfByPredicate([&](const FChartMeta* ChartMeta)
			{
				return ChartMeta->BmsPath == prevStartOption.BmsPath;
			});
			if (index == INDEX_NONE) return;
			ChartSelectUI->ChartList->NavigateToIndex(index);
			ChartSelectUI->ChartList->SetSelectedIndex(index);
		});
		
		// find new charts
		TArray<FDiff> Diffs;
		TSet<FString> PathSet;
		SuccessNewChartCount = 0;
		// TODO: Initialize prevPathSet by db
		for (auto& meta : chartMetas)
		{
			if (bCancelled) break;
			auto path = ChartDBHelper::ToRelativePath(meta->BmsPath);
			PathSet.Add(path);
		}
		IFileManager& FileManager = IFileManager::Get();

		UE_LOG(LogTemp, Warning, TEXT("ChartSelectScreen FindNew"));
		// print bCancelled
		UE_LOG(LogTemp, Warning, TEXT("ChartSelectScreen isCancelled: %d"), (bool)bCancelled);
		for (auto& entry : entries)
		{
			if (bCancelled) break;
			FindNew(Diffs, PathSet, entry, bCancelled);
		}
		UE_LOG(LogTemp, Warning, TEXT("ChartSelectScreen FindNew Done: %d"), Diffs.Num());
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

		if (Diffs.Num() == 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("ChartSelectScreen No new charts"));
			IsScanning = false;
			return;
		}
	
		const bool bSupportMultithreading = FPlatformProcess::SupportsMultithreading();
		const int TaskNum = FMath::Max(FPlatformMisc::NumberOfWorkerThreadsToSpawn() / 2, 1);
		UE_LOG(LogTemp, Warning, TEXT("ChartSelectScreen taskNum: %d"), TaskNum);
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
						if (!isCommitting)
						{
							isCommitting = true;
							dbHelper.CommitTransaction(db);
							dbHelper.BeginTransaction(db);
							isCommitting = false;
						}
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
	
		if (bCancelled) goto scan_end;

		AsyncTask(ENamedThreads::GameThread, [this]()
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
			SetChartMetas(chartMetas);
			// restore selection with CurrentEntryData
			if (CurrentEntryData)
			{
				auto chartMeta = CurrentEntryData->ChartMeta;
				// get index
				int index = chartMetas.IndexOfByPredicate([&](const FChartMeta* ChartMeta)
				{
					return ChartMeta->BmsPath == chartMeta->BmsPath;
				});
				if (index == INDEX_NONE) return;
				ChartSelectUI->ChartList->NavigateToIndex(index);
				ChartSelectUI->ChartList->SetSelectedIndex(index);
			}
			
		});
		
		UE_LOG(LogTemp, Warning, TEXT("ChartSelectScreen End Task"));
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
void AChartSelectScreen::OnMediaOpened(FString OpenedUrl)
{
	auto dim = MediaPlayer->GetVideoTrackDimensions(0, 0);
	UE_LOG(LogTemp, Warning, TEXT("Video Dimension: %d, %d"), dim.X, dim.Y);
	ChartSelectUI->BackgroundSizeBox->SetWidthOverride(dim.X);
	ChartSelectUI->BackgroundSizeBox->SetHeightOverride(dim.Y);
}

void AChartSelectScreen::OnMediaOpenFailed(FString FailedUrl)
{
	UE_LOG(LogTemp, Warning, TEXT("MediaPlayer->OnMediaOpenFailed"));
	
}

void AChartSelectScreen::OnPlaybackResumed()
{
	// ChartSelectUI->BackgroundImage->SetBrushFromMaterial(VideoMaterial);
}

void AChartSelectScreen::OnSeekCompleted()
{
	// MediaPlayer->Play();
	UE_LOG(LogTemp, Warning, TEXT("SeekCompleted"));
}

// Called when the game starts or when spawned
void AChartSelectScreen::BeginPlay()
{
	Super::BeginPlay();
	MediaPlayer->OnMediaOpened.AddDynamic(this, &AChartSelectScreen::OnMediaOpened);
	MediaPlayer->OnPlaybackResumed.AddDynamic(this, &AChartSelectScreen::OnPlaybackResumed);
	MediaPlayer->OnSeekCompleted.AddDynamic(this, &AChartSelectScreen::OnSeekCompleted);
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
					BackgroundImageLock.Lock();
					ChartSelectUI->BackgroundImage->SetBrushFromTexture(nullptr);
					ChartSelectUI->BackgroundImage->SetBrushTintColor(FLinearColor::Black);
					BackgroundImageLock.Unlock();
					ChartSelectUI->StageFileImage->SetBrushFromTexture(nullptr);
					ChartSelectUI->StageFileImage->SetBrushTintColor(FLinearColor::Black);
				}
				else
				{
					ImagePath = FPaths::Combine(chartMeta->Folder, ImagePath);
					TArray<uint8> ImageBytes;
					FFileHelper::LoadFileToArray(ImageBytes, *ImagePath);
					ImageUtils::LoadTexture2D(ImagePath, ImageBytes, IsImageValid, -1, -1, BackgroundImage);
					if (IsImageValid)
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
					UE_LOG(LogTemp, Warning, TEXT("Loading sound"));

					jukebox->Stop();
					jukebox->LoadChart(Chart,  bJukeboxCancelled, MediaPlayer);
					if(bJukeboxCancelled)
					{
						jukebox->Unload();
						delete Chart;
						return;
					}
					if(!jukebox->BGASourceMap.IsEmpty())
					{
						AsyncTask(ENamedThreads::GameThread, [&]()
						{
							if(bJukeboxCancelled) return;
							BackgroundImageLock.Lock();
							if(IsValid(ChartSelectUI) && IsValid(ChartSelectUI->BackgroundImage))
							{
								ChartSelectUI->BackgroundImage->SetBrushTintColor(FLinearColor(0.5f, 0.5f, 0.5f, 0.5f));
								ChartSelectUI->BackgroundImage->SetBrushFromMaterial(VideoMaterial);
							}
							BackgroundImageLock.Unlock();
						});
					} else
					{
						AsyncTask(ENamedThreads::GameThread, [&, ImagePath]()
						{
							if(bJukeboxCancelled) return;
							if(!ImagePath.IsEmpty())
							{
								UTexture2D* Image = nullptr;
								bool IsTextureValid = false;
								TArray<uint8> ImageBytes;
								FFileHelper::LoadFileToArray(ImageBytes, *ImagePath);
								ImageUtils::LoadTexture2D(ImagePath, ImageBytes, IsTextureValid, -1, -1, Image);
								if(IsTextureValid)
								{
									BackgroundImageLock.Lock();
									if(IsValid(ChartSelectUI) && IsValid(ChartSelectUI->BackgroundImage))
									{
										ChartSelectUI->BackgroundImage->SetBrushTintColor(FLinearColor(0.5f, 0.5f, 0.5f, 0.5f));
										ChartSelectUI->BackgroundImage->SetBrushFromTexture(Image);
									}
									BackgroundImageLock.Unlock();
								}
							}
						});
					}
					UE_LOG(LogTemp, Warning, TEXT("Loading sound done"));
					if (bJukeboxCancelled)
					{
						jukebox->Unload();
						delete Chart;
						return;
					}
					jukebox->Start(bCancelled, 0, true);
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
	if (!IsValid(ChartSelectUI)) return;
	if (!IsValid(ChartSelectUI->ChartList)) return;
	ChartListLock.Lock();
	// convert to UChartListEntryData
	TArray<UChartListEntryData*> EntryDatas;
	for (auto& meta : ChartMetas)
	{
		auto entryData = NewObject<UChartListEntryData>();
		entryData->ChartMeta = meta;
		EntryDatas.Add(entryData);
	}
	ChartSelectUI->ChartList->SetListItems(EntryDatas);
	ChartListLock.Unlock();
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
	if(jukebox)
	{
		jukebox->OnGameTick();
	}
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
	UE_LOG(LogTemp, Warning, TEXT("ChartSelectScreen jukebox unloaded"));
	if (BGATask.IsValid())
	{
		BGATask.Wait();
	}
	BackgroundImageLock.Lock();
	ChartListLock.Lock();
	MediaPlayer->Close();
}
