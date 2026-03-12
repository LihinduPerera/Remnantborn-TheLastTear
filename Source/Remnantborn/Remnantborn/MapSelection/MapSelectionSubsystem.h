#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MapSelectionSubsystem.generated.h"

class UMapDataAsset;

UCLASS()
class REMNANTBORN_API UMapSelectionSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category = "Map Selection")
    void LoadAvailableMaps();

    UFUNCTION(BlueprintCallable, Category = "Map Selection")
    TArray<UMapDataAsset*> GetAvailableMaps() const;

    UFUNCTION(BlueprintCallable, Category = "Map Selection")
    UMapDataAsset* GetMapByID(const FName& MapID) const;

    UFUNCTION(BlueprintCallable, Category = "Map Selection")
    UMapDataAsset* GetDefaultMap() const;

private:
    UPROPERTY()
    TArray<UMapDataAsset*> AvailableMaps;

    void LoadMapsFromDirectory(const FString& DirectoryPath);
};