using UnrealBuildTool;

public class TackHttpServer : ModuleRules
{
	public TackHttpServer(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"DeveloperSettings",
				"Tack",
				"HTTPServer"
			}
		);
	}
}
