#include "MatchHistoryCardWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

namespace
{
    constexpr float CardPadding = 12.0f;
    constexpr float RowPadding = 3.0f;
}

void UMatchHistoryCardWidget::NativeConstruct()
{
    Super::NativeConstruct();

    EnsureDynamicLayout();
    bInitialized = true;
    ApplyEntryToWidgets(PendingEntry);
}

void UMatchHistoryCardWidget::InitializeCard(const FProfileMatchHistoryEntry& MatchEntry)
{
    PendingEntry = MatchEntry;

    if (!bInitialized)
    {
        EnsureDynamicLayout();
    }

    ApplyEntryToWidgets(MatchEntry);
}

void UMatchHistoryCardWidget::EnsureDynamicLayout()
{
    if (!WidgetTree)
    {
        return;
    }

    if (!GetRootWidget())
    {
        UBorder* CardBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CardBorder"));
        CardBorder->SetPadding(FMargin(CardPadding));
        CardBorder->SetBrushColor(FLinearColor(0.06f, 0.08f, 0.11f, 0.85f));
        WidgetTree->RootWidget = CardBorder;

        UVerticalBox* RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RootBox"));
        CardBorder->SetContent(RootBox);

        UHorizontalBox* HeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HeaderRow"));
        RootBox->AddChildToVerticalBox(HeaderRow);

        ResultText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ResultText"));
        MapText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MapText"));

        if (ResultText)
        {
            ResultText->SetShadowOffset(FVector2D(1.0f, 1.0f));
            if (UHorizontalBoxSlot* HeaderResultSlot = HeaderRow->AddChildToHorizontalBox(ResultText))
            {
                HeaderResultSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, RowPadding));
                HeaderResultSlot->SetHorizontalAlignment(HAlign_Left);
            }
        }

        if (MapText)
        {
            MapText->SetJustification(ETextJustify::Right);
            if (UHorizontalBoxSlot* HeaderMapSlot = HeaderRow->AddChildToHorizontalBox(MapText))
            {
                HeaderMapSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, RowPadding));
                HeaderMapSlot->SetHorizontalAlignment(HAlign_Fill);
                HeaderMapSlot->SetSize(ESlateSizeRule::Fill);
            }
        }

        UHorizontalBox* MidRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("MidRow"));
        RootBox->AddChildToVerticalBox(MidRow);

        PlacementText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PlacementText"));
        DurationText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DurationText"));

        if (PlacementText)
        {
            if (UHorizontalBoxSlot* PlacementSlot = MidRow->AddChildToHorizontalBox(PlacementText))
            {
                PlacementSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, RowPadding));
                PlacementSlot->SetHorizontalAlignment(HAlign_Left);
                PlacementSlot->SetSize(ESlateSizeRule::Fill);
            }
        }

        if (DurationText)
        {
            DurationText->SetJustification(ETextJustify::Right);
            if (UHorizontalBoxSlot* DurationSlot = MidRow->AddChildToHorizontalBox(DurationText))
            {
                DurationSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, RowPadding));
                DurationSlot->SetHorizontalAlignment(HAlign_Fill);
                DurationSlot->SetSize(ESlateSizeRule::Fill);
            }
        }

        MetaText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MetaText"));
        if (MetaText)
        {
            MetaText->SetAutoWrapText(true);
            if (UVerticalBoxSlot* MetaRowSlot = RootBox->AddChildToVerticalBox(MetaText))
            {
                MetaRowSlot->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 0.0f));
            }
        }
    }
}

void UMatchHistoryCardWidget::ApplyEntryToWidgets(const FProfileMatchHistoryEntry& MatchEntry)
{
    const bool bIsDraw = MatchEntry.bIsDraw;
    const bool bIsWinner = MatchEntry.bIsWinner;
    const FString ResultLabel = bIsDraw ? TEXT("DRAW") : (bIsWinner ? TEXT("WIN") : TEXT("LOSS"));
    const FLinearColor ResultColor = bIsDraw
        ? FLinearColor(0.95f, 0.75f, 0.2f)
        : (bIsWinner ? FLinearColor(0.2f, 0.85f, 0.3f) : FLinearColor(0.9f, 0.25f, 0.25f));

    const FString SafeMapName = MatchEntry.MapName.IsEmpty() ? TEXT("UnknownMap") : MatchEntry.MapName;
    const int32 Placement = MatchEntry.Placement > 0 ? MatchEntry.Placement : (bIsWinner ? 1 : 0);
    const FString PlacementTextValue = Placement > 0
        ? FString::Printf(TEXT("Placement: P%d"), Placement)
        : TEXT("Placement: P?");

    const FString DurationTextValue = FString::Printf(TEXT("Duration: %ds"), FMath::Max(0, MatchEntry.DurationSeconds));

    const FString CharacterLabel = MatchEntry.CharacterId.IsEmpty()
        ? TEXT("Character: Unknown")
        : FString::Printf(TEXT("Character: %s"), *MatchEntry.CharacterId);
    const FString RewardLabel = FString::Printf(TEXT("Reward: +%d"), FMath::Max(0, MatchEntry.RewardAmount));
    const FString MatchDateLabel = MatchEntry.EndedAt.IsEmpty() ? TEXT("Date: Unknown") : FString::Printf(TEXT("Date: %s"), *MatchEntry.EndedAt.Left(10));
    const FString MetaTextValue = FString::Printf(TEXT("%s | %s | %s"), *CharacterLabel, *RewardLabel, *MatchDateLabel);

    if (ResultText)
    {
        ResultText->SetText(FText::FromString(ResultLabel));
        ResultText->SetColorAndOpacity(FSlateColor(ResultColor));
    }

    if (MapText)
    {
        MapText->SetText(FText::FromString(FString::Printf(TEXT("Map: %s"), *SafeMapName)));
    }

    if (PlacementText)
    {
        PlacementText->SetText(FText::FromString(PlacementTextValue));
    }

    if (DurationText)
    {
        DurationText->SetText(FText::FromString(DurationTextValue));
    }

    if (MetaText)
    {
        MetaText->SetText(FText::FromString(MetaTextValue));
    }
}
