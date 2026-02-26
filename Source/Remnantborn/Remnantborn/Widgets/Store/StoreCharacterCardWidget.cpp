#include "StoreCharacterCardWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"

void UStoreCharacterCardWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (BuyButton)
    {
        BuyButton->OnClicked.AddDynamic(this, &UStoreCharacterCardWidget::OnBuyClicked);
    }
}

void UStoreCharacterCardWidget::InitializeCard(const FStoreCharacterInfo& CharacterInfo)
{
    CachedInfo = CharacterInfo;

    if (CharacterName)
    {
        CharacterName->SetText(FText::FromString(CharacterInfo.Name));
    }

    if (CharacterDescription)
    {
        CharacterDescription->SetText(FText::FromString(CharacterInfo.Description));
    }

    if (PriceText)
    {
        PriceText->SetText(FText::FromString(FString::Printf(TEXT("%d"), CharacterInfo.Price)));
    }

    SetOwned(CharacterInfo.bOwned);
}

void UStoreCharacterCardWidget::SetOwned(bool bOwned)
{
    CachedInfo.bOwned = bOwned;

    if (BuyButton)
    {
        BuyButton->SetIsEnabled(!bOwned && CachedInfo.bCanAfford);
    }

    if (BuyButtonText)
    {
        BuyButtonText->SetText(FText::FromString(bOwned ? TEXT("Owned") : TEXT("Purchase")));
    }
}

void UStoreCharacterCardWidget::OnBuyClicked()
{
    if (!CachedInfo.ItemId.IsEmpty())
    {
        OnPurchaseRequested.Broadcast(CachedInfo.ItemId);
    }
}
