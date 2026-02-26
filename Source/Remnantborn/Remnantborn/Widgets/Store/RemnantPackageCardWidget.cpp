#include "RemnantPackageCardWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"

void URemnantPackageCardWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (SelectButton)
    {
        SelectButton->OnClicked.AddDynamic(this, &URemnantPackageCardWidget::OnSelectClicked);
    }
}

void URemnantPackageCardWidget::InitializeCard(const FRemnantPackage& PackageInfo, bool bIsBestValue)
{
    CachedPackageId = PackageInfo.PackageId;

    if (AmountText)
    {
        AmountText->SetText(FText::FromString(FString::Printf(TEXT("%d"), PackageInfo.RemnantAmount)));
    }

    if (PriceText)
    {
        PriceText->SetText(FText::FromString(PackageInfo.DisplayPrice));
    }

    if (PackageNameText)
    {
        PackageNameText->SetText(FText::FromString(PackageInfo.Name));
    }

    if (BestValueText)
    {
        BestValueText->SetVisibility(bIsBestValue ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }
}

void URemnantPackageCardWidget::OnSelectClicked()
{
    if (!CachedPackageId.IsEmpty())
    {
        OnRemnantPackageSelected.Broadcast(CachedPackageId);
    }
}
