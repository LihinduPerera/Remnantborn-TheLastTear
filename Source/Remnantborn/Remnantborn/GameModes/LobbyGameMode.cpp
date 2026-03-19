#include "LobbyGameMode.h"
#include "Remnantborn/Remnantborn/OnlineService/MyOnlineGameInstance.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Remnantborn/Remnantborn/Widgets/CharacterSelection/CharacterSelectionWidget.h"
#include "Remnantborn/Remnantborn/CharacterSelection/CharacterSelectionSubsystem.h"
#include "Remnantborn/Remnantborn/CharacterSelection/CharacterPlayerState.h"
#include "Remnantborn/Remnantborn/MapSelection/MapDataAsset.h"
#include "Remnantborn/Remnantborn/MapSelection/MapSelectionSubsystem.h"
#include "Remnantborn/Remnantborn/OnlineService/LobbyPlayerController/LobbyPlayerController.h"
#include "Remnantborn/Remnantborn/GameModes/LobbyGameState.h"
#include "Remnantborn/Remnantborn/Lobby/LobbyCharacterManager.h"

ALobbyGameMode::ALobbyGameMode()
{
    PlayerStateClass = ACharacterPlayerState::StaticClass();
    PlayerControllerClass = ALobbyPlayerController::StaticClass();
    GameStateClass = ALobbyGameState::StaticClass();
    GameSessionClass = ARemnantbornGameSession::StaticClass();
}

void ALobbyGameMode::BeginPlay()
{
    Super::BeginPlay();

    InitializeCharacterManager();

    UCharacterSelectionSubsystem* CharacterSubsystem = GetGameInstance()->GetSubsystem<UCharacterSelectionSubsystem>();
    if (CharacterSubsystem)
    {
        CharacterSubsystem->LoadAvailableCharacters();
    }

    if (UMapSelectionSubsystem* MapSubsystem = GetGameInstance()->GetSubsystem<UMapSelectionSubsystem>())
    {
        if (UMapDataAsset* DefaultMap = MapSubsystem->GetDefaultMap())
        {
            SetSelectedMap(DefaultMap->MapID);
        }
    }

    // Notify GameInstance that we've entered the lobby - this ensures music continues seamlessly
    if (UMyOnlineGameInstance* GameInstance = Cast<UMyOnlineGameInstance>(GetGameInstance()))
    {
        GameInstance->OnEnteredLobby();
        UE_LOG(LogTemp, Log, TEXT("LobbyGameMode: Called OnEnteredLobby to continue music"));
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

        // Remove lobby character when player leaves
        DespawnLobbyCharacterForPlayer(PC);

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
    
    // Cancel countdown if a player becomes unready during countdown
    if (!bReady && bCountdownActive)
    {
        CancelMatchCountdown();
    }
}

void ALobbyGameMode::OnPlayerSelectedCharacter(APlayerController* PlayerController, UCharacterDataAsset* SelectedCharacter)
{
    if (PlayerController && SelectedCharacter)
    {
        PlayerCharacterSelections.Add(PlayerController, SelectedCharacter);

        // Spawn or update the 3D character in lobby
        SpawnLobbyCharacterForPlayer(PlayerController);

        if (ALobbyGameState* LobbyGS = Cast<ALobbyGameState>(GameState))
        {
            LobbyGS->UpdatePlayerInfo(PlayerController,
                Cast<ALobbyPlayerController>(PlayerController) ? Cast<ALobbyPlayerController>(PlayerController)->IsReady() : false,
                true);
        }

        UpdateGameState();

        // Don't auto-start countdown here anymore
        // Match should only start when all players are explicitly ready
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

void ALobbyGameMode::SetSelectedMap(FName MapID)
{
    if (MapID.IsNone())
    {
        return;
    }

    UMapSelectionSubsystem* MapSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMapSelectionSubsystem>() : nullptr;
    if (!MapSubsystem || !MapSubsystem->GetMapByID(MapID))
    {
        UE_LOG(LogTemp, Warning, TEXT("LobbyGameMode: Attempted to select invalid map ID %s"), *MapID.ToString());
        return;
    }

    if (ALobbyGameState* LobbyGS = Cast<ALobbyGameState>(GameState))
    {
        LobbyGS->SetSelectedMap(MapID);
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
    
    // Check if all players are present, have selected characters, AND are ready
    if (PlayersWithCharacters != MaxPlayers || CurrentPlayerCount != MaxPlayers)
    {
        return false;
    }
    
    // Check if all players are ready
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        if (ALobbyPlayerController* LobbyPC = Cast<ALobbyPlayerController>(It->Get()))
        {
            if (!LobbyPC->IsReady() || !LobbyPC->HasSelectedCharacter())
            {
                return false;
            }
        }
    }
    
    return true;
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

    // Clear all lobby characters before match starts
    ClearAllLobbyCharacters();

    // Lock the lobby to prevent new players from joining
    if (ALobbyGameState* LobbyGS = Cast<ALobbyGameState>(GameState))
    {
        LobbyGS->SetLobbyLocked(true);
    }

    // Get the GameInstance to store character selections (persists through seamless travel)
    UMyOnlineGameInstance* GameInstance = Cast<UMyOnlineGameInstance>(GetGameInstance());
    if (GameInstance)
    {
        UE_LOG(LogTemp, Log, TEXT("LobbyGameMode: Storing character selections in GameInstance before travel"));

        // Store all player character selections in the GameInstance
        for (auto& Pair : PlayerCharacterSelections)
        {
            if (Pair.Key && Pair.Value)
            {
                ACharacterPlayerState* PlayerState = Pair.Key->GetPlayerState<ACharacterPlayerState>();
                if (PlayerState)
                {
                    const FString PlayerName = PlayerState->GetPlayerName();
                    const FName CharacterID = Pair.Value->CharacterID;
                    const FString PlayerKey = GameInstance->BuildPlayerCharacterSelectionKeyFromController(Pair.Key);

                    // Store using a stable key (UniqueNetId when available) to avoid collisions.
                    GameInstance->StorePlayerCharacterSelectionForPlayerController(Pair.Key, CharacterID);

                    // Also set in PlayerState for immediate replication
                    PlayerState->SetSelectedCharacter(Pair.Value);

                    UE_LOG(LogTemp, Log, TEXT("LobbyGameMode: Stored character %s for player %s (key: %s)"),
                        *CharacterID.ToString(), *PlayerName, *PlayerKey);
                }
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("LobbyGameMode: GameInstance is not UMyOnlineGameInstance! Character selections may not persist."));
    }

    // Force replication of player state data and ensure character data is ready
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        if (APlayerController* PC = It->Get())
        {
            if (ACharacterPlayerState* PlayerState = PC->GetPlayerState<ACharacterPlayerState>())
            {
                PlayerState->ForceNetUpdate();
                
                // Double-check that character data is properly cached
                if (!PlayerState->IsCharacterDataReady())
                {
                    UE_LOG(LogTemp, Warning, TEXT("Character data not ready for player %s before travel"), 
                        *PlayerState->GetPlayerName());
                }
                else
                {
                    UE_LOG(LogTemp, Log, TEXT("Character data confirmed ready for player %s: %s"), 
                        *PlayerState->GetPlayerName(), *PlayerState->GetSelectedCharacterID().ToString());
                }
            }
        }
    }

    UpdateSessionSettings();

    // Small delay to ensure all data is replicated before travel
    FTimerHandle TravelTimer;
    FTimerDelegate TravelDelegate;
    TravelDelegate.BindUFunction(this, "ExecuteMatchTravel");
    GetWorld()->GetTimerManager().SetTimer(TravelTimer, TravelDelegate, 1.0f, false);

    // Cleanup widgets
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        if (ALobbyPlayerController* LobbyPC = Cast<ALobbyPlayerController>(It->Get()))
        {
            LobbyPC->Client_CleanupLobbyWidgets();
        }
    }
}

void ALobbyGameMode::ExecuteMatchTravel()
{
    const FString DefaultMapPath = TEXT("/Game/Remnantborn/Levels/TestGround");

    // Switch to gameplay music when game starts
    if (UMyOnlineGameInstance* GameInstance = Cast<UMyOnlineGameInstance>(GetGameInstance()))
    {
        GameInstance->OnMatchStarted();
    }

    FString ResolvedMapPath = DefaultMapPath;
    if (ALobbyGameState* LobbyGS = Cast<ALobbyGameState>(GameState))
    {
        if (UMapSelectionSubsystem* MapSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMapSelectionSubsystem>() : nullptr)
        {
            if (UMapDataAsset* SelectedMap = MapSubsystem->GetMapByID(LobbyGS->SelectedMapID))
            {
                const FString CandidateMapPath = SelectedMap->GetMapPath();
                if (!CandidateMapPath.IsEmpty())
                {
                    ResolvedMapPath = CandidateMapPath;
                }
            }
        }
    }

    FString TravelPath = ResolvedMapPath + "?listen";
    UE_LOG(LogTemp, Log, TEXT("Starting match travel to: %s"), *TravelPath);
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

void ALobbyGameMode::SpawnLobbyCharacterForPlayer(APlayerController* PlayerController)
{
    if (!CharacterManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("LobbyGameMode: CharacterManager not initialized"));
        return;
    }

    UCharacterDataAsset* SelectedCharacter = GetPlayerSelectedCharacter(PlayerController);
    if (SelectedCharacter)
    {
        CharacterManager->SpawnCharacterForPlayer(PlayerController, SelectedCharacter);
    }
}

void ALobbyGameMode::DespawnLobbyCharacterForPlayer(APlayerController* PlayerController)
{
    if (CharacterManager)
    {
        CharacterManager->DespawnCharacterForPlayer(PlayerController);
    }
}

void ALobbyGameMode::ClearAllLobbyCharacters()
{
    if (CharacterManager)
    {
        CharacterManager->ClearAllCharacters();
    }
}

void ALobbyGameMode::InitializeCharacterManager()
{
    if (!CharacterManager)
    {
        CharacterManager = NewObject<ULobbyCharacterManager>(this);
        if (CharacterManager)
        {
            CharacterManager->Initialize();
            CharacterManager->SetMaxPlayers(MaxPlayers);
            
            // Set character manager reference in GameState
            if (ALobbyGameState* LobbyGS = Cast<ALobbyGameState>(GameState))
            {
                LobbyGS->SetCharacterManager(CharacterManager);
            }
            
            UE_LOG(LogTemp, Log, TEXT("LobbyGameMode: CharacterManager initialized"));
        }
    }
}

UCharacterDataAsset* ALobbyGameMode::GetPlayerSelectedCharacter(APlayerController* PlayerController) const
{
    UCharacterDataAsset* const* FoundCharacter = PlayerCharacterSelections.Find(PlayerController);
    return FoundCharacter ? *FoundCharacter : nullptr;
}
