using UnrealBuildTool;

public class TackXR : ModuleRules
{
	public TackXR(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"InputCore",
				"Tack",
				"CoreUObject",
				"Engine",
				"HeadMountedDisplay",
				"Json",
				"JsonUtilities",
				"XRBase"
			}
		);
	}
}