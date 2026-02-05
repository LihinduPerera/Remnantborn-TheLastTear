#include "RemnantbornGameSession.h"

ARemnantbornGameSession::ARemnantbornGameSession()
{
    // GameSession persists through seamless travel by default
}

void ARemnantbornGameSession::StorePlayerCharacterSelection(int32 PlayerId, const FName& CharacterID)
{
    if (PlayerId >= 0 && !CharacterID.IsNone())
    {
        PlayerCharacterSelections.Add(PlayerId, CharacterID);
        UE_LOG(LogTemp, Log, TEXT("GameSession: Stored character selection %s for player %d"), 
            *CharacterID.ToString(), PlayerId);
    }
}

FName ARemnantbornGameSession::GetPlayerCharacterSelection(int32 PlayerId) const
{
    if (const FName* FoundSelection = PlayerCharacterSelections.Find(PlayerId))
    {
        return *FoundSelection;
    }
    return NAME_None;
}

bool ARemnantbornGameSession::HasPlayerCharacterSelection(int32 PlayerId) const
{
    return PlayerCharacterSelections.Contains(PlayerId);
}

void ARemnantbornGameSession::RemovePlayerCharacterSelection(int32 PlayerId)
{
    if (PlayerCharacterSelections.Remove(PlayerId) > 0)
    {
        UE_LOG(LogTemp, Log, TEXT("GameSession: Removed character selection for player %d"), PlayerId);
    }
}

void ARemnantbornGameSession::ClearAllCharacterSelections()
{
    PlayerCharacterSelections.Empty();
    UE_LOG(LogTemp, Log, TEXT("GameSession: Cleared all character selections"));
}
