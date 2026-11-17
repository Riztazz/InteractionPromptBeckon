using UnrealBuildTool;

public class PromptBeckonEditor : ModuleRules
{
	public PromptBeckonEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.NoPCHs;
		bUseUnity = false;
		IWYUSupport = IWYUSupport.Full;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"PromptBeckon",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Kismet",
			"KismetCompiler",
			"UnrealEd",
		});
	}
}
