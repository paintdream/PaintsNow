// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.
using System.IO;

namespace UnrealBuildTool.Rules
{
	public class LeavesExporter : ModuleRules
	{
		private string ThirdPartyPath
		{
			get { return Path.GetFullPath(Path.Combine(ModuleDirectory, "../../../../../../Build64/")); }
		}

		public LeavesExporter(ReadOnlyTargetRules Target) : base(Target)
		{
			PrivatePCHHeaderFile = "Private/LeavesExporterPCH.h";
			MinFilesUsingPrecompiledHeaderOverride = 1;
			PublicIncludePaths.AddRange(
				new string[] {
					// ... add public include paths required here ...
				}
				);

			PrivateIncludePaths.AddRange(
				new string[] {
					"Developer/LeavesExporter/Private",
					// ... add other private include paths required here ...
				}
				);

			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core", "CoreUObject", "Engine", "InputCore", "SlateCore", "Slate", "UnrealEd",
					"LevelEditor", "EditorStyle", "Projects", "SceneOutliner", "Foliage", "Landscape"
					// ... add other public dependencies that you statically link with here ...
				}
				);

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
				}
				);


			DynamicallyLoadedModuleNames.AddRange(
				new string[]
				{
					// ... add any modules that your module loads dynamically here ...
				}
				);

			LoadPaintsNow(Target);
		}

		// from: https://answers.unrealengine.com/questions/76792/link-to-3rd-party-libraries.html
		public bool LoadPaintsNow(ReadOnlyTargetRules Target)
		{
			bool isLibrarySupported = false;

			/////where to pick the library if we're building for windows (32 or 64)
			if (Target.Platform == UnrealTargetPlatform.Win64)
			{
				isLibrarySupported = true;

				UnrealTargetConfiguration configure = Target.Configuration;

				string modulePath = Path.Combine(ThirdPartyPath, "RelWithDebInfo");
				PublicAdditionalLibraries.Add(Path.Combine(modulePath, "PaintsNow.lib"));
				PublicAdditionalLibraries.Add(Path.Combine(modulePath, "BridgeSunset.lib"));
				PublicAdditionalLibraries.Add(Path.Combine(modulePath, "GalaxyWeaver.lib"));
				PublicAdditionalLibraries.Add(Path.Combine(modulePath, "MythForest.lib"));
				PublicAdditionalLibraries.Add(Path.Combine(modulePath, "SnowyStream.lib"));

				string libraryPath = Path.Combine(ThirdPartyPath, "Source/General/Driver/Network/LibEvent/Core/lib/RelWithDebInfo");
				PublicAdditionalLibraries.Add(Path.Combine(libraryPath, "event.lib"));
				PublicAdditionalLibraries.Add(Path.Combine(libraryPath, "event_core.lib"));
				PublicAdditionalLibraries.Add(Path.Combine(libraryPath, "event_extra.lib"));
			}

			return isLibrarySupported;
		}
	}
}
