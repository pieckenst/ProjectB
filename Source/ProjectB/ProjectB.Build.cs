// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ProjectB : ModuleRules
{
    public ProjectB(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "EnhancedInput",
            "PBCharacterMovement",
            "AIModule",
            "StateTreeModule",
            "GameplayStateTreeModule",
            "UMG",
            "Slate"
        });

        PrivateDependencyModuleNames.AddRange(new string[] { });

        PublicIncludePaths.AddRange(new string[] {
            "ProjectB",
            "ProjectB/Variant_Horror",
            "ProjectB/Variant_Horror/UI",
            "ProjectB/Variant_Shooter",
            "ProjectB/Variant_Shooter/AI",
            "ProjectB/Variant_Shooter/UI",
            "ProjectB/Variant_Shooter/Weapons"
        });

        // Uncomment if you are using Slate UI
        // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
    }
}