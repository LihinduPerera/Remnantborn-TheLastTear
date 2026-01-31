// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ElectricDreamsSample : ModuleRules
{
	public ElectricDreamsSample(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] 
		{ 
			"Core", 
			"CoreUObject", 
			"Engine", 
			"InputCore",
			"GameplayAbilities",
			"ContextualAnimation",
			"GameplayTags",
			"GameplayTasks",
			"GameplayBehaviorsModule",
			"DeveloperSettings",
			"OnlineSubsystem",
			"OnlineSubsystemUtils",
			"Networking",
			"Sockets",
			"Slate",
			"SlateCore",
			"UMG",
			"OnlineSubsystemNull",
			"JsonUtilities"  // Add this if not present
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"RHI",
			"AudioModulation",
			"GameplayAbilities",
			"GameplayTasks",
			"GameplayTags",
			"HTTP",
			"Json",
			"JsonUtilities",
			"WebSockets",
			"OnlineSubsystem"
		});
		
		if (Target.Configuration == UnrealTargetConfiguration.Shipping)
		{
			PublicDefinitions.Add("UE_BUILD_SHIPPING=1");
		}

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
