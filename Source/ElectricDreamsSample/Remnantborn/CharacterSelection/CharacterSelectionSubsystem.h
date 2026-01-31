#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CharacterDataAsset.h"
#include "CharacterSelectionSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterSelected, const UCharacterDataAsset*, SelectedCharacter);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCharacterUnlocked, const FName&, CharacterID, bool, bUnlocked);

/**
 * Manages character selection and unlocks across the game
 */
UCLASS()
class ELECTRICDREAMSSAMPLE_API UCharacterSelectionSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    // Begin USubsystem interface
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    // End USubsystem interface

    // Character Management
    UFUNCTION(BlueprintCallable, Category = "Character Selection")
    void LoadAvailableCharacters();

    UFUNCTION(BlueprintCallable, Category = "Character Selection")
    TArray<UCharacterDataAsset*> GetAvailableCharacters() const;

    UFUNCTION(BlueprintCallable, Category = "Character Selection")
    UCharacterDataAsset* GetCharacterByID(const FName& CharacterID) const;

    UFUNCTION(BlueprintCallable, Category = "Character Selection")
    bool IsCharacterUnlocked(const FName& CharacterID) const;

    UFUNCTION(BlueprintCallable, Category = "Character Selection")
    void UnlockCharacter(const FName& CharacterID);

    // Player Selection
    UFUNCTION(BlueprintCallable, Category = "Character Selection")
    void SelectCharacterForPlayer(APlayerController* PlayerController, UCharacterDataAsset* Character);

    UFUNCTION(BlueprintCallable, Category = "Character Selection")
    UCharacterDataAsset* GetSelectedCharacter(APlayerController* PlayerController) const;

    UFUNCTION(BlueprintCallable, Category = "Character Selection")
    void ClearPlayerSelection(APlayerController* PlayerController);

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Character Selection|Events")
    FOnCharacterSelected OnCharacterSelected;

    UPROPERTY(BlueprintAssignable, Category = "Character Selection|Events")
    FOnCharacterUnlocked OnCharacterUnlocked;

private:
    // All loaded character data assets
    UPROPERTY()
    TArray<UCharacterDataAsset*> AvailableCharacters;

    // Map of character IDs to their unlock status
    TMap<FName, bool> CharacterUnlockStatus;

    // Player character selections
    TMap<APlayerController*, UCharacterDataAsset*> PlayerSelections;

    // Helper to load data assets from a directory
    void LoadCharactersFromDirectory(const FString& DirectoryPath);
};