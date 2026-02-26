#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Remnantborn/Remnantborn/Widgets/MatchResults/MatchResultsWidget.h"
#include "Remnantborn/Remnantborn/GameModes/MultiplayerMatchGameState.h"
#include "MultiplayerPlayerController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLocalMatchRewardProcessed, bool, bIsWinner, int32, EliminationOrder);

UCLASS()
class REMNANTBORN_API AMultiplayerPlayerController : public APlayerController
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

	UFUNCTION(Client, Reliable)
	void Client_ShowMatchResults();

	UFUNCTION(Client, Reliable)
	void Client_HideMatchResults();

	/**
	 * Notify client that their player has died.
	 * Called immediately when the local player dies for instant feedback.
	 * 
	 * @param KillerName - Name of the player who killed them (empty if suicide/environmental)
	 */
	UFUNCTION(Client, Reliable)
	void Client_NotifyPlayerDeath(const FString& KillerName);

	/**
	 * Server RPC for clients to notify the server of their character selection.
	 * This is called when clients join the game level to ensure their lobby selection is applied.
	 * 
	 * @param CharacterID - The character ID selected in the lobby
	 */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_NotifyCharacterSelection(FName CharacterID);

	UFUNCTION(Client, Reliable)
	void Client_SubmitMatchReward(bool bIsWinner, float MatchDuration, int32 EliminationOrder, const FString& MatchId);

	UPROPERTY(BlueprintAssignable, Category = "Match|Events")
	FOnLocalMatchRewardProcessed OnLocalMatchRewardProcessed;

private:
	void SetupInputMode();
	void InitializeHUD();
	void SetupGASForPawn(APawn* InPawn);

	/**
	 * Sends character selection to server if we have one stored in GameInstance.
	 * Called from OnRep_PlayerState when PlayerState first replicates.
	 */
	void SendCharacterSelectionToServer();

	// Match results widget
	UPROPERTY()
	UMatchResultsWidget* MatchResultsWidget;

	// Match results widget class
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UMatchResultsWidget> MatchResultsWidgetClass;
};