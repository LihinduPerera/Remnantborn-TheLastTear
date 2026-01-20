#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "FindSessionsCallbackProxy.h"
#include "MyOnlineGameInstance.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionSearchCompleted, bool, bSuccess);

UCLASS()
class ELECTRICDREAMSSAMPLE_API UMyOnlineGameInstance : public UGameInstance
{
    GENERATED_BODY()
    
public:
    UMyOnlineGameInstance();
    
    // Session Functions
    UFUNCTION(BlueprintCallable, Category = "Multiplayer")
    void CreateSession(int32 MaxPlayers = 4, FString SessionName = "MyGameSession");
    
    UFUNCTION(BlueprintCallable, Category = "Multiplayer")
    void FindSessions();
    
    UFUNCTION(BlueprintCallable, Category = "Multiplayer")
    void JoinSession(int32 SessionIndex);
    
    UFUNCTION(BlueprintCallable, Category = "Multiplayer")
    void StartSession();
    
    UFUNCTION(BlueprintCallable, Category = "Multiplayer")
    void DestroySession();
    
    // Blueprint Events
    UPROPERTY(BlueprintAssignable)
    FOnSessionSearchCompleted OnSessionSearchCompleted;
    
    UPROPERTY(BlueprintReadOnly, Category = "Multiplayer")
    TArray<FBlueprintSessionResult> SessionSearchResults;
    
protected:
    // Session Delegates
    void OnCreateSessionComplete(FName SessionName, bool bSuccess);
    void OnFindSessionsComplete(bool bSuccess);
    void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
    void OnDestroySessionComplete(FName SessionName, bool bSuccess);
    void OnStartSessionComplete(FName SessionName, bool bSuccess);
    
private:
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
    
    FOnStartSessionCompleteDelegate StartSessionCompleteDelegate;
    FDelegateHandle StartSessionCompleteDelegateHandle;
};