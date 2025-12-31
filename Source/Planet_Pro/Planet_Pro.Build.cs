// Planet_Pro.Build.cs

using UnrealBuildTool;

public class Planet_Pro : ModuleRules
{
	public Planet_Pro(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { 
			"Core", 
			"CoreUObject", 
			"Engine", 
			"InputCore", 
			"Json", 
			"JsonUtilities", 
			// ▼ 여기 스펠링, 대소문자 정확해야 합니다!
			"PlayFab", 
			"PlayFabCpp", 
			"PlayFabCommon" 
		});
	}
}