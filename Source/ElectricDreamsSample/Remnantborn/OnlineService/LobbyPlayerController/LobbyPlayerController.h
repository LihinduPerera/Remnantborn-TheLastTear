#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ElectricDreamsSample/Remnantborn/CharacterSelection/CharacterDataAsset.h"
#include "LobbyPlayerController.generated.h"

UCLASS()
class ELECTRICDREAMSSAMPLE_API ALobbyPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void SetPlayerReady(bool bReady);

    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void StartMatchCountdown();

    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void CancelMatchCountdown();

    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void SetMaxPlayers(int32 MaxPlayers);

    UFUNCTION(BlueprintPure, Category = "Lobby")
    bool IsHost() const;

    UFUNCTION(BlueprintPure, Category = "Lobby")
    bool IsReady() const { return bIsReady; }

    UFUNCTION(BlueprintPure, Category = "Lobby")
    bool HasSelectedCharacter() const { return bHasSelectedCharacter; }

    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void CreateLobbyWidget();

    UFUNCTION(Client, Reliable, Category = "Lobby")
    void Client_ShowCharacterSelection();

    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void ShowCharacterSelection();

    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void HideCharacterSelection();

    UFUNCTION(Client, Reliable, Category = "Lobby")
    void Client_CleanupLobbyWidgets();

    UFUNCTION(BlueprintPure, Category = "Lobby")
    class ULobbyWidget* GetLobbyWidget() const { return LobbyWidget; }

protected:
    UPROPERTY(ReplicatedUsing = OnRep_IsReady)
    bool bIsReady = false;

    UPROPERTY(ReplicatedUsing = OnRep_HasSelectedCharacter)
    bool bHasSelectedCharacter = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby")
    TSubclassOf<class ULobbyWidget> LobbyWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby")
    TSubclassOf<class UCharacterSelectionWidget> CharacterSelectionWidgetClass;

    UPROPERTY()
    class ULobbyWidget* LobbyWidget;

    UPROPERTY()
    class UCharacterSelectionWidget* CharacterSelectionWidget;

    UFUNCTION()
    void OnRep_IsReady();

    UFUNCTION()
    void OnRep_HasSelectedCharacter();

    UFUNCTION()
    void HandleCharacterSelected(UCharacterDataAsset* CharacterData);

    UFUNCTION()
    void HandleCharacterCancelled(UCharacterDataAsset* CharacterData);

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
    void SetupInputMode();

    UFUNCTION(Server, Reliable, WithValidation)
    void Server_SetPlayerReady(bool bReady);

    UFUNCTION(Server, Reliable, WithValidation)
    void Server_StartMatchCountdown();

    UFUNCTION(Server, Reliable, WithValidation)
    void Server_CancelMatchCountdown();

    UFUNCTION(Server, Reliable, WithValidation)
    void Server_SetMaxPlayers(int32 MaxPlayers);

    UFUNCTION(Server, Reliable, WithValidation)
    void Server_ConfirmCharacterSelection(FName CharacterID);

    void Client_ShowCharacterSelection_Implementation();
    void Client_CleanupLobbyWidgets_Implementation();
};
