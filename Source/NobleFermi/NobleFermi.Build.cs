using UnrealBuildTool;

public class NobleFermi : ModuleRules
{
	public NobleFermi(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

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
			"PhysicsCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"AudioMixer"
		});
	}
}
