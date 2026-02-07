#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Remnantborn/Remnantborn/CharacterSelection/CharacterDataAsset.h"
#include "CharacterSelectionWidget.generated.h"

class UButton;
class UImage;
class UTextBlock;
class UVerticalBox;
class UCharacterEntryWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterConfirmed, UCharacterDataAsset*, SelectedCharacter);

/**
 * Main character selection screen widget
 */
UCLASS()
class REMNANTBORN_API UCharacterSelectionWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    // Initialize the widget with available characters
    UFUNCTION(BlueprintCallable, Category = "Character Selection")
    void InitializeCharacterSelection();

    // Confirm character selection
    UFUNCTION(BlueprintCallable, Category = "Character Selection")
    void ConfirmSelection();

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Character Selection|Events")
    FOnCharacterConfirmed OnCharacterConfirmed;

    UPROPERTY(BlueprintAssignable, Category = "Character Selection|Events")
    FOnCharacterConfirmed OnCharacterCancelled;

protected:
    // Widget components
    UPROPERTY(meta = (BindWidget))
    UVerticalBox* CharacterListContainer;

    UPROPERTY(meta = (BindWidget))
    UImage* SelectedCharacterImage;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* SelectedCharacterName;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* SelectedCharacterDescription;

    UPROPERTY(meta = (BindWidget))
    UButton* ConfirmButton;

    UPROPERTY(meta = (BindWidget))
    UButton* CancelButton;

    // Widget class for character entries
    UPROPERTY(EditAnywhere, Category = "Widgets")
    TSubclassOf<UCharacterEntryWidget> CharacterEntryClass;

    // Event handlers
    UFUNCTION()
    void OnCharacterEntrySelected(UCharacterDataAsset* CharacterData);

    UFUNCTION()
    void OnConfirmClicked();

    UFUNCTION()
    void OnCancelClicked();

private:
    // Currently selected character
    UPROPERTY()
    UCharacterDataAsset* CurrentSelection;

    // All character entry widgets
    TArray<UCharacterEntryWidget*> CharacterEntries;

    // Create character entries
    void CreateCharacterEntries();

    // Update selection display
    void UpdateSelectionDisplay(UCharacterDataAsset* CharacterData);
};