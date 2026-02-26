#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Remnantborn/Remnantborn/OnlineService/UEdsHttpService.h"
#include "RemnantPackageCardWidget.generated.h"

class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRemnantPackageSelected, const FString&, PackageId);

UCLASS()
class REMNANTBORN_API URemnantPackageCardWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Store")
    void InitializeCard(const FRemnantPackage& PackageInfo, bool bIsBestValue = false);

    UPROPERTY(BlueprintAssignable, Category = "Store|Events")
    FOnRemnantPackageSelected OnRemnantPackageSelected;

protected:
    virtual void NativeConstruct() override;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* AmountText;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* PriceText;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* PackageNameText;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* BestValueText;

    UPROPERTY(meta = (BindWidgetOptional))
    UButton* SelectButton;

private:
    UFUNCTION()
    void OnSelectClicked();

    FString CachedPackageId;
};
