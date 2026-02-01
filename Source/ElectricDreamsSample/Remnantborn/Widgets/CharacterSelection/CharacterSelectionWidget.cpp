#include "CharacterSelectionWidget.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "CharacterEntryWidget.h"
#include "ElectricDreamsSample/Remnantborn/CharacterSelection/CharacterSelectionSubsystem.h"
#include "Engine/GameInstance.h"

void UCharacterSelectionWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // Bind button events
    if (ConfirmButton)
    {
        ConfirmButton->OnClicked.AddDynamic(this, &UCharacterSelectionWidget::OnConfirmClicked);
    }

    if (CancelButton)
    {
        CancelButton->OnClicked.AddDynamic(this, &UCharacterSelectionWidget::OnCancelClicked);
    }

    // Initialize character selection
    InitializeCharacterSelection();
}

void UCharacterSelectionWidget::NativeDestruct()
{
    // Clean up event bindings
    OnCharacterConfirmed.RemoveAll(this);
    OnCharacterCancelled.RemoveAll(this);

    // Clean up character entries
    for (UCharacterEntryWidget* Entry : CharacterEntries)
    {
        if (Entry)
        {
            Entry->OnCharacterSelected.RemoveAll(this);
            Entry->RemoveFromParent();
        }
    }
    CharacterEntries.Empty();

    Super::NativeDestruct();
}

void UCharacterSelectionWidget::InitializeCharacterSelection()
{
    if (!CharacterListContainer || !CharacterEntryClass)
    {
        return;
    }

    // Clear existing entries
    CharacterListContainer->ClearChildren();
    CharacterEntries.Empty();

    // Get available characters
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    UCharacterSelectionSubsystem* CharacterSubsystem = World->GetGameInstance()->GetSubsystem<UCharacterSelectionSubsystem>();
    if (!CharacterSubsystem)
    {
        return;
    }

    TArray<UCharacterDataAsset*> AvailableCharacters = CharacterSubsystem->GetAvailableCharacters();

    // Create entry for each character
    for (UCharacterDataAsset* CharacterData : AvailableCharacters)
    {
        if (CharacterData)
        {
            UCharacterEntryWidget* EntryWidget = CreateWidget<UCharacterEntryWidget>(this, CharacterEntryClass);
            if (EntryWidget && CharacterListContainer)
            {
                EntryWidget->InitializeEntry(CharacterData);
                EntryWidget->OnCharacterSelected.AddDynamic(this, &UCharacterSelectionWidget::OnCharacterEntrySelected);
                
                CharacterListContainer->AddChildToVerticalBox(EntryWidget);
                CharacterEntries.Add(EntryWidget);

                // Select first character by default
                if (!CurrentSelection)
                {
                    EntryWidget->SetSelected(true);
                    CurrentSelection = CharacterData;
                    UpdateSelectionDisplay(CharacterData);
                }
            }
        }
    }

    // Update confirm button state
    if (ConfirmButton)
    {
        ConfirmButton->SetIsEnabled(CurrentSelection != nullptr);
    }
}

void UCharacterSelectionWidget::ConfirmSelection()
{
    if (CurrentSelection)
    {
        // Broadcast selection
        OnCharacterConfirmed.Broadcast(CurrentSelection);

        // Get player controller and set selection
        APlayerController* PC = GetOwningPlayer();
        if (PC)
        {
            UCharacterSelectionSubsystem* CharacterSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UCharacterSelectionSubsystem>();
            if (CharacterSubsystem)
            {
                CharacterSubsystem->SelectCharacterForPlayer(PC, CurrentSelection);
            }
        }
    }
}

void UCharacterSelectionWidget::OnCharacterEntrySelected(UCharacterDataAsset* CharacterData)
{
    // Deselect all other entries
    for (UCharacterEntryWidget* Entry : CharacterEntries)
    {
        if (Entry && Entry->GetCharacterData() != CharacterData)
        {
            Entry->SetSelected(false);
        }
    }

    // Update current selection
    CurrentSelection = CharacterData;
    UpdateSelectionDisplay(CharacterData);

    // Enable confirm button
    if (ConfirmButton)
    {
        ConfirmButton->SetIsEnabled(true);
    }
}

void UCharacterSelectionWidget::OnConfirmClicked()
{
    ConfirmSelection();
}

void UCharacterSelectionWidget::OnCancelClicked()
{
    // Broadcast cancellation
    OnCharacterCancelled.Broadcast(nullptr);

    // Remove widget
    RemoveFromParent();
}

void UCharacterSelectionWidget::UpdateSelectionDisplay(UCharacterDataAsset* CharacterData)
{
    if (!CharacterData)
    {
        return;
    }

    // Update image
    if (SelectedCharacterImage && CharacterData->CharacterPortrait)
    {
        SelectedCharacterImage->SetBrushFromTexture(CharacterData->CharacterPortrait);
    }

    // Update text
    if (SelectedCharacterName)
    {
        SelectedCharacterName->SetText(CharacterData->CharacterName);
    }

    if (SelectedCharacterDescription)
    {
        SelectedCharacterDescription->SetText(CharacterData->CharacterDescription);
    }
}