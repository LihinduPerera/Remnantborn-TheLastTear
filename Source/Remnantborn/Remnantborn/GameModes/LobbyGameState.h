#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Remnantborn/Remnantborn/Lobby/LobbyCharacterManager.h"
#include "LobbyGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLobbyStateChanged);

USTRUCT(BlueprintType)
struct FLobbyPlayerInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    int32 PlayerId = 0;

    UPROPERTY(BlueprintReadOnly)
    FString PlayerName = TEXT("");

    UPROPERTY(BlueprintReadOnly)
    bool bIsReady = false;

    UPROPERTY(BlueprintReadOnly)
    bool bHasSelectedCharacter = false;

    FLobbyPlayerInfo() {}
};

UCLASS()
class REMNANTBORN_API ALobbyGameState : public AGameStateBase
{
    GENERATED_BODY()

public:
    ALobbyGameState();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // Lobby state
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Lobby")
    int32 MaxPlayers = 2;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Lobby")
    int32 CurrentPlayerCount = 0;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Lobby")
    bool bCountdownActive = false;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Lobby")
    int32 CountdownTime = 0;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Lobby")
    bool bLobbyLocked = false;

    // Player info array
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Lobby")
    TArray<FLobbyPlayerInfo> PlayerInfoArray;

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Lobby|Events")
    FOnLobbyStateChanged OnLobbyStateChanged;

    // Update functions (called by GameMode on server)
    void UpdatePlayerInfo(APlayerController* PlayerController, bool bReady, bool bHasCharacter);
    void RemovePlayerInfo(APlayerController* PlayerController);
    void SetCountdownState(bool bActive, int32 Time);
    void SetLobbyLocked(bool bLocked);

    // Lobby character display tracking
    UFUNCTION(BlueprintCallable, Category = "Lobby Characters")
    TArray<FLobbyCharacterInfo> GetLobbyCharacters() const;

    UFUNCTION(BlueprintCallable, Category = "Lobby Characters")
    int32 GetSpawnedCharacterCount() const;

protected:
    UFUNCTION()
    void OnRep_MaxPlayers();

    UFUNCTION()
    void OnRep_CurrentPlayerCount();

    UFUNCTION()
    void OnRep_CountdownActive();

    UFUNCTION()
    void OnRep_CountdownTime();

    UFUNCTION()
    void OnRep_PlayerInfoArray();

    UFUNCTION()
    void OnRep_LobbyLocked();

private:
    // Reference to character manager (set by GameMode)
    UPROPERTY()
    class ULobbyCharacterManager* CharacterManager;

public:
    void NotifyStateChanged();
    void SetCharacterManager(class ULobbyCharacterManager* Manager) { CharacterManager = Manager; }
};
