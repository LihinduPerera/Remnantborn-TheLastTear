#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Remnantborn/Remnantborn/CharacterSelection/CharacterDataAsset.h"
#include "OwnedCharacterCardWidget.generated.h"

class UImage;
class UTextBlock;

UCLASS()
class REMNANTBORN_API UOwnedCharacterCardWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Profile")
    void InitializeCard(UCharacterDataAsset* CharacterData, bool bIsPurchased);

protected:
    UPROPERTY(meta = (BindWidgetOptional))
    UImage* CharacterPortrait;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* CharacterName;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* StatusBadge;
};
