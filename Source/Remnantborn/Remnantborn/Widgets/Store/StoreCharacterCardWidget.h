#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Remnantborn/Remnantborn/OnlineService/UEdsHttpService.h"
#include "StoreCharacterCardWidget.generated.h"

class UButton;
class UImage;
class UTextBlock;
class UCharacterDataAsset;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStoreCharacterPurchaseRequested, const FString&, CharacterId);

UCLASS()
class REMNANTBORN_API UStoreCharacterCardWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Store")
    void InitializeCard(const FStoreCharacterInfo& CharacterInfo, UCharacterDataAsset* LocalData = nullptr);

    UFUNCTION(BlueprintCallable, Category = "Store")
    void SetOwned(bool bOwned);

    UPROPERTY(BlueprintAssignable, Category = "Store|Events")
    FOnStoreCharacterPurchaseRequested OnPurchaseRequested;

protected:
    virtual void NativeConstruct() override;

    UPROPERTY(meta = (BindWidgetOptional))
    UImage* CharacterPortrait;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* CharacterName;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* CharacterDescription;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* PriceText;

    UPROPERTY(meta = (BindWidgetOptional))
    UButton* BuyButton;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* BuyButtonText;

private:
    UFUNCTION()
    void OnBuyClicked();

    // keep the last record of info and the associated data asset (if any)
    FStoreCharacterInfo CachedInfo;

    UPROPERTY()
    UCharacterDataAsset* CachedCharacterData = nullptr;
};
