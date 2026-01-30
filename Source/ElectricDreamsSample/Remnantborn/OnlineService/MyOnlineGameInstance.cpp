#include "MyOnlineGameInstance.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "OnlineSessionSettings.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"
#include "Kismet/GameplayStatics.h"

UMyOnlineGameInstance::UMyOnlineGameInstance()
{
    DefaultMapPath = TEXT("/Game/Remnantborn/Levels/TestGround");
    DefaultMaxPlayers = 4;
    bIsHosting = false;
    bIsAuthenticated = false;
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
    
    // Initialize HTTP Manager
    HttpManager = NewObject<UHttpManager>(this);
    if (HttpManager)
    {
        // Configure base URL (you might want to make this configurable)
        HttpManager->Initialize(TEXT("http://localhost:3000/api"));
        
        // Bind HTTP Manager events
        HttpManager->OnLoginComplete.AddDynamic(this, &UMyOnlineGameInstance::HandleLoginComplete);
        HttpManager->OnSignupComplete.AddDynamic(this, &UMyOnlineGameInstance::HandleSignupComplete);
        HttpManager->OnTokenVerified.AddDynamic(this, &UMyOnlineGameInstance::HandleTokenVerified);
        HttpManager->OnProfileLoaded.AddDynamic(this, &UMyOnlineGameInstance::HandleProfileLoaded);
        HttpManager->OnApiError.AddDynamic(this, &UMyOnlineGameInstance::HandleApiError);
        
        // Try to load saved authentication
        LoadSavedAuth();
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create HTTP Manager"));
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
    
    // Clean up HTTP Manager
    if (HttpManager)
    {
        HttpManager->ConditionalBeginDestroy();
        HttpManager = nullptr;
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
    SessionSettings.bIsLANMatch = true; // CRITICAL for LAN
    SessionSettings.bIsDedicated = false; // Listen server
    SessionSettings.bUsesPresence = true; // Needed for LAN discovery
    SessionSettings.bShouldAdvertise = true; // Make it visible
    SessionSettings.bAllowJoinInProgress = true; // Allow late joins
    SessionSettings.bAllowInvites = true;
    SessionSettings.NumPublicConnections = MaxPlayers;
    SessionSettings.NumPrivateConnections = 0;
  
    // Set custom parameters - FIXED: Using proper FVariantData type
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
      
        // Travel to the game map as listen server
        if (!DefaultMapPath.IsEmpty())
        {
            FString TravelPath = DefaultMapPath;
            TravelPath.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);
            TravelPath += TEXT("?listen");
          
            UE_LOG(LogTemp, Log, TEXT("Server traveling to: %s"), *TravelPath);
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
    SessionSearch->bIsLanQuery = true; // CRITICAL for LAN
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
        JoinSession(SessionSearchResults[SessionIndex].SessionResult);
    }
    else
    {
        LastError = "Invalid session index";
        OnJoinSessionFailed.Broadcast(LastError);
    }
}

void UMyOnlineGameInstance::JoinSession(const FBlueprintSessionResult& SessionResult)
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

// === Authentication Methods ===

void UMyOnlineGameInstance::Login(const FString& Email, const FString& Password)
{
    if (HttpManager)
    {
        HttpManager->Login(Email, Password);
    }
    else
    {
        FAuthResponse AuthResponse;
        AuthResponse.bSuccess = false;
        AuthResponse.ErrorMessage = TEXT("HTTP Manager not initialized");
        OnAuthLoginComplete.Broadcast(AuthResponse);
    }
}

void UMyOnlineGameInstance::Signup(const FString& Email, const FString& Password, const FString& Username)
{
    if (HttpManager)
    {
        HttpManager->Signup(Email, Password, Username);
    }
    else
    {
        FAuthResponse AuthResponse;
        AuthResponse.bSuccess = false;
        AuthResponse.ErrorMessage = TEXT("HTTP Manager not initialized");
        OnAuthSignupComplete.Broadcast(AuthResponse);
    }
}

void UMyOnlineGameInstance::DevLogin(const FString& Email)
{
    if (HttpManager)
    {
        HttpManager->DevLogin(Email);
    }
    else
    {
        FAuthResponse AuthResponse;
        AuthResponse.bSuccess = false;
        AuthResponse.ErrorMessage = TEXT("HTTP Manager not initialized");
        OnAuthLoginComplete.Broadcast(AuthResponse);
    }
}

void UMyOnlineGameInstance::Logout()
{
    if (HttpManager)
    {
        HttpManager->Logout();
        bIsAuthenticated = false;
        CurrentUserProfile = FUserProfile();
    }
}

void UMyOnlineGameInstance::LoadSavedAuth()
{
    if (HttpManager)
    {
        if (HttpManager->LoadSavedAuth())
        {
            UE_LOG(LogTemp, Log, TEXT("Attempting to load saved authentication..."));
        }
    }
}

void UMyOnlineGameInstance::GetUserProfile()
{
    if (HttpManager && bIsAuthenticated && !CurrentUserProfile.UserId.IsEmpty())
    {
        HttpManager->GetProfile(CurrentUserProfile.UserId);
    }
}

void UMyOnlineGameInstance::UpdateProfile(const FString& Username, const FString& Bio)
{
    if (HttpManager && bIsAuthenticated && !CurrentUserProfile.UserId.IsEmpty())
    {
        HttpManager->UpdateProfile(CurrentUserProfile.UserId, Username, Bio);
    }
}

void UMyOnlineGameInstance::UpdateGameStats(int32 Level, int32 RemnantCount, const FString& Operation)
{
    if (HttpManager && bIsAuthenticated && !CurrentUserProfile.UserId.IsEmpty())
    {
        HttpManager->UpdateGameStats(CurrentUserProfile.UserId, Level, RemnantCount, Operation);
    }
}

bool UMyOnlineGameInstance::IsLoggedIn() const
{
    return bIsAuthenticated;
}

FUserProfile UMyOnlineGameInstance::GetCurrentUserProfile() const
{
    return CurrentUserProfile;
}

FString UMyOnlineGameInstance::GetAuthToken() const
{
    if (HttpManager)
    {
        return HttpManager->GetAuthToken();
    }
    return FString();
}

// === Event Handlers ===

void UMyOnlineGameInstance::HandleLoginComplete(const FAuthResponse& AuthResponse)
{
    if (AuthResponse.bSuccess)
    {
        bIsAuthenticated = true;
        CurrentUserProfile = AuthResponse.UserProfile;
        UE_LOG(LogTemp, Log, TEXT("Login successful for user: %s"), *CurrentUserProfile.Username);
    }
    else
    {
        bIsAuthenticated = false;
        UE_LOG(LogTemp, Warning, TEXT("Login failed: %s"), *AuthResponse.ErrorMessage);
    }
    
    OnAuthLoginComplete.Broadcast(AuthResponse);
}

void UMyOnlineGameInstance::HandleSignupComplete(const FAuthResponse& AuthResponse)
{
    if (AuthResponse.bSuccess)
    {
        bIsAuthenticated = true;
        CurrentUserProfile = AuthResponse.UserProfile;
        UE_LOG(LogTemp, Log, TEXT("Signup successful for user: %s"), *CurrentUserProfile.Username);
    }
    else
    {
        bIsAuthenticated = false;
        UE_LOG(LogTemp, Warning, TEXT("Signup failed: %s"), *AuthResponse.ErrorMessage);
    }
    
    OnAuthSignupComplete.Broadcast(AuthResponse);
}

void UMyOnlineGameInstance::HandleTokenVerified(const FAuthResponse& AuthResponse)
{
    if (AuthResponse.bSuccess)
    {
        bIsAuthenticated = true;
        CurrentUserProfile = AuthResponse.UserProfile;
        UE_LOG(LogTemp, Log, TEXT("Token verified for user: %s"), *CurrentUserProfile.Username);
        
        // Get full profile
        GetUserProfile();
    }
    else
    {
        bIsAuthenticated = false;
        UE_LOG(LogTemp, Warning, TEXT("Token verification failed: %s"), *AuthResponse.ErrorMessage);
    }
}

void UMyOnlineGameInstance::HandleProfileLoaded(const FUserProfile& UserProfile)
{
    CurrentUserProfile = UserProfile;
    OnProfileUpdated.Broadcast(UserProfile);
    
    UE_LOG(LogTemp, Log, TEXT("Profile loaded: %s (Level: %d, Remnants: %d)"),
           *UserProfile.Username, UserProfile.Level, UserProfile.RemnantCount);
}

void UMyOnlineGameInstance::HandleApiError(const FString& ErrorMessage)
{
    UE_LOG(LogTemp, Error, TEXT("API Error: %s"), *ErrorMessage);
    LastError = ErrorMessage;
}