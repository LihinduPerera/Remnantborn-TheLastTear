#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Remnantborn/Remnantborn/GameModes/RemnantbornGameSession.h"
#include "Remnantborn/Remnantborn/GameModes/MultiplayerMatchGameState.h"
#include "MultiplayerGameMode.generated.h"

class UCharacterDataAsset;

UCLASS()
class REMNANTBORN_API AMultiplayerGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMultiplayerGameMode();

protected:
    virtual void BeginPlay() override;
    virtual void PostLogin(APlayerController* NewPlayer) override;
    virtual void Logout(AController* Exiting) override;
    virtual void PostSeamlessTravel() override;
    virtual void Tick(float DeltaTime) override;

    /**
     * Initialize match tracking when all players are ready
     */
    UFUNCTION()
    void InitializeMatchTracking();

    /**
     * Check for player deaths and update match state
     */
    void CheckForPlayerDeaths();

public:
	/**
	 * Public method for external systems to notify about player death
	 */
	UFUNCTION(BlueprintCallable, Category = "Match")
	void NotifyPlayerDied(APlayerController* PlayerController);

private:
    bool bRewardsDispatched = false;

	void SetupPlayerInput(APlayerController* PlayerController);
    
    UFUNCTION()
    void SpawnPlayerWithCharacter(APlayerController* PlayerController);

    UFUNCTION()
    void RetrySpawnPlayerWithCharacter(APlayerController* PlayerController, int32 RetryCount);

    /**
     * Handle player death and notify game state
     */
    void OnPlayerDied(APlayerController* PlayerController);

    /**
     * Apply character selection from GameSession to PlayerState.
     * This is called when a player joins to restore their lobby selection.
     * 
     * @param PlayerController - The player controller to apply selection for
     * @return true if a selection was found and applied
     */
    bool ApplyCharacterSelectionFromGameSession(APlayerController* PlayerController);

    /**
     * Get the default character (first in the available characters list).
     * Used when a player has no character selection.
     * 
     * @return The default character data asset, or nullptr if none available
     */
    UCharacterDataAsset* GetDefaultCharacter() const;

    /**
     * Apply the default character to a player if they have no selection.
     * 
     * @param PlayerController - The player controller to apply default for
     * @return true if a default character was applied
     */
    bool ApplyDefaultCharacterIfNeeded(APlayerController* PlayerController);
};