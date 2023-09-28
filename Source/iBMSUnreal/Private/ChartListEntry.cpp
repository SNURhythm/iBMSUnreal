// Fill out your copyright notice in the Description page of Project Settings.


#include "ChartListEntry.h"

#include "ChartListEntryData.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"

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

void UChartListEntry::NativeOnListItemObjectSet(UObject* InObject)
{
	EntryData = Cast<UChartListEntryData>(InObject);
	if (EntryData)
	{
		auto ChartMeta = EntryData->ChartMeta;
		TitleText->SetText(FText::FromString(ChartMeta->Title));
		ArtistText->SetText(FText::FromString(ChartMeta->Artist));
		KeyModeText->SetText(FText::FromString(ChartMeta->KeyMode+"k"));
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
		Banner->SetBrushTintColor(FLinearColor::White);
		
		path = FPaths::Combine(ChartMeta->Folder, path);
		IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
		TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(path.EndsWith(".png") ? EImageFormat::PNG : (path.EndsWith(".bmp")? EImageFormat::BMP : EImageFormat::JPEG));
		TArray<uint8> RawFileData;
		if (FFileHelper::LoadFileToArray(RawFileData, *path))
		{
			if (ImageWrapper.IsValid() && ImageWrapper->SetCompressed(RawFileData.GetData(), RawFileData.Num()))
			{
				TArray<uint8> UncompressedBGRA;
				if (ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, UncompressedBGRA))
				{
					UTexture2D* Texture = UTexture2D::CreateTransient(375,100, PF_B8G8R8A8);
					if (Texture != nullptr)
					{
						// crop to 375x100
						// 375x100 is the size of the banner in the game

						// crop
						int32 Width = ImageWrapper->GetWidth();
						int32 Height = ImageWrapper->GetHeight();
						int32 CropWidth = Width;
						int32 CropHeight = Height;
						if(Width > 375) {
							CropWidth = 375;
						}
						if(Height > 100) {
							CropHeight = 100;
						}
						int32 CropX = (Width - CropWidth) / 2;
						int32 CropY = (Height - CropHeight) / 2;
						TArray<uint8> CroppedBGRA;
						CroppedBGRA.SetNumUninitialized(CropWidth * CropHeight * 4);
						for(int32 y = 0; y < CropHeight; y++) {
							for(int32 x = 0; x < CropWidth; x++) {
								int32 srcIndex = ((y + CropY) * Width + x + CropX) * 4;
								int32 dstIndex = (y * CropWidth + x) * 4;
								CroppedBGRA[dstIndex] = UncompressedBGRA[srcIndex];
								CroppedBGRA[dstIndex + 1] = UncompressedBGRA[srcIndex + 1];
								CroppedBGRA[dstIndex + 2] = UncompressedBGRA[srcIndex + 2];
								CroppedBGRA[dstIndex + 3] = UncompressedBGRA[srcIndex + 3];
							}
						}
						// resize
						TArray<uint8> ResizedBGRA;
						ResizedBGRA.SetNumUninitialized(375 * 100 * 4);
						for(int32 y = 0; y < 100; y++) {
							for(int32 x = 0; x < 375; x++) {
								int32 srcIndex = ((y * CropHeight) / 100 * CropWidth + (x * CropWidth) / 375) * 4;
								int32 dstIndex = (y * 375 + x) * 4;
								ResizedBGRA[dstIndex] = CroppedBGRA[srcIndex];
								ResizedBGRA[dstIndex + 1] = CroppedBGRA[srcIndex + 1];
								ResizedBGRA[dstIndex + 2] = CroppedBGRA[srcIndex + 2];
								ResizedBGRA[dstIndex + 3] = CroppedBGRA[srcIndex + 3];
							}
						}
						// copy
						void* TextureData = Texture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
						FMemory::Memcpy(TextureData, ResizedBGRA.GetData(), ResizedBGRA.Num());
						Texture->PlatformData->Mips[0].BulkData.Unlock();
						Texture->UpdateResource();
						Banner->SetBrushFromTexture(Texture);
					}
				}
			}
		}
	}
}
