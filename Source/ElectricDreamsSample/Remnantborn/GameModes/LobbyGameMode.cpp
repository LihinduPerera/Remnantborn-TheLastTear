#include "LobbyGameMode.h"
#include "ElectricDreamsSample/Remnantborn/OnlineService/MyOnlineGameInstance.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "ElectricDreamsSample/Remnantborn/Widgets/CharacterSelection/CharacterSelectionWidget.h"
#include "ElectricDreamsSample/Remnantborn/CharacterSelection/CharacterSelectionSubsystem.h"
#include "ElectricDreamsSample/Remnantborn/CharacterSelection/CharacterPlayerState.h"
#include "ElectricDreamsSample/Remnantborn/OnlineService/LobbyPlayerController/LobbyPlayerController.h"
#include "ElectricDreamsSample/Remnantborn/GameModes/LobbyGameState.h"

ALobbyGameMode::ALobbyGameMode()
{
    PlayerStateClass = ACharacterPlayerState::StaticClass();
    PlayerControllerClass = ALobbyPlayerController::StaticClass();
    GameStateClass = ALobbyGameState::StaticClass();
}

void ALobbyGameMode::BeginPlay()
{
    Super::BeginPlay();

    UCharacterSelectionSubsystem* CharacterSubsystem = GetGameInstance()->GetSubsystem<UCharacterSelectionSubsystem>();
    if (CharacterSubsystem)
    {
        CharacterSubsystem->LoadAvailableCharacters();
    }
}

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    CurrentPlayerCount++;

    if (ALobbyGameState* LobbyGS = Cast<ALobbyGameState>(GameState))
    {
        LobbyGS->CurrentPlayerCount = CurrentPlayerCount;
    }

    ALobbyPlayerController* LobbyPC = Cast<ALobbyPlayerController>(NewPlayer);
    if (LobbyPC)
    {
        LobbyPC->Client_ShowCharacterSelection();
    }

    UpdateGameState();
}

void ALobbyGameMode::Logout(AController* Exiting)
{
    Super::Logout(Exiting);

    APlayerController* PC = Cast<APlayerController>(Exiting);
    if (PC)
    {
        PlayerCharacterSelections.Remove(PC);

        if (ALobbyGameState* LobbyGS = Cast<ALobbyGameState>(GameState))
        {
            LobbyGS->RemovePlayerInfo(PC);
        }
    }

    CurrentPlayerCount = FMath::Max(0, CurrentPlayerCount - 1);

    if (ALobbyGameState* LobbyGS = Cast<ALobbyGameState>(GameState))
    {
        LobbyGS->CurrentPlayerCount = CurrentPlayerCount;
    }

    UpdateGameState();

    if (bCountdownActive && CurrentPlayerCount < MaxPlayers)
    {
        CancelMatchCountdown();
    }
}

void ALobbyGameMode::OnPlayerReadyChanged(APlayerController* PlayerController, bool bReady)
{
    if (ALobbyGameState* LobbyGS = Cast<ALobbyGameState>(GameState))
    {
        LobbyGS->UpdatePlayerInfo(PlayerController, bReady, PlayerCharacterSelections.Contains(PlayerController));
    }

    UpdateGameState();
}

void ALobbyGameMode::OnPlayerSelectedCharacter(APlayerController* PlayerController, UCharacterDataAsset* SelectedCharacter)
{
    if (PlayerController && SelectedCharacter)
    {
        PlayerCharacterSelections.Add(PlayerController, SelectedCharacter);

        if (ALobbyGameState* LobbyGS = Cast<ALobbyGameState>(GameState))
        {
            LobbyGS->UpdatePlayerInfo(PlayerController,
                Cast<ALobbyPlayerController>(PlayerController) ? Cast<ALobbyPlayerController>(PlayerController)->IsReady() : false,
                true);
        }

        UpdateGameState();

        if (CanStartMatch())
        {
            StartMatchCountdown();
        }
    }
}

void ALobbyGameMode::SetMaxPlayers(int32 NewMaxPlayers)
{
    if (NewMaxPlayers != 2 && NewMaxPlayers != 4)
    {
        UE_LOG(LogTemp, Warning, TEXT("LobbyGameMode: Max players must be 2 or 4"));
        return;
    }

    MaxPlayers = NewMaxPlayers;

    UpdateSessionSettings();

    if (ALobbyGameState* LobbyGS = Cast<ALobbyGameState>(GameState))
    {
        LobbyGS->MaxPlayers = MaxPlayers;
    }

    if (bCountdownActive && CurrentPlayerCount < MaxPlayers)
    {
        CancelMatchCountdown();
    }
}

void ALobbyGameMode::StartMatchCountdown()
{
    if (!CanStartMatch() || bCountdownActive)
    {
        return;
    }

    bCountdownActive = true;
    CountdownTime = CountdownDuration;

    if (ALobbyGameState* LobbyGS = Cast<ALobbyGameState>(GameState))
    {
        LobbyGS->SetCountdownState(true, CountdownTime);
    }

    GetWorld()->GetTimerManager().SetTimer(
        CountdownTimerHandle,
        this,
        &ALobbyGameMode::UpdateCountdown,
        1.0f,
        true
    );
}

void ALobbyGameMode::CancelMatchCountdown()
{
    if (!bCountdownActive)
    {
        return;
    }

    bCountdownActive = false;
    GetWorld()->GetTimerManager().ClearTimer(CountdownTimerHandle);

    if (ALobbyGameState* LobbyGS = Cast<ALobbyGameState>(GameState))
    {
        LobbyGS->SetCountdownState(false, 0);
    }
}

void ALobbyGameMode::StartMatchImmediately()
{
    if (!CanStartMatch())
    {
        return;
    }

    StartMatchTravel();
}

bool ALobbyGameMode::CanStartMatch() const
{
    int32 PlayersWithCharacters = PlayerCharacterSelections.Num();
    return PlayersWithCharacters == MaxPlayers && CurrentPlayerCount == MaxPlayers;
}

void ALobbyGameMode::UpdateCountdown()
{
    CountdownTime--;

    if (ALobbyGameState* LobbyGS = Cast<ALobbyGameState>(GameState))
    {
        LobbyGS->CountdownTime = CountdownTime;
    }

    if (CountdownTime <= 0)
    {
        GetWorld()->GetTimerManager().ClearTimer(CountdownTimerHandle);
        StartMatchTravel();
    }
}

void ALobbyGameMode::StartMatchTravel()
{
    if (!CanStartMatch())
    {
        CancelMatchCountdown();
        return;
    }

    for (auto& Pair : PlayerCharacterSelections)
    {
        if (Pair.Key && Pair.Value)
        {
            ACharacterPlayerState* PlayerState = Pair.Key->GetPlayerState<ACharacterPlayerState>();
            if (PlayerState)
            {
                PlayerState->SetSelectedCharacter(Pair.Value);
            }
        }
    }

    UpdateSessionSettings();

    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        if (ALobbyPlayerController* LobbyPC = Cast<ALobbyPlayerController>(It->Get()))
        {
            LobbyPC->Client_CleanupLobbyWidgets();
        }
    }

    FString TravelPath = GameMapPath + "?listen";
    GetWorld()->ServerTravel(TravelPath, true);
}

void ALobbyGameMode::UpdateSessionSettings()
{
    IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
    if (!OnlineSub)
    {
        return;
    }

    IOnlineSessionPtr SessionInterface = OnlineSub->GetSessionInterface();
    if (!SessionInterface.IsValid())
    {
        return;
    }

    FNamedOnlineSession* ExistingSession = SessionInterface->GetNamedSession(NAME_GameSession);
    if (ExistingSession)
    {
        ExistingSession->SessionSettings.bAllowJoinInProgress = !bCountdownActive;
        ExistingSession->SessionSettings.NumPublicConnections = MaxPlayers;

        SessionInterface->UpdateSession(NAME_GameSession, ExistingSession->SessionSettings, true);
    }
}

void ALobbyGameMode::UpdateGameState()
{
    if (ALobbyGameState* LobbyGS = Cast<ALobbyGameState>(GameState))
    {
        LobbyGS->NotifyStateChanged();
    }
}

UCharacterDataAsset* ALobbyGameMode::GetPlayerSelectedCharacter(APlayerController* PlayerController) const
{
    UCharacterDataAsset* const* FoundCharacter = PlayerCharacterSelections.Find(PlayerController);
    return FoundCharacter ? *FoundCharacter : nullptr;
}
