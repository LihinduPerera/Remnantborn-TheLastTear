#include "MapSelectionSubsystem.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "MapDataAsset.h"
#include "Misc/Paths.h"

void UMapSelectionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    LoadAvailableMaps();
}

void UMapSelectionSubsystem::Deinitialize()
{
    AvailableMaps.Empty();

    Super::Deinitialize();
}

void UMapSelectionSubsystem::LoadAvailableMaps()
{
    AvailableMaps.Empty();
    LoadMapsFromDirectory(TEXT("/Game/Remnantborn/Blueprints/DataStructures/MapDataAsset/"));
}

TArray<UMapDataAsset*> UMapSelectionSubsystem::GetAvailableMaps() const
{
    return AvailableMaps;
}

UMapDataAsset* UMapSelectionSubsystem::GetMapByID(const FName& MapID) const
{
    for (UMapDataAsset* MapAsset : AvailableMaps)
    {
        if (MapAsset && MapAsset->MapID == MapID)
        {
            return MapAsset;
        }
    }

    return nullptr;
}

UMapDataAsset* UMapSelectionSubsystem::GetDefaultMap() const
{
    return AvailableMaps.Num() > 0 ? AvailableMaps[0] : nullptr;
}

void UMapSelectionSubsystem::LoadMapsFromDirectory(const FString& DirectoryPath)
{
    FString PackagePath = DirectoryPath;
    if (!PackagePath.StartsWith(TEXT("/Game/")))
    {
        PackagePath = FPaths::Combine(TEXT("/Game"), PackagePath);
    }

    TArray<FAssetData> AssetDataArray;
    FAssetRegistryModule& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

    FARFilter Filter;
    Filter.PackagePaths.Add(*PackagePath);
    Filter.bRecursivePaths = true;
    Filter.ClassPaths.Add(UMapDataAsset::StaticClass()->GetClassPathName());

    AssetRegistry.Get().GetAssets(Filter, AssetDataArray);

    for (const FAssetData& AssetData : AssetDataArray)
    {
        UMapDataAsset* MapAsset = Cast<UMapDataAsset>(AssetData.GetAsset());
        if (MapAsset)
        {
            AvailableMaps.Add(MapAsset);
        }
    }

    AvailableMaps.Sort([](const UMapDataAsset& A, const UMapDataAsset& B) {
        return A.MapID.FastLess(B.MapID);
    });
}