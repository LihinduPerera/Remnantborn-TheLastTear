#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Remnantborn/Remnantborn/OnlineService/UEdsHttpService.h"
#include "RemnantPurchaseWidget.generated.h"

class UButton;
class UEditableTextBox;
class UTextBlock;
class UHorizontalBox;
class UCircularThrobber;
class URemnantPackageCardWidget;
class UStoreSubsystem;
class UWidget;

UCLASS()
class REMNANTBORN_API URemnantPurchaseWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    UFUNCTION(BlueprintCallable, Category = "Store")
    void RefreshPackages();

protected:
    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* BalanceText;

    // gating widgets
    UPROPERTY(meta = (BindWidgetOptional))
    UWidget* ContentPanel;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* LoginRequiredText;

private:
    // helper for toggling login gated UI
    void ApplyLoginGating(bool bIsLoggedIn);


    UPROPERTY(meta = (BindWidgetOptional))
    UHorizontalBox* PackageContainer;

    UPROPERTY(meta = (BindWidgetOptional))
    UCircularThrobber* LoadingSpinner;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* SelectedPackageText;

    UPROPERTY(meta = (BindWidgetOptional))
    UEditableTextBox* CardNumberInput;

    UPROPERTY(meta = (BindWidgetOptional))
    UEditableTextBox* CardExpiryInput;

    UPROPERTY(meta = (BindWidgetOptional))
    UEditableTextBox* CardCVVInput;

    UPROPERTY(meta = (BindWidgetOptional))
    UButton* PayNowButton;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* NotificationText;

    UPROPERTY(EditAnywhere, Category = "Store")
    TSubclassOf<URemnantPackageCardWidget> RemnantPackageCardClass;

private:
    void InitializeStaticPaymentInputs();
    void GetPaymentInputs(FString& OutCardNumber, FString& OutExpiry, FString& OutCVV) const;

    UPROPERTY()
    UStoreSubsystem* StoreSubsystem;

    UPROPERTY()
    FString SelectedPackageId;

    UFUNCTION()
    void HandlePackagesLoaded(const TArray<FRemnantPackage>& Packages);

    UFUNCTION()
    void HandleRemnantPurchaseCompleted(bool bSuccess, const FRemnantPurchaseResponse& Response);

    UFUNCTION()
    void HandleStoreError(const FString& ErrorMessage);

    UFUNCTION()
    void OnPackageSelected(const FString& PackageId);

    UFUNCTION()
    void OnPayNowClicked();

    // authentication/profile callbacks
    UFUNCTION()
    void HandleAuthStateChanged(bool bIsLoggedIn);

    UFUNCTION()
    void HandleProfileUpdated(const FUserProfile& UserProfile);

    bool ValidatePaymentInputs() const;
    void SetLoadingState(bool bLoading);
    void UpdateBalanceText(int32 NewBalance);
    void SetNotification(const FString& Message, bool bIsError);
};
