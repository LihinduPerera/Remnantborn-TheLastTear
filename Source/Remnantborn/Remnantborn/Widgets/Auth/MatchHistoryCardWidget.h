#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Remnantborn/Remnantborn/OnlineService/UEdsHttpService.h"
#include "MatchHistoryCardWidget.generated.h"

class UTextBlock;
class UVerticalBox;

UCLASS()
class REMNANTBORN_API UMatchHistoryCardWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;

    UFUNCTION(BlueprintCallable, Category = "Profile")
    void InitializeCard(const FProfileMatchHistoryEntry& MatchEntry);

protected:
    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* ResultText;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* MapText;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* PlacementText;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* DurationText;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* MetaText;

private:
    void EnsureDynamicLayout();
    void ApplyEntryToWidgets(const FProfileMatchHistoryEntry& MatchEntry);

    bool bInitialized = false;
    FProfileMatchHistoryEntry PendingEntry;
};
