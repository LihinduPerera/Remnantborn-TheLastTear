#include "RemnantbornGameSession.h"

ARemnantbornGameSession::ARemnantbornGameSession()
{
    // GameSession persists through seamless travel by default
}

void ARemnantbornGameSession::StorePlayerCharacterSelection(const FString& PlayerName, const FName& CharacterID)
{
    if (!PlayerName.IsEmpty() && !CharacterID.IsNone())
    {
        PlayerCharacterSelections.Add(PlayerName, CharacterID);
        UE_LOG(LogTemp, Log, TEXT("GameSession: Stored character selection %s for player %s"), 
            *CharacterID.ToString(), *PlayerName);
    }
}

FName ARemnantbornGameSession::GetPlayerCharacterSelection(const FString& PlayerName) const
{
    if (const FName* FoundSelection = PlayerCharacterSelections.Find(PlayerName))
    {
        return *FoundSelection;
    }
    return NAME_None;
}

bool ARemnantbornGameSession::HasPlayerCharacterSelection(const FString& PlayerName) const
{
    return PlayerCharacterSelections.Contains(PlayerName);
}

void ARemnantbornGameSession::RemovePlayerCharacterSelection(const FString& PlayerName)
{
    if (PlayerCharacterSelections.Remove(PlayerName) > 0)
    {
        UE_LOG(LogTemp, Log, TEXT("GameSession: Removed character selection for player %s"), *PlayerName);
    }
}

void ARemnantbornGameSession::ClearAllCharacterSelections()
{
    PlayerCharacterSelections.Empty();
    UE_LOG(LogTemp, Log, TEXT("GameSession: Cleared all character selections"));
}
