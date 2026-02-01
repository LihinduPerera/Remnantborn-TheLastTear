#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerListEntryWidget.generated.h"

UCLASS()
class ELECTRICDREAMSSAMPLE_API UPlayerListEntryWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Player Entry")
    void SetPlayerInfo(const FString& PlayerName, bool bIsReady, bool bHasCharacter);

    UFUNCTION(BlueprintPure, Category = "Player Entry")
    FString GetPlayerName() const { return PlayerName; }

    UFUNCTION(BlueprintPure, Category = "Player Entry")
    bool IsPlayerReady() const { return bIsReady; }

    UFUNCTION(BlueprintPure, Category = "Player Entry")
    bool HasCharacter() const { return bHasCharacter; }

protected:
    UPROPERTY(meta = (BindWidget))
    class UTextBlock* PlayerNameText;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* ReadyStatusText;

    UPROPERTY(meta = (BindWidget))
    class UImage* CharacterIcon;

private:
    UPROPERTY()
    FString PlayerName;

    UPROPERTY()
    bool bIsReady;

    UPROPERTY()
    bool bHasCharacter;

    void UpdateVisuals();
};
