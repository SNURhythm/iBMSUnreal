// Copyright Epic Games, Inc. All Rights Reserved.

using System;
using UnrealBuildTool;

public class iBMSUnreal : ModuleRules
{
	public iBMSUnreal(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "Paper2D"});

		PrivateDependencyModuleNames.AddRange(new string[] { "FMODStudio" });
		
		// Uncomment if you are using Slate UI
		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		PrivateDependencyModuleNames.AddRange(new string[] { "ImageWrapper", "RenderCore", "Paper2D"});
		// Add CoreGraphics, bz2,...
		if (Target.Platform == UnrealTargetPlatform.Mac)
		{
			PublicFrameworks.AddRange(new string[]
			{
				"Cocoa", "CoreFoundation", "IOKit", "CoreAudio", "AudioToolbox", "VideoToolbox", "AVKit", "CoreImage",
				"CoreMedia", "OpenCL", "Accelerate", "OpenGL", "Metal"
			});
			PublicSystemLibraries.AddRange(new string[]
			{
				"bz2", "iconv", "c++", "z"
			});

		}
		string ThirdPartyPath = System.IO.Path.Combine(ModuleDirectory, "../../ThirdParty/");
		if (Target.Platform == UnrealTargetPlatform.IOS)
		{
            PublicAdditionalLibraries.Add(System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/ios/libavcodec.a"));
            PublicAdditionalLibraries.Add(System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/ios/libavdevice.a"));
            PublicAdditionalLibraries.Add(System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/ios/libavfilter.a"));
            PublicAdditionalLibraries.Add(System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/ios/libavformat.a"));
            PublicAdditionalLibraries.Add(System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/ios/libavutil.a"));
            PublicAdditionalLibraries.Add(System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/ios/libswresample.a"));
            PublicAdditionalLibraries.Add(System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/ios/libswscale.a"));
            // add corresponding include path
            PublicIncludePaths.Add(System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/ios/include"));
		} else if (Target.Platform == UnrealTargetPlatform.Mac){
			PublicAdditionalLibraries.Add(System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/mac/libx264.a"));
			PublicAdditionalLibraries.Add(System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/mac/libx265.a"));
            PublicAdditionalLibraries.Add(System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/mac/libavcodec.a"));
            PublicAdditionalLibraries.Add(System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/mac/libavdevice.a"));
            PublicAdditionalLibraries.Add(System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/mac/libavfilter.a"));
            PublicAdditionalLibraries.Add(System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/mac/libavformat.a"));
            PublicAdditionalLibraries.Add(System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/mac/libavutil.a"));
            PublicAdditionalLibraries.Add(System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/mac/libswresample.a"));
            PublicAdditionalLibraries.Add(System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/mac/libswscale.a"));
            PublicAdditionalLibraries.Add(System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/mac/libpostproc.a"));
          
            // add corresponding include path
            PublicIncludePaths.Add(System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/mac/include"));
        }

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
