using UnrealBuildTool;

[SupportedPlatforms("Win64")]
public class ElectricDreamsSampleTargetRulesOverride : TargetRules
{
    public ElectricDreamsSampleTargetRulesOverride(TargetInfo Target) : base(Target)
    {
        // Override settings for packaging
        if (Target.Type == TargetType.Game || Target.Type == TargetType.Editor)
        {
            // Match engine's shared environment
            WindowsPlatform.bStrictConformanceMode = false;
            UndefinedIdentifierWarningLevel = WarningLevel.Off;
            bValidateFormatStrings = false;
        }
    }
}