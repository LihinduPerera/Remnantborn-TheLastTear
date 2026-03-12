#include "MapDataAsset.h"

FString UMapDataAsset::GetMapPath() const
{
    return MapLevel.ToSoftObjectPath().GetLongPackageName();
}