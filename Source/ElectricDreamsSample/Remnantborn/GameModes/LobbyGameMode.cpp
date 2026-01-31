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

ALobbyGameMode::ALobbyGameMode()
{
    // Set player state class
    PlayerStateClass = ACharacterPlayerState::StaticClass();
    
    // Use lobby player controller
    PlayerControllerClass = ALobbyPlayerController::StaticClass();
}

void ALobbyGameMode::BeginPlay()
{
    Super::BeginPlay();
    
    // Initialize character subsystem
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
    
    // Show character selection for new player
    ShowCharacterSelectionForPlayer(NewPlayer);
    
    // Update lobby state
    OnLobbyReadyChanged.Broadcast();
}

void ALobbyGameMode::Logout(AController* Exiting)
{
    Super::Logout(Exiting);
    
    CurrentPlayerCount = FMath::Max(0, CurrentPlayerCount - 1);
    
    // Remove character selection
    PlayerCharacterSelections.Remove(Cast<APlayerController>(Exiting));
    
    // Remove widget
    UCharacterSelectionWidget** WidgetPtr = CharacterSelectionWidgets.Find(Cast<APlayerController>(Exiting));
    if (WidgetPtr && *WidgetPtr)
    {
        (*WidgetPtr)->RemoveFromParent();
        CharacterSelectionWidgets.Remove(Cast<APlayerController>(Exiting));
    }
    
    // Update lobby state
    OnLobbyReadyChanged.Broadcast();
    
    // If we dropped below max players, cancel countdown
    if (bCountdownActive && CurrentPlayerCount < MaxPlayers)
    {
        CancelMatchCountdown();
    }
}

void ALobbyGameMode::ShowCharacterSelectionForPlayer(APlayerController* PlayerController)
{
    if (!PlayerController || !CharacterSelectionWidgetClass)
    {
        return;
    }
    
    // Create character selection widget
    UCharacterSelectionWidget* CharacterWidget = CreateWidget<UCharacterSelectionWidget>(PlayerController, CharacterSelectionWidgetClass);
    if (CharacterWidget)
    {
        CharacterWidget->AddToViewport();
        CharacterWidget->OnCharacterConfirmed.AddDynamic(this, &ALobbyGameMode::HandleCharacterSelected);
        
        CharacterSelectionWidgets.Add(PlayerController, CharacterWidget);
        
        // Set input mode for UI
        FInputModeUIOnly InputMode;
        InputMode.SetWidgetToFocus(CharacterWidget->TakeWidget());
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        PlayerController->SetInputMode(InputMode);
        PlayerController->SetShowMouseCursor(true);
    }
}

bool ALobbyGameMode::HasPlayerSelectedCharacter(APlayerController* PlayerController) const
{
    return PlayerCharacterSelections.Contains(PlayerController) && PlayerCharacterSelections[PlayerController] != nullptr;
}

TArray<APlayerController*> ALobbyGameMode::GetPlayersWithCharacters() const
{
    TArray<APlayerController*> Players;
    
    for (auto& Pair : PlayerCharacterSelections)
    {
        if (Pair.Key && Pair.Value)
        {
            Players.Add(Pair.Key);
        }
    }
    
    return Players;
}

void ALobbyGameMode::SetMaxPlayers(int32 NewMaxPlayers)
{
    // Only allow 2 or 4 players
    if (NewMaxPlayers != 2 && NewMaxPlayers != 4)
    {
        UE_LOG(LogTemp, Warning, TEXT("LobbyGameMode: Max players must be 2 or 4"));
        return;
    }
    
    MaxPlayers = NewMaxPlayers;
    
    // Update session settings
    UpdateSessionSettings();
    
    // If countdown is active and we dropped below max players, cancel countdown
    if (bCountdownActive && CurrentPlayerCount < MaxPlayers)
    {
        CancelMatchCountdown();
    }
    
    OnLobbyReadyChanged.Broadcast();
}

void ALobbyGameMode::StartMatchCountdown()
{
    if (!CanStartMatch() || bCountdownActive)
    {
        return;
    }
    
    bCountdownActive = true;
    CountdownTime = CountdownDuration;
    
    // Start countdown timer
    GetWorld()->GetTimerManager().SetTimer(
        CountdownTimerHandle,
        this,
        &ALobbyGameMode::UpdateCountdown,
        1.0f,
        true
    );
    
    OnLobbyCountdownStarted.Broadcast();
    OnLobbyCountdownUpdated.Broadcast();
}

void ALobbyGameMode::CancelMatchCountdown()
{
    if (!bCountdownActive)
    {
        return;
    }
    
    bCountdownActive = false;
    GetWorld()->GetTimerManager().ClearTimer(CountdownTimerHandle);
    
    OnLobbyCountdownCancelled.Broadcast();
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
    // All players must have selected characters AND we must have the right number of players
    int32 PlayersWithCharacters = GetPlayersWithCharacters().Num();
    return PlayersWithCharacters == MaxPlayers && CurrentPlayerCount == MaxPlayers;
}

void ALobbyGameMode::UpdateCountdown()
{
    CountdownTime--;
    
    OnLobbyCountdownUpdated.Broadcast();
    
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
    
    // Store character selections in player states
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
    
    // Update session to prevent new joins
    UpdateSessionSettings();
    
    // Travel to game map
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
        // Update session to not allow new joins during countdown
        ExistingSession->SessionSettings.bAllowJoinInProgress = !bCountdownActive;
        ExistingSession->SessionSettings.NumPublicConnections = MaxPlayers;
        
        // Update the session
        SessionInterface->UpdateSession(NAME_GameSession, ExistingSession->SessionSettings, true);
    }
}

void ALobbyGameMode::HandleCharacterSelected(UCharacterDataAsset* SelectedCharacter)
{
    APlayerController* PlayerController = nullptr;
    
    // Find which player selected this character
    for (auto& Pair : CharacterSelectionWidgets)
    {
        if (Pair.Key && Pair.Value && Pair.Value->GetIsVisible())
        {
            PlayerController = Pair.Key;
            break;
        }
    }
    
    if (PlayerController && SelectedCharacter)
    {
        // Store selection
        PlayerCharacterSelections.Add(PlayerController, SelectedCharacter);
        
        // Hide widget
        UCharacterSelectionWidget** WidgetPtr = CharacterSelectionWidgets.Find(PlayerController);
        if (WidgetPtr && *WidgetPtr)
        {
            (*WidgetPtr)->RemoveFromParent();
            
            // Reset input mode
            FInputModeGameOnly GameInputMode;
            PlayerController->SetInputMode(GameInputMode);
            PlayerController->SetShowMouseCursor(false);
        }
        
        // Notify selection
        OnCharacterSelectionChanged.Broadcast(PlayerController);
        OnLobbyReadyChanged.Broadcast();
        
        // Auto-start countdown if all players are ready
        if (CanStartMatch())
        {
            StartMatchCountdown();
        }
    }
}

UCharacterDataAsset* ALobbyGameMode::GetPlayerSelectedCharacter(APlayerController* PlayerController) const
{
    UCharacterDataAsset* const* FoundCharacter = PlayerCharacterSelections.Find(PlayerController);
    return FoundCharacter ? *FoundCharacter : nullptr;
}
