using UnrealBuildTool;

public class NobleFermi : ModuleRules
{
	public NobleFermi(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(new string[] {
			ModuleDirectory
		});

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"UMG",
			"Slate",
			"SlateCore",
			"CommonUI",
			"CommonInput",
			"PhysicsCore",
			"AnimationCore",
			"AnimGraphRuntime"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"AudioMixer"
		});

		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] {
				"UnrealEd",
				"AnimGraph",
				"BlueprintGraph"
			});
		}
	}
}
