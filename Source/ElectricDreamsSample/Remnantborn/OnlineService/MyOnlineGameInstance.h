#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "FindSessionsCallbackProxy.h"
#include "MyOnlineGameInstance.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionSearchCompleted, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCreateSessionSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCreateSessionFailed, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnJoinSessionFailed, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSessionDestroyed);

USTRUCT(BlueprintType)
struct FSessionInfo
{
    GENERATED_BODY()
    
    UPROPERTY(BlueprintReadOnly)
    FString SessionName;
    
    UPROPERTY(BlueprintReadOnly)
    int32 CurrentPlayers;
    
    UPROPERTY(BlueprintReadOnly)
    int32 MaxPlayers;
    
    UPROPERTY(BlueprintReadOnly)
    int32 Ping;
    
    UPROPERTY(BlueprintReadOnly)
    FBlueprintSessionResult SessionResult;
};

UCLASS()
class ELECTRICDREAMSSAMPLE_API UMyOnlineGameInstance : public UGameInstance
{
    GENERATED_BODY()
    
public:
    UMyOnlineGameInstance();
    
    // Session Functions
    UFUNCTION(BlueprintCallable, Category = "Multiplayer")
    void CreateSession(FString SessionName = "LAN_Game", int32 MaxPlayers = 4);
    
    UFUNCTION(BlueprintCallable, Category = "Multiplayer")
    void FindSessions();
    
    UFUNCTION(BlueprintCallable, Category = "Multiplayer")
    void JoinSessionByIndex(int32 SessionIndex);
    
    UFUNCTION(BlueprintCallable, Category = "Multiplayer")
    void JoinSession(const FBlueprintSessionResult& SessionResult);
    
    UFUNCTION(BlueprintCallable, Category = "Multiplayer")
    void DestroySession();
    
    UFUNCTION(BlueprintCallable, Category = "Multiplayer")
    void JoinByIP(FString IPAddress, int32 Port = 7777);
    
    UFUNCTION(BlueprintCallable, Category = "Multiplayer")
    void StartGameAsClient(FString IPAddress, int32 Port = 7777);
    
    UFUNCTION(BlueprintCallable, Category = "Multiplayer")
    void StartGameAsServer(FString MapPath, int32 MaxPlayers = 4);
    
    UFUNCTION(BlueprintCallable, Category = "Multiplayer")
    void LeaveGame();
    
    // Blueprint Events
    UPROPERTY(BlueprintAssignable, Category = "Multiplayer|Events")
    FOnSessionSearchCompleted OnSessionSearchCompleted;
    
    UPROPERTY(BlueprintAssignable, Category = "Multiplayer|Events")
    FOnCreateSessionSuccess OnCreateSessionSuccess;
    
    UPROPERTY(BlueprintAssignable, Category = "Multiplayer|Events")
    FOnCreateSessionFailed OnCreateSessionFailed;
    
    UPROPERTY(BlueprintAssignable, Category = "Multiplayer|Events")
    FOnJoinSessionFailed OnJoinSessionFailed;
    
    UPROPERTY(BlueprintAssignable, Category = "Multiplayer|Events")
    FOnSessionDestroyed OnSessionDestroyed;
    
    // Data for Blueprint
    UPROPERTY(BlueprintReadOnly, Category = "Multiplayer")
    TArray<FSessionInfo> SessionSearchResults;
    
    UPROPERTY(BlueprintReadOnly, Category = "Multiplayer")
    FString LastError;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multiplayer")
    FString DefaultMapPath;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multiplayer")
    int32 DefaultMaxPlayers = 4;

protected:
    virtual void Init() override;
    virtual void Shutdown() override;
    
private:
    // Session Delegates
    void OnCreateSessionComplete(FName SessionName, bool bSuccess);
    void OnFindSessionsComplete(bool bSuccess);
    void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
    void OnDestroySessionComplete(FName SessionName, bool bSuccess);
    
    void HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);
    void HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString);
    
    // Connection Functions
    void HostGame();
    void TravelToServer(const FString& Address);
    
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
    
    // Cached variables
    FString PendingSessionName;
    int32 PendingMaxPlayers;
    bool bIsHosting = false;
};