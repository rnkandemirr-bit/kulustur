using UnrealBuildTool;

public class kulustur : ModuleRules
{
	public kulustur(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "Niagara", "Slate", "SlateCore", "UMG", "RenderCore", "EnhancedInput" });
	}
}
