#pragma once

class FUtils
{
public:
	inline static FString GameName = "iBMSUnreal";
	inline static FString TeamName = "SNURhythm";
	static FString GetDocumentsPath(FString SubPath)
	{
#if PLATFORM_IOS
		return FPaths::Combine(GetIOSDocumentsPath(), SubPath);
#else
		// for other platforms, use Documents
		return FPaths::Combine(FPlatformProcess::UserDir(), TeamName, GameName, SubPath);
#endif
	}
};
