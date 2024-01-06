// Fill out your copyright notice in the Description page of Project Settings.


#include "iBMSUnreal/Public/ImageUtils.h"

#include "IImageWrapper.h"
#include "IImageWrapperModule.h"


void ImageUtils::LoadTexture2D(const FString ImageFileName, const TArray<uint8>& RawFileData, bool& IsValid, int32 TargetWidth, int32 TargetHeight, UTexture2D*& OutTexture, bool UpdateResource)
{
	IsValid = false;
	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
	TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(ImageFileName.EndsWith(".png") ? EImageFormat::PNG : (ImageFileName.EndsWith(".bmp")? EImageFormat::BMP : EImageFormat::JPEG));


	if (ImageWrapper.IsValid() && ImageWrapper->SetCompressed(RawFileData.GetData(), RawFileData.Num()))
	{
		TArray<uint8> UncompressedBGRA;
		if (ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, UncompressedBGRA))
		{
			// crop
			int32 Width = ImageWrapper->GetWidth();
			int32 Height = ImageWrapper->GetHeight();
			int32 CropWidth = Width;
			int32 CropHeight = Height;
			if(TargetWidth<0)
			{
				TargetWidth = Width;
			}
			if(TargetHeight<0)
			{
				TargetHeight = Height;
			}
			if(Width > TargetWidth) {
				CropWidth = TargetWidth;
			}
			if(Height > TargetHeight) {
				CropHeight = TargetHeight;
			}
				
			OutTexture = UTexture2D::CreateTransient(TargetWidth,TargetHeight, PF_B8G8R8A8);
				
			if (OutTexture != nullptr)
			{
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
				ResizedBGRA.SetNumUninitialized(TargetWidth * TargetHeight * 4);
				for(int32 y = 0; y < TargetHeight; y++) {
					for(int32 x = 0; x < TargetWidth; x++) {
						int32 srcIndex = ((y * CropHeight) / TargetHeight * CropWidth + (x * CropWidth) / TargetWidth) * 4;
						int32 dstIndex = (y * TargetWidth + x) * 4;
						ResizedBGRA[dstIndex] = CroppedBGRA[srcIndex];
						ResizedBGRA[dstIndex + 1] = CroppedBGRA[srcIndex + 1];
						ResizedBGRA[dstIndex + 2] = CroppedBGRA[srcIndex + 2];
						ResizedBGRA[dstIndex + 3] = CroppedBGRA[srcIndex + 3];
					}
				}
				// copy
				void* TextureData = OutTexture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
				FMemory::Memcpy(TextureData, ResizedBGRA.GetData(), ResizedBGRA.Num());
				OutTexture->GetPlatformData()->Mips[0].BulkData.Unlock();
				IsValid = true;
				if(UpdateResource)
				{
					// this macro is a hacky way to avoid winbase.h's UpdateResource macro
					#define UpdateResource UpdateResource
					OutTexture->UpdateResource();
					#undef UpdateResource
				}
			}
		}
	}
}
