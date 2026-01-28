#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "FindSessionsCallbackProxy.h"
#include "MyOnlineGameInstance.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionSearchCompleted, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCreateSessionSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCreateSessionFailed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnJoinSessionFailed);

UCLASS()
class ELECTRICDREAMSSAMPLE_API UMyOnlineGameInstance : public UGameInstance
{
    GENERATED_BODY()
    
public:
    UMyOnlineGameInstance();
    
    // Session Functions
    UFUNCTION(BlueprintCallable, Category = "Multiplayer")
    void CreateSession(FString SessionName = "MyGameSession", int32 MaxPlayers = 4);
    
    UFUNCTION(BlueprintCallable, Category = "Multiplayer")
    void FindSessions();
    
    UFUNCTION(BlueprintCallable, Category = "Multiplayer")
    void JoinSession(FBlueprintSessionResult SessionResult);
    
    UFUNCTION(BlueprintCallable, Category = "Multiplayer")
    void DestroySession();
    
    // Simple function for direct IP connection
    UFUNCTION(BlueprintCallable, Category = "Multiplayer")
    void JoinByIP(FString IPAddress, int32 Port = 7777);
    
    // Blueprint Events
    UPROPERTY(BlueprintAssignable)
    FOnSessionSearchCompleted OnSessionSearchCompleted;
    
    UPROPERTY(BlueprintAssignable)
    FOnCreateSessionSuccess OnCreateSessionSuccess;
    
    UPROPERTY(BlueprintAssignable)
    FOnCreateSessionFailed OnCreateSessionFailed;
    
    UPROPERTY(BlueprintAssignable)
    FOnJoinSessionFailed OnJoinSessionFailed;
    
    UPROPERTY(BlueprintReadOnly, Category = "Multiplayer")
    TArray<FBlueprintSessionResult> SessionSearchResults;

    // The map path to travel to when creating a session. Set this in the blueprint instance.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multiplayer")
    FString MapPathToTravel;

private:
    // Session Delegates
    void OnCreateSessionComplete(FName SessionName, bool bSuccess);
    void OnFindSessionsComplete(bool bSuccess);
    void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
    void OnDestroySessionComplete(FName SessionName, bool bSuccess);
    
    IOnlineSessionPtr SessionInterface;
    TSharedPtr<FOnlineSessionSearch> SessionSearch;
    
    // Delegate Handles
    FOnCreateSessionCompleteDelegate CreateSessionCompleteDelegate;
    FDelegateHandle CreateSessionCompleteDelegateHandle;
    
    FOnFindSessionsCompleteDelegate FindSessionsCompleteDelegate;
    FDelegateHandle FindSessionsCompleteDelegateHandle;
    
    FOnJoinSessionCompleteDelegate JoinSessionCompleteDelegate;
    FDelegateHandle JoinSessionCompleteDelegateHandle;
    
    FOnDestroySessionCompleteDelegate DestroySessionCompleteDelegate;
    FDelegateHandle DestroySessionCompleteDelegateHandle;
    
    // Cached variables for session recreation
    bool bWaitingForDestroyToCreate = false;
    FString CachedSessionName;
    int32 CachedMaxPlayers;
};