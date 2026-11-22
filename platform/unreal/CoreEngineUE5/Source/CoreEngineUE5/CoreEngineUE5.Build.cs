// Copyright CoreEngine. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class CoreEngineUE5 : ModuleRules
{
	public CoreEngineUE5(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		bUseUnity = false;
		CppStandard = CppStandardVersion.Default;
		bEnableExceptions = true;
		// Keep RTTI disabled on UCLASS modules so AActor/AHUD/AGameModeBase link cleanly on macOS arm64.
		// All RTTI/dynamic_cast logic lives inside the CoreEngineCore dependency module.
		bUseRTTI = false;

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

		PublicSystemIncludePaths.Add(ActiveCoreRoot);

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"CoreEngineCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"Slate",
			"SlateCore",
			"UMG"
		});

		PublicDefinitions.Add("COREENGINE_WITH_UE5=1");
	}
}
