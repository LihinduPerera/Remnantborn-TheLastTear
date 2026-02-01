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

void ALobbyGameState::NotifyStateChanged()
{
    OnLobbyStateChanged.Broadcast();
}
