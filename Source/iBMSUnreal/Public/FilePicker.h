#pragma once
// cross-platform (Windows, Linux, Mac) file picker
class FFilePicker
{
public:
	FString PickDirectory(const FString& Title);
	FString PickFile(const FString& Title, const TArray<TPair<FString, FString>>& FileTypes);
	
};
