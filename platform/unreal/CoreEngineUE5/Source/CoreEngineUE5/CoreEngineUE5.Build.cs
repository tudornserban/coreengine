// Copyright CoreEngine. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class CoreEngineUE5 : ModuleRules
{
	public CoreEngineUE5(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		bUseUnity = false;
		CppStandard = CppStandardVersion.Default; // UE 5.4+ requires C++20; CoreEngine C++17 compiles cleanly under C++20+
		bEnableExceptions = true;
		bUseRTTI = true; // Required for CoreEngine interface dynamic_cast gates (IUnitKinematics, IUnitAgent, etc.)

		// Resolve CoreEngine root whether the plugin is used in-place from platform/unreal/CoreEngineUE5
		// or copied to <UE5Project>/Plugins/CoreEngineUE5 with a bundled CoreEngine root.
		string BundledCoreRoot = Path.GetFullPath(Path.Combine(ModuleDirectory, "..", "..", "ThirdParty", "CoreEngine"));
		string InRepoCoreRoot = Path.GetFullPath(Path.Combine(ModuleDirectory, "..", "..", "..", ".."));
		string ActiveCoreRoot = Directory.Exists(Path.Combine(BundledCoreRoot, "core"))
			? BundledCoreRoot
			: InRepoCoreRoot;

		PublicIncludePaths.AddRange(new string[] {
			Path.Combine(ModuleDirectory, "Public")
		});

		PrivateIncludePaths.AddRange(new string[] {
			Path.Combine(ModuleDirectory, "Private")
		});

		// Add CoreEngine root as a system include path so UE5's -Werror=shadow / strict
		// warning flags do not fail on pure-C++17 CoreEngine headers (e.g. Vector2.h).
		PublicSystemIncludePaths.Add(ActiveCoreRoot);

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"Slate",
			"SlateCore",
			"UMG"
		});

		PublicDefinitions.Add("COREENGINE_WITH_UE5=1");
	}
}
