// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class RemnantbornEditorTarget : TargetRules
{
	public RemnantbornEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
        
		// Use V6 and match engine settings exactly
		DefaultBuildSettings = BuildSettingsVersion.V6;
        
		// Use 5_7 to match engine
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
        
		// Use C++20 as required
		CppStandard = CppStandardVersion.Cpp20;
        
		ExtraModuleNames.AddRange(new string[] { "Remnantborn" });
        
		// IMPORTANT: Match engine's shared environment exactly
		// Don't override anything - let engine use its defaults
        
		// Force the use of shared environment (this is key)
		bOverrideBuildEnvironment = false;
	}
}
