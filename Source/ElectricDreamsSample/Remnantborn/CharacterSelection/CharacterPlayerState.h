#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "CharacterDataAsset.h"
#include "CharacterPlayerState.generated.h"

UCLASS()
class ELECTRICDREAMSSAMPLE_API ACharacterPlayerState : public APlayerState
{
    GENERATED_BODY()

public:
    ACharacterPlayerState();

    UFUNCTION(BlueprintCallable, Category = "Character Selection")
    void SetSelectedCharacter(UCharacterDataAsset* Character);

    UFUNCTION(BlueprintCallable, Category = "Character Selection")
    UCharacterDataAsset* GetSelectedCharacter() const;

    UFUNCTION(BlueprintCallable, Category = "Character Selection")
    FName GetSelectedCharacterID() const;

    UFUNCTION(BlueprintCallable, Category = "Character Selection")
    bool HasSelectedCharacter() const { return !SelectedCharacterID.IsNone(); }

    UFUNCTION(Server, Reliable, WithValidation, Category = "Character Selection")
    void Server_SetSelectedCharacterID(FName CharacterID);

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
    UFUNCTION()
    void OnRep_SelectedCharacterID();

private:
    UPROPERTY(ReplicatedUsing = OnRep_SelectedCharacterID)
    FName SelectedCharacterID;

    UPROPERTY()
    UCharacterDataAsset* CachedCharacterData;

    void CacheCharacterData();
};
