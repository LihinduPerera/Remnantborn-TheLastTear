#include "LobbyPlayerController.h"
#include "ElectricDreamsSample/Remnantborn/GameModes/LobbyGameMode.h"
#include "GameFramework/GameModeBase.h"
#include "Net/UnrealNetwork.h"

void ALobbyPlayerController::BeginPlay()
{
    Super::BeginPlay();
    SetupInputMode();
}

void ALobbyPlayerController::SetupInputMode()
{
    // Set UI input mode for lobby
    FInputModeUIOnly InputMode;
    SetInputMode(InputMode);
    SetShowMouseCursor(true);
    bEnableClickEvents = true;
    bEnableMouseOverEvents = true;
}

bool ALobbyPlayerController::IsHost() const
{
    if (GetWorld())
    {
        AGameModeBase* GameMode = GetWorld()->GetAuthGameMode();
        if (GameMode)
        {
            return GetNetMode() == NM_ListenServer || GetNetMode() == NM_DedicatedServer;
        }
    }
    return false;
}

void ALobbyPlayerController::SetPlayerReady(bool bReady)
{
    if (bIsReady != bReady)
    {
        bIsReady = bReady;
        OnRep_IsReady();
    }
}

void ALobbyPlayerController::OnRep_IsReady()
{
    // Notify UI or other systems about ready state change
    // This will be handled by the lobby widget
}

void ALobbyPlayerController::StartMatchCountdown()
{
    if (!IsHost())
    {
        return;
    }
    
    ALobbyGameMode* LobbyGameMode = Cast<ALobbyGameMode>(GetWorld()->GetAuthGameMode());
    if (LobbyGameMode)
    {
        LobbyGameMode->StartMatchCountdown();
    }
}

void ALobbyPlayerController::CancelMatchCountdown()
{
    if (!IsHost())
    {
        return;
    }
    
    ALobbyGameMode* LobbyGameMode = Cast<ALobbyGameMode>(GetWorld()->GetAuthGameMode());
    if (LobbyGameMode)
    {
        LobbyGameMode->CancelMatchCountdown();
    }
}

void ALobbyPlayerController::SetMaxPlayers(int32 MaxPlayers)
{
    if (!IsHost())
    {
        return;
    }
    
    ALobbyGameMode* LobbyGameMode = Cast<ALobbyGameMode>(GetWorld()->GetAuthGameMode());
    if (LobbyGameMode)
    {
        LobbyGameMode->SetMaxPlayers(MaxPlayers);
    }
}

void ALobbyPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
    DOREPLIFETIME(ALobbyPlayerController, bIsReady);
}
