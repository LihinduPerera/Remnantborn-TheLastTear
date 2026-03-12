#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MapDataAsset.generated.h"

class UTexture2D;
class UWorld;

UCLASS(BlueprintType)
class REMNANTBORN_API UMapDataAsset : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map Info")
    FName MapID;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map Info")
    FText MapName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map Info")
    FText MapDescription;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map Info")
    UTexture2D* MapThumbnail = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map Info")
    TSoftObjectPtr<UWorld> MapLevel;

    UFUNCTION(BlueprintPure, Category = "Map Info")
    FString GetMapPath() const;
};