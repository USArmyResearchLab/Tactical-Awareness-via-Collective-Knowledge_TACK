// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class Rapidjson : ModuleRules
{
    public Rapidjson(ReadOnlyTargetRules Target) : base(Target)
    {
        Type = ModuleType.External;

        PublicIncludePaths.AddRange(
            new string[] {
                Path.Combine(ModuleDirectory, "include")
            }
        );
    }
}