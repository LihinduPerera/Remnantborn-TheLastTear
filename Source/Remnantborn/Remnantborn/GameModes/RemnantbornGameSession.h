#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameSession.h"
#include "RemnantbornGameSession.generated.h"

/**
 * Extended GameSession that stores character selections persistently through seamless travel.
 * This ensures character selections made in the lobby survive the transition to the game map.
 */
UCLASS()
class REMNANTBORN_API ARemnantbornGameSession : public AGameSession
{
    GENERATED_BODY()

public:
    ARemnantbornGameSession();

    /**
     * Store a player's character selection before travel.
     * Call this in LobbyGameMode before ServerTravel.
     * 
     * @param PlayerName - The player's name (more stable than PlayerId across seamless travel)
     * @param CharacterID - The selected character's ID (e.g., "Warrior", "Mage")
     */
    UFUNCTION(BlueprintCallable, Category = "Character Selection")
    void StorePlayerCharacterSelection(const FString& PlayerName, const FName& CharacterID);

    /**
     * Retrieve a player's character selection after travel.
     * Call this in MultiplayerGameMode when a player joins.
     * 
     * @param PlayerName - The player's name (more stable than PlayerId across seamless travel)
     * @return The character ID, or NAME_None if not found
     */
    UFUNCTION(BlueprintCallable, Category = "Character Selection")
    FName GetPlayerCharacterSelection(const FString& PlayerName) const;

    /**
     * Check if a player has a stored character selection.
     * 
     * @param PlayerName - The player's name (more stable than PlayerId across seamless travel)
     * @return true if the player has a selection stored
     */
    UFUNCTION(BlueprintCallable, Category = "Character Selection")
    bool HasPlayerCharacterSelection(const FString& PlayerName) const;

    /**
     * Remove a player's character selection (e.g., when they disconnect).
     * 
     * @param PlayerName - The player's name (more stable than PlayerId across seamless travel)
     */
    UFUNCTION(BlueprintCallable, Category = "Character Selection")
    void RemovePlayerCharacterSelection(const FString& PlayerName);

    /**
     * Clear all stored character selections.
     * Call this when the match ends and you're returning to lobby.
     */
    UFUNCTION(BlueprintCallable, Category = "Character Selection")
    void ClearAllCharacterSelections();

    /**
     * Get all stored character selections for debugging.
     * 
     * @return Map of PlayerName to CharacterID
     */
    UFUNCTION(BlueprintCallable, Category = "Character Selection")
    TMap<FString, FName> GetAllCharacterSelections() const { return PlayerCharacterSelections; }

private:
    /**
     * Map of PlayerName to selected CharacterID.
     * Using player name instead of PlayerId because PlayerId can change during seamless travel.
     * This persists through seamless travel because GameSession is not destroyed during travel.
     */
    UPROPERTY()
    TMap<FString, FName> PlayerCharacterSelections;
};
