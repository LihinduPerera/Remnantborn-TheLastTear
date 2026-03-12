#include "MapSelectorEntryWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Remnantborn/Remnantborn/MapSelection/MapDataAsset.h"

void UMapSelectorEntryWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (SelectButton)
    {
        SelectButton->OnClicked.AddDynamic(this, &UMapSelectorEntryWidget::HandleSelectButtonClicked);
    }
}

void UMapSelectorEntryWidget::NativeDestruct()
{
    if (SelectButton)
    {
        SelectButton->OnClicked.RemoveAll(this);
    }

    Super::NativeDestruct();
}

void UMapSelectorEntryWidget::SetMapInfo(UMapDataAsset* MapData, bool bIsInSelected)
{
    CurrentMapData = MapData;
    bSelected = bIsInSelected;
    UpdateVisuals();
}

void UMapSelectorEntryWidget::HandleSelectButtonClicked()
{
    if (CurrentMapData)
    {
        OnMapSelected.Broadcast(CurrentMapData->MapID);
    }
}

void UMapSelectorEntryWidget::UpdateVisuals()
{
    if (MapNameText)
    {
        MapNameText->SetText(CurrentMapData ? CurrentMapData->MapName : FText::GetEmpty());
    }

    if (MapThumbnailImage)
    {
        MapThumbnailImage->SetBrushFromTexture(CurrentMapData ? CurrentMapData->MapThumbnail : nullptr, true);
        MapThumbnailImage->SetOpacity(bSelected ? 1.0f : 0.75f);
    }

    if (SelectButton)
    {
        SelectButton->SetIsEnabled(CurrentMapData && !bSelected);
        SelectButton->SetRenderOpacity(bSelected ? 0.65f : 1.0f);
    }
}