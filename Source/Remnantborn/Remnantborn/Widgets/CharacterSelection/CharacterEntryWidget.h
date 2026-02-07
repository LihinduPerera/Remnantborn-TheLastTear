#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Remnantborn/Remnantborn/CharacterSelection/CharacterDataAsset.h"
#include "CharacterEntryWidget.generated.h"

class UButton;
class UImage;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterEntrySelected, UCharacterDataAsset*, CharacterData);

/**
 * Widget for a single character entry in the selection list
 */
UCLASS()
class REMNANTBORN_API UCharacterEntryWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;

    // Initialize the entry with character data
    UFUNCTION(BlueprintCallable, Category = "Character Selection")
    void InitializeEntry(UCharacterDataAsset* CharacterData);

    // Set selection state
    UFUNCTION(BlueprintCallable, Category = "Character Selection")
    void SetSelected(bool bIsSelected);

    // Get character data
    UFUNCTION(BlueprintCallable, Category = "Character Selection")
    UCharacterDataAsset* GetCharacterData() const { return CharacterData; }

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Character Selection|Events")
    FOnCharacterEntrySelected OnCharacterSelected;

protected:
    // Widget components
    UPROPERTY(meta = (BindWidget))
    UButton* SelectButton;

    UPROPERTY(meta = (BindWidget))
    UImage* CharacterImage;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* CharacterNameText;

    // Event handler
    UFUNCTION()
    void OnSelectButtonClicked();

private:
    // Character data for this entry
    UPROPERTY()
    UCharacterDataAsset* CharacterData;

    // Is this entry selected?
    bool bIsSelected;
};