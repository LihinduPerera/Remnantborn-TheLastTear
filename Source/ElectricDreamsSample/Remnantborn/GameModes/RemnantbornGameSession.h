#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameSession.h"
#include "RemnantbornGameSession.generated.h"

/**
 * Extended GameSession that stores character selections persistently through seamless travel.
 * This ensures character selections made in the lobby survive the transition to the game map.
 */
UCLASS()
class ELECTRICDREAMSSAMPLE_API ARemnantbornGameSession : public AGameSession
{
    GENERATED_BODY()

public:
    ARemnantbornGameSession();

    /**
     * Store a player's character selection before travel.
     * Call this in LobbyGameMode before ServerTravel.
     * 
     * @param PlayerId - The unique player ID from PlayerState
     * @param CharacterID - The selected character's ID (e.g., "Warrior", "Mage")
     */
    UFUNCTION(BlueprintCallable, Category = "Character Selection")
    void StorePlayerCharacterSelection(int32 PlayerId, const FName& CharacterID);

    /**
     * Retrieve a player's character selection after travel.
     * Call this in MultiplayerGameMode when a player joins.
     * 
     * @param PlayerId - The unique player ID from PlayerState
     * @return The character ID, or NAME_None if not found
     */
    UFUNCTION(BlueprintCallable, Category = "Character Selection")
    FName GetPlayerCharacterSelection(int32 PlayerId) const;

    /**
     * Check if a player has a stored character selection.
     * 
     * @param PlayerId - The unique player ID from PlayerState
     * @return true if the player has a selection stored
     */
    UFUNCTION(BlueprintCallable, Category = "Character Selection")
    bool HasPlayerCharacterSelection(int32 PlayerId) const;

    /**
     * Remove a player's character selection (e.g., when they disconnect).
     * 
     * @param PlayerId - The unique player ID from PlayerState
     */
    UFUNCTION(BlueprintCallable, Category = "Character Selection")
    void RemovePlayerCharacterSelection(int32 PlayerId);

    /**
     * Clear all stored character selections.
     * Call this when the match ends and you're returning to lobby.
     */
    UFUNCTION(BlueprintCallable, Category = "Character Selection")
    void ClearAllCharacterSelections();

    /**
     * Get all stored character selections for debugging.
     * 
     * @return Map of PlayerId to CharacterID
     */
    UFUNCTION(BlueprintCallable, Category = "Character Selection")
    TMap<int32, FName> GetAllCharacterSelections() const { return PlayerCharacterSelections; }

private:
    /**
     * Map of PlayerId to selected CharacterID.
     * This persists through seamless travel because GameSession is not destroyed during travel.
     */
    UPROPERTY()
    TMap<int32, FName> PlayerCharacterSelections;
};
