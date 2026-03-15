#include "RemnantPurchaseWidget.h"

#include "Components/Button.h"
#include "Components/CircularThrobber.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "RemnantPackageCardWidget.h"
#include "Remnantborn/Remnantborn/Store/StoreSubsystem.h"
#include "Remnantborn/Remnantborn/OnlineService/MyOnlineGameInstance.h"

namespace
{
constexpr TCHAR DefaultCardNumber[] = TEXT("1111 1111 1111 1111");
constexpr TCHAR DefaultCardExpiry[] = TEXT("11/11");
constexpr TCHAR DefaultCardCVV[] = TEXT("111");
}

void URemnantPurchaseWidget::NativeConstruct()
{
    Super::NativeConstruct();

    InitializeStaticPaymentInputs();

    SelectedPackageId.Empty();
    if (SelectedPackageText)
    {
        SelectedPackageText->SetText(FText::GetEmpty());
        SelectedPackageText->SetVisibility(ESlateVisibility::Collapsed);
    }

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

    // auth/profile hooks
    if (UMyOnlineGameInstance* GameInstance = GetGameInstance<UMyOnlineGameInstance>())
    {
        GameInstance->OnAuthStateChanged.AddDynamic(this, &URemnantPurchaseWidget::HandleAuthStateChanged);
        GameInstance->OnProfileUpdated.AddDynamic(this, &URemnantPurchaseWidget::HandleProfileUpdated);

        ApplyLoginGating(GameInstance->IsLoggedIn());
        if (GameInstance->IsLoggedIn())
        {
            UpdateBalanceText(GameInstance->GetCurrentUserProfile().RemnantCount);
            RefreshPackages();
        }
    }

    if (GetGameInstance<UMyOnlineGameInstance>() && GetGameInstance<UMyOnlineGameInstance>()->IsLoggedIn())
    {
        RefreshPackages();
    }
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


void URemnantPurchaseWidget::NativeDestruct()
{
    if (UMyOnlineGameInstance* GI = GetGameInstance<UMyOnlineGameInstance>())
    {
        GI->OnAuthStateChanged.RemoveDynamic(this, &URemnantPurchaseWidget::HandleAuthStateChanged);
        GI->OnProfileUpdated.RemoveDynamic(this, &URemnantPurchaseWidget::HandleProfileUpdated);
    }
    Super::NativeDestruct();
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
        if (PackageId.IsEmpty())
        {
            SelectedPackageText->SetText(FText::GetEmpty());
            SelectedPackageText->SetVisibility(ESlateVisibility::Collapsed);
            return;
        }

        FString SelectedPackageName = PackageId;

        if (StoreSubsystem)
        {
            const TArray<FRemnantPackage>& Packages = StoreSubsystem->GetCachedPackages();
            const FRemnantPackage* SelectedPackage = Packages.FindByPredicate([&PackageId](const FRemnantPackage& Package)
            {
                return Package.PackageId == PackageId;
            });

            if (SelectedPackage && !SelectedPackage->Name.IsEmpty())
            {
                SelectedPackageName = SelectedPackage->Name;
            }
        }

        SelectedPackageText->SetText(FText::FromString(FString::Printf(TEXT("Selected Package: %s"), *SelectedPackageName)));
        SelectedPackageText->SetVisibility(ESlateVisibility::Visible);
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

    FString CardNumber;
    FString CardExpiry;
    FString CardCVV;
    GetPaymentInputs(CardNumber, CardExpiry, CardCVV);

    SetLoadingState(true);
    StoreSubsystem->PurchaseRemnants(SelectedPackageId, CardNumber, CardExpiry, CardCVV);
}


// --------------------------------------------------
// GameInstance event handlers
// --------------------------------------------------

void URemnantPurchaseWidget::HandleAuthStateChanged(bool bIsLoggedIn)
{
    ApplyLoginGating(bIsLoggedIn);

    if (bIsLoggedIn)
    {
        SetNotification(TEXT(""), false);
        if (UMyOnlineGameInstance* GI = GetGameInstance<UMyOnlineGameInstance>())
        {
            UpdateBalanceText(GI->GetCurrentUserProfile().RemnantCount);
        }
        RefreshPackages();
    }
    else
    {
        SelectedPackageId.Empty();
        if (SelectedPackageText)
        {
            SelectedPackageText->SetText(FText::GetEmpty());
            SelectedPackageText->SetVisibility(ESlateVisibility::Collapsed);
        }

        UpdateBalanceText(0);
        if (PackageContainer)
        {
            PackageContainer->ClearChildren();
        }
    }
}

void URemnantPurchaseWidget::HandleProfileUpdated(const FUserProfile& UserProfile)
{
    UpdateBalanceText(UserProfile.RemnantCount);
    RefreshPackages();
}

bool URemnantPurchaseWidget::ValidatePaymentInputs() const
{
    FString CardNumber;
    FString Expiry;
    FString CVV;
    GetPaymentInputs(CardNumber, Expiry, CVV);

    const FString SanitizedCard = CardNumber.Replace(TEXT(" "), TEXT("")).Replace(TEXT("-"), TEXT(""));
    if (SanitizedCard.Len() != 16)
    {
        return false;
    }

    for (TCHAR Character : SanitizedCard)
    {
        if (!FChar::IsDigit(Character))
        {
            return false;
        }
    }

    if (Expiry.Len() != 5 || Expiry[2] != TEXT('/'))
    {
        return false;
    }

    if (!FChar::IsDigit(Expiry[0]) || !FChar::IsDigit(Expiry[1]) || !FChar::IsDigit(Expiry[3]) || !FChar::IsDigit(Expiry[4]))
    {
        return false;
    }

    const int32 Month = FCString::Atoi(*Expiry.Left(2));
    if (Month < 1 || Month > 12)
    {
        return false;
    }

    if (CVV.Len() < 3 || CVV.Len() > 4)
    {
        return false;
    }

    for (TCHAR Character : CVV)
    {
        if (!FChar::IsDigit(Character))
        {
            return false;
        }
    }

    return true;
}

void URemnantPurchaseWidget::InitializeStaticPaymentInputs()
{
    if (CardNumberInput && CardNumberInput->GetText().IsEmpty())
    {
        CardNumberInput->SetText(FText::FromString(DefaultCardNumber));
    }

    if (CardExpiryInput && CardExpiryInput->GetText().IsEmpty())
    {
        CardExpiryInput->SetText(FText::FromString(DefaultCardExpiry));
    }

    if (CardCVVInput && CardCVVInput->GetText().IsEmpty())
    {
        CardCVVInput->SetText(FText::FromString(DefaultCardCVV));
    }
}

void URemnantPurchaseWidget::GetPaymentInputs(FString& OutCardNumber, FString& OutExpiry, FString& OutCVV) const
{
    OutCardNumber = CardNumberInput ? CardNumberInput->GetText().ToString().TrimStartAndEnd() : FString();
    OutExpiry = CardExpiryInput ? CardExpiryInput->GetText().ToString().TrimStartAndEnd() : FString();
    OutCVV = CardCVVInput ? CardCVVInput->GetText().ToString().TrimStartAndEnd() : FString();

    if (OutCardNumber.IsEmpty())
    {
        OutCardNumber = DefaultCardNumber;
    }

    if (OutExpiry.IsEmpty())
    {
        OutExpiry = DefaultCardExpiry;
    }

    if (OutCVV.IsEmpty())
    {
        OutCVV = DefaultCardCVV;
    }
}


// --------------------------------------------------
// login gating helper
// --------------------------------------------------

void URemnantPurchaseWidget::ApplyLoginGating(bool bIsLoggedIn)
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
