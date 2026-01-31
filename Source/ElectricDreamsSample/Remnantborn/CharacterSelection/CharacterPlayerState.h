#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "CharacterDataAsset.h"
#include "CharacterPlayerState.generated.h"

/**
 * Player state that stores character selection
 */
UCLASS()
class ELECTRICDREAMSSAMPLE_API ACharacterPlayerState : public APlayerState
{
    GENERATED_BODY()

public:
    ACharacterPlayerState();

    // Character selection
    UFUNCTION(BlueprintCallable, Category = "Character Selection")
    void SetSelectedCharacter(UCharacterDataAsset* Character);

    UFUNCTION(BlueprintCallable, Category = "Character Selection")
    UCharacterDataAsset* GetSelectedCharacter() const;

    UFUNCTION(BlueprintCallable, Category = "Character Selection")
    FName GetSelectedCharacterID() const;

    // Replication
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
    UFUNCTION()
    void OnRep_SelectedCharacterID();

private:
    // Selected character ID (replicated)
    UPROPERTY(ReplicatedUsing = OnRep_SelectedCharacterID)
    FName SelectedCharacterID;

    // Cached character data (not replicated)
    UPROPERTY()
    UCharacterDataAsset* CachedCharacterData;
};