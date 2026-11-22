// Copyright CoreEngine. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class CoreEngineCore : ModuleRules
{
	public CoreEngineCore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		bUseUnity = false;
		CppStandard = CppStandardVersion.Default;
		bEnableExceptions = true;
		// RTTI is enabled ONLY in CoreEngineCore (which has zero UCLASS/UObject subclasses)
		// so CoreEngine's dynamic_cast interface gates work cleanly on macOS/Windows/Linux
		// without pulling in missing typeinfo symbols for AActor/AHUD/AGameModeBase.
		bUseRTTI = true;

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
			"Core"
		});

		PublicDefinitions.Add("COREENGINE_WITH_UE5=1");
	}
}
