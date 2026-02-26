#include "RemnantPurchaseWidget.h"

#include "Components/Button.h"
#include "Components/CircularThrobber.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "RemnantPackageCardWidget.h"
#include "Remnantborn/Remnantborn/Store/StoreSubsystem.h"
#include "Remnantborn/Remnantborn/OnlineService/MyOnlineGameInstance.h"

void URemnantPurchaseWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (PayNowButton)
    {
        PayNowButton->OnClicked.AddDynamic(this, &URemnantPurchaseWidget::OnPayNowClicked);
    }

    StoreSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UStoreSubsystem>() : nullptr;
    if (!StoreSubsystem)
    {
        SetNotification(TEXT("Store subsystem unavailable"), true);
        return;
    }

    StoreSubsystem->OnStorePackagesLoaded.AddDynamic(this, &URemnantPurchaseWidget::HandlePackagesLoaded);
    StoreSubsystem->OnRemnantPurchaseCompleted.AddDynamic(this, &URemnantPurchaseWidget::HandleRemnantPurchaseCompleted);
    StoreSubsystem->OnStoreError.AddDynamic(this, &URemnantPurchaseWidget::HandleStoreError);

    RefreshPackages();
}

void URemnantPurchaseWidget::RefreshPackages()
{
    if (!StoreSubsystem)
    {
        return;
    }

    SetLoadingState(true);
    StoreSubsystem->FetchRemnantPackages();
}

void URemnantPurchaseWidget::HandlePackagesLoaded(const TArray<FRemnantPackage>& Packages)
{
    SetLoadingState(false);

    if (PackageContainer)
    {
        PackageContainer->ClearChildren();
    }

    if (!RemnantPackageCardClass || !PackageContainer)
    {
        return;
    }

    for (const FRemnantPackage& PackageInfo : Packages)
    {
        URemnantPackageCardWidget* Card = CreateWidget<URemnantPackageCardWidget>(this, RemnantPackageCardClass);
        if (!Card)
        {
            continue;
        }

        const bool bIsBestValue = PackageInfo.RemnantAmount >= 2500;
        Card->InitializeCard(PackageInfo, bIsBestValue);
        Card->OnRemnantPackageSelected.AddDynamic(this, &URemnantPurchaseWidget::OnPackageSelected);
        PackageContainer->AddChildToHorizontalBox(Card);
    }

    if (UMyOnlineGameInstance* GameInstance = GetGameInstance<UMyOnlineGameInstance>())
    {
        UpdateBalanceText(GameInstance->GetCurrentUserProfile().RemnantCount);
    }
}

void URemnantPurchaseWidget::HandleRemnantPurchaseCompleted(bool bSuccess, const FRemnantPurchaseResponse& Response)
{
    SetLoadingState(false);

    if (!bSuccess)
    {
        SetNotification(Response.ErrorMessage.IsEmpty() ? TEXT("Remnant purchase failed") : Response.ErrorMessage, true);
        return;
    }

    UpdateBalanceText(Response.NewRemnantCount);
    SetNotification(FString::Printf(TEXT("Purchased %d Remnants (Receipt: %s)"), Response.RemnantsAdded, *Response.ReceiptId), false);
}

void URemnantPurchaseWidget::HandleStoreError(const FString& ErrorMessage)
{
    SetLoadingState(false);
    SetNotification(ErrorMessage, true);
}

void URemnantPurchaseWidget::OnPackageSelected(const FString& PackageId)
{
    SelectedPackageId = PackageId;

    if (SelectedPackageText)
    {
        SelectedPackageText->SetText(FText::FromString(FString::Printf(TEXT("Selected Package: %s"), *PackageId)));
    }
}

void URemnantPurchaseWidget::OnPayNowClicked()
{
    if (!StoreSubsystem)
    {
        return;
    }

    if (SelectedPackageId.IsEmpty())
    {
        SetNotification(TEXT("Select a package first"), true);
        return;
    }

    if (!ValidatePaymentInputs())
    {
        SetNotification(TEXT("Invalid payment input"), true);
        return;
    }

    SetLoadingState(true);
    StoreSubsystem->PurchaseRemnants(
        SelectedPackageId,
        CardNumberInput ? CardNumberInput->GetText().ToString() : TEXT(""),
        CardExpiryInput ? CardExpiryInput->GetText().ToString() : TEXT(""),
        CardCVVInput ? CardCVVInput->GetText().ToString() : TEXT(""));
}

bool URemnantPurchaseWidget::ValidatePaymentInputs() const
{
    const FString CardNumber = CardNumberInput ? CardNumberInput->GetText().ToString().Replace(TEXT(" "), TEXT("")) : TEXT("");
    const FString Expiry = CardExpiryInput ? CardExpiryInput->GetText().ToString() : TEXT("");
    const FString CVV = CardCVVInput ? CardCVVInput->GetText().ToString() : TEXT("");

    return CardNumber.Len() >= 16 && Expiry.Len() == 5 && CVV.Len() >= 3;
}

void URemnantPurchaseWidget::SetLoadingState(bool bLoading)
{
    if (LoadingSpinner)
    {
        LoadingSpinner->SetVisibility(bLoading ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }

    if (PayNowButton)
    {
        PayNowButton->SetIsEnabled(!bLoading);
    }
}

void URemnantPurchaseWidget::UpdateBalanceText(int32 NewBalance)
{
    if (BalanceText)
    {
        BalanceText->SetText(FText::FromString(FString::Printf(TEXT("%d"), NewBalance)));
    }
}

void URemnantPurchaseWidget::SetNotification(const FString& Message, bool bIsError)
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
