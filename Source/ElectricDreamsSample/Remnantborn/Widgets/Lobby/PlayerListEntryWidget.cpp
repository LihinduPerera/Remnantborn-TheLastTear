#include "PlayerListEntryWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"

void UPlayerListEntryWidget::SetPlayerInfo(const FString& Name, bool bReady, bool bCharacter)
{
    PlayerName = Name;
    bIsReady = bReady;
    bHasCharacter = bCharacter;

    UpdateVisuals();
}

void UPlayerListEntryWidget::UpdateVisuals()
{
    if (PlayerNameText)
    {
        PlayerNameText->SetText(FText::FromString(PlayerName));
    }

    if (ReadyStatusText)
    {
        if (bHasCharacter)
        {
            ReadyStatusText->SetText(bIsReady ? FText::FromString("Ready") : FText::FromString("Not Ready"));
            ReadyStatusText->SetColorAndOpacity(bIsReady ? FSlateColor(FLinearColor::Green) : FSlateColor(FLinearColor::Yellow));
        }
        else
        {
            ReadyStatusText->SetText(FText::FromString("Selecting..."));
            ReadyStatusText->SetColorAndOpacity(FSlateColor(FLinearColor::Gray));
        }
    }

    if (CharacterIcon)
    {
        CharacterIcon->SetVisibility(bHasCharacter ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
    }
}
