// Copyright Epic Games, Inc. All Rights Reserved.

using System;
using UnrealBuildTool;

public class iBMSUnreal : ModuleRules
{
	public iBMSUnreal(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "Paper2D", "MediaAssets" });

		PrivateDependencyModuleNames.AddRange(new string[] {  "MediaAssets"  });
		
		// Uncomment if you are using Slate UI
		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		PrivateDependencyModuleNames.AddRange(new string[] { "ImageWrapper", "RenderCore", "Paper2D"});
		// fmod
		PublicIncludePaths	.Add(System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/fmod/include"));
		string configName;
		if (Target.Configuration != UnrealTargetConfiguration.Shipping)
		{
			configName = "L";
			PublicDefinitions.Add("FMODSTUDIO_LINK_LOGGING=1");
		}
		else
		{
			configName = "";
			PublicDefinitions.Add("FMODSTUDIO_LINK_RELEASE=1");
		}
		// Add CoreGraphics, bz2,...
		PublicIncludePaths.Add(System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/include"));
		if (Target.Platform == UnrealTargetPlatform.IOS)
		{
			PublicFrameworks.AddRange(new string[]
			{
				"CoreAudio", "AudioToolbox", "VideoToolbox", "AVKit", "CoreImage",
				"CoreMedia", "Accelerate"
			});
			PublicSystemLibraries.AddRange(new string[]
			{
				"bz2", "iconv", "c++", "z"
			});
            PublicAdditionalLibraries.Add(System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/lib/ios/libavcodec.a"));
            PublicAdditionalLibraries.Add(System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/lib/ios/libavdevice.a"));
            PublicAdditionalLibraries.Add(System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/lib/ios/libavfilter.a"));
            PublicAdditionalLibraries.Add(System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/lib/ios/libavformat.a"));
            PublicAdditionalLibraries.Add(System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/lib/ios/libavutil.a"));
            PublicAdditionalLibraries.Add(System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/lib/ios/libswresample.a"));
            PublicAdditionalLibraries.Add(System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/lib/ios/libswscale.a"));
            PublicAdditionalLibraries.Add(System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/lib/ios/libpostproc.a"));
            PublicAdditionalLibraries.Add(System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/lib/ios/libx264.a"));
            
            // fmod
            PublicAdditionalLibraries.Add(System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/fmod/lib/ios/libfmod"+configName+".a"));
		} else if (Target.Platform == UnrealTargetPlatform.Mac){
			PublicFrameworks.AddRange(new string[]
			{
				"Cocoa", "CoreFoundation", "IOKit", "CoreAudio", "AudioToolbox", "VideoToolbox", "AVKit", "CoreImage",
				"CoreMedia", "OpenCL", "Accelerate", "OpenGL", "Metal"
			});
			PublicSystemLibraries.AddRange(new string[]
			{
				"bz2", "iconv", "c++", "z", "lzma"
			});
			
			PublicAdditionalLibraries.Add(System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/lib/mac/libx264_uni.a"));
			PublicAdditionalLibraries.Add(System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/lib/mac/libx265_uni.a"));
            PublicAdditionalLibraries.Add(System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/lib/mac/libavcodec_uni.a"));
            PublicAdditionalLibraries.Add(System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/lib/mac/libavdevice_uni.a"));
            PublicAdditionalLibraries.Add(System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/lib/mac/libavfilter_uni.a"));
            PublicAdditionalLibraries.Add(System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/lib/mac/libavformat_uni.a"));
            PublicAdditionalLibraries.Add(System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/lib/mac/libavutil_uni.a"));
            PublicAdditionalLibraries.Add(System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/lib/mac/libswresample_uni.a"));
            PublicAdditionalLibraries.Add(System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/lib/mac/libswscale_uni.a"));
            PublicAdditionalLibraries.Add(System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/lib/mac/libpostproc_uni.a"));
            
            // fmod
            PublicAdditionalLibraries.Add(System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/fmod/lib/mac/libfmod"+configName+".dylib"));
            // add runtime
            RuntimeDependencies.Add(System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/fmod/lib/mac/libfmod"+configName+".dylib"));
        } else if (Target.Platform == UnrealTargetPlatform.Win64){
			PublicAdditionalLibraries.Add(System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/lib/win64/avcodec.lib"));
			PublicAdditionalLibraries.Add(System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/lib/win64/avdevice.lib"));
			PublicAdditionalLibraries.Add(System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/lib/win64/avfilter.lib"))	;
			PublicAdditionalLibraries.Add(System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/lib/win64/avformat.lib"));
			PublicAdditionalLibraries.Add(System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/lib/win64/avutil.lib"));
			PublicAdditionalLibraries.Add(System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/lib/win64/swresample.lib"));
			PublicAdditionalLibraries.Add(System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/lib/win64/swscale.lib"));
			PublicAdditionalLibraries.Add(System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/lib/win64/postproc.lib"));
			PublicAdditionalLibraries.Add(System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/lib/win64/libx264.a"));
			// add corresponding dll
			RuntimeDependencies.Add("$(BinaryOutputDir)/"+"avcodec-60.dll", System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/lib/win64/avcodec-60.dll"));
			RuntimeDependencies.Add("$(BinaryOutputDir)/"+"avdevice-60.dll",System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/lib/win64/avdevice-60.dll"));
			RuntimeDependencies.Add("$(BinaryOutputDir)/"+"avfilter-9.dll",System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/lib/win64/avfilter-9.dll"));
			RuntimeDependencies.Add("$(BinaryOutputDir)/"+"avformat-60.dll",System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/lib/win64/avformat-60.dll"));
			RuntimeDependencies.Add("$(BinaryOutputDir)/"+"avutil-58.dll",System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/lib/win64/avutil-58.dll"));
			RuntimeDependencies.Add("$(BinaryOutputDir)/"+"swresample-4.dll",System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/lib/win64/swresample-4.dll"));
			RuntimeDependencies.Add("$(BinaryOutputDir)/"+"swscale-7.dll",System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/lib/win64/swscale-7.dll"));
			RuntimeDependencies.Add("$(BinaryOutputDir)/"+"postproc-57.dll",System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/lib/win64/postproc-57.dll"));
			//RuntimeDependencies.Add("$(BinaryOutputDir)/"+"libx264-164.dll",System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/win64/libx264-164.dll"));
			
			// fmod
			// _vc.lib
			PublicAdditionalLibraries.Add(System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/fmod/lib/win64/fmod"+configName+"_vc.lib"));

			// add runtime
			RuntimeDependencies.Add("$(BinaryOutputDir)/"+"fmod"+configName+".dll",System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/fmod/lib/win64/fmod"+configName+".dll"));

			PublicDelayLoadDLLs.Add("fmod"+configName+".dll");
        } else if (Target.IsInPlatformGroup(UnrealPlatformGroup.Android))
		{
            // jni
            PrivateDependencyModuleNames.AddRange(new string[] { "Launch" });
			string[] archs = new string[] { "arm64-v8a", "x86_64" };
			foreach (string arch in archs)
			{
				PublicAdditionalLibraries.Add(System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/lib/android/" + arch + "/libavcodec.so"));
				PublicAdditionalLibraries.Add(System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/lib/android/" + arch + "/libavdevice.so"));
				PublicAdditionalLibraries.Add(System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/lib/android/" + arch + "/libavfilter.so"));
				PublicAdditionalLibraries.Add(System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/lib/android/" + arch + "/libavformat.so"));
				PublicAdditionalLibraries.Add(System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/lib/android/" + arch + "/libavutil.so"));
				PublicAdditionalLibraries.Add(System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/lib/android/" + arch + "/libswresample.so"));
				PublicAdditionalLibraries.Add(System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/lib/android/" + arch + "/libswscale.so"));
				PublicAdditionalLibraries.Add(System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/ffmpeg/lib/android/" + arch + "/libpostproc.so"));
				
				// fmod
				PublicAdditionalLibraries.Add(System.IO.Path.Combine(ModuleDirectory, "../ThirdParty/fmod/lib/android/" + arch + "/libfmod"+".so"));
				
				string RelAPLPath = Utils.MakePathRelativeTo(System.IO.Path.Combine(ModuleDirectory, "APL.xml"), Target.RelativeEnginePath);
				System.Console.WriteLine("Adding {0}", RelAPLPath);
				AdditionalPropertiesForReceipt.Add("AndroidPlugin", RelAPLPath);
			}
		}

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
