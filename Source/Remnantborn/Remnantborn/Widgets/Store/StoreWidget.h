#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Remnantborn/Remnantborn/OnlineService/UEdsHttpService.h"
#include "StoreWidget.generated.h"

class UTextBlock;
class UWrapBox;
class UCircularThrobber;
class UStoreCharacterCardWidget;
class UStoreSubsystem;

UCLASS()
class REMNANTBORN_API UStoreWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    UFUNCTION(BlueprintCallable, Category = "Store")
    void RefreshStore();

protected:
    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* BalanceText;

    UPROPERTY(meta = (BindWidgetOptional))
    UWrapBox* CharacterGrid;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* ComingSoonText;

    UPROPERTY(meta = (BindWidgetOptional))
    UCircularThrobber* LoadingSpinner;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* NotificationText;

    UPROPERTY(EditAnywhere, Category = "Store")
    TSubclassOf<UStoreCharacterCardWidget> StoreCharacterCardClass;

private:
    UPROPERTY()
    UStoreSubsystem* StoreSubsystem;

    // event handlers
    UFUNCTION()
    void HandleCatalogLoaded(const TArray<FStoreCharacterInfo>& Characters);

    UFUNCTION()
    void HandleCharacterPurchaseCompleted(bool bSuccess, const FCharacterPurchaseResponse& Response);

    UFUNCTION()
    void HandleStoreError(const FString& ErrorMessage);

    UFUNCTION()
    void OnCharacterPurchaseRequested(const FString& CharacterId);

    // authentication/profile callbacks from GameInstance
    UFUNCTION()
    void HandleAuthStateChanged(bool bIsLoggedIn);

    UFUNCTION()
    void HandleProfileUpdated(const FUserProfile& UserProfile);

    void SetLoadingState(bool bLoading);
    void UpdateBalanceText(int32 NewBalance);
    void SetNotification(const FString& Message, bool bIsError);
};
