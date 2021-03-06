// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

using UnrealBuildTool;

public class Timelines : ModuleRules
{
	public Timelines(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		bUseUnity = false;

		PublicDependencyModuleNames.AddRange(
			new[]
			{
				"Core",
				"AIModule",
				"UMG",
				"DeveloperSettings"
			});

		PrivateDependencyModuleNames.AddRange(
			new []
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore"
			});
	}
}