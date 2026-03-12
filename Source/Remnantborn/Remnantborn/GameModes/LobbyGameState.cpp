#include "LobbyGameState.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"

ALobbyGameState::ALobbyGameState()
{
    MaxPlayers = 2;
    CurrentPlayerCount = 0;
    bCountdownActive = false;
    CountdownTime = 0;
    bLobbyLocked = false;
    SelectedMapID = NAME_None;
}

void ALobbyGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ALobbyGameState, MaxPlayers);
    DOREPLIFETIME(ALobbyGameState, CurrentPlayerCount);
    DOREPLIFETIME(ALobbyGameState, bCountdownActive);
    DOREPLIFETIME(ALobbyGameState, CountdownTime);
    DOREPLIFETIME(ALobbyGameState, PlayerInfoArray);
    DOREPLIFETIME(ALobbyGameState, bLobbyLocked);
    DOREPLIFETIME(ALobbyGameState, SelectedMapID);
}

void ALobbyGameState::UpdatePlayerInfo(APlayerController* PlayerController, bool bReady, bool bHasCharacter)
{
    if (!PlayerController)
    {
        return;
    }

    APlayerState* PS = PlayerController->GetPlayerState<APlayerState>();
    int32 PlayerId = PS ? PS->GetPlayerId() : -1;

    FString PlayerName = TEXT("Player");
    if (PS)
    {
        PlayerName = PS->GetPlayerName();
    }

    bool bFound = false;
    for (int32 i = 0; i < PlayerInfoArray.Num(); ++i)
    {
        if (PlayerInfoArray[i].PlayerId == PlayerId)
        {
            PlayerInfoArray[i].bIsReady = bReady;
            PlayerInfoArray[i].bHasSelectedCharacter = bHasCharacter;
            bFound = true;
            break;
        }
    }

    if (!bFound)
    {
        FLobbyPlayerInfo NewInfo;
        NewInfo.PlayerId = PlayerId;
        NewInfo.PlayerName = PlayerName;
        NewInfo.bIsReady = bReady;
        NewInfo.bHasSelectedCharacter = bHasCharacter;
        PlayerInfoArray.Add(NewInfo);
        
        UE_LOG(LogTemp, Log, TEXT("LobbyGameState: Added player %s (ID: %d) - Ready: %s, HasCharacter: %s"), 
            *PlayerName, PlayerId, bReady ? TEXT("Yes") : TEXT("No"), bHasCharacter ? TEXT("Yes") : TEXT("No"));
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("LobbyGameState: Updated player %s (ID: %d) - Ready: %s, HasCharacter: %s"), 
            *PlayerName, PlayerId, bReady ? TEXT("Yes") : TEXT("No"), bHasCharacter ? TEXT("Yes") : TEXT("No"));
    }
}

void ALobbyGameState::RemovePlayerInfo(APlayerController* PlayerController)
{
    if (!PlayerController)
    {
        return;
    }

    APlayerState* PS = PlayerController->GetPlayerState<APlayerState>();
    int32 PlayerId = PS ? PS->GetPlayerId() : -1;

    for (int32 i = PlayerInfoArray.Num() - 1; i >= 0; --i)
    {
        if (PlayerInfoArray[i].PlayerId == PlayerId)
        {
            PlayerInfoArray.RemoveAt(i);
            break;
        }
    }
}

void ALobbyGameState::SetCountdownState(bool bActive, int32 Time)
{
    bCountdownActive = bActive;
    CountdownTime = Time;
}

void ALobbyGameState::SetLobbyLocked(bool bLocked)
{
    bLobbyLocked = bLocked;
}

void ALobbyGameState::SetSelectedMap(FName NewMapID)
{
    if (SelectedMapID == NewMapID)
    {
        return;
    }

    SelectedMapID = NewMapID;
    NotifyStateChanged();
}

void ALobbyGameState::OnRep_MaxPlayers()
{
    OnLobbyStateChanged.Broadcast();
}

void ALobbyGameState::OnRep_CurrentPlayerCount()
{
    OnLobbyStateChanged.Broadcast();
}

void ALobbyGameState::OnRep_CountdownActive()
{
    OnLobbyStateChanged.Broadcast();
}

void ALobbyGameState::OnRep_CountdownTime()
{
    OnLobbyStateChanged.Broadcast();
}

void ALobbyGameState::OnRep_PlayerInfoArray()
{
    OnLobbyStateChanged.Broadcast();
}

void ALobbyGameState::OnRep_LobbyLocked()
{
    OnLobbyStateChanged.Broadcast();
}

void ALobbyGameState::OnRep_SelectedMapID()
{
    OnLobbyStateChanged.Broadcast();
}

TArray<FLobbyCharacterInfo> ALobbyGameState::GetLobbyCharacters() const
{
    if (CharacterManager)
    {
        return CharacterManager->GetAllSpawnedCharacters();
    }
    return TArray<FLobbyCharacterInfo>();
}

int32 ALobbyGameState::GetSpawnedCharacterCount() const
{
    if (CharacterManager)
    {
        return CharacterManager->GetAllSpawnedCharacters().Num();
    }
    return 0;
}

void ALobbyGameState::NotifyStateChanged()
{
    OnLobbyStateChanged.Broadcast();
}
