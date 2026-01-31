#include "LobbyGameMode.h"
#include "ElectricDreamsSample/Remnantborn/OnlineService/MyOnlineGameInstance.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"

ALobbyGameMode::ALobbyGameMode()
{
    // Use a basic PlayerController for lobby
    static ConstructorHelpers::FClassFinder<APlayerController> PlayerControllerBPClass(TEXT("/Game/Remnantborn/Blueprints/GameplayAbilitySystem/Characters/BP_RemnantbornCharacterBase"));
    if (PlayerControllerBPClass.Class != NULL)
    {
        PlayerControllerClass = PlayerControllerBPClass.Class;
    }
}

void ALobbyGameMode::BeginPlay()
{
    Super::BeginPlay();
    
    // Get max players from session settings if available
    UMyOnlineGameInstance* GameInstance = Cast<UMyOnlineGameInstance>(GetGameInstance());
    if (GameInstance)
    {
        // Session settings might store max players, but we'll use our own variable
        // The host will set this when creating the session
    }
}

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);
    
    CurrentPlayerCount++;
    
    // Update lobby state
    OnLobbyReadyChanged.Broadcast();
}

void ALobbyGameMode::Logout(AController* Exiting)
{
    Super::Logout(Exiting);
    
    CurrentPlayerCount = FMath::Max(0, CurrentPlayerCount - 1);
    
    // Update lobby state
    OnLobbyReadyChanged.Broadcast();
    
    // If we dropped below max players, cancel countdown
    if (bCountdownActive && CurrentPlayerCount < MaxPlayers)
    {
        CancelMatchCountdown();
    }
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
        1.0f, // Update every second
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
    return CurrentPlayerCount == MaxPlayers;
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
