// Fill out your copyright notice in the Description page of Project Settings.

using System.IO;
using UnrealBuildTool;

public class  JoltPhysicsTest : ModuleRules
{
	public JoltPhysicsTest(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		
		/*string JoltPath = Path.Combine(ModuleDirectory, "../ThirdParty/JoltPhysics");
		PublicIncludePaths.Add(JoltPath);
		
		string LibPath = Path.Combine(JoltPath, "Build", "Windows", "Distribution");
		PublicAdditionalLibraries.Add(Path.Combine(LibPath, "Jolt.lib"));
		
		System.Console.WriteLine("Jolt Include Path: " + Path.Combine(JoltPath, "Jolt"));*/
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "UnrealJoltPhysics", "AnimationCore", "SteamIntegrationKit" });

		PrivateDependencyModuleNames.AddRange(new string[] { "EnhancedInput", "Niagara", "ProceduralMeshComponent" });
		PrivateDependencyModuleNames.AddRange(new string[] { "GeometryCore", "GeometryFramework" });
		
		
		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
