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
    DefaultMapPath = TEXT("/Game/Remnantborn/Levels/Multiplayer1");
    DefaultMaxPlayers = 4;
    bIsHosting = false;
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
}

void UMyOnlineGameInstance::Shutdown()
{
    if (GEngine)
    {
        GEngine->OnNetworkFailure().RemoveAll(this);
        GEngine->OnTravelFailure().RemoveAll(this);
    }
    
    DestroySession();
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
    SessionSettings.bIsLANMatch = true;              // CRITICAL for LAN
    SessionSettings.bIsDedicated = false;           // Listen server
    SessionSettings.bUsesPresence = true;           // Needed for LAN discovery
    SessionSettings.bShouldAdvertise = true;        // Make it visible
    SessionSettings.bAllowJoinInProgress = true;    // Allow late joins
    SessionSettings.bAllowInvites = true;
    SessionSettings.NumPublicConnections = MaxPlayers;
    SessionSettings.NumPrivateConnections = 0;
    
    // Set custom parameters
    SessionSettings.Set(FName("SESSION_NAME"), SessionName, EOnlineDataAdvertisementType::ViaOnlineService);
    SessionSettings.Set(FName("GAME_VERSION"), FString("1.0"), EOnlineDataAdvertisementType::ViaOnlineService);
    
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
    SessionSearch->bIsLanQuery = true;              // CRITICAL for LAN
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
        
        for (const FOnlineSessionSearchResult& SearchResult : SessionSearch->SearchResults)
        {
            FSessionInfo SessionInfo;
            
            // Extract session name
            FString SessionName;
            if (SearchResult.Session.SessionSettings.Get(FName("SESSION_NAME"), SessionName))
            {
                SessionInfo.SessionName = SessionName;
            }
            else
            {
                SessionInfo.SessionName = FString::Printf(TEXT("Session_%d"), SessionSearchResults.Num());
            }
            
            // Get player counts
            SessionInfo.CurrentPlayers = SearchResult.Session.NumOpenPublicConnections;
            SessionInfo.MaxPlayers = SearchResult.Session.SessionSettings.NumPublicConnections;
            SessionInfo.Ping = SearchResult.PingInMs;
            SessionInfo.SessionResult.OnlineResult = SearchResult;
            
            SessionSearchResults.Add(SessionInfo);
            UE_LOG(LogTemp, Log, TEXT("Found session: %s (%d/%d players)"), 
                *SessionInfo.SessionName, 
                SessionInfo.MaxPlayers - SessionInfo.CurrentPlayers,
                SessionInfo.MaxPlayers);
        }
        
        OnSessionSearchCompleted.Broadcast(true);
    }
    else
    {
        LastError = "No sessions found or search failed";
        UE_LOG(LogTemp, Warning, TEXT("Session search failed"));
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