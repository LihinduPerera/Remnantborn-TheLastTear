#include "MyOnlineGameInstance.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystemTypes.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"


UMyOnlineGameInstance::UMyOnlineGameInstance()
{
    // Initialize delegates
    CreateSessionCompleteDelegate = FOnCreateSessionCompleteDelegate::CreateUObject(this, &UMyOnlineGameInstance::OnCreateSessionComplete);
    FindSessionsCompleteDelegate = FOnFindSessionsCompleteDelegate::CreateUObject(this, &UMyOnlineGameInstance::OnFindSessionsComplete);
    JoinSessionCompleteDelegate = FOnJoinSessionCompleteDelegate::CreateUObject(this, &UMyOnlineGameInstance::OnJoinSessionComplete);
    DestroySessionCompleteDelegate = FOnDestroySessionCompleteDelegate::CreateUObject(this, &UMyOnlineGameInstance::OnDestroySessionComplete);
    StartSessionCompleteDelegate = FOnStartSessionCompleteDelegate::CreateUObject(this, &UMyOnlineGameInstance::OnStartSessionComplete);
}

void UMyOnlineGameInstance::CreateSession(int32 MaxPlayers, FString SessionName)
{
    IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
    if (OnlineSubsystem)
    {
        SessionInterface = OnlineSubsystem->GetSessionInterface();
        
        if (SessionInterface.IsValid())
        {
            // Unbind existing handle if any
            if (CreateSessionCompleteDelegateHandle.IsValid())
            {
                SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
            }
            
            // Bind delegate
            CreateSessionCompleteDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegate);
            
            // Create session settings
            FOnlineSessionSettings SessionSettings;
            SessionSettings.bIsLANMatch = true; // Set to TRUE for LAN
            SessionSettings.NumPublicConnections = MaxPlayers;
            SessionSettings.bShouldAdvertise = true;
            SessionSettings.bAllowJoinInProgress = true;
            SessionSettings.bUsesPresence = true;
            SessionSettings.bAllowJoinViaPresence = true;
            SessionSettings.bUseLobbiesIfAvailable = true;
            
            // Custom session name
            SessionSettings.Set(FName("SESSION_NAME"), SessionName, EOnlineDataAdvertisementType::ViaOnlineService);
            
            // Create session
            if (!SessionInterface->CreateSession(0, NAME_GameSession, SessionSettings))
            {
                UE_LOG(LogTemp, Error, TEXT("Failed to create session"));
                SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Online Subsystem is not available"));
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
        
        // Travel to your game level as a listen server
        UWorld* World = GetWorld();
        if (World)
        {
            FString TravelPath = FString::Printf(TEXT("%s?listen"), *GetWorld()->GetName());
            World->ServerTravel(TravelPath);
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create session"));
    }
}

void UMyOnlineGameInstance::FindSessions()
{
    IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
    if (OnlineSubsystem)
    {
        SessionInterface = OnlineSubsystem->GetSessionInterface();
        
        if (SessionInterface.IsValid())
        {
            // Unbind existing handle
            if (FindSessionsCompleteDelegateHandle.IsValid())
            {
                SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
            }
            
            // Bind delegate
            FindSessionsCompleteDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegate);
            
            // Create search settings
            SessionSearch = MakeShareable(new FOnlineSessionSearch());
            SessionSearch->bIsLanQuery = true; // TRUE for LAN
            SessionSearch->MaxSearchResults = 10;
            SessionSearch->QuerySettings.Set(FName(TEXT("PRESENCE")), true, EOnlineComparisonOp::Equals);

            
            // Clear previous results
            SessionSearchResults.Empty();
            
            // Start search
            if (!SessionInterface->FindSessions(0, SessionSearch.ToSharedRef()))
            {
                UE_LOG(LogTemp, Error, TEXT("Failed to find sessions"));
                SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
                OnSessionSearchCompleted.Broadcast(false);
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Online Subsystem is not available"));
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
        
        // Convert to Blueprint-friendly results
        SessionSearchResults.Empty();
        for (const FOnlineSessionSearchResult& Result : SessionSearch->SearchResults)
        {
            FBlueprintSessionResult BPResult;
            BPResult.OnlineResult = Result;
            SessionSearchResults.Add(BPResult);
        }
        
        OnSessionSearchCompleted.Broadcast(true);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to find sessions"));
        OnSessionSearchCompleted.Broadcast(false);
    }
}

void UMyOnlineGameInstance::JoinSession(int32 SessionIndex)
{
    IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
    if (OnlineSubsystem)
    {
        SessionInterface = OnlineSubsystem->GetSessionInterface();
        
        if (SessionInterface.IsValid())
        {
            // Unbind existing handle
            if (JoinSessionCompleteDelegateHandle.IsValid())
            {
                SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
            }
            
            // Bind delegate
            JoinSessionCompleteDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegate);
            
            // Join the session
            if (SessionSearch->SearchResults.IsValidIndex(SessionIndex))
            {
                if (!SessionInterface->JoinSession(0, NAME_GameSession, SessionSearch->SearchResults[SessionIndex]))
                {
                    UE_LOG(LogTemp, Error, TEXT("Failed to join session"));
                    SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
                }
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Online Subsystem is not available"));
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
        UE_LOG(LogTemp, Log, TEXT("Successfully joined session"));
        
        // Get travel URL and connect
        APlayerController* PlayerController = GetFirstLocalPlayerController();
        if (PlayerController)
        {
            FString TravelURL;
            if (SessionInterface->GetResolvedConnectString(NAME_GameSession, TravelURL))
            {
                PlayerController->ClientTravel(TravelURL, TRAVEL_Absolute);
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to join session"));
    }
}

void UMyOnlineGameInstance::DestroySession()
{
    IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
    if (OnlineSubsystem)
    {
        SessionInterface = OnlineSubsystem->GetSessionInterface();
        
        if (SessionInterface.IsValid())
        {
            // Unbind existing handle
            if (DestroySessionCompleteDelegateHandle.IsValid())
            {
                SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
            }
            
            // Bind delegate
            DestroySessionCompleteDelegateHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegate);
            
            // Destroy session
            if (!SessionInterface->DestroySession(NAME_GameSession))
            {
                SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
            }
        }
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
}

void UMyOnlineGameInstance::StartSession()
{
    IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
    if (OnlineSubsystem)
    {
        SessionInterface = OnlineSubsystem->GetSessionInterface();
        
        if (SessionInterface.IsValid())
        {
            // Unbind existing handle
            if (StartSessionCompleteDelegateHandle.IsValid())
            {
                SessionInterface->ClearOnStartSessionCompleteDelegate_Handle(StartSessionCompleteDelegateHandle);
            }
            
            // Bind delegate
            StartSessionCompleteDelegateHandle = SessionInterface->AddOnStartSessionCompleteDelegate_Handle(StartSessionCompleteDelegate);
            
            // Start session
            if (!SessionInterface->StartSession(NAME_GameSession))
            {
                SessionInterface->ClearOnStartSessionCompleteDelegate_Handle(StartSessionCompleteDelegateHandle);
            }
        }
    }
}

void UMyOnlineGameInstance::OnStartSessionComplete(FName SessionName, bool bSuccess)
{
    if (SessionInterface)
    {
        SessionInterface->ClearOnStartSessionCompleteDelegate_Handle(StartSessionCompleteDelegateHandle);
    }
    
    if (bSuccess)
    {
        UE_LOG(LogTemp, Log, TEXT("Session started"));
    }
}