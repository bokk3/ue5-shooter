using UnrealBuildTool;
using System.Collections.Generic;

public class NobleFermiEditorTarget : TargetRules
{
	public NobleFermiEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("NobleFermi");
	}
}
