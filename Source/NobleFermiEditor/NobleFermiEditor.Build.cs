using UnrealBuildTool;

/** Editor-only tooling for Noble Fermi (asset pipeline helpers called from Scripts/ via editor Python). */
public class NobleFermiEditor : ModuleRules
{
	public NobleFermiEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"UnrealEd",
			"BlueprintGraph",
			"AnimGraph",
			"AnimGraphRuntime",
			"NobleFermi"
		});
	}
}
