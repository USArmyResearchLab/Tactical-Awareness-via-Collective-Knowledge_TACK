using UnrealBuildTool;
using System;

public class Tack : ModuleRules
{
	public Tack(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"GameplayTags",
				"AIModule",
				"DeveloperSettings",
				"Json",
				"JsonUtilities"
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Engine",
				"Slate",
				"SlateCore",
				"Landscape",
				"EyeTracker",
				"InputCore",
				"Networking",
				"Sockets",
				"Foliage",
				"librdkafka",
				"NetCore"
			}
		);
	}
}
