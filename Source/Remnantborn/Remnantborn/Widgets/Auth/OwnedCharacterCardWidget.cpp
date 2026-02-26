#include "OwnedCharacterCardWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"

void UOwnedCharacterCardWidget::InitializeCard(UCharacterDataAsset* CharacterData, bool bIsPurchased)
{
    if (!CharacterData)
    {
        return;
    }

    if (CharacterPortrait && CharacterData->CharacterPortrait)
    {
        CharacterPortrait->SetBrushFromTexture(CharacterData->CharacterPortrait);
    }

    if (CharacterName)
    {
        CharacterName->SetText(CharacterData->CharacterName);
    }

    if (StatusBadge)
    {
        StatusBadge->SetText(FText::FromString(bIsPurchased ? TEXT("Purchased") : TEXT("Free")));
    }
}
