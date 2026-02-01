#pragma once

#include "CoreMinimal.h"
#include "ElectricDreamsSample/Remnantborn/CharacterSelection/CharacterDataAsset.h"
#include "GameFramework/GameModeBase.h"
#include "LobbyGameMode.generated.h"

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

protected:
    UFUNCTION()
    void UpdateCountdown();

    UFUNCTION()
    void StartMatchTravel();

    UFUNCTION()
    void UpdateSessionSettings();

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

    void UpdateGameState();
};
