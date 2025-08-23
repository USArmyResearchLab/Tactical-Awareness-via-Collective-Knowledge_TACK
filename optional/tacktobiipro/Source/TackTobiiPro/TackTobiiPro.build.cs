using UnrealBuildTool;
using System;

public class TackTobiiPro : ModuleRules
{
	public bool IsExternalPluginInstalled(string PluginName)
	{
		foreach(PluginInfo PInfo in Plugins.ReadProjectPlugins(Target.ProjectFile.Directory))
		{
			if(PInfo.Name == PluginName)
				return true;
		}
		return false;
	}

	public TackTobiiPro(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		bEnableUndefinedIdentifierWarnings = false;
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Tack",
				"JsonUtilities",
				"Json",
				"Engine",
			}
		);

		PrivateDependencyModuleNames.Add("TobiiPro");
		
	}
}