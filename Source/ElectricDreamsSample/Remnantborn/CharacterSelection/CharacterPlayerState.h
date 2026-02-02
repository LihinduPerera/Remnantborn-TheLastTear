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

    // Called when character selection is ready (replicated)
    UFUNCTION(BlueprintImplementableEvent, Category = "Character Selection")
    void OnCharacterSelectionReady();

    // Check if character data is ready to use
    UFUNCTION(BlueprintCallable, Category = "Character Selection")
    bool IsCharacterDataReady() const { return CachedCharacterData != nullptr; }

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    
    virtual void BeginPlay() override;

protected:
    UFUNCTION()
    void OnRep_SelectedCharacterID();

private:
    UPROPERTY(ReplicatedUsing = OnRep_SelectedCharacterID)
    FName SelectedCharacterID;

    UPROPERTY()
    UCharacterDataAsset* CachedCharacterData;

    void CacheCharacterData();
    
    // Retry mechanism for character data caching
    FTimerHandle RetryCacheHandle;
    
    void RetryCacheCharacterData(int32 RetryCount = 0);
};
