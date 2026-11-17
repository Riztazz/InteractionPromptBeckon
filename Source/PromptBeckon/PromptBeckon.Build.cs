using UnrealBuildTool;

public class PromptBeckon : ModuleRules
{
	public PromptBeckon(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.NoPCHs;
		bUseUnity = false;
		IWYUSupport = IWYUSupport.Full;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"GameplayTags",
			"UMG",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"DeveloperSettings",
			"Slate",
			"SlateCore",
		});
	}
}
