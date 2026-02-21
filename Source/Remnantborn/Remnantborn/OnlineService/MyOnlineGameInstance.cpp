#include "MyOnlineGameInstance.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "OnlineSessionSettings.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"

UMyOnlineGameInstance::UMyOnlineGameInstance()
{
    DefaultMapPath = TEXT("/Game/Remnantborn/Levels/TestGround");
    DefaultMaxPlayers = 4;
    bIsHosting = false;
    bIsLoggedIn = false;
    LobbyMapPath = TEXT("/Game/Remnantborn/Levels/Lobby");
    
    MusicVolume = 0.5f;
    bShufflePlaylist = true;
    bAutoPlayOnLevelChange = true;
    CurrentTrackIndex = 0;
    MusicAudioComponent = nullptr;
    CurrentPlaylist = nullptr;
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
        HttpService->Initialize(TEXT("http://localhost:3000/api"));
        
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
    
    // Handle music on level changes - OnStart is called after each level loads
    if (bAutoPlayOnLevelChange && GetWorld())
    {
        FString CurrentMapName = GetWorld()->GetMapName();
        CurrentMapName.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);

        UE_LOG(LogTemp, Log, TEXT("OnStart: Current map = %s, MusicState = %d"), *CurrentMapName, (int32)CurrentMusicState);

        // Always restart music based on the current state - don't rely on IsMusicPlaying()
        // because the audio component doesn't survive level transitions
        if (CurrentMapName == TEXT("MainMenu") || CurrentMapName == TEXT("Lobby"))
        {
            CurrentMusicState = EMusicState::Menu;
            PlayMenuMusic();
        }
        else
        {
            CurrentMusicState = EMusicState::Gameplay;
            PlayGameplayMusic();
        }
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
    
    // Destroy existing session if any
    if (SessionInterface->GetNamedSession(NAME_GameSession))
    {
        UE_LOG(LogTemp, Warning, TEXT("Destroying existing session before creating new one"));
        DestroySession();
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
    APlayerController* PlayerController = GetFirstLocalPlayerController();
    if (PlayerController)
    {
        // Reset input mode before traveling
        FInputModeGameOnly InputMode;
        PlayerController->SetInputMode(InputMode);
        PlayerController->SetShowMouseCursor(false);
        PlayerController->bShowMouseCursor = false;
        
        UE_LOG(LogTemp, Log, TEXT("Client traveling to: %s"), *Address);
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
        OnSessionDestroyed.Broadcast();
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
        bIsHosting = false;
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to destroy session"));
    }
    
    OnSessionDestroyed.Broadcast();
}

void UMyOnlineGameInstance::LeaveGame()
{
    // If we're hosting, destroy the session
    if (bIsHosting)
    {
        DestroySession();
    }
    
    // Switch to menu music and return to main menu
    CurrentMusicState = EMusicState::Menu;
    
    // Return to main menu
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

void UMyOnlineGameInstance::UpdateProfile(const FString& Username, const FString& Bio)
{
    if (!HttpService || !bIsLoggedIn || CurrentUserId.IsEmpty() || AuthToken.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot update profile: Not logged in"));
        return;
    }
    
    HttpService->UpdateProfile(CurrentUserId, AuthToken, Username, Bio, FOnSimpleResponse::CreateLambda([this](bool bSuccess)
    {
        if (bSuccess)
        {
            UE_LOG(LogTemp, Log, TEXT("Profile updated successfully"));
            // Refresh profile
            GetUserProfile();
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to update profile"));
        }
    }));
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

void UMyOnlineGameInstance::SetAuthState(bool bLoggedIn, const FString& Token, const FString& UserId, const FUserProfile& Profile)
{
    bIsLoggedIn = bLoggedIn;
    
    if (bLoggedIn)
    {
        AuthToken = Token;
        CurrentUserId = UserId;
        CurrentUserProfile = Profile;
        
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
    CurrentMusicState = EMusicState::Gameplay;
    
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
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("BackgroundMusic: Playing MainMenu playlist"));
    CurrentMusicState = EMusicState::Menu;
    PlayFromPlaylist(&MainMenuPlaylist);
}

void UMyOnlineGameInstance::PlayGameplayMusic()
{
    if (GameplayPlaylist.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("BackgroundMusic: No tracks in Gameplay playlist"));
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("BackgroundMusic: Playing Gameplay playlist (random)"));
    CurrentMusicState = EMusicState::Gameplay;
    bShufflePlaylist = true;
    PlayFromPlaylist(&GameplayPlaylist);
}

void UMyOnlineGameInstance::PlayResultMusic(bool bIsVictory)
{
    if (ResultPlaylist.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("BackgroundMusic: No tracks in Result playlist"));
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("BackgroundMusic: Playing Result playlist"));
    
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
    
    if (MusicAudioComponent)
    {
        MusicAudioComponent->SetVolumeMultiplier(MusicVolume);
    }
    
    UE_LOG(LogTemp, Log, TEXT("BackgroundMusic: Volume set to %f"), MusicVolume);
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
        return;
    }
    
    USoundCue* CurrentTrack = (*CurrentPlaylist)[CurrentTrackIndex];
    if (!CurrentTrack)
    {
        UE_LOG(LogTemp, Warning, TEXT("BackgroundMusic: Invalid track at index %d"), CurrentTrackIndex);
        CurrentTrackIndex = (CurrentTrackIndex + 1) % CurrentPlaylist->Num();
        PlayNextTrack();
        return;
    }
    
    // Clean up previous component
    if (MusicAudioComponent)
    {
        MusicAudioComponent->Stop();
        MusicAudioComponent->OnAudioFinished.Clear();
        MusicAudioComponent->DestroyComponent();
        MusicAudioComponent = nullptr;
    }
    
    // Spawn new 2D sound
    MusicAudioComponent = UGameplayStatics::SpawnSound2D(World, CurrentTrack, MusicVolume);
    
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

