// Planet_Pro.Build.cs

using UnrealBuildTool;

public class Planet_Pro : ModuleRules
{
	public Planet_Pro(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateDependencyModuleNames.AddRange(new string[] { "AdvancedSessions" });
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { 
			"Core", 
			"CoreUObject", 
			"Engine", 
			"InputCore", 
			"Json", 
			"JsonUtilities", 
			"PlayFab", 
			"PlayFabCpp", 
			"PlayFabCommon" ,
			"UMG", 
			"Slate", 
			"SlateCore"
		});
	}
}