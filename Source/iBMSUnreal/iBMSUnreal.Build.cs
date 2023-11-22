// Copyright Epic Games, Inc. All Rights Reserved.

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
		// Add CoreGraphics for Mac
		if (Target.Platform == UnrealTargetPlatform.Mac)
        {
            PublicFrameworks.AddRange(new string[] { "Cocoa", "Carbon", "CoreGraphics", "CoreFoundation", "IOKit" });
            // add includepath for CoreGraphics
            PublicIncludePaths.AddRange(new string[] { "/System/Library/Frameworks/CoreGraphics.framework/Headers",  "/System/Library/Frameworks/Cocoa.framework/Headers" ,  "/System/Library/Frameworks/Carbon.framework/Headers"});
        }
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
