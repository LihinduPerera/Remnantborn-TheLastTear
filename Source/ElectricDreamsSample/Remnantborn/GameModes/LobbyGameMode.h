#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "LobbyGameMode.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLobbyReadyChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLobbyCountdownStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLobbyCountdownUpdated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLobbyCountdownCancelled);

UCLASS()
class ELECTRICDREAMSSAMPLE_API ALobbyGameMode : public AGameModeBase
{
    GENERATED_BODY()
    
public:
    ALobbyGameMode();
    
    virtual void BeginPlay() override;
    virtual void PostLogin(APlayerController* NewPlayer) override;
    virtual void Logout(AController* Exiting) override;
    
    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void SetMaxPlayers(int32 NewMaxPlayers);
    
    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void StartMatchCountdown();
    
    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void CancelMatchCountdown();
    
    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void StartMatchImmediately();
    
    UFUNCTION(BlueprintPure, Category = "Lobby")
    bool CanStartMatch() const;
    
    UFUNCTION(BlueprintPure, Category = "Lobby")
    int32 GetCurrentPlayerCount() const { return CurrentPlayerCount; }
    
    UFUNCTION(BlueprintPure, Category = "Lobby")
    int32 GetMaxPlayers() const { return MaxPlayers; }
    
    UFUNCTION(BlueprintPure, Category = "Lobby")
    bool IsCountdownActive() const { return bCountdownActive; }
    
    UFUNCTION(BlueprintPure, Category = "Lobby")
    int32 GetCountdownTime() const { return CountdownTime; }
    
    // Events
    UPROPERTY(BlueprintAssignable, Category = "Lobby|Events")
    FOnLobbyReadyChanged OnLobbyReadyChanged;
    
    UPROPERTY(BlueprintAssignable, Category = "Lobby|Events")
    FOnLobbyCountdownStarted OnLobbyCountdownStarted;
    
    UPROPERTY(BlueprintAssignable, Category = "Lobby|Events")
    FOnLobbyCountdownUpdated OnLobbyCountdownUpdated;
    
    UPROPERTY(BlueprintAssignable, Category = "Lobby|Events")
    FOnLobbyCountdownCancelled OnLobbyCountdownCancelled;
    
protected:
    UFUNCTION()
    void UpdateCountdown();
    
    UFUNCTION()
    void StartMatchTravel();
    
    UFUNCTION()
    void UpdateSessionSettings();
    
private:
    UPROPERTY(EditDefaultsOnly, Category = "Lobby")
    int32 MaxPlayers = 2; // Default to 2 players
    
    UPROPERTY(EditDefaultsOnly, Category = "Lobby")
    int32 CountdownDuration = 5; // Seconds
    
    UPROPERTY(EditDefaultsOnly, Category = "Lobby")
    FString GameMapPath = "/Game/Remnantborn/Levels/TestGround";
    
    int32 CurrentPlayerCount = 0;
    int32 CountdownTime = 0;
    bool bCountdownActive = false;
    
    FTimerHandle CountdownTimerHandle;
};
