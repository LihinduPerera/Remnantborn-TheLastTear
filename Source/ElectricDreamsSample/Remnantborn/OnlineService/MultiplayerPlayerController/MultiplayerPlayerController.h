#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MultiplayerPlayerController.generated.h"

UCLASS()
class ELECTRICDREAMSSAMPLE_API AMultiplayerPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnRep_PlayerState() override;

public:
	UFUNCTION(Client, Reliable)
	void Client_SetupGameplayInput();

	UFUNCTION(Client, Reliable)
	void Client_InitializeHUD();

	/**
	 * Server RPC for clients to notify the server of their character selection.
	 * This is called when clients join the game level to ensure their lobby selection is applied.
	 * 
	 * @param CharacterID - The character ID selected in the lobby
	 */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_NotifyCharacterSelection(FName CharacterID);

private:
	void SetupInputMode();
	void InitializeHUD();
	void SetupGASForPawn(APawn* InPawn);

	/**
	 * Sends character selection to server if we have one stored in GameInstance.
	 * Called from OnRep_PlayerState when PlayerState first replicates.
	 */
	void SendCharacterSelectionToServer();
};