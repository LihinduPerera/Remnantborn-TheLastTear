#pragma once

#include "CoreMinimal.h"
#include "ElectricDreamsSample/Remnantborn/CharacterSelection/CharacterDataAsset.h"
#include "GameFramework/GameModeBase.h"
#include "LobbyGameMode.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLobbyReadyChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLobbyCountdownStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLobbyCountdownUpdated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLobbyCountdownCancelled);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterSelectionChanged, APlayerController*, PlayerController);

UCLASS()
class ELECTRICDREAMSSAMPLE_API ALobbyGameMode : public AGameModeBase
{
    GENERATED_BODY()
    
public:
    ALobbyGameMode();
    
    virtual void BeginPlay() override;
    virtual void PostLogin(APlayerController* NewPlayer) override;
    virtual void Logout(AController* Exiting) override;
    
    // Character selection
    UFUNCTION(BlueprintCallable, Category = "Character Selection")
    void ShowCharacterSelectionForPlayer(APlayerController* PlayerController);
    
    UFUNCTION(BlueprintCallable, Category = "Character Selection")
    bool HasPlayerSelectedCharacter(APlayerController* PlayerController) const;
    
    UFUNCTION(BlueprintCallable, Category = "Character Selection")
    TArray<APlayerController*> GetPlayersWithCharacters() const;
    
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
    
    // Events
    UPROPERTY(BlueprintAssignable, Category = "Lobby|Events")
    FOnLobbyReadyChanged OnLobbyReadyChanged;
    
    UPROPERTY(BlueprintAssignable, Category = "Lobby|Events")
    FOnLobbyCountdownStarted OnLobbyCountdownStarted;
    
    UPROPERTY(BlueprintAssignable, Category = "Lobby|Events")
    FOnLobbyCountdownUpdated OnLobbyCountdownUpdated;
    
    UPROPERTY(BlueprintAssignable, Category = "Lobby|Events")
    FOnLobbyCountdownCancelled OnLobbyCountdownCancelled;
    
    UPROPERTY(BlueprintAssignable, Category = "Character Selection|Events")
    FOnCharacterSelectionChanged OnCharacterSelectionChanged;
    
protected:
    UFUNCTION()
    void UpdateCountdown();
    
    UFUNCTION()
    void StartMatchTravel();
    
    UFUNCTION()
    void UpdateSessionSettings();
    
    // Character selection
    UFUNCTION()
    void HandleCharacterSelected(UCharacterDataAsset* SelectedCharacter);
    
private:
    UPROPERTY(EditDefaultsOnly, Category = "Lobby")
    int32 MaxPlayers = 2;
    
    UPROPERTY(EditDefaultsOnly, Category = "Lobby")
    int32 CountdownDuration = 5;
    
    UPROPERTY(EditDefaultsOnly, Category = "Lobby")
    FString GameMapPath = "/Game/Remnantborn/Levels/TestGround";
    
    // Character selection widget class
    UPROPERTY(EditDefaultsOnly, Category = "Character Selection")
    TSubclassOf<class UCharacterSelectionWidget> CharacterSelectionWidgetClass;
    
    int32 CurrentPlayerCount = 0;
    int32 CountdownTime = 0;
    bool bCountdownActive = false;
    
    FTimerHandle CountdownTimerHandle;
    
    // Character selections per player
    TMap<APlayerController*, UCharacterDataAsset*> PlayerCharacterSelections;
    
    // Character selection widgets
    TMap<APlayerController*, UCharacterSelectionWidget*> CharacterSelectionWidgets;
};
