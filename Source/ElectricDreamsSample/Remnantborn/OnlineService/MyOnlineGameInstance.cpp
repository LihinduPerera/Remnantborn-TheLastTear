#include "MyOnlineGameInstance.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "OnlineSessionSettings.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Interfaces/OnlineSessionInterface.h"

UMyOnlineGameInstance::UMyOnlineGameInstance()
{
    // Constructor
    MapPathToTravel = TEXT("/Game/Remnantborn/Levels/Multiplayer1"); // Default map
}

void UMyOnlineGameInstance::CreateSession(FString SessionName, int32 MaxPlayers)
{
    // Get the online subsystem
    IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
    if (OnlineSub)
    {
        SessionInterface = OnlineSub->GetSessionInterface();
        
        if (SessionInterface.IsValid())
        {
            // Check if session already exists
            if (SessionInterface->GetNamedSession(NAME_GameSession))
            {
                UE_LOG(LogTemp, Warning, TEXT("Session already exists, destroying old one"));
                DestroySession();
                
                // Store session parameters for later use
                CachedSessionName = SessionName;
                CachedMaxPlayers = MaxPlayers;
                bWaitingForDestroyToCreate = true;
                return;
            }
            
            // Setup session settings
            FOnlineSessionSettings SessionSettings;
            
            // LAN SETTINGS
            SessionSettings.bIsLANMatch = true; // MUST BE TRUE for LAN
            SessionSettings.bUsesPresence = true;
            SessionSettings.NumPublicConnections = MaxPlayers;
            SessionSettings.bShouldAdvertise = true;
            SessionSettings.bAllowJoinInProgress = true;
            SessionSettings.bAllowInvites = true;
            
            // Important for LAN
            SessionSettings.bIsDedicated = false;
            SessionSettings.bUsesStats = false;
            
            // Set custom session name
            SessionSettings.Set(FName("SESSION_NAME"), SessionName, EOnlineDataAdvertisementType::ViaOnlineService);
            
            // Bind delegate
            CreateSessionCompleteDelegate = FOnCreateSessionCompleteDelegate::CreateUObject(this, &UMyOnlineGameInstance::OnCreateSessionComplete);
            CreateSessionCompleteDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegate);
            
            // Create session with first local player
            ULocalPlayer* LocalPlayer = GetFirstGamePlayer();
            if (LocalPlayer)
            {
                if (!SessionInterface->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, SessionSettings))
                {
                    UE_LOG(LogTemp, Error, TEXT("Failed to create session"));
                    SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
                    OnCreateSessionComplete(NAME_GameSession, false);
                }
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("No local player found"));
                OnCreateSessionComplete(NAME_GameSession, false);
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Session interface is not valid"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Online subsystem not found"));
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
        UE_LOG(LogTemp, Log, TEXT("Session created successfully!"));
        
        // Broadcast to blueprint
        OnCreateSessionSuccess.Broadcast();
        
        // Travel to the specified map as a listen server
        UWorld* World = GetWorld();
        if (World)
        {
            FString TravelPath = MapPathToTravel;
            
            // Remove any prefix
            TravelPath.RemoveFromStart(World->StreamingLevelsPrefix);
            
            // Append ?listen to make it a listen server
            TravelPath += TEXT("?listen");
            
            UE_LOG(LogTemp, Log, TEXT("Server traveling to: %s"), *TravelPath);
            World->ServerTravel(TravelPath);
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create session"));
        OnCreateSessionFailed.Broadcast();
    }
}

void UMyOnlineGameInstance::FindSessions()
{
    // Get the online subsystem
    IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
    if (OnlineSub)
    {
        SessionInterface = OnlineSub->GetSessionInterface();
        
        if (SessionInterface.IsValid())
        {
            // Setup search settings for LAN
            SessionSearch = MakeShareable(new FOnlineSessionSearch());
            SessionSearch->bIsLanQuery = true; // Critical for LAN
            SessionSearch->MaxSearchResults = 20;
            
            // For LAN, we can search without presence query
            // Just search all available LAN sessions
            
            // Bind delegate
            FindSessionsCompleteDelegate = FOnFindSessionsCompleteDelegate::CreateUObject(this, &UMyOnlineGameInstance::OnFindSessionsComplete);
            FindSessionsCompleteDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegate);
            
            // Start searching
            ULocalPlayer* LocalPlayer = GetFirstGamePlayer();
            if (LocalPlayer)
            {
                UE_LOG(LogTemp, Log, TEXT("Starting LAN session search..."));
                SessionInterface->FindSessions(*LocalPlayer->GetPreferredUniqueNetId(), SessionSearch.ToSharedRef());
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("No local player for session search"));
                OnFindSessionsComplete(false);
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Session interface is not valid"));
            OnFindSessionsComplete(false);
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Online subsystem not found"));
        OnFindSessionsComplete(false);
    }
}

void UMyOnlineGameInstance::OnFindSessionsComplete(bool bSuccess)
{
    if (SessionInterface)
    {
        SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
    }
    
    SessionSearchResults.Empty();
    
    if (bSuccess && SessionSearch.IsValid())
    {
        UE_LOG(LogTemp, Log, TEXT("Found %d sessions"), SessionSearch->SearchResults.Num());
        
        for (const FOnlineSessionSearchResult& Result : SessionSearch->SearchResults)
        {
            FBlueprintSessionResult BPResult;
            BPResult.OnlineResult = Result;
            SessionSearchResults.Add(BPResult);
            
            // Log found session info
            FString SessionName;
            Result.Session.SessionSettings.Get(FName("SESSION_NAME"), SessionName);
            UE_LOG(LogTemp, Log, TEXT("Found session: %s, Ping: %d"), 
                *SessionName, Result.PingInMs);
        }
        
        OnSessionSearchCompleted.Broadcast(true);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("No sessions found or search failed"));
        OnSessionSearchCompleted.Broadcast(false);
    }
}

void UMyOnlineGameInstance::JoinSession(FBlueprintSessionResult SessionResult)
{
    // Get the online subsystem
    IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
    if (OnlineSub)
    {
        SessionInterface = OnlineSub->GetSessionInterface();
        
        if (SessionInterface.IsValid())
        {
            // Bind delegate
            JoinSessionCompleteDelegate = FOnJoinSessionCompleteDelegate::CreateUObject(this, &UMyOnlineGameInstance::OnJoinSessionComplete);
            JoinSessionCompleteDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegate);
            
            // Join session
            ULocalPlayer* LocalPlayer = GetFirstGamePlayer();
            if (LocalPlayer)
            {
                UE_LOG(LogTemp, Log, TEXT("Attempting to join session..."));
                if (!SessionInterface->JoinSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, SessionResult.OnlineResult))
                {
                    UE_LOG(LogTemp, Error, TEXT("Failed to call JoinSession"));
                    SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
                    OnJoinSessionComplete(NAME_GameSession, EOnJoinSessionCompleteResult::UnknownError);
                }
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("No local player for join session"));
                SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
                OnJoinSessionComplete(NAME_GameSession, EOnJoinSessionCompleteResult::UnknownError);
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Session interface is not valid"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Online subsystem not found"));
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
            APlayerController* PlayerController = GetFirstLocalPlayerController();
            if (PlayerController)
            {
                UE_LOG(LogTemp, Log, TEXT("Traveling to: %s"), *ConnectString);
                PlayerController->ClientTravel(ConnectString, TRAVEL_Absolute);
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to get connect string"));
            // Try manual travel as fallback
            APlayerController* PlayerController = GetFirstLocalPlayerController();
            if (PlayerController)
            {
                // Try default connection
                FString TravelURL = TEXT("127.0.0.1:7777"); // Default localhost
                UE_LOG(LogTemp, Warning, TEXT("Using fallback travel to: %s"), *TravelURL);
                PlayerController->ClientTravel(TravelURL, TRAVEL_Absolute);
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to join session: %d"), static_cast<int32>(Result));
        OnJoinSessionFailed.Broadcast();
    }
}

void UMyOnlineGameInstance::JoinByIP(FString IPAddress, int32 Port)
{
    APlayerController* PlayerController = GetFirstLocalPlayerController();
    if (PlayerController)
    {
        // Format the travel URL
        FString TravelURL = FString::Printf(TEXT("%s:%d"), *IPAddress, Port);
        UE_LOG(LogTemp, Log, TEXT("Joining server at: %s"), *TravelURL);
        PlayerController->ClientTravel(TravelURL, TRAVEL_Absolute);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("No player controller found"));
    }
}

void UMyOnlineGameInstance::DestroySession()
{
    // Get the online subsystem
    IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
    if (OnlineSub)
    {
        SessionInterface = OnlineSub->GetSessionInterface();
        
        if (SessionInterface.IsValid())
        {
            DestroySessionCompleteDelegate = FOnDestroySessionCompleteDelegate::CreateUObject(this, &UMyOnlineGameInstance::OnDestroySessionComplete);
            DestroySessionCompleteDelegateHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegate);
            
            if (!SessionInterface->DestroySession(NAME_GameSession))
            {
                UE_LOG(LogTemp, Error, TEXT("Failed to call DestroySession"));
                SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
                OnDestroySessionComplete(NAME_GameSession, false);
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Session interface is not valid"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Online subsystem not found"));
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
        
        // If we were waiting to create a new session after destruction
        if (bWaitingForDestroyToCreate)
        {
            bWaitingForDestroyToCreate = false;
            CreateSession(CachedSessionName, CachedMaxPlayers);
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to destroy session"));
    }
}