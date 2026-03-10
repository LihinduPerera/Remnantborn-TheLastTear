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

    // listen for auth/profile changes so we can refresh balance & catalog
    if (UMyOnlineGameInstance* GameInstance = GetGameInstance<UMyOnlineGameInstance>())
    {
        GameInstance->OnAuthStateChanged.AddDynamic(this, &UStoreWidget::HandleAuthStateChanged);
        GameInstance->OnProfileUpdated.AddDynamic(this, &UStoreWidget::HandleProfileUpdated);

        // initialize login gating and balance
        ApplyLoginGating(GameInstance->IsLoggedIn());
        if (GameInstance->IsLoggedIn())
        {
            UpdateBalanceText(GameInstance->GetCurrentUserProfile().RemnantCount);
            // we'll call RefreshStore once below when logged in
        }
    }

    if (GetGameInstance<UMyOnlineGameInstance>() && GetGameInstance<UMyOnlineGameInstance>()->IsLoggedIn())
    {
        RefreshStore();
    }
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


void UStoreWidget::NativeDestruct()
{
    if (UMyOnlineGameInstance* GI = GetGameInstance<UMyOnlineGameInstance>())
    {
        GI->OnAuthStateChanged.RemoveDynamic(this, &UStoreWidget::HandleAuthStateChanged);
        GI->OnProfileUpdated.RemoveDynamic(this, &UStoreWidget::HandleProfileUpdated);
    }
    Super::NativeDestruct();
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


// --------------------------------------------------
// login gating helper
// --------------------------------------------------

void UStoreWidget::ApplyLoginGating(bool bIsLoggedIn)
{
    if (ContentPanel)
    {
        ContentPanel->SetVisibility(bIsLoggedIn ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }
    if (LoginRequiredText)
    {
        LoginRequiredText->SetVisibility(bIsLoggedIn ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
        if (!bIsLoggedIn)
        {
            LoginRequiredText->SetText(FText::FromString(TEXT("Please log in to access this feature.")));
        }
    }
}


// --------------------------------------------------
// GameInstance event handlers
// --------------------------------------------------

void UStoreWidget::HandleAuthStateChanged(bool bIsLoggedIn)
{
    ApplyLoginGating(bIsLoggedIn);

    if (bIsLoggedIn)
    {
        // user just logged in or restored session; refresh everything
        SetNotification(TEXT(""), false);
        if (UMyOnlineGameInstance* GI = GetGameInstance<UMyOnlineGameInstance>())
        {
            UpdateBalanceText(GI->GetCurrentUserProfile().RemnantCount);
        }
        RefreshStore();
    }
    else
    {
        // logged out: clear grid and notify via gating
        if (CharacterGrid)
        {
            CharacterGrid->ClearChildren();
        }
        UpdateBalanceText(0);
    }
}

void UStoreWidget::HandleProfileUpdated(const FUserProfile& UserProfile)
{
    UpdateBalanceText(UserProfile.RemnantCount);
    // balance changed; re-evaluate affordability by refreshing catalog
    RefreshStore();
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
