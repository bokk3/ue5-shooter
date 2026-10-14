using UnrealBuildTool;
using System.Collections.Generic;

public class NobleFermiTarget : TargetRules
{
	public NobleFermiTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("NobleFermi");
	}
}
