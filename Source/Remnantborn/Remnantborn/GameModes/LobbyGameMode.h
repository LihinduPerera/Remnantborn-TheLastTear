#pragma once

#include "CoreMinimal.h"
#include "Remnantborn/Remnantborn/CharacterSelection/CharacterDataAsset.h"
#include "Remnantborn/Remnantborn/GameModes/RemnantbornGameSession.h"
#include "Remnantborn/Remnantborn/Lobby/LobbyCharacterManager.h"
#include "GameFramework/GameModeBase.h"
#include "LobbyGameMode.generated.h"

UCLASS()
class REMNANTBORN_API ALobbyGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ALobbyGameMode();

    virtual void BeginPlay() override;
    virtual void PostLogin(APlayerController* NewPlayer) override;
    virtual void Logout(AController* Exiting) override;

    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void OnPlayerReadyChanged(APlayerController* PlayerController, bool bReady);

    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void OnPlayerSelectedCharacter(APlayerController* PlayerController, UCharacterDataAsset* SelectedCharacter);

    // Lobby management
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

    // Character info
    UFUNCTION(BlueprintPure, Category = "Character Selection")
    UCharacterDataAsset* GetPlayerSelectedCharacter(APlayerController* PlayerController) const;

    // Lobby character display management
    UFUNCTION(BlueprintCallable, Category = "Lobby Characters")
    void SpawnLobbyCharacterForPlayer(APlayerController* PlayerController);

    UFUNCTION(BlueprintCallable, Category = "Lobby Characters")
    void DespawnLobbyCharacterForPlayer(APlayerController* PlayerController);

    UFUNCTION(BlueprintCallable, Category = "Lobby Characters")
    void ClearAllLobbyCharacters();

protected:
    UFUNCTION()
    void UpdateCountdown();

    UFUNCTION()
    void StartMatchTravel();

    UFUNCTION()
    void UpdateSessionSettings();

    UFUNCTION()
    void ExecuteMatchTravel();

private:
    UPROPERTY(EditDefaultsOnly, Category = "Lobby")
    int32 MaxPlayers = 2;

    UPROPERTY(EditDefaultsOnly, Category = "Lobby")
    int32 CountdownDuration = 5;

    UPROPERTY(EditDefaultsOnly, Category = "Lobby")
    FString GameMapPath = "/Game/Remnantborn/Levels/TestGround";

    int32 CurrentPlayerCount = 0;
    int32 CountdownTime = 0;
    bool bCountdownActive = false;

    FTimerHandle CountdownTimerHandle;

    // Character selections per player
    TMap<APlayerController*, UCharacterDataAsset*> PlayerCharacterSelections;

    // Lobby character manager for 3D character display
    UPROPERTY()
    class ULobbyCharacterManager* CharacterManager;

    void UpdateGameState();
    void InitializeCharacterManager();
};
