#include "CharacterEntryWidget.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Remnantborn/Remnantborn/CharacterSelection/CharacterDataAsset.h"

void UCharacterEntryWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (SelectButton)
    {
        SelectButton->OnClicked.AddDynamic(this, &UCharacterEntryWidget::OnSelectButtonClicked);
    }

    bIsSelected = false;
}

void UCharacterEntryWidget::InitializeEntry(UCharacterDataAsset* InCharacterData)
{
    CharacterData = InCharacterData;

    if (CharacterData)
    {
        // Set image
        if (CharacterImage && CharacterData->CharacterPortrait)
        {
            CharacterImage->SetBrushFromTexture(CharacterData->CharacterPortrait);
        }

        // Set name
        if (CharacterNameText)
        {
            CharacterNameText->SetText(CharacterData->CharacterName);
        }
    }
}

void UCharacterEntryWidget::SetSelected(bool bInIsSelected)
{
    bIsSelected = bInIsSelected;

    // Update visual state
    if (SelectButton)
    {
        // Change button appearance based on selection
        FLinearColor ButtonColor = bIsSelected ? FLinearColor::Green : FLinearColor::White;
        SelectButton->SetBackgroundColor(ButtonColor);
    }
}

void UCharacterEntryWidget::OnSelectButtonClicked()
{
    if (CharacterData && !bIsSelected)
    {
        SetSelected(true);
        OnCharacterSelected.Broadcast(CharacterData);
    }
}