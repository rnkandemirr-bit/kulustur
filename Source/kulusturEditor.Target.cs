using UnrealBuildTool;

public class kulusturEditorTarget : TargetRules
{
	public kulusturEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("kulustur");
	}
}
