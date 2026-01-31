#include "PlayerListEntryWidget.h"
#include "Components/TextBlock.h"

void UPlayerListEntryWidget::SetPlayerInfo(const FString& PlayerName, bool bIsReady)
{
    if (PlayerNameText)
    {
        PlayerNameText->SetText(FText::FromString(PlayerName));
    }
    
    if (ReadyStatusText)
    {
        FString Status = bIsReady ? "Ready" : "Not Ready";
        ReadyStatusText->SetText(FText::FromString(Status));
    }
}
