#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ElectricDreamsSample/Remnantborn/GameplayAbilitySystem/Characters/RemnantbornCharacterBase.h"
#include "CharacterDataAsset.generated.h"

/**
 * Data asset containing all information about a playable character
 */
UCLASS(BlueprintType)
class ELECTRICDREAMSSAMPLE_API UCharacterDataAsset : public UDataAsset
{
    GENERATED_BODY()

public:
    // Display name of the character
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Info")
    FText CharacterName;

    // Character description
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Info")
    FText CharacterDescription;

    // Portrait image for selection screen
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Info")
    UTexture2D* CharacterPortrait;

    // Gameplay character class to spawn
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Info")
    TSubclassOf<ARemnantbornCharacterBase> CharacterClass;

    // Starting abilities for this character
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Info")
    TArray<TSubclassOf<UGameplayAbility>> StartingAbilities;

    // Character ID for saving/loading
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Info")
    FName CharacterID;
    
    // Is this character unlocked by default?
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Info")
    bool bUnlockedByDefault = true;
};