#include "MatchResultsWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/VerticalBox.h"
#include "Components/ScrollBox.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameModeBase.h"

void UMatchResultsWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // Bind button events
    if (ReturnToLobbyButton)
    {
        ReturnToLobbyButton->OnClicked.AddDynamic(this, &UMatchResultsWidget::OnReturnToLobbyClicked);
    }

    if (PlayAgainButton)
    {
        PlayAgainButton->OnClicked.AddDynamic(this, &UMatchResultsWidget::OnPlayAgainClicked);
    }

    // Initially hide the widget
    SetVisibility(ESlateVisibility::Hidden);

    // Initialize match results
    InitializeMatchResults();
}

void UMatchResultsWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    // Auto-show after match ends with a small delay
    if (bShouldAutoShow && MatchGameState && MatchGameState->IsMatchFinished())
    {
        TimeSinceMatchEnd += InDeltaTime;
        if (TimeSinceMatchEnd >= 1.0f) // Wait 1 second after match ends
        {
            SetVisibility(ESlateVisibility::Visible);
            bShouldAutoShow = false;
        }
    }
}

void UMatchResultsWidget::InitializeMatchResults()
{
    // Get the game state
    MatchGameState = GetWorld() ? GetWorld()->GetGameState<AMultiplayerMatchGameState>() : nullptr;
    
    if (!MatchGameState)
    {
        UE_LOG(LogTemp, Warning, TEXT("MatchResultsWidget: Could not get MultiplayerMatchGameState"));
        return;
    }

    // Bind to match state changes
    MatchGameState->OnMatchStateChanged.AddDynamic(this, &UMatchResultsWidget::OnMatchStateChangedDelegate);

    // Update initial state
    OnMatchStateChanged(MatchGameState->CurrentMatchState);
}

void UMatchResultsWidget::OnMatchStateChanged(EMatchState NewState)
{
    switch (NewState)
    {
        case EMatchState::InProgress:
            SetVisibility(ESlateVisibility::Hidden);
            bShouldAutoShow = false;
            TimeSinceMatchEnd = 0.0f;
            break;

        case EMatchState::Finished:
            // Start the auto-show timer
            bShouldAutoShow = true;
            TimeSinceMatchEnd = 0.0f;
            
            // Update the results display
            UpdateResultsDisplay();
            break;

        case EMatchState::ReturningToLobby:
            // Keep showing during transition
            SetVisibility(ESlateVisibility::Visible);
            break;

        default:
            break;
    }
}

void UMatchResultsWidget::UpdateResultsDisplay()
{
    if (!MatchGameState || !TitleText || !WinnerText || !PlayerResultsBox)
    {
        return;
    }

    // Clear existing results
    PlayerResultsBox->ClearChildren();

    // Set title
    TitleText->SetText(FText::FromString(TEXT("Match Complete!")));

    // Set winner text
    if (MatchGameState->WinnerName == TEXT("Draw"))
    {
        WinnerText->SetText(FText::FromString(TEXT("It's a Draw!")));
    }
    else
    {
        WinnerText->SetText(FText::FromString(FString::Printf(TEXT("Winner: %s"), *MatchGameState->WinnerName)));
    }

    // Get ranked results
    TArray<FPlayerMatchResult> RankedResults = MatchGameState->GetRankedResults();

    // Create player result entries
    for (int32 i = 0; i < RankedResults.Num(); ++i)
    {
        CreatePlayerResultEntry(RankedResults[i], i + 1);
    }

    UE_LOG(LogTemp, Log, TEXT("MatchResultsWidget: Updated display for %d players"), RankedResults.Num());
}

void UMatchResultsWidget::CreatePlayerResultEntry(const FPlayerMatchResult& PlayerResult, int32 Rank)
{
    // Create a simple text entry for now (in a real implementation, you might want a dedicated widget)
    UTextBlock* PlayerEntry = NewObject<UTextBlock>(this);
    if (PlayerEntry)
    {
        FString EntryText;
        if (PlayerResult.bIsWinner)
        {
            EntryText = FString::Printf(TEXT("%d. %s - WINNER (Survived: %.1fs)"), 
                Rank, *PlayerResult.PlayerName, PlayerResult.SurvivalTime);
        }
        else if (PlayerResult.bIsAlive)
        {
            EntryText = FString::Printf(TEXT("%d. %s - Survived (Time: %.1fs)"), 
                Rank, *PlayerResult.PlayerName, PlayerResult.SurvivalTime);
        }
        else
        {
            EntryText = FString::Printf(TEXT("%d. %s - Eliminated #%d (Survived: %.1fs)"), 
                Rank, *PlayerResult.PlayerName, PlayerResult.EliminationOrder, PlayerResult.SurvivalTime);
        }

        PlayerEntry->SetText(FText::FromString(EntryText));
        
        // Style the text
        FSlateFontInfo FontInfo = PlayerEntry->GetFont();
        FontInfo.Size = 18;
        PlayerEntry->SetFont(FontInfo);
        
        if (PlayerResult.bIsWinner)
        {
            PlayerEntry->SetColorAndOpacity(FLinearColor::Green);
        }
        else if (!PlayerResult.bIsAlive)
        {
            PlayerEntry->SetColorAndOpacity(FLinearColor::Gray);
        }
        else
        {
            PlayerEntry->SetColorAndOpacity(FLinearColor::White);
        }

        // Add to the results box
        PlayerResultsBox->AddChildToVerticalBox(PlayerEntry);
    }
}

void UMatchResultsWidget::OnReturnToLobbyClicked()
{
    // Return to main menu/lobby
    if (APlayerController* PC = GetOwningPlayer())
    {
        UGameplayStatics::OpenLevel(PC, FName("LobbyMap"), true);
    }
}

void UMatchResultsWidget::OnPlayAgainClicked()
{
    // Restart the current level for a new match
    if (APlayerController* PC = GetOwningPlayer())
    {
        UGameplayStatics::OpenLevel(PC, FName(GetWorld()->GetMapName()), true);
    }
}

void UMatchResultsWidget::OnMatchStateChangedDelegate(EMatchState NewState)
{
    OnMatchStateChanged(NewState);
}