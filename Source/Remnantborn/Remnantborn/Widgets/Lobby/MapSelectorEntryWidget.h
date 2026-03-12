#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MapSelectorEntryWidget.generated.h"

class UButton;
class UImage;
class UMapDataAsset;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMapEntrySelected, FName, MapID);

UCLASS()
class REMNANTBORN_API UMapSelectorEntryWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    UFUNCTION(BlueprintCallable, Category = "Map Entry")
    void SetMapInfo(UMapDataAsset* MapData, bool bIsSelected);

    UPROPERTY(BlueprintAssignable, Category = "Map Entry|Events")
    FOnMapEntrySelected OnMapSelected;

protected:
    UPROPERTY(meta = (BindWidget))
    UTextBlock* MapNameText;

    UPROPERTY(meta = (BindWidget))
    UImage* MapThumbnailImage;

    UPROPERTY(meta = (BindWidget))
    UButton* SelectButton;

private:
    UPROPERTY()
    UMapDataAsset* CurrentMapData = nullptr;

    bool bSelected = false;

    UFUNCTION()
    void HandleSelectButtonClicked();

    void UpdateVisuals();
};