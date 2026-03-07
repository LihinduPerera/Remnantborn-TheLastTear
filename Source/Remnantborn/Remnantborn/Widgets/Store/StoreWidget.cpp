#include "StoreWidget.h"

#include "Components/TextBlock.h"
#include "Components/WrapBox.h"
#include "Components/CircularThrobber.h"
#include "Blueprint/WidgetTree.h"
#include "StoreCharacterCardWidget.h"
#include "Remnantborn/Remnantborn/Store/StoreSubsystem.h"
#include "Remnantborn/Remnantborn/CharacterSelection/CharacterSelectionSubsystem.h"
#include "Remnantborn/Remnantborn/OnlineService/MyOnlineGameInstance.h"

void UStoreWidget::NativeConstruct()
{
    Super::NativeConstruct();

    StoreSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UStoreSubsystem>() : nullptr;
    if (!StoreSubsystem)
    {
        SetNotification(TEXT("Store subsystem unavailable"), true);
        return;
    }

    StoreSubsystem->OnStoreCatalogLoaded.AddDynamic(this, &UStoreWidget::HandleCatalogLoaded);
    StoreSubsystem->OnCharacterPurchaseCompleted.AddDynamic(this, &UStoreWidget::HandleCharacterPurchaseCompleted);
    StoreSubsystem->OnStoreError.AddDynamic(this, &UStoreWidget::HandleStoreError);

    RefreshStore();
}

void UStoreWidget::RefreshStore()
{
    if (!StoreSubsystem)
    {
        return;
    }

    SetLoadingState(true);
    SetNotification(TEXT(""), false);
    StoreSubsystem->FetchStoreCatalog();
}

void UStoreWidget::HandleCatalogLoaded(const TArray<FStoreCharacterInfo>& Characters)
{
    SetLoadingState(false);

    if (CharacterGrid)
    {
        CharacterGrid->ClearChildren();
    }

    if (ComingSoonText)
    {
        ComingSoonText->SetVisibility(Characters.Num() == 0 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }

    if (Characters.Num() == 0)
    {
        return;
    }

    if (!StoreCharacterCardClass || !CharacterGrid)
    {
        return;
    }

    
    // early grab the character subsystem once to avoid repeatedly querying
    UCharacterSelectionSubsystem* CharSub = GetGameInstance() ? GetGameInstance()->GetSubsystem<UCharacterSelectionSubsystem>() : nullptr;

    for (const FStoreCharacterInfo& Character : Characters)
    {
        UStoreCharacterCardWidget* Card = CreateWidget<UStoreCharacterCardWidget>(this, StoreCharacterCardClass);
        if (!Card)
        {
            continue;
        }

        // try to resolve local data asset if this is a character item
        UCharacterDataAsset* LocalData = nullptr;
        if (CharSub && Character.ItemType.Equals(TEXT("character"), ESearchCase::IgnoreCase))
        {
            LocalData = CharSub->GetCharacterByID(FName(*Character.ItemId));
        }

        Card->InitializeCard(Character, LocalData);
        Card->OnPurchaseRequested.AddDynamic(this, &UStoreWidget::OnCharacterPurchaseRequested);
        CharacterGrid->AddChildToWrapBox(Card);
    }

    if (UMyOnlineGameInstance* GameInstance = GetGameInstance<UMyOnlineGameInstance>())
    {
        UpdateBalanceText(GameInstance->GetCurrentUserProfile().RemnantCount);
    }
}

void UStoreWidget::HandleCharacterPurchaseCompleted(bool bSuccess, const FCharacterPurchaseResponse& Response)
{
    SetLoadingState(false);

    if (!bSuccess)
    {
        SetNotification(Response.ErrorMessage.IsEmpty() ? TEXT("Purchase failed") : Response.ErrorMessage, true);
        return;
    }

    UpdateBalanceText(Response.NewRemnantCount);
    SetNotification(TEXT("Character purchased"), false);
    RefreshStore();
}

void UStoreWidget::HandleStoreError(const FString& ErrorMessage)
{
    SetLoadingState(false);
    SetNotification(ErrorMessage, true);
}

void UStoreWidget::OnCharacterPurchaseRequested(const FString& CharacterId)
{
    if (!StoreSubsystem || CharacterId.IsEmpty())
    {
        return;
    }

    SetLoadingState(true);
    StoreSubsystem->PurchaseCharacter(CharacterId);
}

void UStoreWidget::SetLoadingState(bool bLoading)
{
    if (LoadingSpinner)
    {
        LoadingSpinner->SetVisibility(bLoading ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }
}

void UStoreWidget::UpdateBalanceText(int32 NewBalance)
{
    if (BalanceText)
    {
        BalanceText->SetText(FText::FromString(FString::Printf(TEXT("%d"), NewBalance)));
    }
}

void UStoreWidget::SetNotification(const FString& Message, bool bIsError)
{
    if (!NotificationText)
    {
        return;
    }

    NotificationText->SetText(FText::FromString(Message));
    NotificationText->SetColorAndOpacity(bIsError
        ? FSlateColor(FLinearColor(1.0f, 0.2f, 0.2f, 1.0f))
        : FSlateColor(FLinearColor(0.2f, 1.0f, 0.2f, 1.0f)));
}
