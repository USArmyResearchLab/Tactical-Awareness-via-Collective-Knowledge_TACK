// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class librdkafka : ModuleRules
{
	public librdkafka(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.External;

		PublicIncludePaths.AddRange(
			new string[] {
				Path.Combine(ModuleDirectory, "include")
			}
		);

		if(Target.Platform == UnrealTargetPlatform.Win64){
			
			PublicAdditionalLibraries.Add(
				Path.Combine(ModuleDirectory, "lib/x64-Win/rdkafka.lib")
			);

			RuntimeDependencies.Add("$(TargetOutputDir)/rdkafka.dll", Path.Combine(ModuleDirectory,"lib/x64-Win/rdkafka.dll"));
			RuntimeDependencies.Add("$(BinaryOutputDir)/rdkafka.dll", Path.Combine(ModuleDirectory,"lib/x64-Win/rdkafka.dll"));


			PublicAdditionalLibraries.Add(
				Path.Combine(ModuleDirectory, "lib/x64-Win/lz4.lib")
			);
			
			RuntimeDependencies.Add("$(TargetOutputDir)/lz4.dll", Path.Combine(ModuleDirectory,"lib/x64-Win/lz4.dll"));
			RuntimeDependencies.Add("$(BinaryOutputDir)/lz4.dll", Path.Combine(ModuleDirectory,"lib/x64-Win/lz4.dll"));
		}else if(Target.Platform == UnrealTargetPlatform.Linux)
		{
			PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory,"lib/x64-linux/liblz4.so"));
			RuntimeDependencies.Add("$(TargetOutputDir)/liblz4.so", Path.Combine(ModuleDirectory,"lib/x64-linux/liblz4.so"));
			RuntimeDependencies.Add("$(BinaryOutputDir)/liblz4.so", Path.Combine(ModuleDirectory,"lib/x64-linux/liblz4.so"));

			PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory,"lib/x64-linux/librdkafka.so"));
			RuntimeDependencies.Add("$(TargetOutputDir)/librdkafka.so", Path.Combine(ModuleDirectory,"lib/x64-linux/librdkafka.so"));
			RuntimeDependencies.Add("$(BinaryOutputDir)/librdkafka.so", Path.Combine(ModuleDirectory,"lib/x64-linux/librdkafka.so"));
			

			RuntimeDependencies.Add("$(TargetOutputDir)/librdkafka.so.1", Path.Combine(ModuleDirectory,"lib/x64-linux/librdkafka.so.1"));
			RuntimeDependencies.Add("$(BinaryOutputDir)/librdkafka.so.1", Path.Combine(ModuleDirectory,"lib/x64-linux/librdkafka.so.1"));

		}
		else if(Target.Platform == UnrealTargetPlatform.Android)
		{
			AdditionalPropertiesForReceipt.Add("AndroidPlugin",  Path.Combine(ModuleDirectory,"librdkafka_APL.xml"));

			PublicAdditionalLibraries.Add( Path.Combine(ModuleDirectory,"lib/arm64-v8a/librdkafka.so"));

			RuntimeDependencies.Add(Path.Combine(ModuleDirectory, "lib/arm64-v8a/librdkafka.so"), StagedFileType.NonUFS); //maybe this works

		}
		// else if(Target.Platform == UnrealTargetPlatform.HoloLens)
		// {
		//	PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "lib/arm64-Win/rdkafka.lib"));

			// RuntimeDependencies.Add("$(TargetOutputDir)/rdkafka.dll", Path.Combine(ModuleDirectory, "lib/arm64-Win/rdkafka.dll"));
			// RuntimeDependencies.Add("$(BinaryOutputDir)/rdkafka.dll", Path.Combine(ModuleDirectory, "lib/arm64-Win/rdkafka.dll"));

		//	RuntimeDependencies.Add(Path.Combine(ModuleDirectory, "lib/arm64-Win/rdkafka.dll"), StagedFileType.NonUFS);
		// }
	}
}