// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class RemnantbornTarget : TargetRules
{
	public RemnantbornTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
        
		// Use V6 and match engine settings exactly
		DefaultBuildSettings = BuildSettingsVersion.V6;
        
		// Use 5_7 to match engine
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
        
		// Use C++20 as required
		CppStandard = CppStandardVersion.Cpp20;
        
		ExtraModuleNames.AddRange(new string[] { "Remnantborn" });
        
		// IMPORTANT: Match engine's shared environment exactly
		// Don't override anything - let engine use its defaults
	}
}
