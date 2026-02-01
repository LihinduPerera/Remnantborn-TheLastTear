#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LobbyWidget.generated.h"

class ALobbyGameState;

UCLASS()
class ELECTRICDREAMSSAMPLE_API ULobbyWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void UpdatePlayerList();

    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void SetReadyStatus(bool bIsReady);

    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void OnReadyButtonClicked();

    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void StartMatchCountdown();

    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void CancelMatchCountdown();

    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void SetMaxPlayers(int32 MaxPlayers);

    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void OnSet2PlayersClicked();

    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void OnSet4PlayersClicked();

    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void LeaveLobby();

    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void ShowCharacterSelection();

protected:
    UFUNCTION()
    void HandleLobbyStateChanged();

    UFUNCTION()
    void HandleCountdownStarted();

    UFUNCTION()
    void HandleCountdownUpdated();

    UFUNCTION()
    void HandleCountdownCancelled();

    // Widget components
    UPROPERTY(meta = (BindWidget))
    class UVerticalBox* PlayerListContainer;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* PlayerCountText;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* StatusText;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* CountdownText;

    UPROPERTY(meta = (BindWidget))
    class UButton* ReadyButton;

    UPROPERTY(meta = (BindWidget))
    class UButton* StartButton;

    UPROPERTY(meta = (BindWidget))
    class UButton* CancelButton;

    UPROPERTY(meta = (BindWidget))
    class UButton* LeaveButton;

    UPROPERTY(meta = (BindWidget))
    class UButton* Set2PlayersButton;

    UPROPERTY(meta = (BindWidget))
    class UButton* Set4PlayersButton;

    UPROPERTY(meta = (BindWidget))
    class UButton* SelectCharacterButton;

    UPROPERTY(meta = (BindWidget))
    class UHorizontalBox* HostControlsContainer;

    // Widget class for player list entries
    UPROPERTY(EditAnywhere, Category = "Widgets")
    TSubclassOf<class UPlayerListEntryWidget> PlayerListEntryClass;

public:
    void UpdateUI();

private:
    class ALobbyGameState* GetLobbyGameState() const;
    class ALobbyPlayerController* GetLobbyPlayerController() const;
    void UpdatePlayerCountText();
    void UpdateCountdownText();
    void UpdateHostControls();
    void UpdateReadyButton();

    TArray<class UPlayerListEntryWidget*> PlayerEntries;
};
