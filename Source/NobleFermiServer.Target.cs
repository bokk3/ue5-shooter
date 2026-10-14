using UnrealBuildTool;
using System.Collections.Generic;

public class NobleFermiServerTarget : TargetRules
{
	public NobleFermiServerTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Server;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("NobleFermi");
		bUseChecksInShipping = true;
	}
}
