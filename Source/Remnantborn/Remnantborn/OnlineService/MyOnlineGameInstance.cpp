#include "MyOnlineGameInstance.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "OnlineSessionSettings.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Engine/LocalPlayer.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Remnantborn/Remnantborn/CharacterSelection/CharacterSelectionSubsystem.h"

#if WITH_EDITOR
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#endif

UMyOnlineGameInstance::UMyOnlineGameInstance()
{
    DefaultMapPath = TEXT("/Game/Remnantborn/Levels/TestGround");
    DefaultMaxPlayers = 4;
    bIsHosting = false;
    bIsLoggedIn = false;
    LobbyMapPath = TEXT("/Game/Remnantborn/Levels/Lobby");
    
    MenuMusicVolume = 0.5f;
    GameplayMusicVolume = 0.5f;
    ResultMusicVolume = 0.5f;
    MusicVolume = 0.5f; // Legacy
    bShufflePlaylist = true;
    bAutoPlayOnLevelChange = true;
    CurrentTrackIndex = 0;
    MusicAudioComponent = nullptr;
    CurrentPlaylist = nullptr;
    
    // Initialize new music system variables
    PendingMusicState = EMusicState::Menu;
    bMusicWasPlayingBeforeTravel = false;
    bIsInMatch = false;
    bMatchHasEnded = false;
}

void UMyOnlineGameInstance::Init()
{
    Super::Init();
    
    // Initialize Online Subsystem
    IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
    if (OnlineSub)
    {
        UE_LOG(LogTemp, Log, TEXT("Online Subsystem: %s"), *OnlineSub->GetSubsystemName().ToString());
    }
    
    // Bind to network events
    if (GEngine)
    {
        GEngine->OnNetworkFailure().AddUObject(this, &UMyOnlineGameInstance::HandleNetworkFailure);
        GEngine->OnTravelFailure().AddUObject(this, &UMyOnlineGameInstance::HandleTravelFailure);
    }
    
    // Initialize HTTP Service
    HttpService = NewObject<UEdsHttpService>(this);
    if (HttpService)
    {
        HttpService->Initialize(TEXT("https://remnantborn-thelasttear.onrender.com/api"));
        // HttpService->Initialize(TEXT("http://localhost:3000/api"));
        
        // Try to load saved authentication
        LoadSavedAuth();
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create HTTP Service"));
    }
}

void UMyOnlineGameInstance::OnStart()
{
    Super::OnStart();

    if (!GetWorld())
    {
        UE_LOG(LogTemp, Error, TEXT("MusicSystem: OnStart - No World!"));
        return;
    }

    if (!bAutoPlayOnLevelChange)
    {
        UE_LOG(LogTemp, Warning, TEXT("MusicSystem: AutoPlay is disabled!"));
        return;
    }

    FString CurrentMapName = GetWorld()->GetMapName();
    CurrentMapName.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);

    UE_LOG(LogTemp, Log, TEXT("MusicSystem: OnStart - Map=%s, CurrentState=%d, InMatch=%d, MatchEnded=%d, IsPlaying=%d"),
        *CurrentMapName, (int32)CurrentMusicState, bIsInMatch, bMatchHasEnded, IsMusicPlaying());

    // Determine map type
    bool bIsMenuMap = CurrentMapName.Contains(TEXT("Menu")) || CurrentMapName.Contains(TEXT("Lobby"));
    bool bIsGameMap = !bIsMenuMap && !CurrentMapName.Contains(TEXT("Entry"));

    // Check if audio component is valid in current world - if not, we need to restart music
    bool bNeedsRestart = false;
    if (MusicAudioComponent && MusicAudioComponent->IsValidLowLevel())
    {
        if (MusicAudioComponent->GetWorld() != GetWorld())
        {
            UE_LOG(LogTemp, Log, TEXT("MusicSystem: Audio component is from old world, needs restart"));
            bNeedsRestart = true;
        }
    }
    else
    {
        bNeedsRestart = true;
    }

    // Priority 1: Match has ended - play result music (for both host and clients)
    if (bMatchHasEnded && CurrentMusicState == EMusicState::Result)
    {
        UE_LOG(LogTemp, Log, TEXT("MusicSystem: Match ended state detected - ensuring result music is playing"));
        // Music should already be playing from OnMatchEnded, but ensure it's running
        if (!IsMusicPlaying() || bNeedsRestart)
        {
            PlayResultMusic(true);
        }
    }
    // Priority 2: In a match on a game map - play/continue gameplay music
    else if (bIsInMatch && bIsGameMap && CurrentMusicState == EMusicState::Gameplay)
    {
        UE_LOG(LogTemp, Log, TEXT("MusicSystem: In match on game map - ensuring gameplay music is playing"));
        // Gameplay music should have been started by OnMatchStarted, but check if it's playing
        if (!IsMusicPlaying() || bNeedsRestart)
        {
            PlayGameplayMusic();
        }
    }
    // Priority 3: Menu/Lobby maps - play menu music
    else if (bIsMenuMap)
    {
        UE_LOG(LogTemp, Log, TEXT("MusicSystem: Menu/Lobby map detected"));
        bIsInMatch = false;
        bMatchHasEnded = false;

        // Play menu music if not playing or needs restart
        if (!IsMusicPlaying() || bNeedsRestart)
        {
            CurrentMusicState = EMusicState::Menu;
            PlayMenuMusic();
        }
        else
        {
            CurrentMusicState = EMusicState::Menu;
        }
    }
    // Priority 4: Catch-all - if we need to restart but don't know the state, use the current state
    else if (bNeedsRestart && CurrentMusicState != EMusicState::None)
    {
        UE_LOG(LogTemp, Log, TEXT("MusicSystem: Catch-all - restarting music with current state"));
        switch (CurrentMusicState)
        {
            case EMusicState::Menu:
                PlayMenuMusic();
                break;
            case EMusicState::Gameplay:
                PlayGameplayMusic();
                break;
            case EMusicState::Result:
                PlayResultMusic(true);
                break;
            default:
                // Default to menu music
                CurrentMusicState = EMusicState::Menu;
                PlayMenuMusic();
                break;
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("MusicSystem: Unexpected state - Map=%s, IsMenuMap=%d, IsGameMap=%d, InMatch=%d"),
            *CurrentMapName, bIsMenuMap, bIsGameMap, bIsInMatch);
    }
}


void UMyOnlineGameInstance::Shutdown()
{
    if (GEngine)
    {
        GEngine->OnNetworkFailure().RemoveAll(this);
        GEngine->OnTravelFailure().RemoveAll(this);
    }
    
    DestroySession();
    
    // Clean up music
    if (MusicAudioComponent)
    {
        MusicAudioComponent->Stop();
        MusicAudioComponent->OnAudioFinished.Clear();
        MusicAudioComponent->DestroyComponent();
        MusicAudioComponent = nullptr;
    }
    
    // Clean up HTTP Service
    if (HttpService)
    {
        HttpService->ConditionalBeginDestroy();
        HttpService = nullptr;
    }
    
    Super::Shutdown();
}

void UMyOnlineGameInstance::CreateSession(FString SessionName, int32 MaxPlayers)
{
    PendingSessionName = SessionName;
    PendingMaxPlayers = MaxPlayers;
    bPendingCreateSessionAfterDestroy = false;
    bPendingReturnToMainMenu = false;
    LastError = "";
    
    IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
    if (!OnlineSub)
    {
        LastError = "Online Subsystem not found";
        OnCreateSessionFailed.Broadcast(LastError);
        return;
    }
    
    SessionInterface = OnlineSub->GetSessionInterface();
    if (!SessionInterface.IsValid())
    {
        LastError = "Session Interface not valid";
        OnCreateSessionFailed.Broadcast(LastError);
        return;
    }
    
    // Destroy existing session first, then continue creation from OnDestroySessionComplete.
    if (SessionInterface->GetNamedSession(NAME_GameSession))
    {
        UE_LOG(LogTemp, Warning, TEXT("CreateSession: Existing session found, destroying before re-hosting"));
        bPendingCreateSessionAfterDestroy = true;
        DestroySession();
        return;
    }
    
    // Setup LAN session settings
    FOnlineSessionSettings SessionSettings;
    SessionSettings.bIsLANMatch = true;
    SessionSettings.bIsDedicated = false;
    SessionSettings.bUsesPresence = true;
    SessionSettings.bShouldAdvertise = true;
    SessionSettings.bAllowJoinInProgress = true;
    SessionSettings.bAllowInvites = true;
    SessionSettings.NumPublicConnections = MaxPlayers;
    SessionSettings.NumPrivateConnections = 0;
    
    // Set custom parameters
    SessionSettings.Set(FName(TEXT("SESSION_NAME")), FOnlineSessionSetting(SessionName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing));
    SessionSettings.Set(FName(TEXT("GAME_VERSION")), FOnlineSessionSetting(FString("1.0"), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing));
    
    // Bind delegate
    CreateSessionCompleteDelegate = FOnCreateSessionCompleteDelegate::CreateUObject(this, &UMyOnlineGameInstance::OnCreateSessionComplete);
    CreateSessionCompleteDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegate);
    
    // Create session with first local player
    ULocalPlayer* LocalPlayer = GetFirstGamePlayer();
    if (!LocalPlayer)
    {
        LastError = "No local player found";
        SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
        OnCreateSessionFailed.Broadcast(LastError);
        return;
    }
    
    if (!SessionInterface->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, SessionSettings))
    {
        LastError = "Failed to create session";
        SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
        OnCreateSessionComplete(NAME_GameSession, false);
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("Creating LAN session: %s"), *SessionName);
    }
}

void UMyOnlineGameInstance::OnCreateSessionComplete(FName SessionName, bool bSuccess)
{
    if (SessionInterface)
    {
        SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
    }

    if (bSuccess)
    {
        UE_LOG(LogTemp, Log, TEXT("Session created successfully: %s"), *SessionName.ToString());
        bIsHosting = true;
        OnCreateSessionSuccess.Broadcast();

        // Travel to the LOBBY map instead of game map
        if (!LobbyMapPath.IsEmpty())
        {
            // Prepare for travel - preserves music state seamlessly
            PrepareForLevelTravel();

            FString TravelPath = LobbyMapPath;
            TravelPath.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);
            TravelPath += TEXT("?listen");

            UE_LOG(LogTemp, Log, TEXT("Server traveling to lobby: %s"), *TravelPath);
            GetWorld()->ServerTravel(TravelPath);
        }
    }
    else
    {
        LastError = "Failed to create session";
        UE_LOG(LogTemp, Error, TEXT("Session creation failed"));
        OnCreateSessionFailed.Broadcast(LastError);
    }
}

void UMyOnlineGameInstance::FindSessions()
{
    LastError = "";
    SessionSearchResults.Empty();
    
    IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
    if (!OnlineSub)
    {
        LastError = "Online Subsystem not found";
        OnSessionSearchCompleted.Broadcast(false);
        return;
    }
    
    SessionInterface = OnlineSub->GetSessionInterface();
    if (!SessionInterface.IsValid())
    {
        LastError = "Session Interface not valid";
        OnSessionSearchCompleted.Broadcast(false);
        return;
    }
    
    // Setup search for LAN games
    SessionSearch = MakeShareable(new FOnlineSessionSearch());
    SessionSearch->bIsLanQuery = true;
    SessionSearch->MaxSearchResults = 20;
    SessionSearch->TimeoutInSeconds = 5;
    
    // Bind delegate
    FindSessionsCompleteDelegate = FOnFindSessionsCompleteDelegate::CreateUObject(this, &UMyOnlineGameInstance::OnFindSessionsComplete);
    FindSessionsCompleteDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegate);
    
    ULocalPlayer* LocalPlayer = GetFirstGamePlayer();
    if (!LocalPlayer)
    {
        LastError = "No local player for session search";
        OnSessionSearchCompleted.Broadcast(false);
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("Starting LAN session search..."));
    if (!SessionInterface->FindSessions(*LocalPlayer->GetPreferredUniqueNetId(), SessionSearch.ToSharedRef()))
    {
        LastError = "Failed to start session search";
        SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
        OnSessionSearchCompleted.Broadcast(false);
    }
}

void UMyOnlineGameInstance::OnFindSessionsComplete(bool bSuccess)
{
    if (SessionInterface)
    {
        SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
    }
    
    if (bSuccess && SessionSearch.IsValid())
    {
        UE_LOG(LogTemp, Log, TEXT("Found %d sessions"), SessionSearch->SearchResults.Num());
        
        // Clear previous results
        SessionSearchResults.Empty();
        
        for (const FOnlineSessionSearchResult& SearchResult : SessionSearch->SearchResults)
        {
            FSessionInfo SessionInfo;
            
            // Extract session name
            FString SessionName = TEXT("LAN Session");
            
            // Try to get the custom session name from settings
            const FOnlineSessionSettings& Settings = SearchResult.Session.SessionSettings;
            
            // Check if SESSION_NAME exists in settings
            const FOnlineSessionSetting* NameSetting = Settings.Settings.Find(FName(TEXT("SESSION_NAME")));
            if (NameSetting)
            {
                // Get the value from the setting
                if (NameSetting->Data.GetType() == EOnlineKeyValuePairDataType::String)
                {
                    NameSetting->Data.GetValue(SessionName);
                }
            }
            else
            {
                // Fallback: Use the OwningUserName as session name
                SessionName = SearchResult.Session.OwningUserName;
                if (SessionName.IsEmpty() || SessionName.Equals(TEXT("Unknown")))
                {
                    SessionName = FString::Printf(TEXT("LAN Session %d"), SessionSearchResults.Num() + 1);
                }
            }
            
            SessionInfo.SessionName = SessionName;
            
            // Get player counts
            SessionInfo.MaxPlayers = SearchResult.Session.SessionSettings.NumPublicConnections;
            SessionInfo.CurrentPlayers = FMath::Max(0, SessionInfo.MaxPlayers - SearchResult.Session.NumOpenPublicConnections);
            
            // Ensure valid values
            if (SessionInfo.MaxPlayers <= 0) SessionInfo.MaxPlayers = 4; // Default
            
            // Get ping - use actual ping value
            SessionInfo.Ping = SearchResult.PingInMs;
            
            // Store the session result
            SessionInfo.SessionResult.OnlineResult = SearchResult;
            
            SessionSearchResults.Add(SessionInfo);
            UE_LOG(LogTemp, Log, TEXT("Found session: %s (%d/%d players, %d ms ping)"),
                *SessionInfo.SessionName,
                SessionInfo.CurrentPlayers,
                SessionInfo.MaxPlayers,
                SessionInfo.Ping);
        }
        
        OnSessionSearchCompleted.Broadcast(true);
    }
    else
    {
        LastError = "No sessions found or search failed";
        UE_LOG(LogTemp, Warning, TEXT("Session search failed or no sessions found"));
        SessionSearchResults.Empty();
        OnSessionSearchCompleted.Broadcast(false);
    }
}

void UMyOnlineGameInstance::JoinSessionByIndex(int32 SessionIndex)
{
    if (SessionSearchResults.IsValidIndex(SessionIndex))
    {
        JoinSessionByResult(SessionSearchResults[SessionIndex].SessionResult);
    }
    else
    {
        LastError = "Invalid session index";
        OnJoinSessionFailed.Broadcast(LastError);
    }
}

void UMyOnlineGameInstance::JoinSessionByResult(const FBlueprintSessionResult& SessionResult)
{
    LastError = "";
    bPendingReturnToMainMenu = false;
    bPendingCreateSessionAfterDestroy = false;
    
    IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
    if (!OnlineSub)
    {
        LastError = "Online Subsystem not found";
        OnJoinSessionFailed.Broadcast(LastError);
        return;
    }
    
    SessionInterface = OnlineSub->GetSessionInterface();
    if (!SessionInterface.IsValid())
    {
        LastError = "Session Interface not valid";
        OnJoinSessionFailed.Broadcast(LastError);
        return;
    }

    // OSS refuses joining when a local GameSession already exists.
    if (SessionInterface->GetNamedSession(NAME_GameSession))
    {
        UE_LOG(LogTemp, Warning, TEXT("JoinSession: Existing local session found, destroying before joining"));
        PendingJoinSessionResult = SessionResult;
        bPendingJoinSessionAfterDestroy = true;
        DestroySession();
        return;
    }
    
    // Bind delegate
    JoinSessionCompleteDelegate = FOnJoinSessionCompleteDelegate::CreateUObject(this, &UMyOnlineGameInstance::OnJoinSessionComplete);
    JoinSessionCompleteDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegate);
    
    ULocalPlayer* LocalPlayer = GetFirstGamePlayer();
    if (!LocalPlayer)
    {
        LastError = "No local player for join session";
        OnJoinSessionFailed.Broadcast(LastError);
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("Joining session..."));
    if (!SessionInterface->JoinSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, SessionResult.OnlineResult))
    {
        LastError = "Failed to join session";
        SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
        OnJoinSessionComplete(NAME_GameSession, EOnJoinSessionCompleteResult::UnknownError);
    }
}

void UMyOnlineGameInstance::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
    if (SessionInterface)
    {
        SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
    }
    
    if (Result == EOnJoinSessionCompleteResult::Success)
    {
        UE_LOG(LogTemp, Log, TEXT("Successfully joined session!"));
        
        // Get connection string and travel to server
        FString ConnectString;
        if (SessionInterface->GetResolvedConnectString(SessionName, ConnectString))
        {
            UE_LOG(LogTemp, Log, TEXT("Traveling to server: %s"), *ConnectString);
            TravelToServer(ConnectString);
        }
        else
        {
            LastError = "Failed to get connection string";
            UE_LOG(LogTemp, Error, TEXT("%s"), *LastError);
            OnJoinSessionFailed.Broadcast(LastError);
        }
    }
    else
    {
        switch (Result)
        {
            case EOnJoinSessionCompleteResult::SessionIsFull:
                LastError = "Session is full";
                break;
            case EOnJoinSessionCompleteResult::SessionDoesNotExist:
                LastError = "Session no longer exists";
                break;
            case EOnJoinSessionCompleteResult::CouldNotRetrieveAddress:
                LastError = "Could not retrieve server address";
                break;
            default:
                LastError = "Failed to join session";
                break;
        }
        
        UE_LOG(LogTemp, Error, TEXT("Join session failed: %s"), *LastError);
        OnJoinSessionFailed.Broadcast(LastError);
    }
}

void UMyOnlineGameInstance::JoinByIP(FString IPAddress, int32 Port)
{
    FString ConnectString = FString::Printf(TEXT("%s:%d"), *IPAddress, Port);
    UE_LOG(LogTemp, Log, TEXT("Joining by IP: %s"), *ConnectString);
    TravelToServer(ConnectString);
}

void UMyOnlineGameInstance::StartGameAsClient(FString IPAddress, int32 Port)
{
    JoinByIP(IPAddress, Port);
}

void UMyOnlineGameInstance::StartGameAsServer(FString MapPath, int32 MaxPlayers)
{
    if (!MapPath.IsEmpty())
    {
        DefaultMapPath = MapPath;
    }
    CreateSession(FString::Printf(TEXT("Server_%d"), FMath::RandRange(1000, 9999)), MaxPlayers);
}

void UMyOnlineGameInstance::TravelToServer(const FString& Address)
{
    PrepareForLevelTravel();
    
    // Set music state for client joining a game - they should play gameplay music
    bIsInMatch = true;
    bMatchHasEnded = false;
    CurrentMusicState = EMusicState::Gameplay;
    
    APlayerController* PlayerController = GetFirstLocalPlayerController();
    if (PlayerController)
    {
        // Reset input mode before traveling
        FInputModeGameOnly InputMode;
        PlayerController->SetInputMode(InputMode);
        PlayerController->SetShowMouseCursor(false);
        PlayerController->bShowMouseCursor = false;
        
        UE_LOG(LogTemp, Log, TEXT("Client traveling to: %s - will play gameplay music on arrival"), *Address);
        PlayerController->ClientTravel(Address, TRAVEL_Absolute);
    }
    else
    {
        LastError = "No player controller found";
        OnJoinSessionFailed.Broadcast(LastError);
    }
}

void UMyOnlineGameInstance::DestroySession()
{
    IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
    if (!OnlineSub)
    {
        return;
    }
    
    SessionInterface = OnlineSub->GetSessionInterface();
    if (!SessionInterface.IsValid())
    {
        return;
    }
    
    // Check if session exists
    if (!SessionInterface->GetNamedSession(NAME_GameSession))
    {
        bIsHosting = false;
        OnSessionDestroyed.Broadcast();

        // Execute deferred actions immediately when no destroy call is required.
        if (bPendingCreateSessionAfterDestroy)
        {
            ContinueCreateSessionAfterDestroy();
            return;
        }

        if (bPendingJoinSessionAfterDestroy)
        {
            ContinueJoinSessionAfterDestroy();
            return;
        }

        if (bPendingReturnToMainMenu)
        {
            bPendingReturnToMainMenu = false;
            ReturnToMainMenuLevel();
        }

        return;
    }
    
    DestroySessionCompleteDelegate = FOnDestroySessionCompleteDelegate::CreateUObject(this, &UMyOnlineGameInstance::OnDestroySessionComplete);
    DestroySessionCompleteDelegateHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegate);
    
    if (!SessionInterface->DestroySession(NAME_GameSession))
    {
        SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
        OnSessionDestroyed.Broadcast();
    }
}

void UMyOnlineGameInstance::OnDestroySessionComplete(FName SessionName, bool bSuccess)
{
    if (SessionInterface)
    {
        SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
    }
    
    if (bSuccess)
    {
        UE_LOG(LogTemp, Log, TEXT("Session destroyed"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to destroy session"));
    }

    bIsHosting = false;
    const bool bShouldCreateAfterDestroy = bPendingCreateSessionAfterDestroy;
    const bool bShouldJoinAfterDestroy = bPendingJoinSessionAfterDestroy;
    const bool bShouldReturnToMainMenu = bPendingReturnToMainMenu;
    bPendingCreateSessionAfterDestroy = false;
    bPendingJoinSessionAfterDestroy = false;
    bPendingReturnToMainMenu = false;
    
    OnSessionDestroyed.Broadcast();

    if (bShouldCreateAfterDestroy)
    {
        ContinueCreateSessionAfterDestroy();
        return;
    }

    if (bShouldJoinAfterDestroy)
    {
        ContinueJoinSessionAfterDestroy();
        return;
    }

    if (bShouldReturnToMainMenu)
    {
        ReturnToMainMenuLevel();
    }
}

void UMyOnlineGameInstance::LeaveGame()
{
    PrepareForLevelTravel();

    // Reset music state for menu
    bIsInMatch = false;
    bMatchHasEnded = false;
    CurrentMusicState = EMusicState::Menu;

    // Clear selections from the previous lobby/match lifecycle.
    ClearLocalCharacterSelection();
    ClearAllCharacterSelections();

    bPendingCreateSessionAfterDestroy = false;
    bPendingReturnToMainMenu = true;

    UE_LOG(LogTemp, Log, TEXT("LeaveGame: Returning to main menu with music state reset"));

    // Destroy active session for both host and clients before returning to menu.
    if (HasNamedGameSession())
    {
        DestroySession();
        return;
    }

    bPendingReturnToMainMenu = false;
    ReturnToMainMenuLevel();
}

bool UMyOnlineGameInstance::HasNamedGameSession() const
{
    IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
    if (!OnlineSub)
    {
        return false;
    }

    IOnlineSessionPtr LocalSessionInterface = OnlineSub->GetSessionInterface();
    return LocalSessionInterface.IsValid() && LocalSessionInterface->GetNamedSession(NAME_GameSession) != nullptr;
}

void UMyOnlineGameInstance::ContinueCreateSessionAfterDestroy()
{
    bPendingCreateSessionAfterDestroy = false;
    UE_LOG(LogTemp, Log, TEXT("CreateSession: Previous session destroyed, continuing host flow"));
    CreateSession(PendingSessionName, PendingMaxPlayers);
}

void UMyOnlineGameInstance::ContinueJoinSessionAfterDestroy()
{
    bPendingJoinSessionAfterDestroy = false;
    UE_LOG(LogTemp, Log, TEXT("JoinSession: Previous session destroyed, continuing join flow"));
    JoinSessionByResult(PendingJoinSessionResult);
}

void UMyOnlineGameInstance::ReturnToMainMenuLevel()
{
    UGameplayStatics::OpenLevel(this, FName("/Game/Remnantborn/Levels/MainMenu"), true);
}

void UMyOnlineGameInstance::HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
    UE_LOG(LogTemp, Error, TEXT("Network Failure: %s"), *ErrorString);
    LastError = ErrorString;
    
    // Return to main menu on network failure
    if (World && World->IsGameWorld())
    {
        UGameplayStatics::OpenLevel(World, FName("/Game/Remnantborn/Levels/MainMenu"));
    }
}

void UMyOnlineGameInstance::HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString)
{
    UE_LOG(LogTemp, Error, TEXT("Travel Failure: %s"), *ErrorString);
    LastError = ErrorString;
    OnJoinSessionFailed.Broadcast(ErrorString);
}

// === Authentication Methods (Updated with lambda callbacks) ===

void UMyOnlineGameInstance::Login(const FString& Email, const FString& Password)
{
    if (!HttpService)
    {
        UE_LOG(LogTemp, Error, TEXT("HTTP Service not initialized"));
        return;
    }
    
    HttpService->Login(Email, Password, FOnAuthResponse::CreateLambda([this](const FAuthResponse& Response)
    {
        if (Response.bSuccess && !Response.Token.IsEmpty())
        {
            // Save token and update state
            SetAuthState(true, Response.Token, Response.UserProfile.UserId, Response.UserProfile);
            
            // Save to local storage
            if (HttpService)
            {
                HttpService->SaveAuth(Response.Token, Response.UserProfile.UserId);
            }
            
            UE_LOG(LogTemp, Log, TEXT("Login successful for user: %s"), *Response.UserProfile.Username);
            
            // Get full profile
            GetUserProfile();
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Login failed: %s"), *Response.ErrorMessage);
            // Still broadcast state change (to false)
            OnAuthStateChanged.Broadcast(false);
        }
    }));
}

void UMyOnlineGameInstance::Signup(const FString& Email, const FString& Password, const FString& Username)
{
    if (!HttpService)
    {
        UE_LOG(LogTemp, Error, TEXT("HTTP Service not initialized"));
        return;
    }
    
    HttpService->Signup(Email, Password, Username, FOnAuthResponse::CreateLambda([this](const FAuthResponse& Response)
    {
        if (Response.bSuccess && !Response.Token.IsEmpty())
        {
            // Save token and update state
            SetAuthState(true, Response.Token, Response.UserProfile.UserId, Response.UserProfile);
            
            // Save to local storage
            if (HttpService)
            {
                HttpService->SaveAuth(Response.Token, Response.UserProfile.UserId);
            }
            
            UE_LOG(LogTemp, Log, TEXT("Signup successful for user: %s"), *Response.UserProfile.Username);
            
            // Get full profile
            GetUserProfile();
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Signup failed: %s"), *Response.ErrorMessage);
            OnAuthStateChanged.Broadcast(false);
        }
    }));
}

void UMyOnlineGameInstance::DevLogin(const FString& Email)
{
    if (!HttpService)
    {
        UE_LOG(LogTemp, Error, TEXT("HTTP Service not initialized"));
        return;
    }
    
    HttpService->DevLogin(Email, FOnAuthResponse::CreateLambda([this](const FAuthResponse& Response)
    {
        if (Response.bSuccess && !Response.Token.IsEmpty())
        {
            // Save token and update state
            SetAuthState(true, Response.Token, Response.UserProfile.UserId, Response.UserProfile);
            
            // Save to local storage
            if (HttpService)
            {
                HttpService->SaveAuth(Response.Token, Response.UserProfile.UserId);
            }
            
            UE_LOG(LogTemp, Log, TEXT("Dev login successful for user: %s"), *Response.UserProfile.Username);
            
            // Get full profile
            GetUserProfile();
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Dev login failed: %s"), *Response.ErrorMessage);
            OnAuthStateChanged.Broadcast(false);
        }
    }));
}

void UMyOnlineGameInstance::Logout()
{
    bIsLoggedIn = false;
    AuthToken.Empty();
    CurrentUserId.Empty();
    CurrentUserProfile = FUserProfile();
    
    // Clear saved auth
    if (HttpService)
    {
        HttpService->ClearAuth();
    }
    
    // Broadcast state change
    OnAuthStateChanged.Broadcast(false);
    OnProfileUpdated.Broadcast(FUserProfile());
    
    UE_LOG(LogTemp, Log, TEXT("User logged out"));
}

void UMyOnlineGameInstance::LoadSavedAuth()
{
    if (!HttpService)
    {
        return;
    }
    
    FString SavedToken, SavedUserId;
    if (HttpService->LoadSavedAuth(SavedToken, SavedUserId))
    {
        UE_LOG(LogTemp, Log, TEXT("Found saved auth: UserId=%s"), *SavedUserId);
        
        // Store temporarily
        AuthToken = SavedToken;
        CurrentUserId = SavedUserId;
        
        // Verify the token
        HttpService->VerifyToken(SavedToken, FOnAuthResponse::CreateLambda([this, SavedToken, SavedUserId](const FAuthResponse& Response)
        {
            if (Response.bSuccess)
            {
                // Token is valid, set auth state
                SetAuthState(true, SavedToken, SavedUserId, Response.UserProfile);
                
                // Get full profile
                GetUserProfile();
                
                UE_LOG(LogTemp, Log, TEXT("Token verified for user: %s"), *Response.UserProfile.Username);
            }
            else
            {
                // Token invalid, clear saved auth
                AuthToken.Empty();
                CurrentUserId.Empty();
                
                if (HttpService)
                {
                    HttpService->ClearAuth();
                }
                
                UE_LOG(LogTemp, Warning, TEXT("Saved token verification failed: %s"), *Response.ErrorMessage);
                OnAuthStateChanged.Broadcast(false);
            }
        }));
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("No saved auth found"));
        OnAuthStateChanged.Broadcast(false);
    }
}

void UMyOnlineGameInstance::GetUserProfile()
{
    if (!HttpService || !bIsLoggedIn || CurrentUserId.IsEmpty() || AuthToken.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot get profile: Not logged in"));
        return;
    }
    
    HttpService->GetProfile(CurrentUserId, AuthToken, FOnProfileResponse::CreateLambda([this](const FUserProfile& Profile)
    {
        if (Profile.bIsValid)
        {
            CurrentUserProfile = Profile;

            if (UCharacterSelectionSubsystem* CharacterSubsystem = GetSubsystem<UCharacterSelectionSubsystem>())
            {
                CharacterSubsystem->SyncUnlocksFromBackend(Profile.PurchasedItems);
            }
            
            // Broadcast profile update
            OnProfileUpdated.Broadcast(Profile);
            
            UE_LOG(LogTemp, Log, TEXT("Profile loaded: %s (Level: %d, Remnants: %d)"),
                   *Profile.Username, Profile.Level, Profile.RemnantCount);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to load profile"));
        }
    }));
}

void UMyOnlineGameInstance::GetMyProfile()
{
    if (!HttpService || !bIsLoggedIn || AuthToken.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot get my profile: Not logged in"));
        return;
    }
    
    HttpService->GetMyProfile(AuthToken, FOnProfileResponse::CreateLambda([this](const FUserProfile& Profile)
    {
        if (Profile.bIsValid)
        {
            CurrentUserProfile = Profile;

            if (UCharacterSelectionSubsystem* CharacterSubsystem = GetSubsystem<UCharacterSelectionSubsystem>())
            {
                CharacterSubsystem->SyncUnlocksFromBackend(Profile.PurchasedItems);
            }
            
            // Broadcast profile update
            OnProfileUpdated.Broadcast(Profile);
            
            UE_LOG(LogTemp, Log, TEXT("My profile loaded: %s (Level: %d, Remnants: %d)"),
                   *Profile.Username, Profile.Level, Profile.RemnantCount);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to load my profile"));
        }
    }));
}

void UMyOnlineGameInstance::UpdateProfile(const FString& Username, const FString& Bio)
{
    if (!HttpService || !bIsLoggedIn || CurrentUserId.IsEmpty() || AuthToken.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot update profile: Not logged in"));
        return;
    }
    
    HttpService->UpdateProfile(CurrentUserId, AuthToken, Username, Bio, FOnProfileUpdateResponse::CreateLambda([this](const FProfileUpdateResponse& Response)
    {
        if (Response.bSuccess)
        {
            UE_LOG(LogTemp, Log, TEXT("Profile updated successfully"));
            // Refresh profile
            GetUserProfile();
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to update profile: %s"), *Response.ErrorMessage);
        }
    }));
}

void UMyOnlineGameInstance::UpdateProfileWithAvatar(const FString& Username, const FString& Bio, const FString& AvatarUrl)
{
    if (!HttpService || !bIsLoggedIn || CurrentUserId.IsEmpty() || AuthToken.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot update profile with avatar: Not logged in"));
        return;
    }
    
    HttpService->UpdateProfileWithAvatar(CurrentUserId, AuthToken, Username, Bio, AvatarUrl, FOnProfileUpdateResponse::CreateLambda([this](const FProfileUpdateResponse& Response)
    {
        if (Response.bSuccess)
        {
            UE_LOG(LogTemp, Log, TEXT("Profile with avatar updated successfully"));
            // Refresh profile
            GetUserProfile();
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to update profile with avatar: %s"), *Response.ErrorMessage);
        }
    }));
}

void UMyOnlineGameInstance::UploadAvatar(const FString& FilePath)
{
    if (!HttpService || !bIsLoggedIn || AuthToken.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot upload avatar: Not logged in"));
        return;
    }
    
    if (FilePath.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot upload avatar: File path is empty"));
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("Uploading avatar from: %s"), *FilePath);
    
    HttpService->UploadAvatar(AuthToken, FilePath, FOnAvatarUploadResponse::CreateLambda([this](const FAvatarUploadResponse& Response)
    {
        if (Response.bSuccess)
        {
            UE_LOG(LogTemp, Log, TEXT("Avatar uploaded successfully: %s"), *Response.AvatarUrl);
            
            // Update the current profile with the new avatar URL
            CurrentUserProfile.AvatarUrl = Response.AvatarUrl;
            OnProfileUpdated.Broadcast(CurrentUserProfile);

            // let any listeners know upload finished
            OnAvatarUploadComplete.Broadcast(true);

            // refresh profile from backend to make sure all other fields (updated_at, etc.) are synced
            GetUserProfile();
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to upload avatar: %s"), *Response.ErrorMessage);
            OnAvatarUploadComplete.Broadcast(false);
        }
    }));
}

bool UMyOnlineGameInstance::PickImageFile(FString& OutFilePath)
{
#if WITH_EDITOR
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform)
	{
		UE_LOG(LogTemp, Warning, TEXT("DesktopPlatform not available"));
		return false;
	}

    void* ParentWindowHandle = nullptr;
    TArray<FString> OutFiles;
    const bool bResult = DesktopPlatform->OpenFileDialog(
        ParentWindowHandle,
        TEXT("Select Avatar Image"),
        TEXT(""),
        TEXT(""),
        TEXT("Image Files (*.png;*.jpg;*.jpeg;*.gif;*.webp)|*.png;*.jpg;*.jpeg;*.gif;*.webp"),
        EFileDialogFlags::None,
        OutFiles);
    
	if (bResult && OutFiles.Num() > 0)
	{
		OutFilePath = OutFiles[0];
		return true;
	}
	return false;
#else
	UE_LOG(LogTemp, Warning, TEXT("PickImageFile is only available in editor builds."));
	return false;
#endif
}

void UMyOnlineGameInstance::UpdateGameStats(int32 Level, int32 RemnantCount, const FString& Operation)
{
    if (!HttpService || !bIsLoggedIn || CurrentUserId.IsEmpty() || AuthToken.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot update game stats: Not logged in"));
        return;
    }
    
    HttpService->UpdateGameStats(CurrentUserId, AuthToken, Level, RemnantCount, Operation, 
        FOnSimpleResponse::CreateLambda([this](bool bSuccess)
        {
            if (bSuccess)
            {
                UE_LOG(LogTemp, Log, TEXT("Game stats updated successfully"));
                // Refresh profile
                GetUserProfile();
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Failed to update game stats"));
            }
        }));
}

void UMyOnlineGameInstance::SubmitMatchReward(bool bIsWinner, float MatchDuration, int32 EliminationOrder, const FString& MatchId)
{
    if (!HttpService || !bIsLoggedIn || AuthToken.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot submit match reward: Not logged in"));
        return;
    }

    HttpService->SubmitMatchReward(AuthToken, bIsWinner, MatchDuration, EliminationOrder, MatchId,
        FOnMatchRewardResponse::CreateLambda([this](const FMatchRewardResponse& Response)
        {
            if (Response.bSuccess)
            {
                CurrentUserProfile.RemnantCount = Response.NewRemnantCount;
                OnProfileUpdated.Broadcast(CurrentUserProfile);
                OnMatchRewardReceived.Broadcast(Response.RewardAmount, Response.NewRemnantCount, Response.bIsWinner);

                UE_LOG(LogTemp, Log, TEXT("Match reward granted: +%d (new balance: %d)"),
                    Response.RewardAmount,
                    Response.NewRemnantCount);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Match reward submission failed: %s"), *Response.ErrorMessage);
            }
        }));
}

void UMyOnlineGameInstance::SubmitMatchComplete(const FMatchCompleteRequest& MatchRequest)
{
    if (!HttpService || !bIsLoggedIn || AuthToken.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot submit match completion: Not logged in"));
        OnMatchCompleteSubmitted.Broadcast(false, MatchRequest.MatchId, false);
        return;
    }

    if (MatchRequest.MatchId.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot submit match completion: MatchId is empty"));
        OnMatchCompleteSubmitted.Broadcast(false, MatchRequest.MatchId, false);
        return;
    }

    HttpService->SubmitMatchComplete(AuthToken, MatchRequest,
        FOnMatchCompleteResponse::CreateLambda([this](const FMatchCompleteResponse& Response)
        {
            if (Response.bSuccess)
            {
                if (Response.MyNewRemnantCount > 0 || Response.MyRewardAmount > 0)
                {
                    CurrentUserProfile.RemnantCount = Response.MyNewRemnantCount;
                    OnProfileUpdated.Broadcast(CurrentUserProfile);
                    OnMatchRewardReceived.Broadcast(Response.MyRewardAmount, Response.MyNewRemnantCount, Response.bMyIsWinner);
                }

                UE_LOG(LogTemp, Log, TEXT("Match completion submitted: match=%s, participants=%d, rewards=%d, idempotent=%s"),
                    *Response.MatchId,
                    Response.ParticipantsSaved,
                    Response.RewardsProcessed,
                    Response.bIdempotentReplay ? TEXT("YES") : TEXT("NO"));

                OnMatchCompleteSubmitted.Broadcast(true, Response.MatchId, Response.bIdempotentReplay);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Match completion submission failed: %s"), *Response.ErrorMessage);
                OnMatchCompleteSubmitted.Broadcast(false, Response.MatchId, Response.bIdempotentReplay);
            }
        }));
}

void UMyOnlineGameInstance::SetAuthState(bool bLoggedIn, const FString& Token, const FString& UserId, const FUserProfile& Profile)
{
    bIsLoggedIn = bLoggedIn;
    
    if (bLoggedIn)
    {
        AuthToken = Token;
        CurrentUserId = UserId;
        CurrentUserProfile = Profile;

        if (UCharacterSelectionSubsystem* CharacterSubsystem = GetSubsystem<UCharacterSelectionSubsystem>())
        {
            CharacterSubsystem->SyncUnlocksFromBackend(Profile.PurchasedItems);
        }
        
        // Broadcast both events
        OnAuthStateChanged.Broadcast(true);
        OnProfileUpdated.Broadcast(Profile);
    }
    else
    {
        AuthToken.Empty();
        CurrentUserId.Empty();
        CurrentUserProfile = FUserProfile();
        
        OnAuthStateChanged.Broadcast(false);
        OnProfileUpdated.Broadcast(FUserProfile());
    }
}

void UMyOnlineGameInstance::CreateSessionWithLobby(FString SessionName, int32 MaxPlayers, FString InLobbyMapPath)
{
    // Only allow 2 or 4 players
    if (MaxPlayers != 2 && MaxPlayers != 4)
    {
        MaxPlayers = 2; // Default to 2
    }
    
    LobbyMapPath = InLobbyMapPath;
    PendingMaxPlayers = MaxPlayers;
    CreateSession(SessionName, MaxPlayers);
}

void UMyOnlineGameInstance::SetLobbyMapPath(const FString& NewLobbyMapPath)
{
    LobbyMapPath = NewLobbyMapPath;
}

// === Character Selection Persistence ===

void UMyOnlineGameInstance::SetLocalCharacterSelection(FName CharacterID)
{
    LocalCharacterSelection = CharacterID;
    UE_LOG(LogTemp, Log, TEXT("Local character selection set to: %s"), *CharacterID.ToString());
}

void UMyOnlineGameInstance::ClearLocalCharacterSelection()
{
    LocalCharacterSelection = NAME_None;
    UE_LOG(LogTemp, Log, TEXT("Local character selection cleared"));
}

FString UMyOnlineGameInstance::BuildPlayerCharacterSelectionKey(const APlayerState* PlayerState) const
{
    if (!PlayerState)
    {
        return FString();
    }

    const FUniqueNetIdRepl& UniqueId = PlayerState->GetUniqueId();
    const TSharedPtr<const FUniqueNetId>& UniqueNetId = UniqueId.GetUniqueNetId();
    if (UniqueNetId.IsValid())
    {
        return FString::Printf(TEXT("UID:%s"), *UniqueNetId->ToString());
    }

    const int32 PlayerId = PlayerState->GetPlayerId();
    if (PlayerId != INDEX_NONE)
    {
        return FString::Printf(TEXT("PID:%d"), PlayerId);
    }

    const FString PlayerName = PlayerState->GetPlayerName().TrimStartAndEnd();
    if (!PlayerName.IsEmpty())
    {
        return FString::Printf(TEXT("NAME:%s"), *PlayerName);
    }

    return FString();
}

void UMyOnlineGameInstance::StorePlayerCharacterSelectionForPlayerState(const APlayerState* PlayerState, const FName& CharacterID)
{
    if (!PlayerState || CharacterID.IsNone())
    {
        return;
    }

    const FString PlayerKey = BuildPlayerCharacterSelectionKey(PlayerState);
    if (!PlayerKey.IsEmpty())
    {
        StorePlayerCharacterSelection(PlayerKey, CharacterID);
    }
}

FName UMyOnlineGameInstance::GetPlayerCharacterSelectionForPlayerState(const APlayerState* PlayerState) const
{
    if (!PlayerState)
    {
        return NAME_None;
    }

    const FString PlayerKey = BuildPlayerCharacterSelectionKey(PlayerState);
    if (!PlayerKey.IsEmpty())
    {
        if (const FName* FoundSelection = PlayerCharacterSelections.Find(PlayerKey))
        {
            return *FoundSelection;
        }
    }

    return NAME_None;
}

void UMyOnlineGameInstance::StorePlayerCharacterSelectionForPlayerController(const APlayerController* PlayerController, const FName& CharacterID)
{
    if (!PlayerController || CharacterID.IsNone())
    {
        return;
    }

    const FString ControllerKey = BuildPlayerCharacterSelectionKeyFromController(PlayerController);
    if (!ControllerKey.IsEmpty())
    {
        StorePlayerCharacterSelection(ControllerKey, CharacterID);
    }

    const APlayerState* PlayerState = PlayerController->GetPlayerState<APlayerState>();
    StorePlayerCharacterSelectionForPlayerState(PlayerState, CharacterID);
}

FName UMyOnlineGameInstance::GetPlayerCharacterSelectionForPlayerController(const APlayerController* PlayerController) const
{
    if (!PlayerController)
    {
        return NAME_None;
    }

    const FString ControllerKey = BuildPlayerCharacterSelectionKeyFromController(PlayerController);
    if (!ControllerKey.IsEmpty())
    {
        if (const FName* FoundByControllerKey = PlayerCharacterSelections.Find(ControllerKey))
        {
            return *FoundByControllerKey;
        }
    }

    const APlayerState* PlayerState = PlayerController->GetPlayerState<APlayerState>();
    return GetPlayerCharacterSelectionForPlayerState(PlayerState);
}

FString UMyOnlineGameInstance::BuildPlayerCharacterSelectionKeyFromController(const APlayerController* PlayerController) const
{
    if (!PlayerController)
    {
        return FString();
    }

    return FString::Printf(TEXT("PC:%s"), *PlayerController->GetName());
}

void UMyOnlineGameInstance::StorePlayerCharacterSelection(const FString& PlayerName, const FName& CharacterID)
{
    if (!PlayerName.IsEmpty() && !CharacterID.IsNone())
    {
        PlayerCharacterSelections.Add(PlayerName, CharacterID);
        UE_LOG(LogTemp, Log, TEXT("GameInstance: Stored character selection %s for player %s"), 
            *CharacterID.ToString(), *PlayerName);
    }
}

FName UMyOnlineGameInstance::GetPlayerCharacterSelection(const FString& PlayerName) const
{
    if (const FName* FoundSelection = PlayerCharacterSelections.Find(PlayerName))
    {
        return *FoundSelection;
    }
    return NAME_None;
}

bool UMyOnlineGameInstance::HasPlayerCharacterSelection(const FString& PlayerName) const
{
    return PlayerCharacterSelections.Contains(PlayerName);
}

void UMyOnlineGameInstance::RemovePlayerCharacterSelection(const FString& PlayerName)
{
    if (PlayerCharacterSelections.Remove(PlayerName) > 0)
    {
        UE_LOG(LogTemp, Log, TEXT("GameInstance: Removed character selection for player %s"), *PlayerName);
    }
}

void UMyOnlineGameInstance::ClearAllCharacterSelections()
{
    PlayerCharacterSelections.Empty();
    UE_LOG(LogTemp, Log, TEXT("GameInstance: Cleared all character selections"));
}

void UMyOnlineGameInstance::TravelToGameLevel(FString GameMapPath)
{
    // Switch to gameplay music before traveling
    OnMatchStarted();
    
    // Only server should initiate the travel
    if (!bIsHosting)
    {
        UE_LOG(LogTemp, Warning, TEXT("Only server can initiate travel to game level"));
        return;
    }
    
    // Use default map path if none provided
    if (GameMapPath.IsEmpty())
    {
        GameMapPath = DefaultMapPath;
    }
    
    if (GetWorld())
    {
        // Remove streaming levels prefix if present
        GameMapPath.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);
        
        FString TravelPath = GameMapPath + TEXT("?listen");
        
        UE_LOG(LogTemp, Log, TEXT("Server traveling to game level: %s"), *TravelPath);
        GetWorld()->ServerTravel(TravelPath);
    }
}

// === Background Music Implementation ===

void UMyOnlineGameInstance::SetMusicState(EMusicState NewState)
{
    CurrentMusicState = NewState;
    
    switch (NewState)
    {
        case EMusicState::Menu:
            PlayMenuMusic();
            break;
        case EMusicState::Gameplay:
            PlayGameplayMusic();
            break;
        case EMusicState::Result:
            PlayResultMusic(true);
            break;
        case EMusicState::None:
        default:
            StopBackgroundMusic();
            break;
    }
}

void UMyOnlineGameInstance::StartBackgroundMusic()
{
    PlayMenuMusic();
}

void UMyOnlineGameInstance::PlayMenuMusic()
{
    if (MainMenuPlaylist.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("BackgroundMusic: No tracks in MainMenu playlist"));
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(1, 5.0f, FColor::Red, TEXT("ERROR: No tracks in MainMenu playlist!"));
        }
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("BackgroundMusic: Playing MainMenu playlist (%d tracks)"), MainMenuPlaylist.Num());
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(1, 5.0f, FColor::Green, FString::Printf(TEXT("Playing Menu Music (%d tracks)"), MainMenuPlaylist.Num()));
    }
    CurrentMusicState = EMusicState::Menu;
    PlayFromPlaylist(&MainMenuPlaylist);
}

void UMyOnlineGameInstance::PlayGameplayMusic()
{
    if (GameplayPlaylist.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("BackgroundMusic: No tracks in Gameplay playlist"));
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(2, 5.0f, FColor::Red, TEXT("ERROR: No tracks in Gameplay playlist!"));
        }
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("BackgroundMusic: Playing Gameplay playlist (%d tracks)"), GameplayPlaylist.Num());
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(2, 5.0f, FColor::Green, FString::Printf(TEXT("Playing Gameplay Music (%d tracks)"), GameplayPlaylist.Num()));
    }
    CurrentMusicState = EMusicState::Gameplay;
    bShufflePlaylist = true;
    PlayFromPlaylist(&GameplayPlaylist);
}

void UMyOnlineGameInstance::PlayResultMusic(bool bIsVictory)
{
    if (ResultPlaylist.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("BackgroundMusic: No tracks in Result playlist"));
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(3, 5.0f, FColor::Red, TEXT("ERROR: No tracks in Result playlist!"));
        }
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("BackgroundMusic: Playing Result playlist (%d tracks)"), ResultPlaylist.Num());
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(3, 5.0f, FColor::Green, FString::Printf(TEXT("Playing Result Music (%d tracks)"), ResultPlaylist.Num()));
    }
    
    CurrentMusicState = EMusicState::Result;
    bShufflePlaylist = false;
    PlayFromPlaylist(&ResultPlaylist);
}

UWorld* UMyOnlineGameInstance::GetMusicWorld() const
{
    UWorld* World = GetWorld();
    if (!World && GEngine)
    {
        World = GEngine->GetWorldFromContextObject(this, EGetWorldErrorMode::ReturnNull);
    }
    return World;
}

void UMyOnlineGameInstance::PlayFromPlaylist(TArray<USoundCue*>* Playlist)
{
    if (!Playlist || Playlist->Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("BackgroundMusic: Empty playlist provided"));
        return;
    }
    
    UWorld* World = GetMusicWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("BackgroundMusic: No world found"));
        return;
    }
    
    CurrentPlaylist = Playlist;
    
    if (bShufflePlaylist)
    {
        CurrentTrackIndex = FMath::RandRange(0, Playlist->Num() - 1);
    }
    else
    {
        CurrentTrackIndex = 0;
    }
    
    PlayNextTrack();
}

void UMyOnlineGameInstance::StopBackgroundMusic()
{
    if (MusicAudioComponent)
    {
        MusicAudioComponent->Stop();
        UE_LOG(LogTemp, Log, TEXT("BackgroundMusic: Stopped"));
    }
}

void UMyOnlineGameInstance::SetMusicVolume(float Volume)
{
    MusicVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
    MenuMusicVolume = MusicVolume;
    GameplayMusicVolume = MusicVolume;
    ResultMusicVolume = MusicVolume;
    
    if (MusicAudioComponent)
    {
        // Apply volume based on current music state
        switch (CurrentMusicState)
        {
            case EMusicState::Menu:
                MusicAudioComponent->SetVolumeMultiplier(MenuMusicVolume);
                break;
            case EMusicState::Gameplay:
                MusicAudioComponent->SetVolumeMultiplier(GameplayMusicVolume);
                break;
            case EMusicState::Result:
                MusicAudioComponent->SetVolumeMultiplier(ResultMusicVolume);
                break;
            default:
                MusicAudioComponent->SetVolumeMultiplier(MusicVolume);
                break;
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("BackgroundMusic: Volume set to %f (all music types)"), MusicVolume);
}

void UMyOnlineGameInstance::SetMenuMusicVolume(float Volume)
{
    MenuMusicVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
    UE_LOG(LogTemp, Log, TEXT("BackgroundMusic: Menu volume set to %f"), MenuMusicVolume);
    
    if (CurrentMusicState == EMusicState::Menu && MusicAudioComponent)
    {
        MusicAudioComponent->SetVolumeMultiplier(MenuMusicVolume);
    }
}

void UMyOnlineGameInstance::SetGameplayMusicVolume(float Volume)
{
    GameplayMusicVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
    UE_LOG(LogTemp, Log, TEXT("BackgroundMusic: Gameplay volume set to %f"), GameplayMusicVolume);
    
    if (CurrentMusicState == EMusicState::Gameplay && MusicAudioComponent)
    {
        MusicAudioComponent->SetVolumeMultiplier(GameplayMusicVolume);
    }
}

void UMyOnlineGameInstance::SetResultMusicVolume(float Volume)
{
    ResultMusicVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
    UE_LOG(LogTemp, Log, TEXT("BackgroundMusic: Result volume set to %f"), ResultMusicVolume);
    
    if (CurrentMusicState == EMusicState::Result && MusicAudioComponent)
    {
        MusicAudioComponent->SetVolumeMultiplier(ResultMusicVolume);
    }
}

bool UMyOnlineGameInstance::IsMusicPlaying() const
{
    return MusicAudioComponent && MusicAudioComponent->IsPlaying();
}

void UMyOnlineGameInstance::PlayNextTrack()
{
    if (!CurrentPlaylist || CurrentPlaylist->Num() == 0)
    {
        return;
    }

    UWorld* World = GetMusicWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("BackgroundMusic: No world found in PlayNextTrack"));
        
        // Try to get world from context - audio component might need to be recreated after travel
        if (GEngine)
        {
            World = GEngine->GetWorldFromContextObject(this, EGetWorldErrorMode::ReturnNull);
        }
        
        if (!World)
        {
            UE_LOG(LogTemp, Error, TEXT("BackgroundMusic: Still no world found, cannot play track"));
            return;
        }
    }

    USoundCue* CurrentTrack = (*CurrentPlaylist)[CurrentTrackIndex];
    if (!CurrentTrack)
    {
        UE_LOG(LogTemp, Warning, TEXT("BackgroundMusic: Invalid track at index %d"), CurrentTrackIndex);
        CurrentTrackIndex = (CurrentTrackIndex + 1) % CurrentPlaylist->Num();
        PlayNextTrack();
        return;
    }

    // Clean up previous component - check if it's still valid first
    if (MusicAudioComponent && MusicAudioComponent->IsValidLowLevel())
    {
        // Check if the component is in a valid world
        if (MusicAudioComponent->GetWorld() != World || !MusicAudioComponent->GetWorld())
        {
            // Audio component is from old world, destroy it
            UE_LOG(LogTemp, Log, TEXT("BackgroundMusic: Audio component from old world, cleaning up"));
            MusicAudioComponent->OnAudioFinished.Clear();
            MusicAudioComponent->DestroyComponent();
            MusicAudioComponent = nullptr;
        }
        else if (MusicAudioComponent->IsPlaying())
        {
            MusicAudioComponent->Stop();
            MusicAudioComponent->OnAudioFinished.Clear();
            MusicAudioComponent->DestroyComponent();
            MusicAudioComponent = nullptr;
        }
    }

    // Determine the appropriate volume based on current music state
    float CurrentVolume = MusicVolume;
    switch (CurrentMusicState)
    {
        case EMusicState::Menu:
            CurrentVolume = MenuMusicVolume;
            break;
        case EMusicState::Gameplay:
            CurrentVolume = GameplayMusicVolume;
            break;
        case EMusicState::Result:
            CurrentVolume = ResultMusicVolume;
            break;
        default:
            CurrentVolume = MenuMusicVolume;
            break;
    }

    // Spawn new 2D sound in current world
    MusicAudioComponent = UGameplayStatics::SpawnSound2D(World, CurrentTrack, CurrentVolume);

    if (MusicAudioComponent)
    {
        MusicAudioComponent->OnAudioFinished.AddDynamic(this, &UMyOnlineGameInstance::OnTrackFinished);
        UE_LOG(LogTemp, Log, TEXT("BackgroundMusic: Playing track %d: %s"), CurrentTrackIndex, *CurrentTrack->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("BackgroundMusic: Failed to spawn sound"));
    }
}

void UMyOnlineGameInstance::OnTrackFinished()
{
    if (!CurrentPlaylist || CurrentPlaylist->Num() == 0)
    {
        return;
    }
    
    if (bShufflePlaylist)
    {
        int32 NewIndex;
        do
        {
            NewIndex = FMath::RandRange(0, CurrentPlaylist->Num() - 1);
        } while (NewIndex == CurrentTrackIndex && CurrentPlaylist->Num() > 1);
        
        CurrentTrackIndex = NewIndex;
    }
    else
    {
        CurrentTrackIndex = (CurrentTrackIndex + 1) % CurrentPlaylist->Num();
    }
    
    PlayNextTrack();
}

// === Redesigned Music System Implementation ===

void UMyOnlineGameInstance::PrepareForLevelTravel()
{
    UE_LOG(LogTemp, Log, TEXT("MusicSystem: PrepareForLevelTravel - CurrentState=%d, IsPlaying=%d"),
        (int32)CurrentMusicState, IsMusicPlaying());

    bMusicWasPlayingBeforeTravel = IsMusicPlaying();

    // Don't stop music here - let it continue seamlessly through travel
    // Only stop it explicitly if needed for specific transitions
}

void UMyOnlineGameInstance::OnEnteredLobby()
{
    UE_LOG(LogTemp, Log, TEXT("MusicSystem: OnEnteredLobby"));

    bIsInMatch = false;
    bMatchHasEnded = false;
    CurrentMusicState = EMusicState::Menu;

    // Play menu music immediately when entering lobby
    // This ensures music continues through seamless travel from main menu
    PlayMenuMusic();

    UE_LOG(LogTemp, Log, TEXT("MusicSystem: Menu music started in lobby"));
}

void UMyOnlineGameInstance::OnMatchStarted()
{
    UE_LOG(LogTemp, Log, TEXT("MusicSystem: OnMatchStarted - playing gameplay music immediately"));

    bIsInMatch = true;
    bMatchHasEnded = false;
    CurrentMusicState = EMusicState::Gameplay;

    // Play gameplay music IMMEDIATELY - don't wait for OnStart()
    // This ensures music continues through seamless travel from lobby to game
    PlayGameplayMusic();

    UE_LOG(LogTemp, Log, TEXT("MusicSystem: Gameplay music started"));
}

void UMyOnlineGameInstance::OnMatchEnded(bool bIsVictory)
{
    UE_LOG(LogTemp, Log, TEXT("MusicSystem: OnMatchEnded - Victory=%d"), bIsVictory);

    bIsInMatch = false;
    bMatchHasEnded = true;
    CurrentMusicState = EMusicState::Result;

    // Play result music immediately for all players (host and clients)
    PlayResultMusic(bIsVictory);

    UE_LOG(LogTemp, Log, TEXT("MusicSystem: Result music should now be playing on all clients"));
}

void UMyOnlineGameInstance::OnReturningToLobby()
{
    UE_LOG(LogTemp, Log, TEXT("MusicSystem: OnReturningToLobby"));

    bIsInMatch = false;
    bMatchHasEnded = false;
    CurrentMusicState = EMusicState::Menu;

    // Stop current music first - we'll restart it after travel completes
    StopBackgroundMusic();

    // Note: We don't play music here immediately because we're about to travel to a new level
    // The lobby's OnEnteredLobby() or OnStart() will handle playing menu music after the level loads
    UE_LOG(LogTemp, Log, TEXT("MusicSystem: Returning to lobby - music will play after level loads"));
}

void UMyOnlineGameInstance::ResumeMenuMusic()
{
    UE_LOG(LogTemp, Log, TEXT("MusicSystem: ResumeMenuMusic"));

    bIsInMatch = false;
    bMatchHasEnded = false;
    CurrentMusicState = EMusicState::Menu;

    PlayMenuMusic();
}

