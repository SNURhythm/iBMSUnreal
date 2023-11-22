// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class iBMSUnrealTarget : TargetRules
{
	public iBMSUnrealTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V4;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_3;
		ExtraModuleNames.Add("iBMSUnreal");
		//
		// bBuildDeveloperTools = true;
		// bCompileRecast = false;
		// bCompileSpeedTree = false;
		// bCompileCEF3 = false;
		// bCompileFreeType = false;
		// bCompileRecast = false;
		// bCompileNavmeshSegmentLinks = false;
		// bCompileNavmeshClusterLinks = false;
		// bUseLauncherChecks = false;
	}
}
