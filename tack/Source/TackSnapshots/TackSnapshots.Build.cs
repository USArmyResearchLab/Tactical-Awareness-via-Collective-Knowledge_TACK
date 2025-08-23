using UnrealBuildTool;

public class TackSnapshots : ModuleRules
{
	public TackSnapshots(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"Tack",
				"CoreUObject",
				"Engine",
				"MediaIOCore",		// Snapshot system
				"ImageWriteQueue", // Snapshot system
				"Json",
				"JsonUtilities"	
			}
		);
	}
}