// Fill out your copyright notice in the Description page of Project Settings.


#include "ChartListEntry.h"

#include "ChartListEntryData.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "iBMSUnreal/Public/ImageUtils.h"
#include "Tasks/Task.h"

void UChartListEntry::OnButtonClicked()
{
	if (OnEntryClicked.IsBound())
	{
		OnEntryClicked.Execute(EntryData);
	}
}

void UChartListEntry::NativeConstruct()
{
	Super::NativeConstruct();
	//Button->OnClicked.AddDynamic(this, &UChartListEntry::OnButtonClicked);
}
void UChartListEntry::NativeDestruct()
{
	Super::NativeDestruct();
	FScopeLock Lock(&BannerTextureLock);
	if(IsValid(CurrentBannerTexture)) {
		CurrentBannerTexture->ReleaseResource();
		CurrentBannerTexture->GetPlatformData()->Mips[0].BulkData.RemoveBulkData();
		CurrentBannerTexture = nullptr;
	}
}
void UChartListEntry::NativeOnListItemObjectSet(UObject* InObject)
{
	EntryData = Cast<UChartListEntryData>(InObject);
	if (EntryData)
	{
		auto ChartMeta = EntryData->ChartMeta;
		TitleText->SetText(FText::FromString(ChartMeta->Title + (ChartMeta->SubTitle.IsEmpty() ? "" : " " + ChartMeta->SubTitle)));
		ArtistText->SetText(FText::FromString(ChartMeta->Artist));
		KeyModeText->SetText(FText::FromString(FString::Printf(TEXT("%dK"), ChartMeta->KeyMode)));
		PlayLevelText->SetText(FText::FromString(FString::Printf(TEXT("%g"), ChartMeta->PlayLevel)));
		PlayLevelText->SetShadowColorAndOpacity(FLinearColor::Black);
		// set playlevel text color by difficulty (1~5)
		switch(ChartMeta->Difficulty)
		{
		case 1:
			PlayLevelText->SetColorAndOpacity(FLinearColor::Green);
			break;
		case 2:
			PlayLevelText->SetColorAndOpacity(FLinearColor::Blue);
			break;
		case 3:
			PlayLevelText->SetColorAndOpacity(FLinearColor::Yellow);
			break;
		case 4:
			PlayLevelText->SetColorAndOpacity(FLinearColor::Red);
			break;
		case 5:
			PlayLevelText->SetColorAndOpacity(FLinearColor::Black);
			// set shadow color to white
			PlayLevelText->SetShadowColorAndOpacity(FLinearColor::White);
			break;
		default:
			PlayLevelText->SetColorAndOpacity(FLinearColor::White);
			break;
		}
		auto path = ChartMeta->Banner;
		if(path.IsEmpty()) {
			path = ChartMeta->StageFile;
		}
		if(path.IsEmpty()) {
			path = ChartMeta->BackBmp;
		}
		if(path.IsEmpty()) {
			path = ChartMeta->Preview;
		}
		if(path.IsEmpty()) {
			// set to blank, black
			Banner->SetBrushFromTexture(nullptr);
			Banner->SetBrushTintColor(FLinearColor::Black);
			return;
		}
		Banner->SetBrushTintColor(FLinearColor::Black);
		Banner->SetBrushFromTexture(nullptr);

		path = FPaths::Combine(ChartMeta->Folder, path);
		UE::Tasks::Launch(UE_SOURCE_LOCATION, [this, path]() {

			TArray<uint8>* ImageBytes = new TArray<uint8>();
			FFileHelper::LoadFileToArray(*ImageBytes, *path);
			
			
			AsyncTask(ENamedThreads::GameThread, [this, path, ImageBytes]()
			{
				FScopeLock Lock(&BannerTextureLock);
				if(IsValid(CurrentBannerTexture)) {
					CurrentBannerTexture->ReleaseResource();
					CurrentBannerTexture->GetPlatformData()->Mips[0].BulkData.RemoveBulkData();
					CurrentBannerTexture = nullptr;
				}
				// check if entry is re-bound
				if (EntryData == nullptr || EntryData->ChartMeta == nullptr || EntryData->ChartMeta->Folder != FPaths::GetPath(path))
				{
					UE_LOG(LogTemp, Warning, TEXT("ChartListEntry: Entry is re-bound, aborting image load"));
					delete ImageBytes;
					return;
				}
				UTexture2D* Texture = nullptr;
				bool IsImageValid = false;
				ImageUtils::LoadTexture2D(path, *ImageBytes, IsImageValid, 375, 100, Texture, false);
				delete ImageBytes;
				if (IsImageValid)
				{
					// this macro is a hacky way to avoid winbase.h's UpdateResource macro
					#define UpdateResource UpdateResource
					Texture->UpdateResource();
					#undef UpdateResource
					CurrentBannerTexture = Texture;
					Banner->SetBrushTintColor(FLinearColor::White);
					Banner->SetBrushFromTexture(Texture);
				}
				else
				{
					Banner->SetBrushFromTexture(nullptr);
					Banner->SetBrushTintColor(FLinearColor::Black);
				}
			});
		});

		

	}
}
